#ifndef UTIL_SINGLETON_HPP
#define UTIL_SINGLETON_HPP

#include <assert.h>
#include <mutex>

namespace util
{

template <class T>
class Singleton
{
public:
    Singleton() = delete;
    ~Singleton() = delete;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    template <typename... Args>
    static T& instance(Args&&... args)
    {
        std::call_once(onceFlag_, [&]
        {
            instance_ = new T(std::forward<Args>(args)...);
            ::atexit(destroy);
        });
        assert(instance_ != nullptr);
        return *instance_;
    }

private:
    static void destroy()
    {
        delete instance_;
        instance_ = nullptr;
    }

    static std::once_flag onceFlag_;
    static T* instance_;
};

template <class T>
std::once_flag Singleton<T>::onceFlag_;

template <class T>
T* Singleton<T>::instance_ = nullptr;

} // namespace util

#endif // UTIL_SINGLETON_HPP
