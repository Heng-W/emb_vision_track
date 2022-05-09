#ifndef NET_TIMER_QUEUE_H
#define NET_TIMER_QUEUE_H

#include <set>
#include <unordered_map>
#include "channel.h"
#include "callbacks.h"

namespace net
{

class EventLoop;
class Timer;

class TimerList
{
public:
    TimerList(EventLoop* loop);
    ~TimerList();

    DISALLOW_COPY_AND_ASSIGN(TimerList);

    // 线程安全
    int64_t addTimer(const TimerCallback& cb, Timestamp when, int64_t interval = 0);
    void removeTimer(int64_t timerId);

    void handleRead();

private:
    using Entry = std::pair<Timestamp, Timer*>;
    using Timers = std::set<Entry>;
    using ActiveTimers = std::unordered_map<int64_t, Timer*>;

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;

    // timers sorted by expiration
    Timers timers_;
    ActiveTimers activeTimers_;
};

} // namespace net

#endif // NET_TIMER_QUEUE_H
