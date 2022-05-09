
#include "tcp_server.h"

#include <stdio.h> // snprintf
#include "../util/logger.h"
#include "acceptor.h"
#include "event_loop.h"
#include "event_loop_thread_pool.h"
#include "socket.h"

namespace net
{

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     Option option)
    : loop_(CHECK_NOTNULL(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_("Tcp Server"),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      started_(0),
      nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const InetAddress & peerAddr)
    {
        this->newConnection(sockfd, peerAddr);
    });
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG(TRACE) << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop([conn] { conn->connectDestroyed(); });
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(numThreads >= 0);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_.fetch_add(1) == 0)
    {
        threadPool_->start(threadInitCallback_);

        assert(!acceptor_->listening());
        loop_->runInLoop([this] { acceptor_->listen(); });
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    EventLoop* ioLoop = threadPool_->getNextLoop();

    LOG(INFO) << "TcpServer::newConnection [" << name_
              << "] - new connection [" << ipPort_ << "#" << nextConnId_
              << "] from " << peerAddr.toIpPort();

    auto conn = std::make_shared<TcpConnection>(ioLoop, nextConnId_, sockfd, peerAddr);
    connections_[nextConnId_++] = conn;
    // set callbacks
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr & conn) { this->removeConnection(conn); });

    ioLoop->runInLoop([conn] { conn->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop([this, conn]
    {
        loop_->assertInLoopThread();
        LOG(INFO) << "TcpServer::removeConnectionInLoop [" << name_
                  << "] - connection " << ipPort_ << "#" << conn->id();
        size_t n = connections_.erase(conn->id());
        (void)n;
        assert(n == 1);
        EventLoop* ioLoop = conn->getLoop();
        ioLoop->queueInLoop([conn] { conn->connectDestroyed(); });
    });
}

} // namespace net
