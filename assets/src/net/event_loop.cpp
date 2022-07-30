
#include "event_loop.h"

#include <unistd.h>
#include <sys/eventfd.h>
#include <algorithm>
#include "../util/logger.h"
#include "channel.h"
#include "poller.h"
#include "timer_list.h"

namespace net
{

namespace
{

using ChannelList = Poller::ChannelList;

thread_local net::EventLoop* t_loopInThisThread = nullptr;
constexpr int kMaxPollTimeMs = 10000;

int createEventfd()
{
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
    {
        SYSLOG(FATAL) << "Failed in eventfd";
    }
    return fd;
}

void printChannels(const ChannelList& channels)
{
    for (const Channel* channel : channels)
    {
        LOG(TRACE) << "{" << channel->reventsToString() << "} ";
    }
}

} // namespace

EventLoop* EventLoop::loopInThisThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : quit_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId_(std::this_thread::get_id()),
      poller_(new Poller(this)),
      timerList_(new TimerList(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG(DEBUG) << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread)
    {
        LOG(FATAL) << "Another EventLoop " << t_loopInThisThread
                   << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback([this](Timestamp) { this->handleRead(); });
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG(DEBUG) << "EventLoop " << this << " of thread " << threadId_
               << " destructs in thread " << std::this_thread::get_id();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    assertInLoopThread();
    quit_ = false;
    LOG(TRACE) << "EventLoop " << this << " start looping";

    ChannelList activeChannels;

    while (!quit_)
    {
        activeChannels.clear();
        int64_t pollTimeMs = kMaxPollTimeMs;
        Timestamp nextExpiration = timerList_->nextExpiration();
        if (nextExpiration.valid())
        {
            pollTimeMs = (nextExpiration - Timestamp::now()).toMsec();
            if (pollTimeMs < 0) pollTimeMs = 0;
            else if (pollTimeMs > kMaxPollTimeMs) pollTimeMs = kMaxPollTimeMs;
        }
        pollReturnTime_ = poller_->poll(static_cast<int>(pollTimeMs), &activeChannels);
        ++iteration_;
        if (LOG_IS_ON(TRACE)) printChannels(activeChannels);

        timerList_->doTimerEvent();
        for (Channel* channel : activeChannels)
        {
            channel->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }
    LOG(TRACE) << "EventLoop " << this << " stop looping";
}

void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {
        wakeup();
    }
}

size_t EventLoop::queueSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingFunctors_.size();
}

int64_t EventLoop::addTimer(const TimerCallback& cb, Timestamp time, int64_t interval)
{
    return timerList_->addTimer(cb, time, interval);
}

int64_t EventLoop::addTimer(const TimerCallback& cb, int64_t delay, int64_t interval)
{
    return timerList_->addTimer(cb, Timestamp::now() + util::msec(delay), interval);
}

void EventLoop::removeTimer(int64_t timerId)
{
    return timerList_->removeTimer(timerId);
}

void EventLoop::abortNotInLoopThread() const
{
    LOG(FATAL) << "EventLoop::abortNotInLoopThread - EventLoop " << this
               << " was created in threadId_ = " << threadId_
               << ", current thread id = " <<  std::this_thread::get_id();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG(ERROR) << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG(ERROR) << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;
}


} // namespace net

