package pers.hw.evtrack;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.net.SocketAddress;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.graphics.Bitmap;

import pers.hw.evtrack.net.Buffer;
import pers.hw.evtrack.net.TcpClient;
import pers.hw.evtrack.net.TcpConnection;


public class Client extends TcpClient {

    private static final int HEADER_LEN = 4;
    private static final int MIN_MESSAGE_LEN = 2;
    private static final int MAX_MESSAGE_LEN = 64 * 1024 * 1024;


    public Client(SocketAddress serverAddr, final String userName, final String pwd) {
        super(serverAddr);
        super.setCallbacks(new TcpClient.Callbacks() {
            @Override
            public void onConnection(TcpConnection conn) {
                Log.i("onConnection", conn.getLocalAddr().toString() + " -> " +
                        conn.getPeerAddr().toString() + " is " +
                        (conn.isConnected() ? "UP" : "DOWN"));
                if (conn.isConnected()) {
                    Client.this.userName = userName;
                    Buffer buf = createBuffer(Command.LOGIN);
                    buf.appendInt32(userName.length());
                    buf.append(userName.getBytes());
                    buf.appendInt32(pwd.length());
                    buf.append(pwd.getBytes());
                    send(buf);
                }
            }

            @Override
            public void onMessage(TcpConnection conn, Buffer buf) {
                while (buf.readableBytes() >= HEADER_LEN + MIN_MESSAGE_LEN) {
                    int len = buf.peekInt32();
                    if (len > MAX_MESSAGE_LEN || len < MIN_MESSAGE_LEN) {
                        Log.e("message", "message length invalid: " + len);
                        break;
                    } else if (buf.readableBytes() >= HEADER_LEN + len) {
                        buf.retrieve(HEADER_LEN);
                        ByteArrayInputStream bis = new ByteArrayInputStream(buf.data(), buf.peek(), len);
                        try {
                            parseMessage(new DataInputStream(bis));
                        } catch (IOException e) {
                            Log.e("IOException", e.toString());
                        }
                        buf.retrieve(len);
                    } else {
                        break;
                    }
                }
            }

            @Override
            public void onWriteComplete(TcpConnection conn) {

            }
        });

    }

    public static Buffer createBuffer(Command cmd, int initialSize) {
        Buffer buf = new Buffer(HEADER_LEN + MIN_MESSAGE_LEN + initialSize, HEADER_LEN);
        buf.appendInt16(cmd.ordinal());
        return buf;
    }

    public static Buffer createBuffer(Command cmd) {
        Buffer buf = new Buffer(HEADER_LEN + MIN_MESSAGE_LEN, HEADER_LEN);
        buf.appendInt16(cmd.ordinal());
        return buf;
    }

    public void send(Buffer buf) {
        buf.prependInt32(buf.readableBytes());
        TcpConnection conn = connection();
        if (conn != null) {
            conn.send(buf);
        }
    }

    public void send(Command cmd) {
        send(createBuffer(cmd));
    }

    private MainActivity mActivity;
    private Handler mHandler;
    private Handler[] handler = new Handler[MainActivity.FRAG_MAX];

    public void setMainActivity(MainActivity activity) {
        this.mActivity = activity;
    }

    public void setMainHandler(Handler mHandler) {
        this.mHandler = mHandler;
    }

    public void setHandler(Handler handler, int idx) {
        this.handler[idx] = handler;
    }


    private void sendToFragment(int idx, int what) {
        if (handler[idx] != null) {
            Message msg = handler[idx].obtainMessage(what);
            handler[idx].sendMessage(msg);
        }
    }

    private void sendToFragment(int idx, int what, Bundle bundle) {
        if (handler[idx] != null) {
            Message msg = handler[idx].obtainMessage(what);
            msg.setData(bundle);
            handler[idx].sendMessage(msg);
        }
    }

    private void sendToFragment(int idx, int what, Object obj) {
        if (handler[idx] != null) {
            Message msg = handler[idx].obtainMessage(what, obj);
            handler[idx].sendMessage(msg);
        }
    }

    private void sendToFragment(int idx, int what, int arg1) {
        if (handler[idx] != null) {
            Message msg = handler[idx].obtainMessage(what, arg1, 0);
            handler[idx].sendMessage(msg);
        }
    }

