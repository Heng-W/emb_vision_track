#ifndef UTIL_LOGGER_H
#define UTIL_LOGGER_H

#include <functional>
#include "log_stream.h"

namespace util
{

class Logger
{
public:
    using OutputFunc = std::function<void(const char* msg, int len)>;
    using FlushFunc = std::function<void()>;

    // 日志等级
    enum LogLevel {TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF};

    Logger(const char* file, int line, LogLevel level, bool sysLog = false);
    Logger(const char* file, int line, LogLevel level, const char* func);
    ~Logger();

    LogStream& stream() { return stream_; }

    static LogLevel minLogLevel() { return minLogLevel_; }
    static void setMinLogLevel(LogLevel level) { minLogLevel_ = level; }
    static void setOutput(const OutputFunc& output) { output_ = output; }
    static void setFlush(const FlushFunc& flush) { flush_ = flush; }

private:
    Logger(LogLevel level, int savedErrno, const char* file, int line);

    static LogLevel minLogLevel_;
    static OutputFunc output_;
    static FlushFunc flush_;

    LogStream stream_;
    LogLevel level_;
};


// 参考glog实现，此类用于显式忽略条件日志宏中的值
// 避免如“未使用计算值”、“语句无效”之类的编译器警告
struct LogMessageVoidify
{
    // 使用的运算符优先级应低于<<但高于？:
    void operator&(std::ostream&) {}
};

// 内部使用的宏
#define M_LOG_TRACE util::Logger(__FILE__, __LINE__, util::Logger::TRACE, __func__)
#define M_LOG_DEBUG util::Logger(__FILE__, __LINE__, util::Logger::DEBUG, __func__)
#define M_LOG_INFO util::Logger(__FILE__, __LINE__, util::Logger::INFO)
#define M_LOG_WARN util::Logger(__FILE__, __LINE__, util::Logger::WARN)
#define M_LOG_ERROR util::Logger(__FILE__, __LINE__, util::Logger::ERROR)
#define M_LOG_FATAL util::Logger(__FILE__, __LINE__, util::Logger::FATAL)

#define M_LOG_SYS_ERROR util::Logger(__FILE__, __LINE__, util::Logger::ERROR, true)
#define M_LOG_SYS_FATAL util::Logger(__FILE__, __LINE__, util::Logger::FATAL, true)

#define M_LOG(level) M_LOG_ ## level.stream()
#define M_SYSLOG(level) M_LOG_SYS_ ## level.stream()

#define M_LOG_IF(level, condition) \
    !(condition) ? (void)0 : util::LogMessageVoidify() & M_LOG(level)
#define M_SYSLOG_IF(level, condition) \
    !(condition) ? (void)0 : util::LogMessageVoidify() & M_SYSLOG(level)

// 用户宏
#define LOG_IS_ON(level) (util::Logger::level >= util::Logger::minLogLevel())

#define LOG(level) M_LOG_IF(level, LOG_IS_ON(level))
#define LOG_IF(level, condition) M_LOG_IF(level, (condition) && LOG_IS_ON(level))

#define SYSLOG(level) M_SYSLOG_IF(level, LOG_IS_ON(level))
#define SYSLOG_IF(level, condition) M_SYSLOG_IF(level, (condition) && LOG_IS_ON(level))

#ifndef NDEBUG
#define DLOG(level) LOG(level)
#define DLOG_IF(level, condition) LOG_IF(level, condition)
#else // release模式下不输出
#define DLOG(level) LOG_IF(level, false)
#define DLOG_IF(level, condition) LOG_IF(level, false)
#endif

// 检查输入非空
#define CHECK_NOTNULL(val) \
    ::util::checkNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

template <typename T>
inline T* checkNotNull(const char* file, int line, const char* names, T* ptr)
{
    if (ptr == nullptr)
    {
        Logger(file, line, Logger::FATAL).stream() << names;
    }
    return ptr;
}

} // namespace util

#endif // UTIL_LOGGER_H
