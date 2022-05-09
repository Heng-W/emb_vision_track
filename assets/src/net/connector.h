#ifndef NET_CONNECTOR_H
#define NET_CONNECTOR_H

#include <functional>
#include <memory>
#include "../util/common.h"
#include "inet_address.h"

namespace net
{

class Channel;
class EventLoop;

class Connector : public std::enable_shared_from_this<Connector>
{
public:
    using NewConnectionCallback = std::function<void(int sockfd)> ;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    DISALLOW_COPY_AND_ASSIGN(Connector);

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    void start(); // 可在任意线程调用
    void restart(); // 只能在loop线程调用
    void stop(); // 可在任意线程调用

    const InetAddress& serverAddress() const { return serverAddr_; }

private:
    enum State { kDisconnected, kConnecting, kConnected };

    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setState(State s) { state_ = s; }
    void startInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();

    EventLoop* loop_;
    InetAddress serverAddr_;
    bool connect_;
    State state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
    int64_t timerId_;
};

} // namespace net

#endif // NET_CONNECTOR_H

