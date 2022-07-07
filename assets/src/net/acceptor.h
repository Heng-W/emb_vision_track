#ifndef NET_ACCEPTOR_H
#define NET_ACCEPTOR_H

#include "channel.h"
#include "socket.h"

namespace net
{

class EventLoop;
class InetAddress;

class Acceptor
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    DISALLOW_COPY_AND_ASSIGN(Acceptor);

    void listen();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    bool listening() const { return listening_; }

private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    int idleFd_;
};

} // namespace net

#endif // NET_ACCEPTOR_H
