#ifndef EVT_SERVER_H
#define EVT_SERVER_H

#include <mutex>
#include <unordered_map>
#include "net/tcp_server.h"
#include "broadcast_service.h"
#include "session.h"

namespace evt
{

class Server
{
public:

    Server(net::EventLoop* loop, const net::InetAddress& listenAddr);

    void start() { server_.start(); }

    void addUser(uint32_t userId, const std::weak_ptr<Session>& session)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(users_.find(userId) == users_.end());
        users_[userId] = session;
    }

    void removeUser(uint32_t userId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(users_.find(userId) != users_.end());
        users_.erase(userId);
    }

    SessionPtr findUser(uint32_t userId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = users_.find(userId);
        return it != users_.end() ? it->second.lock() : nullptr;
    }

    net::BroadcastService* broadcastService() { return &broadcastService_; }

private:

    net::TcpServer server_;
    net::BroadcastService broadcastService_;
    std::unordered_map<int64_t, SessionPtr> sessions_;
    std::unordered_map<uint32_t, std::weak_ptr<Session>> users_;

    mutable std::mutex mutex_;
};

} // namespace evt

#endif // EVT_SERVER_H
