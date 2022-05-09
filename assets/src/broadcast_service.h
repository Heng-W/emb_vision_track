#ifndef NET_BROADCAST_SERVICE_H
#define NET_BROADCAST_SERVICE_H

#include <memory>
#include <functional>
#include <set>

namespace net
{
class EventLoop;
class TcpConnection;
class Buffer;

class BroadcastService
{
public:
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using TcpConnectionWeakptr = std::weak_ptr<TcpConnection>;
    using Predicate = std::function<bool(const TcpConnectionWeakptr&)>;

    BroadcastService(EventLoop* loop);

    void addConnection(const TcpConnectionPtr& conn);
    void removeConnection(const TcpConnectionPtr& conn);
    void clearConnection();

    void broadcast(const void* data, int len);
    void broadcast(std::string&& message);
    void broadcast(Buffer&& buf);

    void broadcast(const void* data, int len, const Predicate& pred);
    void broadcast(std::string&& message, const Predicate& pred);
    void broadcast(Buffer&& buf, const Predicate& pred);

    void broadcastExcept(const void* data, int len, const TcpConnectionWeakptr& conn)
    { broadcast(data, len, Except(conn)); }

    void broadcastExcept(std::string&& message, const TcpConnectionWeakptr& conn)
    { broadcast(std::move(message), Except(conn)); }

    void broadcastExcept(Buffer&& buf, const TcpConnectionWeakptr& conn)
    { broadcast(std::move(buf), Except(conn)); }

private:

    void broadcastInLoop(const void* data, int len);
    void broadcastInLoop(const void* data, int len, const Predicate& pred);

    struct Except
    {
        Except(const TcpConnectionWeakptr& conn): conn_(conn) {}

        bool operator()(const TcpConnectionWeakptr& cur) const
        {
            return cur.owner_before(conn_) || conn_.owner_before(cur);
        }
    private:
        TcpConnectionWeakptr conn_;
    };

    EventLoop* loop_;
    std::set<TcpConnectionWeakptr, std::owner_less<TcpConnectionWeakptr>> connections_;
};

} // namespace net

#endif // NET_BROADCAST_SERVICE_H
