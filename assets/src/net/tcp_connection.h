#ifndef NET_TCP_CONNECTION_H
#define NET_TCP_CONNECTION_H

#include "../util/common.h"
#include "../util/any.hpp"
#include "callbacks.h"
#include "buffer.h"
#include "inet_address.h"

struct tcp_info; // in <netinet/tcp.h>

namespace net
{

class EventLoop;
class Socket;
class Channel;

// TCP连接（接口类）, 可供服务端和客户端使用
class TcpConnection: public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, int64_t id, int sockfd, const InetAddress& peerAddr);
    ~TcpConnection();

    DISALLOW_COPY_AND_ASSIGN(TcpConnection);

    void send(const void* data, int len);
    void send(std::string&& message);
    void send(Buffer&& buf);

    void shutdown();
    void forceClose();

    void startRead();
    void stopRead();

    void connectEstablished();
    void connectDestroyed();

    void setTcpNoDelay(bool on);
    bool getTcpInfo(struct tcp_info*) const;
    std::string getTcpInfoString() const;

    // set callbacks
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }


    EventLoop* getLoop() const { return loop_; }
    int64_t id() const { return id_; }
    const InetAddress& localAddr() const { return localAddr_; }
    const InetAddress& peerAddr() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    Buffer* inputBuffer() { return &inputBuffer_; }
    Buffer* outputBuffer() { return &outputBuffer_; }

    void setContext(const util::Any& context) { context_ = context; }
    void setContext(util::Any&& context) { context_ = std::move(context); }
    const util::Any& getContext() const { return context_; }
    util::Any* getMutableContext() { return &context_; }

private:
    enum State { kDisconnected, kConnecting, kConnected, kDisconnecting };

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();
    void forceCloseInLoop();

    void setState(State state) { state_ = state; }

    const char* stateToString() const;


    EventLoop* loop_;
    int64_t id_;
    State state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;

    // callbacks
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    util::Any context_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

} // namespace net

#endif // NET_TCP_CONNECTION_H

