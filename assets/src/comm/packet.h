#ifndef EVT_PACKET_H
#define EVT_PACKET_H

#include "common.h"
#include <string>
#include <stdarg.h>


namespace EVTrack
{

class Packet
{
public:
    Packet(uint8* msg);

    uint8* getMessage()
    {
        return head_;
    }

    void pack();
    uint16 getBodyLength();
    uint16 getLength();

    void writeHeader(Command cmd);
    void writeString(std::string s);
    void writeString(char* s);

    void writeBytes(char* dat, int len);


    void writeByteArray(int count, ...);

    void writeFloat(float num);

    void writeUint8(uint8 num);
    void writeUint16(uint16 num);
    void writeUint32(uint32 num);
    void writeUint64(uint64 num);

    void writeInt8(int8 num);
    void writeInt16(int16 num);
    void writeInt32(int32 num);
    void writeInt64(int64 num);

private:
    uint8* head_;
    uint8* tail_;


};


class PacketReader
{

public:

    PacketReader(uint8* msg);

    bool isHeader();

    uint16 getLength();
    void reset();

    std::string readString();

    void readByteArray(int count, uint8* array);

    float readFloat();

    uint8 readUint8();
    uint16 readUint16();
    uint32 readUint32();
    uint64 readUint64();

    int8 readInt8();
    int16 readInt16();
    int32 readInt32();
    int64 readInt64();

private:
    uint8* head_;
    uint8* tail_;

};


}

#endif //EVT_PACKET_H
