#ifndef NET_DATA_STREAM_H
#define NET_DATA_STREAM_H

#include <assert.h>
#include <string>
#include <algorithm>
#include "net/endian.h"

namespace net
{

class DataStreamWriter
{
public:
    DataStreamWriter(void* buf)
        : cur_(static_cast<uint8_t*>(buf)),
          begin_(cur_)
    {}

    template <typename T>
    void writeFixed64(const T& value)
    {
        static_assert(sizeof(T) == 8, "should be 64 bits");
        *reinterpret_cast<uint64_t*>(cur_) = hostToNetwork64(*reinterpret_cast<const uint64_t*>(&value));
        cur_ += sizeof(T);
    }

    template <typename T>
    void writeFixed32(const T& value)
    {
        static_assert(sizeof(T) == 4, "should be 32 bits");
        *reinterpret_cast<uint32_t*>(cur_) = hostToNetwork32(*reinterpret_cast<const uint32_t*>(&value));
        cur_ += sizeof(T);
    }

    template <typename T>
    void writeFixed16(const T& value)
    {
        static_assert(sizeof(T) == 2, "should be 16 bits");
        *reinterpret_cast<uint16_t*>(cur_) = hostToNetwork16(*reinterpret_cast<const uint16_t*>(&value));
        cur_ += sizeof(T);
    }

    template <typename T>
    void writeFixed8(const T& value)
    {
        static_assert(sizeof(T) == 1, "should be 8 bits");
        *cur_++ = *reinterpret_cast<const uint8_t*>(&value);
    }

    void writeByteArray(const void* data, int len)
    {
        writeFixed32(len);
        cur_ = std::copy_n(static_cast<const uint8_t*>(data), len, cur_);
    }

    void writeString(const std::string& str)
    {
        writeByteArray(str.data(), str.size());
    }

    int size() const { return cur_ - begin_; }
    const uint8_t* cur() const { return cur_; }

private:

    uint8_t* cur_;
    const uint8_t* begin_;
};


class DataStreamReader
{
public:
    DataStreamReader(const void* begin, const void* end)
        : cur_(static_cast<const uint8_t*>(begin)),
          end_(static_cast<const uint8_t*>(end))
    {}

    template <typename T>
    void readFixed64(T* value)
    {
        static_assert(sizeof(T) == 8, "should be 64 bits");
        assert(end_ - cur_ >= static_cast<int>(sizeof(T)));
        *reinterpret_cast<uint64_t*>(value) = networkToHost64(*reinterpret_cast<const uint64_t*>(cur_));
        cur_ += sizeof(T);
    }

    template <typename T>
    void readFixed32(T* value)
    {
        static_assert(sizeof(T) == 4, "should be 32 bits");
        assert(end_ - cur_ >= static_cast<int>(sizeof(T)));
        *reinterpret_cast<uint32_t*>(value) = networkToHost32(*reinterpret_cast<const uint32_t*>(cur_));
        cur_ += sizeof(T);
    }

    template <typename T>
    void readFixed16(T* value)
    {
        static_assert(sizeof(T) == 2, "should be 16 bits");
        assert(end_ - cur_ >= static_cast<int>(sizeof(T)));
        *reinterpret_cast<uint16_t*>(value) = networkToHost16(*reinterpret_cast<const uint16_t*>(cur_));
        cur_ += sizeof(T);
    }

    template <typename T>
    void readFixed8(T* value)
    {
        static_assert(sizeof(T) == 1, "should be 8 bits");
        assert(end_ - cur_ >= static_cast<int>(sizeof(T)));
        *reinterpret_cast<uint8_t*>(value) = *cur_;
        cur_ += sizeof(T);
    }

    // 字节流读取到buf
    void readByteArray(void* buf, int len)
    {
        std::copy_n(cur_, len, static_cast<uint8_t*>(buf));
        cur_ += len;
    }

    void readString(std::string* str)
    {
        uint32_t len = readFixed32<uint32_t>();
        *str = std::string(reinterpret_cast<const char*>(cur_), len);
        cur_ += len;
    }

    template <typename T>
    T readFixed64()
    {
        T value;
        readFixed64(&value);
        return value;
    }

    template <typename T>
    T readFixed32()
    {
        T value;
        readFixed32(&value);
        return value;
    }

    template <typename T>
    T readFixed16()
    {
        T value;
        readFixed16(&value);
        return value;
    }

    template <typename T>
    T readFixed8()
    {
        T value;
        readFixed8(&value);
        return value;
    }

    std::string readString()
    {
        std::string res;
        readString(&res);
        return res;
    }

    operator bool() const { return cur_ != end_; }

    const uint8_t* cur() const { return cur_; }
    const uint8_t* end() const { return end_; }

private:
    const uint8_t* cur_;
    const uint8_t* end_;
};

} // namespace net

#endif // NET_DATA_STREAM_H
