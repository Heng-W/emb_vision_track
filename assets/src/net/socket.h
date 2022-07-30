#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "../util/common.h"

struct tcp_info; // in <netinet/tcp.h>

namespace net
{

class InetAddress;

namespace sockets
{

int createTcp();
int createTcpNonBlock();
int createUdp();
int createUdpNonBlock();
void setNonBlock(int sockfd, bool on = true);

void bind(int sockfd, const InetAddress& addr);
void listen(int sockfd);
int accept(int sockfd, InetAddress* peerAddr);

int connect(int sockfd, const InetAddress& addr);

int read(int sockfd, void* buf, int len);
int write(int sockfd, const void* buf, int len);
void close(int sockfd);

int recvByUdp(int sockfd, void* buf, int len, InetAddress* peerAddr);
int sendByUdp(int sockfd, const void* buf, int len, const InetAddress& addr);

void shutdownWrite(int sockfd);
void shutdownRead(int sockfd);
void shutdownReadWrite(int sockfd);

void setKeepAlive(int sockfd, bool on);
void setTcpNoDelay(int sockfd, bool on);
void setReuseAddr(int sockfd, bool on);
void setReusePort(int sockfd, bool on);

int getSocketError(int sockfd);

InetAddress getLocalAddr(int sockfd);
InetAddress getPeerAddr(int sockfd);
bool isSelfConnect(int sockfd);

bool getTcpInfo(int sockfd, struct tcp_info* tcpi);
bool getTcpInfoString(int sockfd, char* buf, int len);

} // namespace sockets


class Socket
{
public:
    explicit Socket(int sockfd): sockfd_(sockfd) {}
    ~Socket() { sockets::close(sockfd_); }

    DISALLOW_COPY_AND_ASSIGN(Socket);

    void bind(const InetAddress& localAddr) { sockets::bind(sockfd_, localAddr); }
    void listen() { sockets::listen(sockfd_); }
    int accept(InetAddress* peerAddr) { return sockets::accept(sockfd_, peerAddr); }

    void shutdownWrite() { sockets::shutdownWrite(sockfd_); }

    void setKeepAlive(bool on) { sockets::setKeepAlive(sockfd_, on); }
    void setTcpNoDelay(bool on) { sockets::setTcpNoDelay(sockfd_, on); }
    void setReuseAddr(bool on) { sockets::setReuseAddr(sockfd_, on); }
    void setReusePort(bool on) { sockets::setReusePort(sockfd_, on); }

    bool getTcpInfo(struct tcp_info* tcpi) const
    { return sockets::getTcpInfo(sockfd_, tcpi); }

    bool getTcpInfoString(char* buf, int len) const
    { return sockets::getTcpInfoString(sockfd_, buf, len); }

    int fd() const { return sockfd_; }

private:
    const int sockfd_;
};

} // namespace net

#endif // NET_SOCKET_H
