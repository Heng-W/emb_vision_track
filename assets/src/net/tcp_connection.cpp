
#include "tcp_connection.h"

#include <errno.h>
#include "../util/logger.h"
#include "event_loop.h"
#include "socket.h"
#include "channel.h"

namespace net
{

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG(TRACE) << conn->localAddr().toIpPort() << " -> "
               << conn->peerAddr().toIpPort() << " is "
               << (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr&, Buffer* buf, Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, int64_t id, int sockfd, const InetAddress& peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      id_(id),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(sockets::getLocalAddr(sockfd)),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
    channel_->setReadCallback([this](Timestamp t) { this->handleRead(t); });
    channel_->setWriteCallback([this] { this->handleWrite(); });
    channel_->setCloseCallback([this] { this->handleClose(); });
    channel_->setErrorCallback([this] { this->handleError(); });
    socket_->setKeepAlive(true);
    LOG(DEBUG) << "TcpConnection::constructor[" <<  id_ << "] at " << this << " fd=" << sockfd;
}

TcpConnection::~TcpConnection()
{
    LOG(DEBUG) << "TcpConnection::destructor[" <<  id_ << "] at " << this
               << " fd=" << channel_->fd()
               << " state=" << stateToString();
    assert(state_ == kDisconnected);
}

void TcpConnection::send(const void* data, int len)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(data, len);
        }
        else
        {
            auto pMessage = std::make_shared<std::string>(static_cast<const char*>(data), len);
            loop_->queueInLoop([this, pMessage]
            {
                this->sendInLoop(pMessage->data(), pMessage->size());
            });
        }
    }
}

void TcpConnection::send(std::string&& message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message.data(), message.size());
        }
        else
        {
            auto pMessage = std::make_shared<std::string>(std::move(message));
            loop_->queueInLoop([this, pMessage]
            {
                this->sendInLoop(pMessage->data(), pMessage->size());
            });
        }
    }
}

void TcpConnection::send(Buffer&& buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.peek(), buf.readableBytes());
        }
        else
        {
            auto pBuf = std::make_shared<Buffer>(std::move(buf));
            loop_->queueInLoop([this, pBuf]
            {
                this->sendInLoop(pBuf->peek(), pBuf->readableBytes());
            });
        }
    }
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        LOG(WARN) << "disconnected, give up writing";
        return;
    }
    // 输出缓冲区为空，直接发送
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                SYSLOG(ERROR) << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ &&
                oldLen < highWaterMark_ &&
                highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop([this] { this->shutdownInLoop(); });
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        handleClose();
    }
}

void TcpConnection::startRead()
{
    loop_->runInLoop([this]
    {
        loop_->assertInLoopThread();
        if (!reading_ || !channel_->isReading())
        {
            channel_->enableReading();
            reading_ = true;
        }
    });
}

void TcpConnection::stopRead()
{
    loop_->runInLoop([this]
    {
        loop_->assertInLoopThread();
        if (reading_ || channel_->isReading())
        {
            channel_->disableReading();
            reading_ = false;
        }
    });
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    int n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        SYSLOG(ERROR) << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        int n = sockets::write(channel_->fd(), outputBuffer_.peek(),
                               outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            SYSLOG(ERROR) << "TcpConnection::handleWrite";
        }
    }
    else
    {
        LOG(TRACE) << "Connection fd = " << channel_->fd()
                   << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG(TRACE) << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    // must be the last line
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG(ERROR) << "TcpConnection::handleError [" << id_
               << "] - SO_ERROR = " << err << " " << strerror(err);
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
    return socket_->getTcpInfo(tcpi);
}

std::string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof(buf));
    return buf;
}

const char* TcpConnection::stateToString() const
{
    switch (state_)
    {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

} // namespace net
