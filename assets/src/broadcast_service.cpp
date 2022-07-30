
#include "broadcast_service.h"
#include "util/logger.h"
#include "net/event_loop.h"
#include "net/buffer.h"
#include "net/tcp_connection.h"
#include "net/tcp_server.h"

namespace net
{

BroadcastService::BroadcastService(TcpServer* server)
    : server_(server),
      loop_(server->getLoop())
{
}

void BroadcastService::broadcast(const void* data, int len)
{
    if (loop_->isInLoopThread())
    {
        broadcastInLoop(data, len);
    }
    else
    {
        auto buf = std::make_shared<Buffer>(len);
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
        auto buf = std::make_shared<Buffer>(len);
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
    for (const auto& x : server_->connections())
    {
        x.second->send(data, len);
    }
}

void BroadcastService::broadcastInLoop(const void* data, int len, const Predicate& pred)
{
    assert(loop_->isInLoopThread());
    for (const auto& x : server_->connections())
    {
        if (pred(x.second->id()))
        {
            x.second->send(data, len);
        }
    }
}

} // namespace net
