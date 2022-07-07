
#include "timestamp.h"
#include <stdio.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using std::string;

namespace util
{

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp should be same size as int64_t");

string Timestamp::toString() const
{
    char buf[64];
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%lld.%06lld", (long long int)seconds, (long long int)microseconds);
    return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds) const
{
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    struct tm tm_time = *localtime(&seconds);

    char buf[64];

    if (showMicroseconds)
    {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}

Timestamp Timestamp::now()
{
    using namespace std;
    auto now = chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now());
    int64_t microSeconds = now.time_since_epoch().count();
    return Timestamp(microSeconds);
}

} // namespace util
