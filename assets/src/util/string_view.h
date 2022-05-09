#ifndef UTIL_STRING_VIEW_H
#define UTIL_STRING_VIEW_H

#include <string.h>
#include <string>
#include <ostream>

namespace util
{

class StringView
{
    friend std::ostream& operator<<(std::ostream& out, const StringView& str)
    {
        for (const char& c : str) out << c;
        return out;
    }

public:
    constexpr StringView(): data_(nullptr), size_(0) {}

    constexpr StringView(const char* str): data_(str), size_(strLen(data_)) {}
    constexpr StringView(const char* data, int size): data_(data), size_(size) {}
    StringView(const std::string& str): data_(str.data()), size_(str.size()) {}


    void reset(const char* data, int size) { data_ = data; size_ = size; }
    void reset(const char* str) { data_ = str; size_ = strLen(str); }

    char operator[](int pos) const { return data_[pos]; }

    std::string toString() const { return std::string(data_, size_); }

    void clear() { data_ = nullptr; size_ = 0; }

    constexpr bool empty() const { return size_ == 0; }
    constexpr const char* data() const { return data_; }
    constexpr int size() const { return size_; }

    constexpr const char* begin() const { return data_; }
    constexpr const char* end() const { return data_ + size_; }

    void removePrefix(int n) { data_ += n; size_ -= n; }
    void removeSuffix(int n) { size_ -= n; }

    int compare(const StringView& rhs) const
    {
        int ret = memcmp(data_, rhs.data_, size_ < rhs.size_ ? size_ : rhs.size_);
        if (ret == 0)
        {
            if (size_ < rhs.size_) ret = -1;
            else if (size_ > rhs.size_) ret = 1;
        }
        return ret;
    }

    bool equal(const StringView& rhs) const
    { return size_ == rhs.size_ && memcmp(data_, rhs.data_, size_) == 0; }

    bool startsWith(const StringView& rhs) const
    { return size_ >= rhs.size_ && memcmp(data_, rhs.data_, rhs.size_) == 0; }

    static constexpr std::size_t hashFunc(const char* str, const char* end, std::size_t res = 0)
    { return str != end ? hashFunc(str + 1, end, res * 131 + *str) : res; }

private:
    static constexpr int strLen(const char* str, int res = 0)
    { return *str ? strLen(str + 1, res + 1) : res; }


    const char* data_;
    int size_;
};


inline bool operator<(const StringView& lhs, const StringView& rhs)
{ return lhs.compare(rhs) < 0; }
inline bool operator>(const StringView& lhs, const StringView& rhs)
{ return rhs < lhs; }
inline bool operator<=(const StringView& lhs, const StringView& rhs)
{ return !(lhs > rhs); }
inline bool operator>=(const StringView& lhs, const StringView& rhs)
{ return !(lhs < rhs); }

inline bool operator==(const StringView& lhs, const StringView& rhs)
{ return lhs.equal(rhs); }
inline bool operator!=(const StringView& lhs, const StringView& rhs)
{ return !(lhs == rhs); }

} // namespace util


namespace std
{

template<>
struct hash<util::StringView>
{
    constexpr size_t operator()(const util::StringView& str) const
    { return util::StringView::hashFunc(str.begin(), str.end()); }
};

} // namespace std

#endif // UTIL_STRING_VIEW_H
