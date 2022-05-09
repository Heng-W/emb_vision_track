#ifndef NET_BUFFER_H
#define NET_BUFFER_H

#include <assert.h>
#include <string.h>
#include <memory>
#include <algorithm>
#include "endian.h"

namespace net
{

/**
 * 缓冲区
 *
 * +-------------------+------------------+------------------+
 * | prependable bytes |  readable bytes  |  writable bytes  |
 * |                   |     (CONTENT)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0      <=      readerIndex   <=   writerIndex    <=     size
 */
class Buffer
{
public:
    explicit Buffer(size_t initialSize = 1024, size_t prependSize = 0)
        : capacity_(initialSize),
          prependSize_(prependSize),
          readerIndex_(std::min(prependSize_, capacity_)),
          writerIndex_(readerIndex_)
    { buffer_ = capacity_ > 0 ?  alloc_.allocate(capacity_) : nullptr; }

    ~Buffer() { free(); }

    Buffer(const Buffer& rhs): Buffer(rhs.capacity_, rhs.prependSize_)
    { append(rhs.peek(), rhs.readableBytes()); }

    Buffer(Buffer&& rhs) noexcept
        : buffer_(rhs.buffer_),
          capacity_(rhs.capacity_),
          prependSize_(rhs.prependSize_),
          readerIndex_(rhs.readerIndex_),
          writerIndex_(rhs.writerIndex_)
    {
        rhs.buffer_ = nullptr;
        rhs.capacity_ = rhs.readerIndex_ = rhs.writerIndex_ = 0;
    }

    Buffer& operator=(const Buffer& rhs) { return *this = Buffer(rhs); }

    Buffer& operator=(Buffer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            free();
            buffer_ = rhs.buffer_;
            capacity_ = rhs.capacity_;
            prependSize_ = rhs.prependSize_;
            readerIndex_ = rhs.readerIndex_;
            writerIndex_ = rhs.writerIndex_;

            rhs.buffer_ = nullptr;
            rhs.capacity_ = rhs.readerIndex_ = rhs.writerIndex_ = 0;
        }
        return *this;
    }

    void swap(Buffer& rhs) noexcept
    {
        using std::swap;
        swap(buffer_, rhs.buffer_);
        swap(capacity_, rhs.capacity_);
        swap(prependSize_, rhs.prependSize_);
        swap(readerIndex_, rhs.readerIndex_);
        swap(writerIndex_, rhs.writerIndex_);
    }

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return capacity_ - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    const char* peek() const { return begin() + readerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }
    char* beginWrite() { return begin() + writerIndex_; }

    std::string toString() const { return std::string(peek(), readableBytes()); }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    // 移动reader位置
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        assert(capacity_ >= prependSize_);
        readerIndex_ = prependSize_;
        writerIndex_ = prependSize_;
    }

    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    // 添加数据
    void append(const void* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy_n(static_cast<const char*>(data), len, beginWrite());
        hasWritten(len);
    }

    void append(const std::string& str)
    {
        append(str.data(), str.size());
    }

    void appendUInt64(uint64_t x)
    {
        uint64_t be64 = hostToNetwork64(x);
        append(&be64, sizeof(be64));
    }

    void appendUInt32(uint32_t x)
    {
        uint32_t be32 = hostToNetwork32(x);
        append(&be32, sizeof(be32));
    }

    void appendUInt16(uint16_t x)
    {
        uint16_t be16 = hostToNetwork16(x);
        append(&be16, sizeof(be16));
    }

    void appendUInt8(uint8_t x)
    {
        append(&x, sizeof(x));
    }

    // 读取数据
    uint64_t readUInt64()
    {
        uint64_t result = peekUInt64();
        retrieve(sizeof(uint64_t));
        return result;
    }

    uint32_t readUInt32()
    {
        uint32_t result = peekUInt32();
        retrieve(sizeof(uint32_t));
        return result;
    }

    uint16_t readUInt16()
    {
        uint16_t result = peekUInt16();
        retrieve(sizeof(uint16_t));
        return result;
    }

    uint8_t readUInt8()
    {
        uint8_t result = peekUInt8();
        retrieve(sizeof(uint8_t));
        return result;
    }

    // peek数据，不改变reader位置
    uint64_t peekUInt64() const
    {
        assert(readableBytes() >= sizeof(uint64_t));
        return networkToHost64(*reinterpret_cast<const uint64_t*>(peek()));
    }

    uint32_t peekUInt32() const
    {
        assert(readableBytes() >= sizeof(uint32_t));
        return networkToHost32(*reinterpret_cast<const uint32_t*>(peek()));
    }

    uint16_t peekUInt16() const
    {
        assert(readableBytes() >= sizeof(uint16_t));
        return networkToHost16(*reinterpret_cast<const uint16_t*>(peek()));
    }

    uint8_t peekUInt8() const
    {
        assert(readableBytes() >= sizeof(uint8_t));
        return *peek();
    }

    // 预置数据
    void prepend(const void* data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    void prependUInt64(uint64_t x)
    {
        uint64_t be64 = hostToNetwork64(x);
        prepend(&be64, sizeof(be64));
    }

    void prependUInt32(uint32_t x)
    {
        uint32_t be32 = hostToNetwork32(x);
        prepend(&be32, sizeof(be32));
    }

    void prependUInt16(uint16_t x)
    {
        uint16_t be16 = hostToNetwork16(x);
        prepend(&be16, sizeof(be16));
    }

    void prependUInt8(uint8_t x)
    {
        prepend(&x, sizeof x);
    }

    // 查找字符
    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char* findCRLF(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char* findEOL() const
    {
        const void* eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    const char* findEOL(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void* eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char*>(eol);
    }

    void shrink(size_t reserve)
    {
        Buffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(toString());
        swap(other);
    }

    const char* begin() const { return buffer_; }
    char* begin() { return buffer_; }

    size_t capacity() const { return capacity_; }

    // 将数据直接读到buffer
    ssize_t readFd(int fd, int* savedErrno);

private:

    void free()
    {
        if (buffer_) alloc_.deallocate(buffer_, capacity_);
    }

    void makeSpace(size_t len)
    {
        size_t readable = readableBytes();
        if (writableBytes() + prependableBytes() < len + prependSize_) // not enough, realloc
        {
            size_t oldSize = prependSize_ + readable;
            size_t newCap = oldSize + std::max(oldSize, len); // 新长度为旧长度两倍，或旧长度+新增个数
            char* newBuf = alloc_.allocate(newCap);
            std::copy_n(buffer_ + readerIndex_, readable, newBuf + prependSize_);
            free();
            buffer_ = newBuf;
            capacity_ = newCap;
        }
        else // enough, move readable data to the front
        {
            assert(prependSize_ < readerIndex_);
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + prependSize_);
        }
        readerIndex_ = prependSize_;
        writerIndex_ = readerIndex_ + readable;
    }

    using Alloc = std::allocator<char>;
    static Alloc alloc_;
    static constexpr const char* kCRLF = "\r\n";

    char* buffer_;
    size_t capacity_;
    size_t prependSize_;

    size_t readerIndex_;
    size_t writerIndex_;
};

} // namespace net

#endif // NET_BUFFER_H

