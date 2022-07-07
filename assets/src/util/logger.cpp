
#include "logger.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include "timestamp.h"

namespace util
{

static const char* LogLevelName[] =
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

Logger::LogLevel Logger::minLogLevel_ = []() -> Logger::LogLevel
{
    if (::getenv("LOG_TRACE"))
        return Logger::TRACE;
    else if (::getenv("LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}();

Logger::OutputFunc Logger::output_ = [](const char* msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
    (void)n;
};

Logger::FlushFunc Logger::flush_ = [] { fflush(stdout); };


Logger::Logger(LogLevel level, int savedErrno, const char* file, int line)
    : level_(level)
{
    stream_ << Timestamp::now().toFormattedString() << " ";
    stream_ << LogLevelName[level];
    stream_ << std::this_thread::get_id() << " ";
    stream_ << file << ':' << line << "] ";
    if (savedErrno != 0)
    {
        stream_ << strerror(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

Logger::Logger(const char* file, int line, LogLevel level, const char* func)
    : Logger(level, 0, file, line) { stream_ << func << ' '; }

Logger::Logger(const char* file, int line, LogLevel level, bool sysLog)
    : Logger(level, sysLog ? errno : 0, file, line) {}

Logger::~Logger()
{
    stream_ << '\n';
    output_(stream_.pbase(), stream_.pcount());
    if (level_ == FATAL)
    {
        flush_();
        abort();
    }
}

} // namespace util

