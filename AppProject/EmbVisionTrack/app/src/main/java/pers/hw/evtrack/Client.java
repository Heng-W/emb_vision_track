package pers.hw.evtrack;


import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.graphics.Bitmap;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.Date;
import java.text.SimpleDateFormat;


public class Client {
    private SocketThread st;
    private Socket socket;
    private DataOutputStream out;

    private Handler mHandler;
    private Message msg;
    private Handler[] handler;

    public boolean isConnected;

    public static final int IMAGE_W = 320;
    public static final int IMAGE_H = 240;

    public static float DEFAULT_ANGLE_H = 90;
    public static float DEFAULT_ANGLE_V = 90;


    public String cmdResult;
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
    public int userID;
    public String userName;
    public int userType;

    public int clientCnt;

    public String noticeMsg;

    public byte[] image = new byte[65535];
    public int imageLen;
    public Bitmap bitmap;


    public static final String IP_DEFAULT = "hengw.tpddns.cn";
    public static final int PORT_DEFAULT = 18000;

    private String ip = IP_DEFAULT;
    private int port = PORT_DEFAULT;


    public Client() {
        handler = new Handler[MainActivity.FRAG_MAX];
    }


    public void connect() throws IOException {

        socket = new Socket();
        socket.connect(new InetSocketAddress(ip, port), 5000);
        //socket.setSoTimeout(10000);//io流超时时间

    }

    public int requestLogin(int userID, String name, String pwd) throws IOException {

        this.userID = userID;

        byte[] b;
        if (userType == 2) {//guest
            b = new byte[3];

        } else if (userID == 0x1) {//login by name
            b = new byte[35];
            for (int i = 0; i < name.length(); i++) {
                b[3 + i] = (byte)name.charAt(i);
            }
            for (int i = 0; i < pwd.length(); i++) {
                b[19 + i] = (byte)pwd.charAt(i);
            }
        } else {
            b = new byte[19];
            for (int i = 0; i < pwd.length(); i++) {
                b[3 + i] = (byte)pwd.charAt(i);
            }
        }

        b[0] = (byte)(userType);

        b[1] = (byte)(userID & 0xff);
        b[2] = (byte)((userID >> 8) & 0xff);

        out = new DataOutputStream(socket.getOutputStream());
        out.write(b);
        out.flush();

        b = new byte[4];

        socket.getInputStream().read(b);

        PacketReader pr = new PacketReader();
        pr.setMessage(b);
        int ret = pr.readInt32();

        if (ret == 0) {
            b = new byte[18];
            socket.getInputStream().read(b);
            pr.setMessage(b);

            this.userID = pr.readUint16();
            this.userName = pr.readString();

            isConnected = true;
            st = new SocketThread(socket);
            st.start();
        } else {
            isConnected = false;
        }
        return ret;
    }

    public void setMainHandler(Handler mHandler) {
        this.mHandler = mHandler;
    }

    public void setHandler(Handler handler, int idx) {
        this.handler[idx] = handler;
    }


    private void sendToFragment(int idx, int label) {
        msg = new Message();
        msg.what = label;
        if (handler[idx] != null) {
            handler[idx].sendMessage(msg);
        }
    }

    private void sendToFragment(int idx, int label, int arg1) {
        msg = new Message();
        msg.what = label;
        msg.arg1 = arg1;
        if (handler[idx] != null) {
            handler[idx].sendMessage(msg);
        }
    }

    public void writeToServer(Command cmd) {
        try {

            sendPacket(newPacket(cmd));
        } catch (IOException e) {
            Log.e("IOExp", e.toString());
        }
    }

    public void writeToServer(Command cmd, String str) {
        try {

            Packet p = newPacket(cmd);
            p.writeString(str);
            sendPacket(p);
        } catch (IOException e) {
            Log.e("IOExp", e.toString());
        }
    }

    public Packet newPacket(Command cmd)  {

        Packet p = new Packet();
        p.writeHeader(cmd.ordinal());
        return p;

    }

    public void sendPacket(Packet p) throws IOException {

        if (isConnected) {
            out.write(p.pack());
            out.flush();
        }
    }



    public void setServerAddr(String ip, int port) {
        this.ip = ip;
        this.port = port;
    }

    public String getServerIp() {
        return ip;
    }

    public int getServerPort() {
        return port;
    }

    public Socket getSocket() {
        return socket;
    }


