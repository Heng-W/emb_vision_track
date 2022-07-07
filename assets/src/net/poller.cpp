
#include "poller.h"

#include <assert.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include "../util/logger.h"
#include "event_loop.h"
#include "channel.h"

namespace net
{

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        SYSLOG(FATAL) << "Poller::Poller";
    }
}

Poller::~Poller()
{
    ::close(epollfd_);
}

Timestamp Poller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG(TRACE) << "fd total count " << channels_.size();
    int numEvents = ::epoll_wait(epollfd_, events_.data(), events_.size(), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG(TRACE) << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG(TRACE) << "nothing happened";
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            SYSLOG(ERROR) << "Poller::poll()";
        }
    }
    return now;
}

void Poller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        assert(hasChannel(channel));
        channel->setRevents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

bool Poller::hasChannel(Channel* channel) const
{
    ownerLoop_->assertInLoopThread();
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

void Poller::updateChannel(Channel* channel)
{
    ownerLoop_->assertInLoopThread();
    const int index = channel->index();
    const int fd = channel->fd();
    LOG(TRACE) << "fd = " << fd << " events = " << channel->events()
               << " index = " << index;
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else // index == kDeleted
        {
            assert(hasChannel(channel));
        }
        update(EPOLL_CTL_ADD, channel);
        channel->setIndex(kAdded);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        assert(hasChannel(channel));
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Poller::removeChannel(Channel* channel)
{
    ownerLoop_->assertInLoopThread();
    int fd = channel->fd();
    LOG(TRACE) << "fd = " << fd;
    assert(hasChannel(channel));
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd); // remove in map
    (void)n;
    assert(n == 1);

    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}

void Poller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;

    int fd = channel->fd();
    LOG(TRACE) << "epoll_ctl op = " << operationToString(operation)
               << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            SYSLOG(ERROR) << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
        else
        {
            SYSLOG(FATAL) << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
    }
}

const char* Poller::operationToString(int op)
{
    switch (op)
    {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
    }
}

} // namespace net
