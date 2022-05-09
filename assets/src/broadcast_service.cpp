
#include "broadcast_service.h"
#include "util/logger.h"
#include "net/event_loop.h"
#include "net/buffer.h"
#include "net/tcp_connection.h"


namespace net
{

BroadcastService::BroadcastService(EventLoop* loop)
    : loop_(loop)
{
}


void BroadcastService::addConnection(const TcpConnectionPtr& conn)
{
    TcpConnectionWeakptr connWeakptr = conn;
    loop_->runInLoop([this, connWeakptr]()
    {
        connections_.insert(connWeakptr);
    });
}

void BroadcastService::removeConnection(const TcpConnectionPtr& conn)
{
    TcpConnectionWeakptr connWeakptr = conn;
    loop_->runInLoop([this, connWeakptr]()
    {
        connections_.erase(connWeakptr);
    });
}

void BroadcastService::clearConnection()
{
    loop_->runInLoop([this]()
    {
        connections_.clear();
    });
}

void BroadcastService::broadcast(const void* data, int len)
{
    if (loop_->isInLoopThread())
    {
        broadcastInLoop(data, len);
    }
    else
    {
        auto buf = std::make_shared<Buffer>(len, 0);
        buf->append(data, len);
        loop_->queueInLoop([this, buf]
        {
            broadcastInLoop(buf->peek(), buf->readableBytes());
        });
    }
}

void BroadcastService::broadcast(const void* data, int len, const Predicate& pred)
{
    if (loop_->isInLoopThread())
    {
        broadcastInLoop(data, len, pred);
    }
    else
    {
        auto buf = std::make_shared<Buffer>(len, 0);
        buf->append(data, len);
        loop_->queueInLoop([this, buf, pred]
        {
            broadcastInLoop(buf->peek(), buf->readableBytes(), pred);
        });
    }
}

void BroadcastService::broadcast(std::string&& message)
{
    if (loop_->isInLoopThread())
    {
        broadcastInLoop(message.data(), message.size());
    }
    else
    {
        auto msg = std::make_shared<std::string>(std::move(message));
        loop_->queueInLoop([this, msg]
        {
            broadcastInLoop(msg->data(), msg->size());
        });
    }
}

void BroadcastService::broadcast(std::string&& message, const Predicate& pred)
{
    if (loop_->isInLoopThread())
    {
        broadcastInLoop(message.data(), message.size(), pred);
    }
    else
    {
        auto msg = std::make_shared<std::string>(std::move(message));
        loop_->queueInLoop([this, msg, pred]
        {
            broadcastInLoop(msg->data(), msg->size(), pred);
        });
    }
}

void BroadcastService::broadcast(Buffer&& buf)
{
    if (loop_->isInLoopThread())
    {
        broadcastInLoop(buf.peek(), buf.readableBytes());
    }
    else
    {
        auto bufPtr = std::make_shared<Buffer>(std::move(buf));
        loop_->queueInLoop([this, bufPtr]
        {
            broadcastInLoop(bufPtr->peek(), bufPtr->readableBytes());
        });
    }
}

void BroadcastService::broadcast(Buffer&& buf, const Predicate& pred)
{
    if (loop_->isInLoopThread())
    {
        broadcastInLoop(buf.peek(), buf.readableBytes(), pred);
    }
    else
    {
        auto bufPtr = std::make_shared<Buffer>(std::move(buf));
        loop_->queueInLoop([this, bufPtr, pred]
        {
            broadcastInLoop(bufPtr->peek(), bufPtr->readableBytes(), pred);
        });
    }
}

void BroadcastService::broadcastInLoop(const void* data, int len)
{
    assert(loop_->isInLoopThread());
    for (const auto& x : connections_)
    {
        auto guard = x.lock();
        if (guard) guard->send(data, len);
    }
}

void BroadcastService::broadcastInLoop(const void* data, int len, const Predicate& pred)
{
    assert(loop_->isInLoopThread());
    for (const auto& x : connections_)
    {
        if (pred(x))
        {
            auto guard = x.lock();
            if (guard) guard->send(data, len);
        }
    }
}

} // namespace net
