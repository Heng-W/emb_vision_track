
#include "event_loop_thread_pool.h"

#include <assert.h>
#include <stdio.h>
#include <sstream>
#include "../util/count_down_latch.h"
#include "event_loop.h"

namespace net
{

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
    : baseLoop_(baseLoop),
      started_(false),
      numThreads_(0),
      next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    for (auto& loop : loops_)
    {
        if (loop) loop->quit();
    }
    for (auto& thread : threads_)
    {
        thread->join();
    }
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    assert(!started_);
    assert(numThreads_ >= 0);
    baseLoop_->assertInLoopThread();

    started_ = true;

    if (numThreads_ == 0)
    {
        if (cb) cb(baseLoop_);
        return;
    }

    util::CountDownLatch latch(numThreads_);
    loops_.resize(numThreads_);

    for (int i = 0; i < numThreads_; ++i)
    {
        threads_.emplace_back(new std::thread([this, i, &cb, &latch]
        {
            EventLoop loop;

            if (cb) cb(&loop);
            loops_[i] = &loop;

            latch.countDown();

            loop.loop();

            loops_[i] = nullptr;
        }));
    }
    latch.wait();
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = baseLoop_;

    if (!loops_.empty())
    {
        // round-robin
        loop = loops_[next_++];
        if (next_ >= static_cast<int>(loops_.size())) next_ = 0;
    }
    return loop;
}

std::string EventLoopThreadPool::info() const
{
    std::ostringstream oss;
    oss << "print threads id info " << std::endl;
    for (size_t i = 0; i < threads_.size(); ++i)
    {
        oss << i << ": id = " << threads_[i]->get_id() << std::endl;
    }
    return oss.str();
}

} // namespace net

