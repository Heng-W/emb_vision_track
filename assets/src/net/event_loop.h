#ifndef NET_EVENT_LOOP_H
#define NET_EVENT_LOOP_H

#include <vector>
#include <thread>
#include <mutex>
#include "../util/common.h"
#include "../util/any.hpp"
#include "callbacks.h"

namespace net
{

class Channel;
class Poller;
class TimerList;

// 事件循环（接口类），Reactor模型
class EventLoop
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    DISALLOW_COPY_AND_ASSIGN(EventLoop);

    // 开始循环
    // 必须在创建对象的同一线程中调用
    void loop();

    // 退出循环
    void quit();

    // 在当前loop线程中则立即运行fn，否则加入执行队列（线程安全）
    template <class Fn, class... Args>
    void runInLoop(Fn&& fn, Args&& ... args)
    {
        if (isInLoopThread())
        {
            fn(std::forward<Args>(args)...);
        }
        else
        {
            queueInLoop(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    // 成员函数指针特化
    template <class Tp, class Res, class Class, class... Args>
    void runInLoop(Res(Class::*pmf)(Args...), Tp&& object, Args&& ... args)
    {
        if (isInLoopThread())
        {
            std::mem_fn(pmf)(std::forward<Tp>(object), std::forward<Args>(args)...);
        }
        else
        {
            queueInLoop(pmf, std::forward<Tp>(object), std::forward<Args>(args)...);
        }
    }

    template <class Res, class Class, class... Args>
    void runInLoop(Res(Class::*pmf)(Args...), Class* object, Args&& ... args)
    {
        if (isInLoopThread())
        {
            (object->*pmf)(std::forward<Args>(args)...);
        }
        else
        {
            queueInLoop(pmf, object, std::forward<Args>(args)...);
        }
    }

    // 加入执行队列（线程安全）
    template <class Fn, class... Args>
    void queueInLoop(Fn&& fn, Args&& ... args)
    {
        auto cb = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingFunctors_.emplace_back(std::move(cb));
        }

        if (!isInLoopThread() || callingPendingFunctors_)
        {
            wakeup();
        }
    }

    // 执行队列大小
    size_t queueSize() const;

    // 定时器接口（线程安全）
    int64_t addTimer(const TimerCallback& cb, Timestamp when, int64_t periodMs = 0);
    int64_t addTimer(const TimerCallback& cb, int64_t delayMs, int64_t periodMs = 0);
    void removeTimer(int64_t timerId);

    void assertInLoopThread() const
    {
#ifndef NDEBUG
        if (!isInLoopThread()) abortNotInLoopThread();
#endif
    }

    bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }
    std::thread::id threadId() const { return threadId_; }
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    int64_t iteration() const { return iteration_; }

    void setContext(const util::Any& context) { context_ = context; }
    void setContext(util::Any&& context) { context_ = std::move(context); }
    const util::Any& getContext() const { return context_; }
    util::Any* getMutableContext() { return &context_; }

    static EventLoop* loopInThisThread();

private:
    friend class Channel;

    void abortNotInLoopThread() const;
    void wakeup();
    void handleRead(); // wake up
    void doPendingFunctors();


    bool quit_;
    bool callingPendingFunctors_;
    int64_t iteration_;
    const std::thread::id threadId_;
    Timestamp pollReturnTime_;

    std::unique_ptr<Poller> poller_; // 复用器
    std::unique_ptr<TimerList> timerList_; // 定时器列表

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    util::Any context_;

    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
};

} // namespace net

#endif // NET_EVENT_LOOP_H
