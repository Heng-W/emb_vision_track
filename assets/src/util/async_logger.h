#ifndef UTIL_ASYNC_LOGGER_H
#define UTIL_ASYNC_LOGGER_H

#include <assert.h>
#include <string.h>
#include <vector>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace util
{

class AsyncLogger
{
public:

    AsyncLogger(const std::string& basename,
                off_t rollSize,
                int flushInterval = 3);

    ~AsyncLogger()
    {
        if (running_)
        {
            stop();
        }
    }

    void append(const char* logline, int len);

    void start()
    {
        running_ = true;
        thread_.reset(new std::thread([this] { threadFunc(); }));
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_->join();
    }

private:

    void threadFunc();

    template <int SIZE>
    class FixedBuffer
    {
    public:
        FixedBuffer() : cur_(data_) {}

        void append(const char*  buf, size_t len)
        {
            assert(static_cast<size_t>(avail()) > len);
            memcpy(cur_, buf, len);
            cur_ += len;
        }

        const char* data() const { return data_; }
        int length() const { return static_cast<int>(cur_ - data_); }

        char* current() { return cur_; }
        int avail() const { return static_cast<int>(end() - cur_); }
        void add(size_t len) { cur_ += len; }

        void reset() { cur_ = data_; }
        void fill(int val) { memset(data_, val, sizeof data_); }

    private:
        const char* end() const { return data_ + sizeof data_; }
        char data_[SIZE];
        char* cur_;
    };

    // using Buffer = std::array<uint8_t, 40960>;
    using Buffer = FixedBuffer<40960>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;

    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    BufferPtr currentBuffer_;
    BufferVector buffers_;
};

} // namespace util

#endif // UTIL_ASYNC_LOGGER_H
