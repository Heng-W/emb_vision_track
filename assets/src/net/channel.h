#ifndef NET_CHANNEL_H
#define NET_CHANNEL_H

#include <functional>
#include <memory>
#include "../util/common.h"
#include "../util/timestamp.h"

namespace net
{

class EventLoop;

// IO通道
class Channel
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(util::Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    DISALLOW_COPY_AND_ASSIGN(Channel);

    void handleEvent(util::Timestamp receiveTime);

    void remove();

    // 将channel和shared_ptr绑定
    void tie(const std::shared_ptr<void>& obj) { tie_ = obj; tied_ = true; }

    // callbacks
    void setReadCallback(const ReadEventCallback& cb) { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb) { writeCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb) { closeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb) { errorCallback_ = cb; }

    // events
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    int events() const { return events_; }


    EventLoop* ownerLoop() const { return loop_; }
    int fd() const { return fd_; }
    void doNotLogHup() { logHup_ = false; }

    // for Poller
    void setRevents(int revents) { revents_ = revents; }
    void setIndex(int index) { index_ = index; }
    int index() const { return index_; }

    // for debug
    std::string eventsToString() const { return eventsToString(fd_, events_); }
    std::string reventsToString() const { return eventsToString(fd_, revents_); }

private:

    void update();
    void handleEventWithGuard(util::Timestamp receiveTime);

    static std::string eventsToString(int fd, int ev);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;

    int events_; // 请求的事件
    int revents_; // poller返回的事件
    int index_; // used by Poller
    bool logHup_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

} // namespace net

#endif // NET_CHANNEL_H
