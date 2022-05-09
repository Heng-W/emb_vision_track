#ifndef NET_TCP_CLIENT_H
#define NET_TCP_CLIENT_H

#include <mutex>
#include "tcp_connection.h"

namespace net
{

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient
{
public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr);
    ~TcpClient();

    DISALLOW_COPY_AND_ASSIGN(TcpClient);

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }

    EventLoop* getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }

    void setName(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }

    // 设置回调函数（非线程安全）
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr connector_;
    TcpConnectionPtr connection_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    bool retry_;   // atomic
    bool connect_; // atomic
    int nextConnId_;
    std::string name_;
    mutable std::mutex mutex_;
};

} // namespace net

#endif // NET_TCP_CLIENT_H

