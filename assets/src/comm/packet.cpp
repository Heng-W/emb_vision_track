
#include "packet.h"
#include <string.h>
#include <iostream>


namespace EVTrack
{

Packet::Packet(uint8* msg):
    head_(msg),
    tail_(msg)
{

}


void Packet::pack()
{
    uint16 len = getBodyLength();
    *(head_ + 4) = (uint8)(len & 0xff);
    *(head_ + 5) = (uint8)((len >> 8) & 0xff);

}


uint16 Packet::getBodyLength()
{
    return tail_ - head_ - 6;
}

uint16 Packet::getLength()
{
    return tail_ - head_;
}


void Packet::writeHeader(Command cmd)
{
    tail_ = head_;
    writeUint16(0x7e7f);//起始位
    writeUint16(static_cast<uint16>(cmd));//消息命令ID
    writeUint16(0);//消息体长度
}


void Packet::writeString(std::string s)
{
    strcpy((char*)tail_, s.c_str());
    tail_ += (s.size() + 1);
}

void Packet::writeString(char* s)
{
    while ((*tail_++ = *s++) != '\0');
}

void Packet::writeBytes(char* dat, int len)
{
    memcpy(tail_, dat, len);
    tail_ += len;
}

void Packet::writeByteArray(int count, ...)
{
    if (count <= 0)
        return;
    va_list arg_ptr;
    va_start(arg_ptr, count);

    for (int i = 0; i < count; i++)
    {
        *tail_++ = (uint8)va_arg(arg_ptr, int);
    }
    va_end(arg_ptr);
}


void Packet::writeFloat(float num)
{
    memcpy(tail_, &num, sizeof(num));
    tail_ += 4;
}


void Packet::writeUint8(uint8 num)
{
    *tail_++ = num;

}


void Packet::writeUint16(uint16 num)
{
    *tail_++ = (uint8)(num & 0xff);
    *tail_++ = (uint8)((num >> 8) & 0xff);

}

void Packet::writeUint32(uint32 num)
{
    for (uint8 i = 0; i < 4; i++)
    {
        *tail_++ = (uint8)(num & 0xff);
        num >>= 8;
    }

}

void Packet::writeUint64(uint64 num)
{
    for (uint8 i = 0; i < 8; i++)
    {
        *tail_++ = (uint8)(num & 0xff);
        num >>= 8;
    }

}


void Packet::writeInt8(int8 num)
{
    writeUint8((uint8)num);

}

void Packet::writeInt16(int16 num)
{
    writeUint16((uint16)num);
}

void Packet::writeInt32(int32 num)
{
    writeUint32((uint32)num);
}

void Packet::writeInt64(int64 num)
{
    writeUint64((uint64)num);
}



PacketReader::PacketReader(uint8* msg): head_(msg)
{
    tail_ = head_;
}

bool PacketReader::isHeader()
{
    return readUint16() == 0x7e7f;
}

uint16 PacketReader::getLength()
{
    return tail_ - head_;
}

void PacketReader::reset()
{
    tail_ = head_;
}

std::string PacketReader::readString()
{
    const char* c = (char*)tail_;
    std::string str(c);
    tail_ += (str.size() + 1);
    return str;
}


void PacketReader::readByteArray(int count, uint8* array)
{
    if (count <= 0)
        return;
    while (--count)
        *array++ = *tail_++;
    *array = *tail_++;
}


float PacketReader::readFloat()
{
    float* f = (float*)tail_;
    tail_ += 4;
    return *f;
}

uint8 PacketReader::readUint8()
{
    return *tail_++;
}


uint16 PacketReader::readUint16()
{
    return readUint8() | readUint8() << 8;
}

uint32 PacketReader::readUint32()
{
    return readUint16() | readUint16() << 16;

}

uint64 PacketReader::readUint64()
{
    return readUint32() | (uint64)readUint32() << 32;
}

int8 PacketReader::readInt8()
{
    return *tail_++;
}


int16 PacketReader::readInt16()
{
    return readUint8() | readInt8() << 8;
}

int32 PacketReader::readInt32()
{
    return readUint16() | readInt16() << 16;

}

int64 PacketReader::readInt64()
{
    return readUint32() | (int64)readInt32() << 32;
}



}

