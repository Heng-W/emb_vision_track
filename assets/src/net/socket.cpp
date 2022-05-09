
#include "socket.h"

#include <stdio.h> // snprintf
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "../util/logger.h"
#include "inet_address.h"

namespace net
{

namespace sockets
{

static const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr)
{ return reinterpret_cast<const struct sockaddr*>(addr); }

static struct sockaddr* sockaddr_cast(struct sockaddr_in* addr)
{ return reinterpret_cast<struct sockaddr*>(addr); }

int createTcp()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        SYSLOG(FATAL) << "sockets::createTcp";
    }
    return sockfd;
}

int createTcpNonBlock()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        SYSLOG(FATAL) << "sockets::createTcpNonblock";
    }
    return sockfd;
}

int createUdp()
{
    int sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if (sockfd < 0)
    {
        SYSLOG(FATAL) << "sockets::createUdp";
    }
    return sockfd;
}

int createUdpNonBlock()
{
    int sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    if (sockfd < 0)
    {
        SYSLOG(FATAL) << "sockets::createUdpNonBlock";
    }
    return sockfd;
}

void setNonBlock(int sockfd, bool on)
{
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    if (flags < 0)
    {
        SYSLOG(FATAL) << "fcntl F_GETFL";
    }
    flags = on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (::fcntl(sockfd, F_SETFL, flags) < 0)
    {
        SYSLOG(FATAL) << "fcntl F_SETFL";
    }
}

void bind(int sockfd, const InetAddress& addr)
{
    const struct sockaddr_in& sockAddr = addr.getSockAddr();
    int ret = ::bind(sockfd, sockaddr_cast(&sockAddr), static_cast<socklen_t>(sizeof(sockAddr)));
    if (ret < 0)
    {
        SYSLOG(FATAL) << "sockets::bind";
    }
}

void listen(int sockfd)
{
    if (::listen(sockfd, SOMAXCONN) < 0)
    {
        SYSLOG(FATAL) << "sockets::listen";
    }
}

int accept(int sockfd, InetAddress* peerAddr)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));

    int connfd = ::accept4(sockfd, sockaddr_cast(&addr),
                           &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0)
    {
        int savedErrno = errno;
        SYSLOG(ERROR) << "sockets::accept";
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                errno = savedErrno; // expected errors
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                LOG(FATAL) << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG(FATAL) << "unknown error of ::accept " << savedErrno;
                break;
        }
    }
    else
    {
        peerAddr->setSockAddr(addr);
    }
    return connfd;
}

int connect(int sockfd, const InetAddress& addr)
{
    const struct sockaddr_in& sockAddr = addr.getSockAddr();
    return ::connect(sockfd, sockaddr_cast(&sockAddr),
                     static_cast<socklen_t>(sizeof(sockAddr)));
}

int read(int sockfd, void* buf, int len)
{
    return ::read(sockfd, buf, len);
}

int write(int sockfd, const void* buf, int len)
{
    return ::write(sockfd, buf, len);
}

void close(int sockfd)
{
    if (::close(sockfd) < 0)
    {
        SYSLOG(ERROR) << "sockets::close";
    }
}

int recvByUdp(int sockfd, void* buf, int len, InetAddress* peerAddr)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
    int n = ::recvfrom(sockfd, buf, len, 0, sockaddr_cast(&addr), &addrlen);
    peerAddr->setSockAddr(addr);
    return n;
}

int sendByUdp(int sockfd, const void* buf, int len, const InetAddress& addr)
{
    const struct sockaddr_in& sockAddr = addr.getSockAddr();
    return ::sendto(sockfd, buf, len, 0, sockaddr_cast(&sockAddr),
                    static_cast<socklen_t>(sizeof(sockAddr)));
}

void shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        SYSLOG(ERROR) << "sockets::shutdownWrite";
    }
}

void shutdownRead(int sockfd)
{
    if (::shutdown(sockfd, SHUT_RD) < 0)
    {
        SYSLOG(ERROR) << "sockets::shutdownRead";
    }
}

void shutdownReadWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_RDWR) < 0)
    {
        SYSLOG(ERROR) << "sockets::shutdownReadWrite";
    }
}

void setKeepAlive(int sockfd, bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
                 &optval, static_cast<socklen_t>(sizeof(optval)));
}

void setTcpNoDelay(int sockfd, bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
                 &optval, static_cast<socklen_t>(sizeof(optval)));
}

void setReuseAddr(int sockfd, bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof(optval)));
}

void setReusePort(int sockfd, bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        SYSLOG(ERROR) << "SO_REUSEPORT failed.";
    }
#else
    if (on)
    {
        LOG(ERROR) << "SO_REUSEPORT is not supported.";
    }
#endif
}

int getSocketError(int sockfd)
{
    int optval;
    socklen_t optLen = static_cast<socklen_t>(sizeof(optval));
    int ret = ::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optLen);
    return ret < 0 ? errno : optval;
}

InetAddress getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
    if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
    {
        SYSLOG(ERROR) << "sockets::getLocalAddr";
    }
    return localaddr;
}

InetAddress getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    memset(&peeraddr, 0, sizeof(peeraddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
    if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
    {
        SYSLOG(ERROR) << "sockets::getPeerAddr";
    }
    return peeraddr;
}

bool isSelfConnect(int sockfd)
{
    InetAddress localaddr = getLocalAddr(sockfd);
    InetAddress peeraddr = getPeerAddr(sockfd);
    return localaddr.portNetEndian() == peeraddr.portNetEndian() &&
           localaddr.ipNetEndian() == peeraddr.ipNetEndian();
}

bool getTcpInfo(int sockfd, struct tcp_info* tcpi)
{
    socklen_t len = sizeof(*tcpi);
    memset(tcpi, 0, len);
    return ::getsockopt(sockfd, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool getTcpInfoString(int sockfd, char* buf, int len)
{
    struct tcp_info tcpi;
    bool ok = getTcpInfo(sockfd, &tcpi);
    if (ok)
    {
        snprintf(buf, len, "unrecovered=%u "
                 "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                 "lost=%u retrans=%u rtt=%u rttvar=%u "
                 "sshthresh=%u cwnd=%u total_retrans=%u",
                 tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
                 tcpi.tcpi_rto,          // Retransmit timeout in usec
                 tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
                 tcpi.tcpi_snd_mss,
                 tcpi.tcpi_rcv_mss,
                 tcpi.tcpi_lost,         // Lost packets
                 tcpi.tcpi_retrans,      // Retransmitted packets out
                 tcpi.tcpi_rtt,          // Smoothed round trip time in usec
                 tcpi.tcpi_rttvar,       // Medium deviation
                 tcpi.tcpi_snd_ssthresh,
                 tcpi.tcpi_snd_cwnd,
                 tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    }
    return ok;
}

} // namespace sockets

} // namespace net
