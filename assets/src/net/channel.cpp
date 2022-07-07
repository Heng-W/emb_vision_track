
#include "channel.h"

#include <poll.h>
#include <sstream>
#include "../util/logger.h"
#include "event_loop.h"
#include "poller.h"

namespace net
{

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(Poller::kNew),
      logHup_(true),
      tied_(false)
{
}

Channel::~Channel()
{
    if (loop_->isInLoopThread())
    {
        assert(!loop_->poller_->hasChannel(this));
    }
}

void Channel::update()
{
    loop_->assertInLoopThread();
    loop_->poller_->updateChannel(this);
}

void Channel::remove()
{
    loop_->assertInLoopThread();
    assert(isNoneEvent());
    loop_->poller_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        auto guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG(TRACE) << reventsToString();
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (logHup_)
        {
            LOG(WARN) << "fd = " << fd_ << " Channel::handleEvent() POLLHUP";
        }
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & POLLNVAL)
    {
        LOG(WARN) << "fd = " << fd_ << " Channel::handleEvent() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
}

std::string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN) oss << "IN ";
    if (ev & POLLPRI) oss << "PRI ";
    if (ev & POLLOUT) oss << "OUT ";
    if (ev & POLLHUP) oss << "HUP ";
    if (ev & POLLRDHUP) oss << "RDHUP ";
    if (ev & POLLERR) oss << "ERR ";
    if (ev & POLLNVAL) oss << "NVAL ";
    return oss.str();
}

} // namespace net