    public String getTime() {
        return new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date());
    }

    public String getDetailTime() {
        return new SimpleDateFormat("HH:mm:ss").format(new Date());
    }


    private class SocketThread extends Thread {

        private Socket socket;
        private DataInputStream in;
        private PacketReader pr;



        public SocketThread(Socket socket) throws IOException {
            this.socket = socket;
            in = new DataInputStream(socket.getInputStream());
            pr = new PacketReader();

            //pw = new PrintWriter(socket.getOutputStream());
            // br = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        }

        @Override
        public void run() {
            fg:
            while (true) {
                try {

                    byte[] b = new byte[6];

                    {
                        int readLen = 0;
                        int actualLen;

                        while (readLen < 6) {

                            if ((actualLen = in.read(b, readLen, 6 - readLen)) < 0) {
                                msg = new Message();
                                msg.what = 1;
                                break fg;
                            }
                            readLen += actualLen;
                        }
                    }

                    pr.setMessage(b);


                    if (pr.readUint16() != 0x7e7f) {
                        Log.v("packet", "error");
                        continue;
                    }

                    int cmd = pr.readUint16();

                    int len = pr.readUint16();


                    if (len > 0) {

                        b = new byte[len];


                        int readLen = 0;
                        int actualLen;

                        while (readLen < len) {

                            if ((actualLen = in.read(b, readLen, len - readLen)) < 0) {
                                msg = new Message();
                                msg.what = 1;
                                break fg;
                            }
                            readLen += actualLen;
                        }

                        pr.setMessage(b);
                    }
                    //Log.d("cmd",String.valueOf(cmd));
                    if (cmd == Command.init.ordinal()) {
                        trackFlag = (pr.readInt8() != 0) ? true : false;
                        motionAutoCtlFlag = (pr.readInt8() != 0) ? true : false;
                        fieldAutoCtlFlag = (pr.readInt8() != 0) ? true : false;
                        useMultiScale = (pr.readInt8() != 0) ? true : false;


                        leftCtlVal = pr.readUint32();
                        rightCtlVal = pr.readUint32();

                        DEFAULT_ANGLE_H = pr.readFloat();
                        DEFAULT_ANGLE_V = pr.readFloat();
                        angleH = pr.readFloat();
                        angleV = pr.readFloat();

                        for (int i = 0; i < 4; i++) {
                            for (int j = 0; j < 2; j++) {
                                pidParams[i][j] = pr.readFloat();
                            }
                        }
                        msg = new Message();
                        msg.what = 88;
                        mHandler.sendMessage(msg);


                    } else if (cmd == Command.reset.ordinal()) {

                        int type = pr.readInt32();
                        if ((type & 0x1) != 0) {
                            trackFlag = false;
                            motionAutoCtlFlag = false;
                            fieldAutoCtlFlag = false;
                            // MotionControl::stopSignal = true;
                        }
                        if ((type & 0x2) != 0) {
                            angleH = DEFAULT_ANGLE_H;
                            angleV = DEFAULT_ANGLE_V;
                            fieldAutoCtlFlag = false;
                        }
                        if ((type & 0x4) != 0) {
                            leftCtlVal = 0;
                            rightCtlVal = 0;
                        }


                        msg = new Message();
                        msg.what = 22;
                        msg.arg1 = pr.readUint16();
                        mHandler.sendMessage(msg);

                        sendToFragment(0, 1);
                        sendToFragment(1, 1);
                        sendToFragment(2, 1);
                        sendToFragment(1, 24);

                        //sendToFragment(3, 1);

                    }  else if (cmd == Command.message.ordinal()) {

                        angleH = pr.readFloat();

                        angleV = pr.readFloat();
                        leftCtlVal = pr.readInt32();

                        rightCtlVal = pr.readInt32();

                        xpos = pr.readInt32();
                        ypos = pr.readInt32();
                        width = pr.readInt32();
                        height = pr.readInt32();

                        sendToFragment(2, 22);
                    } else if (cmd == Command.image.ordinal()) {

                        int totalLen = pr.readUint32();
                        int nlen = pr.readUint32();

                        int ofs = pr.readUint32();


                        if (ofs == 0 && totalLen > image.length) {
                            image = new byte[(int)(1.2 * totalLen)];
                            Log.v("img cap", String.valueOf(image.length));
                        }

                        int readLen = 0, actualLen;
                        while (readLen < nlen) {

                            if ((actualLen = in.read(image, ofs + readLen, nlen - readLen)) < 0) {
                                msg = new Message();
                                msg.what = 1;
                                break fg;

                            }
                            readLen += actualLen;
                        }

                        if (ofs + nlen == totalLen) {
                            imageLen = totalLen;

                            bitmap = ImageConvert.getPicFromBytes(image, totalLen, null);

                            sendToFragment(1, 10);


                        }

                    } else if (cmd == Command.locate.ordinal()) {

                        xpos = pr.readInt32();
                        ypos = pr.readInt32();
                        width = pr.readInt32();
                        height = pr.readInt32();

                        sendToFragment(1, 11);

                    } else if (cmd == Command.start_track.ordinal()) {
                        trackFlag = true;

                        sendToFragment(1, 21, pr.readUint16());


                    }   else if (cmd == Command.stop_track.ordinal()) {
                        trackFlag = false;
                        sendToFragment(1, 21, pr.readUint16());

                    }   else if (cmd == Command.set_target.ordinal()) {
                        sendToFragment(1, 22);


                    } else if (cmd == Command.enable_motion_autoctl.ordinal()) {
                        motionAutoCtlFlag = true;
                        sendToFragment(0, 1, pr.readUint16());

                    } else if (cmd == Command.disable_motion_autoctl.ordinal()) {
                        motionAutoCtlFlag = false;
                        sendToFragment(0, 1, pr.readUint16());


                    } else if (cmd == Command.stop_motor.ordinal()) {
                        sendToFragment(0, 26);

                    } else if (cmd == Command.set_motion_ctl_pid.ordinal()) {
                        sendToFragment(0, 10, pr.readUint16());

                    } else if (cmd == Command.set_field_ctl_pid.ordinal()) {
                        sendToFragment(0, 10, pr.readUint16());

                    } else if (cmd == Command.update_pid.ordinal()) {

                        for (int i = 0; i < 4; i++) {
                            for (int j = 0; j < 2; j++) {
                                pidParams[i][j] = pr.readFloat();
                            }
                        }
                        sendToFragment(0, 20);
                    } else if (cmd == Command.enable_field_autoctl.ordinal()) {
                        fieldAutoCtlFlag = true;
                        sendToFragment(1, 24, pr.readUint16());

                    } else if (cmd == Command.disable_field_autoctl.ordinal()) {
                        fieldAutoCtlFlag = false;
                        angleH = pr.readFloat();
                        angleV = pr.readFloat();
                        sendToFragment(1, 24, pr.readUint16());


                    } else if (cmd == Command.set_servo_val.ordinal()) {

                        sendToFragment(1, 22);
                    } else if (cmd == Command.system_cmd.ordinal()) {
                        if (len == 0) {
                            cmdResult = "";
                        } else {
                            cmdResult = new String(pr.readBytes(len));
                        }

                        sendToFragment(3, 5);
                    } else if (cmd == Command.enable_multi_scale.ordinal()) {
                        useMultiScale = true;
                        sendToFragment(1, 25, pr.readUint16());

                    } else if (cmd == Command.disable_multi_scale.ordinal()) {
                        useMultiScale = false;

                        sendToFragment(1, 25, pr.readUint16());
                    } else if (cmd == Command.update_client_cnt.ordinal()) {
                        clientCnt = pr.readInt32();
                        sendToFragment(3, 20);
                    } else if (cmd == Command.notice.ordinal()) {
                        noticeMsg = pr.readString();
                        msg = new Message();
                        msg.what = 50;
                        mHandler.sendMessage(msg);
                    } else if (cmd == Command.send_to_all_clients.ordinal()) {
                        sendToFragment(3, 22);
                    } else if (cmd == Command.logout.ordinal()) {

                        msg = new Message();
                        msg.what = 7;
                        msg.arg1 = pr.readInt32();
                        break fg;
                    } else if (cmd == Command.exit.ordinal()) {
                        msg = new Message();
                        msg.what = 0;
                        break fg;
                    }

                } catch (Exception e) {
                    Log.v("recv exp", e.toString());
                    msg = new Message();
                    msg.what = 2;
                    break fg;
                }
            }
            try {
                out.close();
                in.close();
                socket.close();
                socket = null;
                isConnected = false;
                mHandler.sendMessage(msg);
            } catch (IOException ex) {
                Log.v("IOExp", ex.toString());
            }
        }

    }


}