    private void sendToFragment(int idx, int what, int arg1, int arg2) {
        if (handler[idx] != null) {
            Message msg = handler[idx].obtainMessage(what, arg1, arg2);
            handler[idx].sendMessage(msg);
        }
    }

    private void sendToFragment(int idx, int what, int arg1, int arg2, Object obj) {
        if (handler[idx] != null) {
            Message msg = handler[idx].obtainMessage(what, arg1, arg2, obj);
            handler[idx].sendMessage(msg);
        }
    }


    public static final int IMAGE_W = 640;
    public static final int IMAGE_H = 480;

    public float angleHDefault = 90;
    public float angleVDefault = 90;

    public int width, height;
    public int xpos = IMAGE_W / 2, ypos = IMAGE_H / 2;

    public boolean trackFlag;
    public boolean motionAutoCtlFlag;
    public boolean fieldAutoCtlFlag;

    public boolean useMultiScale;

    public int leftCtlVal;
    public int rightCtlVal;

    public float angleH, angleV;
    public float[][] pidParams = new float[4][2];

    int clientCnt = 0;

    public int userID;
    public String userName;
    public int userType;


    private void parseMessage(DataInputStream in) throws IOException {

        Command cmd = Command.values()[in.readUnsignedShort()];
        Log.d("command", cmd.toString());
        switch (cmd) {
            case LOGIN: {
                final byte[] b = new byte[in.available()];
                in.read(b);
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            ByteArrayInputStream bis = new ByteArrayInputStream(b);
                            DataInputStream in = new DataInputStream(bis);

                            int ret = in.readInt();
                            if (ret == 0) {
                                userID = in.readInt();
                                userType = in.readUnsignedByte();
                            }
                            Message msg = mHandler.obtainMessage(12, ret, 0);
                            mHandler.sendMessage(msg);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                });
                break;
            }
            case INIT: {
                final byte[] b = new byte[in.available()];
                in.read(b);
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            ByteArrayInputStream bis = new ByteArrayInputStream(b);
                            DataInputStream in = new DataInputStream(bis);
                            trackFlag = in.readBoolean();
                            useMultiScale = in.readBoolean();

                            motionAutoCtlFlag = in.readBoolean();
                            fieldAutoCtlFlag = in.readBoolean();

                            leftCtlVal = in.readInt();
                            rightCtlVal = in.readInt();

                            angleHDefault = in.readFloat();
                            angleVDefault = in.readFloat();

                            angleH = in.readFloat();
                            angleV = in.readFloat();

                            for (int i = 0; i < 4; i++) {
                                for (int j = 0; j < 2; j++) {
                                    pidParams[i][j] = in.readFloat();
                                }
                            }
                            Message msg = mHandler.obtainMessage(88);
                            mHandler.sendMessage(msg);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                });
                break;
            }
            case RESET: {
                final int type = in.readInt();
                final int flag = in.readUnsignedByte();
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if ((type & 0x1) != 0) {
                            trackFlag = false;
                            motionAutoCtlFlag = false;
                            fieldAutoCtlFlag = false;
                        }
                        if ((type & 0x2) != 0) {
                            angleH = angleHDefault;
                            angleV = angleVDefault;
                            fieldAutoCtlFlag = false;
                        }
                        if ((type & 0x4) != 0) {
                            leftCtlVal = 0;
                            rightCtlVal = 0;
                            motionAutoCtlFlag = false;
                        }
                        Message msg = mHandler.obtainMessage(22, type, flag);
                        mHandler.sendMessage(msg);

                        sendToFragment(0, 1);
                        sendToFragment(1, 1);
                        sendToFragment(2, 1);
                        sendToFragment(1, 24);
                    }
                });
                break;
            }
            case MESSAGE: {
                final byte[] b = new byte[in.available()];
                in.read(b);
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            ByteArrayInputStream bis = new ByteArrayInputStream(b);
                            DataInputStream in = new DataInputStream(bis);

                            angleH = in.readFloat();
                            angleV = in.readFloat();

                            leftCtlVal = in.readInt();
                            rightCtlVal = in.readInt();

                            sendToFragment(2, 22);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                });
                break;
            }
            case IMAGE: {
                int size = in.readInt();
                byte[] img = new byte[size];
                in.read(img);
                Bitmap bitmap = ImageConvert.getPicFromBytes(img, size, null);
                sendToFragment(1, 10, bitmap);
                break;
            }
            case LOCATE: {
                final int[] result = new int[4];
                for (int i = 0; i < 4; i++) {
                    result[i] = in.readInt();
                }
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        xpos = result[0];
                        ypos = result[1];
                        width = result[2];
                        height = result[3];
                        sendToFragment(1, 11);
                    }
                });
                break;
            }
            case START_TRACK: {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        trackFlag = true;
                    }
                });
                sendToFragment(1, 21, in.readUnsignedByte());
                break;
            }
            case STOP_TRACK: {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        trackFlag = false;
                    }
                });
                sendToFragment(1, 21, in.readUnsignedByte());
                break;
            }
            case SET_TARGET: {
                sendToFragment(1, 22);
                break;
            }
            case ENABLE_MULTI_SCALE: {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        useMultiScale = true;
                    }
                });
                sendToFragment(1, 25, in.readUnsignedByte());
                break;
            }
            case DISABLE_MULTI_SCALE: {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        useMultiScale = false;
                    }
                });
                sendToFragment(1, 25, in.readUnsignedByte());
                break;
            }
            case ENABLE_MOTION_AUTOCTL: {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        motionAutoCtlFlag = true;
                    }
                });
                sendToFragment(0, 1, in.readUnsignedByte());
                break;
            }
            case DISABLE_MOTION_AUTOCTL: {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        motionAutoCtlFlag = false;
                    }
                });
                sendToFragment(0, 1, in.readUnsignedByte());
                break;
            }
            case STOP_MOTOR: {
                sendToFragment(0, 26);
                break;
            }
            case ENABLE_FIELD_AUTOCTL: {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        fieldAutoCtlFlag = true;
                    }
                });
                sendToFragment(1, 24, in.readUnsignedByte());
                break;
            }
            case DISABLE_FIELD_AUTOCTL: {
                final float val1 = in.readFloat();
                final float val2 = in.readFloat();
                final int flag = in.readUnsignedByte();

                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        fieldAutoCtlFlag = false;
                        angleH = val1;
                        angleV = val2;
                        sendToFragment(1, 24, flag);
                    }
                });
                break;
            }
            case SET_SERVO_VAL: {
                sendToFragment(1, 22);
                break;
            }
            case SET_MOTION_CTL_PID: {
                sendToFragment(0, 10, in.readUnsignedByte());
                break;
            }
            case SET_FIELD_CTL_PID: {
                sendToFragment(0, 10, in.readUnsignedByte());
                break;
            }
            case UPDATE_PID: {
                final byte[] b = new byte[in.available()];
                in.read(b);
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            ByteArrayInputStream bis = new ByteArrayInputStream(b);
                            DataInputStream in = new DataInputStream(bis);
                            for (int i = 0; i < 4; i++) {
                                for (int j = 0; j < 2; j++) {
                                    pidParams[i][j] = in.readFloat();
                                }
                            }
                            sendToFragment(0, 20);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                });
                break;
            }
            case SYSTEM_CMD: {
                int len = in.readInt();
                String cmdResult;
                if (len == 0) {
                    cmdResult = "";
                } else {
                    byte[] b = new byte[len];
                    in.read(b);
                    cmdResult = new String(b);
                }
                sendToFragment(3, 5, cmdResult);
                break;
            }
            case UPDATE_CLIENT_CNT: {
                final int val = in.readInt();
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        clientCnt = val;
                        sendToFragment(3, 20);
                    }
                });
                break;
            }
            case NOTICE: {
                int len = in.readInt();
                byte[] b = new byte[len];
                in.read(b);
                String noticeMsg = new String(b);
                Message msg = mHandler.obtainMessage(50, noticeMsg);
                mHandler.sendMessage(msg);
                break;
            }
            case SEND_TO_ALL_CLIENTS: {
                sendToFragment(3, 22);
                break;
            }
            case LOGOUT: {
                Message msg = mHandler.obtainMessage(7);
                mHandler.sendMessage(msg);
                break;
            }
            case GET_CLIENT_CNT: {
                final int cnt = in.readInt();
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        clientCnt = cnt;
                    }
                });
                break;
            }
        }
    }

}
