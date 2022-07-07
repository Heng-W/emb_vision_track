#ifndef UTIL_ANY_HPP
#define UTIL_ANY_HPP

#include <algorithm>
#include "type.h"

namespace util
{

class Any
{
    template <class ValueType>
    friend const ValueType* any_cast(const Any* any)
    {
        assert(any && any->content_);
        return &down_cast<Holder<ValueType>*>(any->content_)->held;
    }

public:
    Any(): content_(nullptr) {}
    ~Any() { if (content_) delete content_; }

    template <class ValueType>
    Any(ValueType&& value)
        : content_(new Holder<typename std::decay<ValueType>::type>(
                       std::forward<ValueType>(value))) {}

    Any(const Any& rhs) : content_(rhs.content_ ? rhs.content_->clone() : nullptr) {}
    Any(Any&& rhs) noexcept: content_(rhs.content_) { rhs.content_ = nullptr; }

    Any& operator=(const Any& rhs)
    {
        Any(rhs).swap(*this);
        return *this;
    }

    Any& operator=(Any&& rhs) noexcept
    {
        if (this != &rhs)
        {
            if (content_) delete content_;
            content_ = rhs.content_;
            rhs.content_ = nullptr;
        }
        return *this;
    }

    template <class ValueType>
    Any& operator=(ValueType&& value)
    {
        Any(std::forward<ValueType>(value)).swap(*this);
        return *this;
    }

    void swap(Any& rhs) { std::swap(content_, rhs.content_); }

    // 类型转换重载
    operator const void* () const { return content_; }

    const std::type_info& type_info() const
    { return content_ ? content_->type_info() : typeid(void); }

private:
    struct PlaceHolder
    {
        virtual ~PlaceHolder() = default;
        virtual PlaceHolder* clone() const = 0;
        virtual const std::type_info& type_info() const = 0;
    };

    template <class ValueType>
    struct Holder: PlaceHolder
    {
        Holder(const ValueType& value): held(value) {}
        Holder(ValueType&& value): held(std::move(value)) {}

        PlaceHolder* clone() const override { return new Holder(held); }
        const std::type_info& type_info() const override { return typeid(ValueType); }

        ValueType held;
    };

    PlaceHolder* content_;
};

template <class ValueType>
inline ValueType* any_cast(Any* any)
{
    auto* val = any_cast<ValueType>(const_cast<const Any*>(any));
    return const_cast<ValueType*>(val);
}

template <class ValueType>
inline ValueType any_cast(const Any& any)
{
    using T = typename std::decay<ValueType>::type;
    const T* ptr = any_cast<T>(&any);
    assert(ptr);
    return *ptr;
}

template <class ValueType>
inline ValueType any_cast(Any& any)
{
    using T = typename std::decay<ValueType>::type;
    T* ptr = any_cast<T>(&any);
    assert(ptr);
    return *ptr;
}

template <class ValueType>
inline ValueType any_cast(Any&& any)
{
    static_assert(!std::is_reference<ValueType>::value, "type should not be reference");
    using T = typename std::decay<ValueType>::type;
    T* ptr = any_cast<T>(&any);
    assert(ptr);
    return *ptr;
}

} // namespace util

#endif // UTIL_ANY_HPP

