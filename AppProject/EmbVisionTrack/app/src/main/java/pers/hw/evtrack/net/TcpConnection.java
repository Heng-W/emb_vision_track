package pers.hw.evtrack.net;

import java.io.IOException;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Logger;

public class TcpConnection {

    private static final Logger logger = Logger.getLogger(TcpConnection.class.getName());

    private static final int MAX_READ_SIZE = 65536;

    private final AtomicInteger state = new AtomicInteger();
    private final Socket socket;
    private final long sendThreadId;

    private final SocketAddress localAddr;
    private final SocketAddress peerAddr;

    private final BlockingQueue<Buffer> buffersToSend = new LinkedBlockingQueue<>();

    private Callbacks callbacks;

    private final Buffer inputBuffer = new Buffer();

    private enum State {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    }

    public interface Callbacks {

        void onMessage(TcpConnection conn, Buffer buf);

        void onWriteComplete(TcpConnection conn);
    }

    TcpConnection(Socket socket, SocketAddress peerAddr) {
        setState(State.kConnecting);
        this.socket = socket;
        this.sendThreadId = Thread.currentThread().getId();
        this.localAddr = socket.getLocalSocketAddress();
        this.peerAddr = peerAddr;
        logger.fine("Create TcpConnection");
    }

    public void setCallbacks(Callbacks cb) {
        this.callbacks = cb;
    }

    public SocketAddress getLocalAddr() {
        return localAddr;
    }

    public SocketAddress getPeerAddr() {
        return peerAddr;
    }

    public boolean isConnected() {
        return state.get() == State.kConnected.ordinal();
    }

    public void send(byte[] data) {
        send(data, 0, data.length);
    }

    public void send(byte[] data, int off, int len) {
        if (state.get() == State.kConnected.ordinal()) {
            if (isInSendThread()) {
                sendInThread(data, off, len);
            } else {
                Buffer buf = new Buffer(len);
                buf.append(data, off, len);
                try {
                    buffersToSend.put(buf);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void send(String data) {
        if (state.get() == State.kConnected.ordinal()) {
            if (isInSendThread()) {
                sendInThread(data.getBytes(), 0, data.length());
            } else {
                Buffer buf = new Buffer(data.length());
                buf.append(data.getBytes());
                try {
                    buffersToSend.put(buf);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void send(Buffer buf) {
        if (state.get() == State.kConnected.ordinal()) {
            if (isInSendThread()) {
                sendInThread(buf.data(), buf.peek(), buf.readableBytes());
            } else {
                try {
                    buffersToSend.put(buf);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private boolean sendInThread(byte[] data, int off, int len) {
        try {
            socket.getOutputStream().write(data, off, len);
            if (callbacks != null && buffersToSend.isEmpty()) {
                callbacks.onWriteComplete(this);
            }
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    public void doSendEvent() {
        while (true) {
            Buffer buf;
            try {
                buf = buffersToSend.take();
            } catch (InterruptedException e) {
                e.printStackTrace();
                break;
            }
            logger.finest(stateToString());
            if (state.get() == State.kDisconnected.ordinal()) {
                logger.warning("disconnected, give up writing");
                return;
            }
            if (buf.readableBytes() > 0) {
                if (!sendInThread(buf.data(), buf.peek(), buf.readableBytes())) {
                    break;
                }
            } else if (state.get() == State.kDisconnecting.ordinal()) {
                break;
            }
        }
        try {
            socket.shutdownOutput();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void doRecvEvent() {
        while (true) {
            inputBuffer.ensureWritableBytes(MAX_READ_SIZE);
            int n;
            try {
                n = socket.getInputStream().read(inputBuffer.data(), inputBuffer.beginWrite(), MAX_READ_SIZE);
            } catch (IOException e) {
                e.printStackTrace();
                break;
            }
            logger.finest(stateToString());
            if (n > 0) {
                inputBuffer.hasWritten(n);
                if (callbacks != null) {
                    callbacks.onMessage(this, inputBuffer);
                }
            } else if (n == 0) {
                break;
            } else {
                logger.warning("doRecvEvent read");
                break;
            }
        }
        if (state.get() == State.kConnected.ordinal()) {
            setState(State.kDisconnected);
            try {
                buffersToSend.put(new Buffer(0));
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void shutdown() {
        if (state.get() == State.kConnected.ordinal()) {
            setState(State.kDisconnecting);
            try {
                buffersToSend.put(new Buffer(0));
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void connectEstablished() {
        assert(state.get() == State.kConnecting.ordinal());
        setState(State.kConnected);
    }

    public void connectDestroyed() {
        setState(State.kDisconnected);
        try {
            socket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void setTcpNoDelay(boolean on) {
        try {
            socket.setTcpNoDelay(on);
        } catch (SocketException e) {
            e.printStackTrace();
        }
    }

    private boolean isInSendThread() {
        return sendThreadId == Thread.currentThread().getId();
    }

    private void setState(State s) {
        state.set(s.ordinal());
    }

    private String stateToString() {
        return State.values()[state.get()].toString();
    }

}
