package pers.hw.evtrack.net;

import java.io.IOException;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Logger;

import pers.hw.evtrack.Command;

public class TcpConnection {

    private static final Logger logger = Logger.getLogger(TcpConnection.class.getName());

    private static final int MAX_READ_SIZE = 65536;

    private final AtomicInteger state = new AtomicInteger();
    private final Socket socket;

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
        void onConnection(TcpConnection conn);

        void onMessage(TcpConnection conn, Buffer buf);

        void onWriteComplete(TcpConnection conn);
    }

    TcpConnection(Socket socket, SocketAddress peerAddr) {
        setState(State.kConnecting);
        this.socket = socket;
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
            try {
                Buffer buf = new Buffer(len, 0);
                buf.append(data, off, len);
                buffersToSend.put(buf);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void send(String data) {
        if (state.get() == State.kConnected.ordinal()) {
            try {
                Buffer buf = new Buffer(data.length(), 0);
                buf.append(data.getBytes());
                buffersToSend.put(buf);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void send(Buffer buf) {
        if (state.get() == State.kConnected.ordinal()) {
            try {
                buffersToSend.put(buf);
            } catch (InterruptedException e) {
                e.printStackTrace();
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
        while (state.get() == State.kConnected.ordinal()) {
            Buffer buf;
            try {
                buf = buffersToSend.take();
            } catch (InterruptedException e) {
                e.printStackTrace();
                break;
            }
            logger.finest(stateToString());
            if (state.get() != State.kConnected.ordinal() ||
                    !sendInThread(buf.data(), buf.peek(), buf.readableBytes())) {
                break;
            }
        }
        setState(State.kDisconnecting);
        try {
            socket.shutdownOutput();
            socket.shutdownInput();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void doRecvEvent() {
        while (state.get() == State.kConnected.ordinal()) {
            inputBuffer.ensureWritableBytes(MAX_READ_SIZE);
            int n;
            try {
                n = socket.getInputStream().read(inputBuffer.data(), inputBuffer.beginWrite(), MAX_READ_SIZE);
            } catch (IOException e) {
                e.printStackTrace();
                break;
            }
            logger.finest(stateToString());
            if (state.get() != State.kConnected.ordinal()) {
                break;
            }
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
        shutdown();
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

        if (callbacks != null) callbacks.onConnection(this);
    }

    public void connectDestroyed() {
        assert(state.get() == State.kDisconnecting.ordinal());
        setState(State.kDisconnected);

        if (callbacks != null) callbacks.onConnection(this);

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

    private void setState(State s) {
        state.set(s.ordinal());
    }

    private String stateToString() {
        return State.values()[state.get()].toString();
    }

}
