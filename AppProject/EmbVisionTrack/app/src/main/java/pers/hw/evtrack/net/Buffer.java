package pers.hw.evtrack.net;

public class Buffer {

    private byte[] buf;
    private int prependSize;
    private int readerIndex;
    private int writerIndex;

    public Buffer() {
        this(1024, 0);
    }

    public Buffer(int initialSize) {
        this(initialSize, 0);
    }

    public Buffer(int initialSize, int prependSize) {
        this.prependSize = prependSize;
        this.readerIndex = Math.min(prependSize, initialSize);
        this.writerIndex = readerIndex;
        if (initialSize > 0) {
            buf = new byte[initialSize];
        } else {
            buf = null;
        }
    }

    public int readableBytes() {
        return writerIndex - readerIndex;
    }

    public int writableBytes() {
        return buf.length - writerIndex;
    }

    public int prependableBytes() {
        return readerIndex;
    }

    public byte[] data() {
        return buf;
    }

    public int peek() {
        return readerIndex;
    }

    public int beginWrite() {
        return writerIndex;
    }

    public void hasWritten(int len) {
        writerIndex += len;
    }

    public void unwrite(int len) {
        assert (len <= readableBytes());
        writerIndex -= len;
    }

    public void retrieve(int len) {
        assert (len <= readableBytes());
        if (len < readableBytes()) {
            readerIndex += len;
        } else {
            retrieveAll();
        }
    }

    public void retrieveAll() {
        readerIndex = writerIndex = prependSize;
    }

    public void append(byte[] data) {
        append(data, 0, data.length);
    }

    public void append(byte[] data, int off, int len) {
        ensureWritableBytes(len);
        System.arraycopy(data, off, this.buf, writerIndex, len);
        hasWritten(len);
    }

    public void appendInt8(int num) {
        ensureWritableBytes(1);
        buf[writerIndex++] = (byte) (num & 0xff);
    }

    public void appendInt16(int num) {
        ensureWritableBytes(2);
        buf[writerIndex++] = (byte) ((num >>> 8) & 0xff);
        buf[writerIndex++] = (byte) (num & 0xff);
    }

    public void appendInt32(int num) {
        ensureWritableBytes(4);
        buf[writerIndex++] = (byte) ((num >>> 24) & 0xff);
        buf[writerIndex++] = (byte) ((num >>> 16) & 0xff);
        buf[writerIndex++] = (byte) ((num >>> 8) & 0xff);
        buf[writerIndex++] = (byte) (num & 0xff);
    }

    public void appendInt64(long num) {
        appendInt32((int) ((num >>> 32) & 0xffffffffL));
        appendInt32((int) (num & 0xffffffffL));
    }

    public void appendFloat(float val) {
        appendInt32(Float.floatToIntBits(val));
    }

    public void appendDouble(double val) {
        appendInt64(Double.doubleToLongBits(val));
    }

    public int peekInt8() {
        return buf[readerIndex] & 0xff;
    }

    public int peekInt16() {
        return (buf[readerIndex] & 0xff) << 8 | (buf[readerIndex + 1] & 0xff);
    }

    public int peekInt32() {
        int res = 0;
        res |= (buf[readerIndex] & 0xff) << 24;
        res |= (buf[readerIndex + 1] & 0xff) << 16;
        res |= (buf[readerIndex + 2] & 0xff) << 8;
        res |= buf[readerIndex + 3] & 0xff;
        return res;
    }

    public long peekInt64() {
        long res = 0;
        for (int i = 0; i < 8; ++i) {
            res |= (buf[readerIndex + i] & 0xffL) << (8 * (7 - i));
        }
        return res;
    }

    public int readInt8() {
        return buf[readerIndex++] & 0xff;
    }

    public int readInt16() {
        int res = 0;
        res |= (buf[readerIndex++] & 0xff) << 8;
        res |= buf[readerIndex++] & 0xff;
        return res;
    }

    public int readInt32() {
        int res = 0;
        res |= (buf[readerIndex++] & 0xff) << 24;
        res |= (buf[readerIndex++] & 0xff) << 16;
        res |= (buf[readerIndex++] & 0xff) << 8;
        res |= buf[readerIndex++] & 0xff;
        return res;
    }

    public long readInt64() {
        long res = 0;
        for (int i = 0; i < 8; ++i) {
            res |= (buf[readerIndex++] & 0xffL) << (8 * (7 - i));
        }
        return res;
    }

    public void prepend(byte[] data, int off, int len) {
        assert (len <= prependableBytes());
        readerIndex -= len;
        System.arraycopy(data, off, this.buf, readerIndex, len);
    }

    public void prependInt8(int num) {
        readerIndex -= 1;
        buf[readerIndex] = (byte) (num & 0xff);
    }

    public void prependInt16(int num) {
        readerIndex -= 2;
        buf[readerIndex] = (byte) ((num >>> 8) & 0xff);
        buf[readerIndex + 1] = (byte) (num & 0xff);
    }

    public void prependInt32(int num) {
        readerIndex -= 4;
        buf[readerIndex] = (byte) ((num >>> 24) & 0xff);
        buf[readerIndex + 1] = (byte) ((num >>> 16) & 0xff);
        buf[readerIndex + 2] = (byte) ((num >>> 8) & 0xff);
        buf[readerIndex + 3] = (byte) (num & 0xff);
    }

    public void prependInt64(long num) {
        prependInt32((int) (num & 0xffffffffL));
        prependInt32((int) ((num >>> 32) & 0xffffffffL));
    }

    public void ensureWritableBytes(int len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        assert (writableBytes() >= len);
    }

    private void makeSpace(int len) {
        int readable = readableBytes();
        if (writableBytes() + prependableBytes() < len + prependSize) {
            int oidSize = prependSize + readable;
            byte[] newBuf = new byte[oidSize + Math.max(oidSize, len)]; // 扩容
            System.arraycopy(buf, readerIndex, newBuf, prependSize, readable);
            buf = newBuf;
        } else {
            System.arraycopy(buf, readerIndex, buf, prependSize, readable); // 数据前移
        }
        readerIndex = prependSize;
        writerIndex = readerIndex + readable;
    }

}
