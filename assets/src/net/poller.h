#ifndef NET_POLLER_H
#define NET_POLLER_H

#include <vector>
#include <unordered_map>
#include "../util/common.h"
#include "../util/timestamp.h"

struct epoll_event;

namespace net
{

class EventLoop;
class Channel;

class Poller
{
public:
    using ChannelList = std::vector<Channel*>;

    enum ChannelIndex
    {
        kNew = -1, // not exist
        kAdded = 1, // exist, fd is watched
        kDeleted = 2, // exist, but called EPOLL_CTL_DEL
    };

    Poller(EventLoop* loop);
    ~Poller();

    DISALLOW_COPY_AND_ASSIGN(Poller);

    util::Timestamp poll(int timeoutMs, ChannelList* activeChannels);

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel) const;

private:
    // fd到Channel*的映射
    using ChannelMap = std::unordered_map<int, Channel*>;

    EventLoop* ownerLoop_;
    ChannelMap channels_;

    // use epoll
private:
    using EventList = std::vector<epoll_event>;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    static const char* operationToString(int op);

    static const int kInitEventListSize = 16;

    int epollfd_;
    EventList events_;
};

} // namespace net

#endif // NET_POLLER_H
