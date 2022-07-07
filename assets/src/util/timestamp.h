#ifndef UTIL_TIMESTAMP_H
#define UTIL_TIMESTAMP_H

#include <stdint.h>
#include <string>

namespace util
{

class Duration;

// 时间戳
class Timestamp
{
public:
    Timestamp(): microSecondsSinceEpoch_(0) {}

    explicit Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

    Timestamp& operator+=(int64_t microSeconds)
    {
        this->microSecondsSinceEpoch_ += microSeconds;
        return *this;
    }

    Timestamp& operator-=(int64_t microSeconds)
    {
        this->microSecondsSinceEpoch_ -= microSeconds;
        return *this;
    }

    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    // for internal usage.
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    time_t secondsSinceEpoch() const
    { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    Duration durationSinceEpoch() const;

    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }

    static Timestamp fromUnixTime(time_t t)
    { return fromUnixTime(t, 0); }

    static Timestamp fromUnixTime(time_t t, int microSeconds)
    { return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microSeconds); }

    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};


class Duration
{
public:
    Duration(): microSeconds_(0) {}

    explicit Duration(int64_t microSeconds): microSeconds_(microSeconds) {}

    int64_t toUsec() const { return microSeconds_; }
    int64_t toMsec() const { return microSeconds_ / 1000; }
    int64_t toSec() const { return microSeconds_ / (1000 * 1000); }
    int64_t toMinute() const { return toSec() / 60; }
    int64_t toHour() const { return toMinute() / 60; }
    int64_t toDay() const { return toHour() / 24; }

private:
    int64_t microSeconds_;
};

inline Duration usec(int64_t val) { return Duration(val); }
inline Duration msec(int64_t val) { return Duration(val * 1000); }
inline Duration sec(int64_t val) { return Duration(val * 1000 * 1000); }
inline Duration minute(int64_t val) { return Duration(val * 1000 * 1000 * 60); }
inline Duration hour(int64_t val) { return Duration(val * 1000 * 1000 * 60 * 60); }
inline Duration day(int64_t val) { return Duration(val * 1000 * 1000 * 60 * 60 * 24); }


inline bool operator<(Timestamp lhs, Timestamp rhs)
{ return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch(); }
inline bool operator>(Timestamp lhs, Timestamp rhs) { return rhs < lhs; }
inline bool operator<=(Timestamp lhs, Timestamp rhs) { return !(lhs > rhs); }
inline bool operator>=(Timestamp lhs, Timestamp rhs) { return !(lhs < rhs); }

inline bool operator==(Timestamp lhs, Timestamp rhs)
{ return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch(); }
inline bool operator!=(Timestamp lhs, Timestamp rhs) { return !(lhs == rhs); }


inline Timestamp operator+(Timestamp timestamp, Duration dur)
{ return Timestamp(timestamp.microSecondsSinceEpoch() + dur.toUsec()); }

inline Timestamp operator+(Duration dur, Timestamp timestamp)
{ return timestamp + dur; }

inline Timestamp operator-(Timestamp timestamp, Duration dur)
{ return Timestamp(timestamp.microSecondsSinceEpoch() - dur.toUsec()); }

inline Duration operator-(Timestamp high, Timestamp low)
{ return usec(high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch()); }

inline Duration Timestamp::durationSinceEpoch() const
{ return usec(microSecondsSinceEpoch_); }

} // namespace util

#endif // UTIL_TIMESTAMP_H

