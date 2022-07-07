#ifndef EVT_CONTROL_EVENT_LOOP_H
#define EVT_CONTROL_EVENT_LOOP_H

#include <vector>
#include <atomic>
#include <functional>
#include <condition_variable>

#include "field_control.h"
#include "motion_control.h"

namespace evt
{

class ControlEventLoop
{
public:
    using Functor = std::function<void()>;

    ControlEventLoop();
    ~ControlEventLoop() = default;

    DISALLOW_COPY_AND_ASSIGN(ControlEventLoop);

    void loop(); // 事件循环

    void quit()
    {
        quit_ = true;
        notEmpty_.notify_one();
    }

    template <class Fn, class... Args>
    void queueInLoop(Fn&& fn, Args&& ... args)
    {
        auto cb = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingFunctors_.emplace_back(std::move(cb));
        }
        notEmpty_.notify_one();
    }

    FieldControl& fieldControl() { return fieldControl_; }
    MotionControl& motionControl() { return motionControl_; }

private:

    FieldControl fieldControl_;
    MotionControl motionControl_;

    std::vector<Functor> pendingFunctors_;

    std::atomic<bool> quit_;

    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
};

} // namespace evt

#endif // EVT_CONTROL_EVENT_LOOP_H
