#ifndef NET_BROADCAST_SERVICE_H
#define NET_BROADCAST_SERVICE_H

#include <functional>

namespace net
{

class TcpServer;
class EventLoop;
class Buffer;

class BroadcastService
{
public:
    using Predicate = std::function<bool(int64_t connId)>;

    BroadcastService(TcpServer* server);

    void broadcast(const void* data, int len);
    void broadcast(std::string&& message);
    void broadcast(Buffer&& buf);

    void broadcast(const void* data, int len, const Predicate& pred);
    void broadcast(std::string&& message, const Predicate& pred);
    void broadcast(Buffer&& buf, const Predicate& pred);

    void broadcastExcept(const void* data, int len, int64_t connId)
    { broadcast(data, len, Except(connId)); }

    void broadcastExcept(std::string&& message, int64_t connId)
    { broadcast(std::move(message), Except(connId)); }

    void broadcastExcept(Buffer&& buf, int64_t connId)
    { broadcast(std::move(buf), Except(connId)); }

private:
    struct Except
    {
        Except(int64_t connId): connId_(connId) {}
        bool operator()(int64_t cur) const { return cur != connId_; }
    private:
        int64_t connId_;
    };

    void broadcastInLoop(const void* data, int len);
    void broadcastInLoop(const void* data, int len, const Predicate& pred);

    TcpServer* server_;
    EventLoop* loop_;
};

} // namespace net

#endif // NET_BROADCAST_SERVICE_H
