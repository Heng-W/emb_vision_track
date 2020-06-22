package pers.hw.evtrack;


import androidx.fragment.app.Fragment;
import android.widget.*;
import android.os.Message;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.View.*;
import android.view.MotionEvent;
import android.util.Log;
import android.text.*;

import java.io.*;
import pers.hw.evtrack.view.SteerView;


public class FirstFragment extends Fragment {

    private View view;
    private MainActivity mActivity;
    private Client client;

    private Toast toast;

    private TextView driveModeTv;

    private SteerView steer;

    private TextView speedTv;

    private int onMoveCnt;

    private int speedAvg;

    private float steerAngle;

    private int leftCtlVal;
    private int rightCtlVal;

    private boolean isAhead;
    private boolean motorStopFlag = true;

    private boolean editListenFlag = true;

    private EditText[][] paramEdit = new EditText[4][2];


    private Handler editHandler = new Handler();
    private Handler mHandler;
    private FragHandler fHandler = new FragHandler();


    private class FragHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            // TODO: Implement this method
            switch (msg.what) {
                case 0:
                //startActivityForResult(new Intent(mActivity,SettingActivity.class),1);
                case 1:
                    if (client.motionAutoCtlFlag) {
                        driveModeTv.setText("模式：自动驾驶");
                    } else {
                        driveModeTv.setText("模式：手动驾驶");
                    }

                    if (msg.arg1 == client.userID) {
                        toast.setText("设置成功");
                    } else {
                        if (client.motionAutoCtlFlag) {

                            toast.setText("ID为" + msg.arg1 + "的用户设置了自动驾驶");
                        } else {
                            toast.setText("ID为" + msg.arg1 + "的用户设置了手动驾驶");

                        }

                    }
                    toast.show();

                    break;
                case 10:
                    if (msg.arg1 == client.userID) {
                        toast.setText("设置成功");
                    } else {
                        client.writeToServer(Command.update_pid);
                        toast.setText("ID为" + msg.arg1 + "的用户设置了PID参数");

                    }
                    toast.show();
                    break;
                case 12:
                    writeParam();
                    break;
                case 20:
                    editListenFlag = false;
                    for (int i = 0; i < 4; i++) {
                        for (int j = 0; j < 2; j++) {

                            paramEdit[i][j].setText(String.valueOf(client.pidParams[i][j]));
                        }
                    }
                    editListenFlag = true;
                    break;


                default:
                    break;
            }
            super.handleMessage(msg);
        }

    }

    public static FirstFragment newInstance() {

        return new FirstFragment();
    }


    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        view = inflater.inflate(R.layout.tab_first, container, false);
        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        // TODO: Implement this method
        super.onActivityCreated(savedInstanceState);
        mActivity = (MainActivity)getActivity();
        client = mActivity.getClient();
        client.setHandler(fHandler, 0);
        mHandler = mActivity.getHandler();
        toast = Toast.makeText(getActivity(), "", Toast.LENGTH_SHORT);
        initView();

    }

    private void initView() {
        driveModeTv =  view.findViewById(R.id.drive_mode);
        if (client.motionAutoCtlFlag) {
            driveModeTv.setText("模式：自动驾驶");
        } else {
            driveModeTv.setText("模式：手动驾驶");

        }

        steer = view.findViewById(R.id.steer);

        steer.setOnTouchListener(new SteerView.OnTouchListener() {

            @Override
            public void onMove(SteerView view, float angle) {
                steerAngle = angle;
                steer.setRotation(angle);

                if (++onMoveCnt > 20) {
                    onMoveCnt = 0;

                    updateLeftVal();
                    updateRightVal();

                    writeMotorVal();
                    //driveModeTv.setText(String.valueOf(angle));

                }
            }
        });

        ImageButton aheadBtn = view.findViewById(R.id.ahead_btn);
        aheadBtn.setOnTouchListener(new OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent e) {

                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        if (client.userType == 2) {
                            Message msg = new Message();
                            msg.what = 100;
                            mHandler.sendMessage(msg);
                            return true;
                        }
                        if (client.motionAutoCtlFlag) {
                            toast.setText("自动模式下无法操控");
                            toast.show();
                            return true;
                        }

                        isAhead = true;
                        motorStopFlag = false;
                        updateLeftVal();
                        updateRightVal();
                        writeMotorVal();
                        break;

                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        if (client.userType == 2) {
                            return true;
                        }
                        if (client.motionAutoCtlFlag) {
                            return true;
                        }


                        leftCtlVal = 0;
                        rightCtlVal = 0;
                        writeMotorVal();

                        motorStopFlag = true;


                        break;
                    default:
                        break;
                }
                return true;
            }
        });

        ImageButton reverseBtn = view.findViewById(R.id.reverse_btn);
        reverseBtn.setOnTouchListener(new OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent e) {

                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        if (client.userType == 2) {
                            Message msg = new Message();
                            msg.what = 100;
                            mHandler.sendMessage(msg);
                            return true;
                        }

                        if (client.motionAutoCtlFlag) {
                            toast.setText("自动模式下无法操控");
                            toast.show();

                            return true;
                        }

                        isAhead = false;
                        motorStopFlag = false;
                        updateLeftVal();
                        updateRightVal();
                        writeMotorVal();
                        break;

                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        if (client.userType == 2) {
                            return true;
                        }

                        if (client.motionAutoCtlFlag) {
                            return true;
                        }

                        leftCtlVal = 0;
                        rightCtlVal = 0;
                        writeMotorVal();

                        motorStopFlag = true;



                        break;
                    default:
                        break;
                }
                return true;
            }
        });

        speedTv = view.findViewById(R.id.speed_tv);


        SeekBar speedBar = view.findViewById(R.id.speed_bar);
        speedBar.setMax(500);
        speedBar.setProgress(50);
        speedBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                speedTv.setText("速度: " + String.valueOf((int)(100.0f * progress / seekBar.getMax())) + "%");
            }
            public void onStartTrackingTouch(SeekBar seekBar) {}
            public void onStopTrackingTouch(SeekBar seekBar) {
                speedAvg = seekBar.getProgress();
                updateLeftVal();
                updateRightVal();
                writeMotorVal();
            }
        });
		
        speedAvg = speedBar.getProgress();

        ImageButton driveModeBtn = view.findViewById(R.id.drive_mode_btn);
        driveModeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (client.userType == 2) {
                    Message msg = new Message();
                    msg.what = 100;
                    mHandler.sendMessage(msg);
                    return;
                }
                if (client.motionAutoCtlFlag) {
                    client.writeToServer(Command.disable_motion_autoctl);

                } else {
                    client.writeToServer(Command.enable_motion_autoctl);

                }
            }

        });


        ImageButton motorStopBtn = view.findViewById(R.id.motor_stop_btn);
        motorStopBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (client.userType == 2) {
                    Message msg = new Message();
                    msg.what = 100;
                    mHandler.sendMessage(msg);
                    return;
                }

                client.writeToServer(Command.disable_motion_autoctl);
                leftCtlVal = 0;
                rightCtlVal = 0;
                writeMotorVal();

                motorStopFlag = true;
                //client.writeToServer(Command.stop_motor);
            }

        });

        paramEdit[0][0] = view.findViewById(R.id.edit_servo1_p);
        paramEdit[0][1] = view.findViewById(R.id.edit_servo1_d);
        paramEdit[1][0] = view.findViewById(R.id.edit_servo2_p);
        paramEdit[1][1] = view.findViewById(R.id.edit_servo2_d);
        paramEdit[2][0] = view.findViewById(R.id.edit_dir_p);
        paramEdit[2][1] = view.findViewById(R.id.edit_dir_d);
        paramEdit[3][0] = view.findViewById(R.id.edit_dist_p);
        paramEdit[3][1] = view.findViewById(R.id.edit_dist_i);

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 2; j++) {
                final int idx = i * 2 + j;


                paramEdit[i][j].setText(String.valueOf(client.pidParams[i][j]));

                paramEdit[i][j].addTextChangedListener(new TextWatcher() {
                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                    }
                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                    }
                    @Override
                    public void afterTextChanged(Editable s) {
                        if (!editListenFlag) return;

                        editHandler.removeCallbacks(delayRun);

                        editIdx = idx;

                        editStr = s.toString();
                        //延迟800ms，如果不再输入字符，则执行该线程的run方法
                        editHandler.postDelayed(delayRun, 1000);


                    }



                });
            }
        }



    }


    private int editIdx;
    private String editStr;



    /**
     * 延迟线程，看是否还有下一个字符输入
     */
    private Runnable delayRun = new Runnable() {


        @Override
        public void run() {

            Message msg = new Message();
            msg.what = 12;
            fHandler.sendMessage(msg);


        }
    };

    private void writeParam() {
        if (editStr == null) return;
        Packet p;

        if (editIdx < 4) {
            p = client.newPacket(Command.set_field_ctl_pid);
            p.writeInt32(editIdx);
        } else {
            p = client.newPacket(Command.set_motion_ctl_pid);
            p.writeInt32(editIdx - 4);

        }
        p.writeFloat(Float.parseFloat(editStr));
        try {
            client.sendPacket(p);
        } catch (IOException e) {
            Log.v("set_motion_pid", e.toString());
        }
    }


    public void writeMotorVal() {
        if (client.motionAutoCtlFlag || motorStopFlag)
            return;

        Packet p = client.newPacket(Command.set_motor_val);

        p.writeInt32(leftCtlVal);
        p.writeInt32(rightCtlVal);

        try {
            client.sendPacket(p);
        } catch (IOException e) {
            Log.v("set_motor_val", e.toString());
        }
    }

    public void updateLeftVal() {
        leftCtlVal = (int)(speedAvg * (1 + steerAngle / 90));
        if (!isAhead) {
            leftCtlVal = -leftCtlVal;
        }
    }

    public void updateRightVal() {
        rightCtlVal = (int)(speedAvg * (1 - steerAngle / 90));
        if (!isAhead) {
            rightCtlVal = -rightCtlVal;
        }
    }


}
