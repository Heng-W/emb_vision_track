#ifndef UTIL_COUNT_DOWN_LATCH_H
#define UTIL_COUNT_DOWN_LATCH_H

#include <condition_variable>
#include "common.h"

namespace util
{

class CountDownLatch
{
public:
    explicit CountDownLatch(int count): count_(count) {}

    DISALLOW_COPY_AND_ASSIGN(CountDownLatch);

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return count_ == 0; });
    }

    void countDown()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (--count_ == 0) cond_.notify_all();
    }

    int getCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    int count_;
};

} // namespace util

#endif // UTIL_COUNT_DOWN_LATCH_H
