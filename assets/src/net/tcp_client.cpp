
#include "tcp_client.h"

#include <stdio.h> // snprintf
#include "../util/logger.h"
#include "connector.h"
#include "event_loop.h"
#include "socket.h"

namespace net
{

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(CHECK_NOTNULL(loop)),
      connector_(new Connector(loop, serverAddr)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1),
      name_("Tcp Client")
{
    connector_->setNewConnectionCallback([this](int sockfd) { this->newConnection(sockfd); });
    LOG(INFO) << "TcpClient::TcpClient[" << name_
              << "] - connector " << connector_.get();
}

TcpClient::~TcpClient()
{
    LOG(INFO) << "TcpClient::~TcpClient[" << name_
              << "] - connector " << connector_.get();
    TcpConnectionPtr conn;
    bool unique = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn)
    {
        assert(loop_ == conn->getLoop());
        loop_->runInLoop([this, conn]
        {
            conn->setCloseCallback([this](const TcpConnectionPtr & conn)
            {
                loop_->queueInLoop([conn] { conn->connectDestroyed(); });
            });
        });

        if (unique)
        {
            conn->forceClose();
        }
    }
    else
    {
        connector_->stop();
    }
}

void TcpClient::connect()
{
    LOG(INFO) << "TcpClient::connect[" << name_ << "] - connecting to "
              << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();

    InetAddress peerAddr = sockets::getPeerAddr(sockfd);
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, nextConnId_, sockfd, peerAddr);
    ++nextConnId_;

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr & conn) { this->removeConnection(conn); });

    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop([conn] { conn->connectDestroyed(); });
    if (retry_ && connect_)
    {
        LOG(INFO) << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                  << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}

} // namespace net

