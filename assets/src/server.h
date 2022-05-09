#ifndef EVT_SERVER_H
#define EVT_SERVER_H

#include "net/tcp_server.h"
#include "broadcast_service.h"

namespace evt
{

class Server
{
public:

    Server(net::EventLoop* loop, const net::InetAddress& listenAddr);

    void start() { server_.start(); }

private:
    void parseMessage(const net::TcpConnectionPtr& conn, const char* data, int len);

    net::TcpServer server_;
    net::BroadcastService broadcastService_;
};

} // namespace evt

#endif // EVT_SERVER_H
