
#include "async_logger.h"

#ifdef _WIN32
#include <process.h>
#include <Winsock2.h>
#else
#include <unistd.h>
#endif
#include "timestamp.h"

using std::string;

namespace util
{

namespace
{

void writeToFile(const char* buf, int len, FILE* fp)
{
    int written = 0;
    while (written != len)
    {
        int remain = len - written;
#ifdef _WIN32
        int n = ::fwrite(buf + written, 1, remain, fp);
#else
        int n = ::fwrite_unlocked(buf + written, 1, remain, fp);
#endif
        if (n != remain)
        {
            int err = ferror(fp);
            if (err)
            {
                fprintf(stderr, "write failed %s\n", strerror(err));
                break;
            }
        }
        written += n;
    }
}

string getLogFileName(const string& basename, time_t* now)
{
    string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    *now = time(nullptr);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, now);
#else
    localtime_r(now, &tm);
#endif
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;

    char hostbuf[256];
    ::gethostname(hostbuf, sizeof(hostbuf));
    filename += hostbuf;

    char pidbuf[32];
    snprintf(pidbuf, sizeof(pidbuf), ".%d", getpid());
    filename += pidbuf;

    filename += ".log";

    return filename;
}

} // namespace

AsyncLogger::AsyncLogger(const std::string& basename,
                         off_t rollSize,
                         int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      currentBuffer_(new Buffer()),
      buffers_()
{
    currentBuffer_->fill(0);
    buffers_.reserve(16);
}


void AsyncLogger::append(const char* logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        buffers_.push_back(std::move(currentBuffer_));

        currentBuffer_.reset(new Buffer());

        currentBuffer_->append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLogger::threadFunc()
{
    time_t lastRoll_;
    string filename = getLogFileName(basename_, &lastRoll_);
    FILE* fp = ::fopen(filename.c_str(), "ae");

    BufferPtr newBuffer(new Buffer);
    newBuffer->fill(0);
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_)
    {
        assert(newBuffer && newBuffer->length() == 0);
        assert(buffersToWrite.empty());

        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty())
            {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer);
            buffersToWrite.swap(buffers_);
        }

        assert(!buffersToWrite.empty());

        time_t now;
        string filename = getLogFileName(basename_, &now);

        if (now > lastRoll_)
        {
            lastRoll_ = now;
            ::fclose(fp);
            fp = ::fopen(filename.c_str(), "ae");
        }

        if (buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     buffersToWrite.size() - 1);
            fputs(buf, stderr);
            writeToFile(buf, strlen(buf), fp);
            buffersToWrite.erase(buffersToWrite.begin() + 1, buffersToWrite.end());
        }

        for (const auto& buffer : buffersToWrite)
        {
            writeToFile(buffer->data(), buffer->length(), fp);
        }

        buffersToWrite.resize(1);

        if (!newBuffer)
        {
            newBuffer = std::move(buffersToWrite.back());
            newBuffer->reset();
            buffersToWrite.pop_back();
        }

        buffersToWrite.clear();
        ::fflush(fp);
    }
    ::fflush(fp);
    ::fclose(fp);
}

} // namespace util

