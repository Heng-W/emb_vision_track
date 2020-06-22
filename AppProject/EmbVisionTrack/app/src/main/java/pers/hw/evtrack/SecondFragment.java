package pers.hw.evtrack;


import androidx.fragment.app.Fragment;
import android.text.TextWatcher;
import android.text.Editable;
import android.graphics.Color;
import android.graphics.Bitmap;
import android.os.Message;
import android.os.Bundle;
import android.os.Handler;
import android.os.Environment;
import android.content.Context;
import android.view.*;
import android.view.View.*;
import android.view.ViewTreeObserver.*;
import android.widget.*;
import android.util.Log;

import java.io.*;
import pers.hw.evtrack.view.ImageSurfaceView;
import pers.hw.evtrack.view.RoundMenuView;


public class SecondFragment extends Fragment {

    private View view;
    private MainActivity mActivity;
    private Client client;

    private float angleHSet = 90;
    private float angleVSet = 90;

    private Bitmap bitmap;

    private Toast toast;

    private ImageView cameraImage;
    private TextView trackModeTv;

    private TextView panModeTv;
    private ImageButton panModeBtn;

    private Button trackCtlBtn;
    private Button setTargetBtn;

    private ImageSurfaceView imageSurface;

    private Timer timer;



    private Handler mHandler;
    private FragHandler fHandler = new FragHandler();

