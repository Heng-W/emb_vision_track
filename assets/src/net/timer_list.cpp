
#include "timer_list.h"

#include <atomic>
#include "event_loop.h"

namespace net
{

class Timer
{
public:
    Timer(const TimerCallback& cb, Timestamp when, int64_t periodMs = 0)
        : callback_(cb),
          expiration_(when),
          periodMs_(periodMs),
          id_(++numCreated_)
    {}

    DISALLOW_COPY_AND_ASSIGN(Timer);

    void run() const { callback_(); }

    int64_t id() const { return id_; }
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return periodMs_ > 0; }

    void restart(Timestamp now)
    { expiration_ = repeat() ? (now + util::msec(periodMs_)) : Timestamp::invalid(); }

    static int64_t numCreated() { return numCreated_; }

private:
    static std::atomic<int64_t> numCreated_;

    TimerCallback callback_;
    Timestamp expiration_;
    int64_t periodMs_;
    int64_t id_;
};

std::atomic<int64_t> Timer::numCreated_(0);


TimerList::TimerList(EventLoop* loop)
    : loop_(loop),
      timers_()
{
}

TimerList::~TimerList()
{
    for (const Entry& timer : timers_)
    {
        delete timer.second;
    }
}

int64_t TimerList::addTimer(const TimerCallback& cb, Timestamp when, int64_t periodMs)
{
    Timer* timer = new Timer(cb, when, periodMs);
    loop_->runInLoop([this, timer]
    {
        timers_.insert({timer->expiration(), timer});
        activeTimers_[timer->id()] = timer;
        assert(timers_.size() == activeTimers_.size());
    });
    return timer->id();
}

void TimerList::removeTimer(int64_t timerId)
{
    loop_->runInLoop([this, timerId]
    {
        auto it = activeTimers_.find(timerId);
        if (it == activeTimers_.end()) return; // id无效

        Timer* timer = it->second;
        timers_.erase({timer->expiration(), timer});
        activeTimers_.erase(it);
        delete timer;
    });
}

void TimerList::doTimerEvent()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());

    for (auto it = timers_.begin(); it != timers_.end();)
    {
        Timer* timer = it->second;
        if (timer->expiration() <= now)
        {
            timer->run();
            it = timers_.erase(it);
            if (!timer->repeat())
            {
                activeTimers_.erase(timer->id());
                delete timer;
            }
            else
            {
                timer->restart(now);
                timers_.insert({timer->expiration(), timer});
            }
        }
        else
        {
            break;
        }
    }
}

Timestamp TimerList::nextExpiration() const
{
    loop_->assertInLoopThread();
    return !timers_.empty() ? timers_.begin()->second->expiration() : Timestamp::invalid();
}

} // namespace net
