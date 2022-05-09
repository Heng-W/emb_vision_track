#ifndef NET_EVENT_LOOP_THREAD_POOL_H
#define NET_EVENT_LOOP_THREAD_POOL_H

#include <functional>
#include <vector>
#include <thread>
#include "../util/common.h"

namespace net
{

class EventLoop;

class EventLoopThreadPool
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool();

    DISALLOW_COPY_AND_ASSIGN(EventLoopThreadPool);

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback& cb = nullptr);

    // round-robin
    EventLoop* getNextLoop();

    std::string info() const;

    bool started() const { return started_; }

private:
    EventLoop* baseLoop_; // 主线程的eventLoop
    bool started_;
    int numThreads_;
    int next_; // loop索引
    std::vector<std::unique_ptr<std::thread>> threads_;
    std::vector<EventLoop*> loops_;
};

} // namespace net

#endif // NET_EVENT_LOOP_THREAD_POOL_H
