package pers.hw.evtrack;


import java.util.List;
import java.util.ArrayList;
import java.lang.Byte;

import com.google.common.primitives.Bytes;


class Packet {

    private List<Byte> msg = new ArrayList<>();

    public byte[] pack() {
        writeLength();
        return Bytes.toArray(msg);
    }

    public void writeHeader(int msgID) {
        writeUint16(0x7e7f);
        writeUint16(msgID);
        writeUint16(0);

    }

    public  void writeLength() {
        int len = msg.size() - 6;
        if (len < 65535) {

            msg.set(4, (byte)(len & 0xff));
            msg.set(5, (byte)((len >> 8) & 0xff));
        } else {

            msg.set(4, (byte) 0xff);
            msg.set(5, (byte) 0xff);
            for (int i = 0; i < 4; i++) {
                msg.add(6 + i, (byte)(len & 0xff));
                len >>= 8;
            }
        }

    }

    public void writeUint8(int num) {

        msg.add((byte)(num & 0xff));

    }

    public void writeUint16(int num) {

        msg.add((byte)(num & 0xff));
        msg.add((byte)((num >> 8) & 0xff));

    }

    public void writeUint32(int num) {

        for (int i = 0; i < 4; i++) {
            msg.add((byte)(num & 0xff));
            num >>= 8;
        }

    }

    public void writeUint64(int num) {

        for (int i = 0; i < 8; i++) {
            msg.add((byte)(num & 0xff));
            num >>= 8;
        }

    }

    public void writeFloat(float val) {

        writeInt32(Float.floatToIntBits(val));

    }

    public void writeInt8(int num) {
        writeUint8(num);
    }

    public void writeInt16(int num) {
        writeUint16(num);
    }

    public void writeInt32(int num) {
        writeUint32(num);
    }

    public void writeInt64(int num) {
        writeUint64(num);
    }


    public void writeByteArray(byte... b) {
        for (int i = 0; i < b.length; i++) {
            msg.add(b[i]);
        }

    }


    public void writeString(String str) {
        for (int i = 0; i < str.length(); i++) {
            msg.add((byte)str.charAt(i));
        }
        msg.add((byte)'\0');


    }

}


class PacketReader {

    private byte[] msg;
    private int idx;


    public void setMessage(byte[] msg) {
        this.msg = msg;
        idx = 0;
    }

    public int readUint8() {
        return msg[idx++] & 0xff;
    }

    public int readUint16() {

        return  readUint8() | readUint8() << 8;
    }

    public  int readUint32() {

        return readUint16() | readUint16() << 16;
    }

    public long readUint64() {

        return readUint32() | readUint32() << 32;
    }

    public int readInt8() {

        return (int)msg[idx++];
    }

    public int readInt16() {

        return  readUint8() | readInt8() << 8;
    }

    public  int readInt32() {

        return readUint16() | readInt16() << 16;
    }

    public long readInt64() {

        return readUint32() | readInt32() << 32;
    }


    public float readFloat() {

        return Float.intBitsToFloat(readInt32());
    }

    public void readByteArray(int count) {
        if (count <= 0)
            return;

        byte[] b = new byte[count];
        for (int i = 0; i < count; i++) {
            b[i] = msg[idx++];
        }

    }

    public String readString() {
        StringBuilder sb = new StringBuilder();
        while (msg[idx] != '\0') {
            sb.append((char)msg[idx++]);
        }
        idx++;
        return sb.toString();
    }

    public byte[] readBytes(int len) {
        byte[] b = new byte[len];
        for (int i = 0; i < len; i++) {
            b[i] = msg[idx++];

        }
        return b;
    }

}


