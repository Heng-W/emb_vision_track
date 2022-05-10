
#include "connector.h"

#include <string.h>
#include <errno.h>
#include "../util/logger.h"
#include "channel.h"
#include "event_loop.h"
#include "socket.h"

namespace net
{

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs),
      timerId_(-1)
{
    LOG(DEBUG) << "constructor[" << this << "]";
}

Connector::~Connector()
{
    LOG(DEBUG) << "destructor[" << this << "]";
    assert(!channel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(&Connector::startInLoop, this);
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG(DEBUG) << "do not connect";
    }
}

void Connector::stop()
{
    connect_ = false;
    if (timerId_ >= 0) loop_->removeTimer(timerId_);
    loop_->queueInLoop([this]
    {
        if (state_ == kConnecting)
        {
            setState(kDisconnected);
            int sockfd = removeAndResetChannel();
            retry(sockfd);
        }
    });
}


void Connector::connect()
{
    int sockfd = sockets::createTcpNonBlock();
    int ret = sockets::connect(sockfd, serverAddr_);
    int savedErrno = (ret == 0) ? 0 : errno;

    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            SYSLOG(ERROR) << "connect error in Connector::connect";
            sockets::close(sockfd);
            break;
        default:
            SYSLOG(ERROR) << "Unexpected error in Connector::connect";
            sockets::close(sockfd);
            break;
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback([this] { this->handleWrite(); });
    channel_->setErrorCallback([this] { this->handleError(); });
    channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop([this] { channel_.reset(); });
    return sockfd;
}

void Connector::handleWrite()
{
    LOG(TRACE) << "Connector::handleWrite " << state_;

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err)
        {
            LOG(WARN) << "Connector::handleWrite - SO_ERROR = "
                      << err << " " << strerror(err);
            retry(sockfd);
        }
        else if (sockets::isSelfConnect(sockfd))
        {
            LOG(WARN) << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else
    {
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG(ERROR) << "Connector::handleError state=" << state_;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG(TRACE) << "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG(INFO) << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
                  << " in " << retryDelayMs_ << " milliseconds. ";
        timerId_ = loop_->addTimer(std::bind(&Connector::startInLoop, shared_from_this()), retryDelayMs_);
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG(DEBUG) << "do not connect";
    }
}

} // namespace net

