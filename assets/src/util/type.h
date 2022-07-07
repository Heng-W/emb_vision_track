#ifndef UTIL_TYPE_H
#define UTIL_TYPE_H

#include <assert.h>
#include <memory>

namespace util
{

template <typename To, typename From>
inline To implicit_cast(const From& f)
{
    return f;
}

// use like this: down_cast<T*>(foo)
template <typename To, typename From>
inline To down_cast(From* f)
{
    if (false)
    {
        implicit_cast<From*, To>(0);
    }
    assert(f == nullptr || dynamic_cast<To>(f) != nullptr); // RTTI: debug mode only
    return static_cast<To>(f);
}

template <typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
    return ptr.get();
}

template <typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
    return ptr.get();
}

template <typename To, typename From>
inline std::shared_ptr<To> down_pointer_cast(const std::shared_ptr<From>& f)
{
    if (false)
    {
        implicit_cast<From*, To*>(0);
    }
    assert(f == nullptr || dynamic_cast<To*>(get_pointer(f)) != nullptr);
    return std::static_pointer_cast<To>(f);
}

template <typename To, typename From, typename Del>
inline std::unique_ptr<To, Del> down_pointer_cast(std::unique_ptr<From, Del>&& f)
{
    if (false)
    {
        implicit_cast<From*, To*>(0);
    }
    assert(f == nullptr || dynamic_cast<To*>(get_pointer(f)) != nullptr);
    auto to = static_cast<To*>(f.release());
    return std::unique_ptr<To, Del>(to, std::move(f.get_deleter()));
}

} // namespace util

#endif // UTIL_TYPE_H
