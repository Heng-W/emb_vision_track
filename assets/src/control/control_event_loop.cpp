
#include "control_event_loop.h"

#include <algorithm>
#include "../util/timestamp.h"

using namespace util;


namespace evt
{

ControlEventLoop::ControlEventLoop()
    : quit_(false)
{
}


void ControlEventLoop::loop()
{
    while (!quit_)
    {
        auto now = Timestamp::now();
        static Timestamp expiration = now;

        std::vector<Functor> functors;
        int64_t waitTimeMs = std::max((expiration - now).toMsec(), static_cast<int64_t>(0));
        {
            std::unique_lock<std::mutex> lock(mutex_);
            notEmpty_.wait_for(lock, std::chrono::milliseconds(waitTimeMs), [this]
            { return !pendingFunctors_.empty() || quit_; });
            functors.swap(pendingFunctors_);
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (expiration <= now)
        {
            expiration = now + util::msec(50);
            motionControl_.perform();
            fieldControl_.perform();
        }
        for (const Functor& functor : functors) functor();
    }
}


} // namespace evt
