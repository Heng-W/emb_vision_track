package pers.hw.evtrack.net;

import java.io.IOException;
import java.net.Socket;
import java.net.SocketAddress;
import java.util.logging.Logger;

public class TcpClient {

    private static final Logger logger = Logger.getLogger(TcpClient.class.getName());

    private TcpConnection connection;

    private Callbacks callbacks;

    private boolean quit = false;
    private boolean recv = false;
    private boolean retry = false;
    private boolean connect = true;
    private boolean fixedRetryDelay = false;

    private long retryDelayMs = 500;
    private static final long kMaxRetryDelayMs = 30 * 1000;

    private final SocketAddress serverAddress;

    private final Thread recvThread;


    public void setCallbacks(Callbacks cb) {
        this.callbacks = cb;
    }

    public interface Callbacks {
        void onConnection(TcpConnection conn);

        void onMessage(TcpConnection conn, Buffer buf);

        void onWriteComplete(TcpConnection conn);
    }

    public TcpClient(SocketAddress serverAddress) {
        this.serverAddress = serverAddress;
        recvThread = new Thread(new Runnable() {
            @Override
            public void run() {
                recvThreadFunc();
            }
        });
        recvThread.setDaemon(true);
        recvThread.start();
        logger.info("Create TcpClient");
    }

    public void start() {
        logger.info("start connect");
        connect = true;
        doConnectEvent();
    }

    public void stop() {
        connect = false;

        TcpConnection conn;
        synchronized (this) {
            conn = connection;
        }

        if (conn != null) {
            conn.shutdown();
        } else {
            synchronized (this) {
                notify();
            }
        }
    }

    public TcpConnection connection() {
        synchronized (this) {
            return connection;
        }
    }

    public void waitForFinished() {
        synchronized (this) {
            quit = true;
            notify();
        }
        try {
            recvThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void setRetryDelay(int delayMs, boolean fixed) {
        retryDelayMs = delayMs;
        fixedRetryDelay = fixed;
    }

    public void enableRetry() {
        retry = true;
    }

    private void doConnectEvent() {
        while (true) {
            Socket socket = tryConnect();
            if (socket == null) {
                if (!connect) return;
                logger.info("retry connecting to " + serverAddress.toString());

                synchronized (this) {
                    if (!connect) return;
                    try {
                        wait(retryDelayMs);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                if (!fixedRetryDelay) {
                    retryDelayMs = Math.min(retryDelayMs * 2, kMaxRetryDelayMs);
                }

            } else {
                logger.fine("connect success");
                newConnection(socket);
                return;
            }
        }
    }

    private Socket tryConnect() {
        Socket socket = new Socket();
        try {
            socket.connect(serverAddress, 5000);
        } catch (IOException e) {
            logger.warning("connect fail");
        }
        if (socket.isConnected()) {
            return socket;
        } else {
            return null;
        }
    }

    private void newConnection(Socket socket) {
        SocketAddress peerAddr = socket.getRemoteSocketAddress();
        TcpConnection conn = new TcpConnection(socket, peerAddr);

        conn.setCallbacks(new TcpConnection.Callbacks() {

            @Override
            public void onConnection(TcpConnection conn) {
                if (callbacks != null) {
                    callbacks.onConnection(conn);
                } else {
                    logger.info(conn.getLocalAddr().toString() + " -> " +
                            conn.getPeerAddr().toString() + " is " +
                            (conn.isConnected() ? "UP" : "DOWN"));
                }
            }

            @Override
            public void onMessage(TcpConnection conn, Buffer buf) {
                if (callbacks != null) {
                    callbacks.onMessage(conn, buf);
                } else {
                    buf.retrieveAll();
                }
            }

            @Override
            public void onWriteComplete(TcpConnection conn) {
                if (callbacks != null) {
                    callbacks.onWriteComplete(conn);
                }
            }

        });

        synchronized (this) {
            connection = conn;
        }
        if (!connect) return;
        conn.connectEstablished();

        synchronized (this) {
            recv = true;
            notify();
        }

        conn.doSendEvent();

        synchronized (this) {
            try {
                while (recv && !quit) {
                    wait();
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            connection = null;
        }
        conn.connectDestroyed();

        logger.info("removeConnection");
        if (retry && connect) {
            logger.info("reconnecting ");
            start();
        }
    }

    private void recvThreadFunc() {
        while (!quit) {
            synchronized (this) {
                try {
                    while (!recv && !quit) {
                        wait();
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                if (quit) {
                    return;
                }
            }
            logger.finest("start recv");

            connection.doRecvEvent();

            synchronized (this) {
                recv = false;
                notify();
            }
        }
    }

}
