
#include "timer_list.h"

#include <unistd.h>
#include <string.h>
#include <sys/timerfd.h>
#include <functional>
#include <atomic>
#include "../util/logger.h"
#include "event_loop.h"

namespace net
{

namespace
{

int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        SYSLOG(FATAL) << "Failed in timerfd_create";
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                           - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) microseconds = 100;
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    LOG(TRACE) << "TimerList::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof(howmany))
    {
        LOG(ERROR) << "TimerList::handleRead() reads " << n << " bytes instead of 8";
    }
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        SYSLOG(ERROR) << "timerfd_settime()";
    }
}

} // namespace


class Timer
{
public:
    Timer(const TimerCallback& cb, Timestamp when, int64_t periodMs = 0)
        : callback_(cb),
          expiration_(when),
          periodMs_(periodMs),
          id_(++s_numCreated_)
    {}

    DISALLOW_COPY_AND_ASSIGN(Timer);

    void run() const { callback_(); }

    int64_t id() const { return id_; }
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return periodMs_ > 0; }

    void restart(Timestamp now)
    { expiration_ = repeat() ? (now + util::msec(periodMs_)) : Timestamp::invalid(); }

    static int64_t numCreated() { return s_numCreated_; }

private:
    static std::atomic<int64_t> s_numCreated_;

    TimerCallback callback_;
    Timestamp expiration_;
    int64_t periodMs_;
    int64_t id_;
};

std::atomic<int64_t> Timer::s_numCreated_(0);


TimerList::TimerList(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_()
{
    timerfdChannel_.setReadCallback([this](Timestamp) { this->handleRead(); });
    timerfdChannel_.enableReading();
}

TimerList::~TimerList()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for (const Entry& timer : timers_)
    {
        delete timer.second;
    }
}

int64_t TimerList::addTimer(const TimerCallback& cb, Timestamp when, int64_t periodMs)
{
    Timer* timer = new Timer(cb, when, periodMs);
    loop_->runInLoop([this, timer]
    {
        loop_->assertInLoopThread();
        bool earliestChanged = false;
        Timestamp when = timer->expiration();
        auto it = timers_.begin();
        if (it == timers_.end() || when < it->first)
        {
            earliestChanged = true;
        }
        timers_.insert({when, timer});
        activeTimers_[timer->id()] = timer;
        assert(timers_.size() == activeTimers_.size());
        if (earliestChanged)
        {
            resetTimerfd(timerfd_, when);
        }
    });
    return timer->id();
}

void TimerList::removeTimer(int64_t timerId)
{
    loop_->runInLoop([this, timerId]
    {
        loop_->assertInLoopThread();

        auto it = activeTimers_.find(timerId);
        if (it == activeTimers_.end()) return; // id无效

        Timer* timer = it->second;
        timers_.erase({timer->expiration(), timer});
        activeTimers_.erase(it);
        delete timer;
    });
}

void TimerList::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    for (auto it = timers_.begin(); it != timers_.end();)
    {
        Timer* timer = it->second;
        if (timer->expiration() <= now)
        {
            timer->run();
            it = timers_.erase(it);
            if (!timer->repeat())
            {
                activeTimers_.erase(timer->id());
                delete timer;
            }
            else
            {
                timer->restart(now);
                timers_.insert({timer->expiration(), timer});
            }
        }
        else
        {
            break;
        }
    }
    Timestamp nextExpire;
    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }
    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}


} // namespace net
