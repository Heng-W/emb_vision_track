
#include "server.h"

#include "util/logger.h"
#include "session.h"

namespace evt
{

Server::Server(net::EventLoop* loop,
               const net::InetAddress& listenAddr)
    : server_(loop, listenAddr),
      broadcastService_(&server_)
{
    server_.setConnectionCallback([this](const net::TcpConnectionPtr & conn)
    {
        LOG(INFO) << "Server - " << conn->peerAddr().toIpPort() << " -> "
                  << conn->localAddr().toIpPort() << " is "
                  << (conn->connected() ? "UP" : "DOWN");
        if (conn->connected())
        {
            SessionPtr session(new Session(this, conn));
            std::lock_guard<std::mutex> lock(mutex_);
            assert(sessions_.find(conn->id()) == sessions_.end());
            sessions_[conn->id()] = std::move(session);
        }
        else
        {
            std::lock_guard<std::mutex> lock(mutex_);
            assert(sessions_.find(conn->id()) != sessions_.end());
            sessions_.erase(conn->id());
        }
    });
}

} // namespace evt
