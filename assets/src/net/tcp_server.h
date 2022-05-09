#ifndef NET_TCP_SERVER_H
#define NET_TCP_SERVER_H

#include <atomic>
#include <unordered_map>
#include "tcp_connection.h"

namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

// TCP服务器（接口类），支持单线程和线程池模型
class TcpServer
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option { kNoReusePort, kReusePort };

    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              Option option = kNoReusePort);
    ~TcpServer();

    DISALLOW_COPY_AND_ASSIGN(TcpServer);

    /**
     * 设置处理线程数
     *
     * 始终在loop所属线程中接受新连接
     * 必须在调用start()前调用此函数
     * @param numThreads
     * - 0 表示所有I/O在loop所属线程中，不会创建线程（默认值）
     * - 1 表示所有I/O在另一个线程中
     * - N 表示一个具有N个线程的线程池，以轮询方式（round-robin）分配新连接
     */
    void setThreadNum(int numThreads);

    // 启动服务器（线程安全）
    void start();

    // set callbacks
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setThreadInitCallback(const ThreadInitCallback& cb)
    { threadInitCallback_ = cb; }

    // 在调用start()后有效
    std::shared_ptr<EventLoopThreadPool> threadPool() const { return threadPool_; }

    EventLoop* getLoop() const { return loop_; }
    const std::string& ipPort() const { return ipPort_; }

    void setName(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }

private:
    using ConnectionMap = std::unordered_map<int64_t, TcpConnectionPtr>;

    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);


    EventLoop* loop_; // acceptor loop
    const std::string ipPort_;
    std::string name_;

    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    // callbacks
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;

    std::atomic<int> started_;
    int64_t nextConnId_;
    ConnectionMap connections_;
};

} // namespace net

#endif // NET_TCP_SERVER_H