    private class FragHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            // TODO: Implement this method
            switch (msg.what) {
                case 0:

                    break;
                case 1:
                    if (client.trackFlag) {
                        trackCtlBtn.setText("停止追踪");
                    } else {
                        trackCtlBtn.setText("开始追踪");

                    }
                    if (client.fieldAutoCtlFlag) {
                        panModeTv.setText("云台模式：自动");
                    } else {
                        panModeTv.setText("云台模式：手动");

                    }


                    break;
                case 10:
                    //toast.setText(String.valueOf(client.image.length));
                    //toast.show();
                    if (isHidden()) return;

                    //toast.setText(String.valueOf(newBitmap.getAllocationByteCount()));
                    //toast.show();
                    cameraImage.setImageBitmap(client.bitmap);

                    if (bitmap != null)
                        bitmap.recycle();
                    bitmap = client.bitmap;


                    break;

                case 11:
                    float x = (float)(client.xpos - client.width / 2) / Client.IMAGE_W;
                    float y = (float)(client.ypos - client.height / 2) / Client.IMAGE_H;
                    float w = (float)client.width / Client.IMAGE_W;
                    float h = (float)client.height / Client.IMAGE_H;


                    if (client.trackFlag) {
                  
                        imageSurface.showResult(x, y, w, h);
                    }
                    break;
                case 15:
                    client.writeToServer(Command.image);

                    break;

                case 21:
                    if (client.trackFlag) {
                        trackCtlBtn.setText("停止追踪");
                    } else {
                        trackCtlBtn.setText("开始追踪");

                    }

                    if (msg.arg1 == client.userID) {
                        toast.setText("设置成功");
                    } else {
                        if (client.trackFlag) {

                            toast.setText("ID为" + msg.arg1 + "的用户开启了追踪");
                        } else {
                            toast.setText("ID为" + msg.arg1 + "的用户关闭了追踪");

                        }

                    }

                    toast.show();
                    break;

                case 22:
                    toast.setText("设置成功");
                    toast.show();

                    break;

                case 24:
                    if (client.fieldAutoCtlFlag) {
                        panModeTv.setText("云台模式：自动");
                    } else {
                        angleHSet = client.angleH;
                        angleVSet = client.angleV;

                        panModeTv.setText("云台模式：手动");

                    }

                    if (msg.arg1 == client.userID) {
                        toast.setText("设置成功");
                    } else {
                        if (client.fieldAutoCtlFlag) {

                            toast.setText("ID为" + msg.arg1 + "的用户设置云台为自动模式");
                        } else {
                            toast.setText("ID为" + msg.arg1 + "的用户设置云台为手动模式");

                        }

                    }

                    toast.show();
                    break;

                case 25:
                    if (client.useMultiScale) {
                        trackModeTv.setText("追踪模式：多尺度");
                    } else {
                        trackModeTv.setText("追踪模式：单尺度");

                    }
                    if (msg.arg1 == client.userID) {
                        toast.setText("设置成功");
                    } else {
                        if (client.useMultiScale) {

                            toast.setText("ID为" + msg.arg1 + "的用户开启了多尺度追踪");
                        } else {
                            toast.setText("ID为" + msg.arg1 + "的用户关闭了多尺度追踪");

                        }

                    }
                    toast.show();
                    break;
                default:
                    break;
            }
            super.handleMessage(msg);
        }

    }


    public static SecondFragment newInstance() {

        return new SecondFragment();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        view = inflater.inflate(R.layout.tab_second, container, false);
        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        // TODO: Implement this method
        super.onActivityCreated(savedInstanceState);

        mActivity = (MainActivity)getActivity();
        client = mActivity.getClient();
        client.setHandler(fHandler, 1);
        mHandler = mActivity.getHandler();

        toast = Toast.makeText(getActivity(), "", Toast.LENGTH_SHORT);
        initView();


    }


    private void initView() {

        angleHSet = client.angleH;
        angleVSet = client.angleV;

        cameraImage = view.findViewById(R.id.img);

        cameraImage.setOnTouchListener(new OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent e) {
                v.getParent().requestDisallowInterceptTouchEvent(true);
                return true;
            }
        });


        trackModeTv = view.findViewById(R.id.track_mode_tv);

        trackCtlBtn = view.findViewById(R.id.track_ctl_btn);
        if (client.trackFlag) {
            trackCtlBtn.setText("停止追踪");
        } else {
            trackCtlBtn.setText("开始追踪");

        }

        if (client.useMultiScale) {
            trackModeTv.setText("追踪模式：多尺度");
        } else {
            trackModeTv.setText("追踪模式：单尺度");

        }

        trackCtlBtn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (client.userType == 2) {
                    Message msg = new Message();
                    msg.what = 100;
                    mHandler.sendMessage(msg);
                    return;
                }
                if (client.trackFlag) {
                    client.writeToServer(Command.stop_track);
                } else {
                    client.writeToServer(Command.start_track);

                }

            }
        });

        setTargetBtn = view.findViewById(R.id.set_target_btn);
        setTargetBtn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (client.userType == 2) {
                    Message msg = new Message();
                    msg.what = 100;
                    mHandler.sendMessage(msg);
                    return;
                }
                if (imageSurface != null) {
                    if (imageSurface.isDrawEnabled()) {
                        imageSurface.disableDraw();
                        setTargetBtn.setText("选择目标");

                    } else {
                        imageSurface.enableDraw();
                        setTargetBtn.setText("取消选择");
                    }
                }

            }
        });


        imageSurface = view.findViewById(R.id.img_surface);
        imageSurface.setOnEventListener(new ImageSurfaceView.OnEventListener() {

            @Override
            public void onTouchOver(float x, float y, float w, float h) {
                if (w < 0.05 || h < 0.05) {
                    toast.setText("目标太小，请重试");
                    toast.show();
                    return;
                }

                Packet p = client.newPacket(Command.set_target);
                p.writeUint16((int)(x * Client.IMAGE_W));
                p.writeUint16((int)(y * Client.IMAGE_H));
                p.writeUint16((int)(w * Client.IMAGE_W));
                p.writeUint16((int)(h * Client.IMAGE_H));
                try {
                    client.sendPacket(p);
                } catch (IOException e) {
                    Log.v("IOExp", "set target");
                }
                imageSurface.disableDraw();

                setTargetBtn.setText("选择目标");
            }

        });

        panModeTv = view.findViewById(R.id.pan_mode_tv);

        if (client.fieldAutoCtlFlag) {
            panModeTv.setText("云台模式：自动");
        } else {
            panModeTv.setText("云台模式：手动");

        }

        panModeBtn = view.findViewById(R.id.pan_mode_btn);

        panModeBtn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (client.userType == 2) {
                    Message msg = new Message();
                    msg.what = 100;
                    mHandler.sendMessage(msg);
                    return;
                }
                if (client.fieldAutoCtlFlag) {
                    client.writeToServer(Command.disable_field_autoctl);
                } else {
                    client.writeToServer(Command.enable_field_autoctl);

                }

            }
        });


        RoundMenuView roundMenu = view.findViewById(R.id.round_menu);
        roundMenu.setOnMenuClickListener(new RoundMenuView.OnMenuClickListener() {

            @Override
            public void onClick(RoundMenuView roundMenu, int idx) {
                if (client.userType == 2) {
                    Message msg = new Message();
                    msg.what = 100;
                    mHandler.sendMessage(msg);
                    return;
                }
                if (client.fieldAutoCtlFlag) return;

                switch (idx) {
                    case 0:
                        angleVSet -= 5;
                        break;
                    case 1:

                        angleHSet -= 5;

                        break;
                    case 2:

                        angleVSet += 5;

                        break;
                    case 3:

                        angleHSet += 5;

                        break;
                    default:
                        break;
                }
                if (angleHSet > 180) angleHSet = 180;
                else if (angleHSet < 0) angleHSet = 0;

                if (angleVSet > 180) angleVSet = 180;
                else if (angleVSet < 0) angleVSet = 0;

                Packet p = client.newPacket(Command.set_servo_val);
                p.writeInt32((int)(angleHSet * 100));
                p.writeInt32((int)(angleVSet * 100));
                try {
                    client.sendPacket(p);
                } catch (IOException e) {
                    Log.v("set_servo_val", e.toString());
                }



            }
        });


    }


    @Override
    public void onStart() {
        // TODO: Implement this method
        super.onStart();

        startTimer();
    }

    @Override
    public void onStop() {
        // TODO: Implement this method
        super.onStop();

        stopTimer();
    }



    @Override
    public void onHiddenChanged(boolean hidden) {
        // TODO: Implement this method
        super.onHiddenChanged(hidden);
        if (hidden) {
            stopTimer();
        } else {
            startTimer();

        }
    }

    private void startTimer() {
        if (timer == null) {
            timer = new Timer();
            timer.start();
        }

    }


    private void stopTimer() {
        if (timer != null) {
            if (timer.isAlive()) {
                timer.interrupt();
            }
            timer = null;
        }
    }


    private class Timer extends Thread {

        @Override
        public void run() {
            while (client.isConnected) {
                try {
                    Message msg = new Message();
                    msg.what = 15;
                    fHandler.sendMessage(msg);
                    Thread.sleep(80);
                } catch (InterruptedException e) {
                    break;
                }
            }
        }
    }


}
