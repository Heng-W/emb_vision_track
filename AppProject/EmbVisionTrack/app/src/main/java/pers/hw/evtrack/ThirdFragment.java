package pers.hw.evtrack;


import androidx.fragment.app.Fragment;
import android.os.Message;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.View.OnClickListener;
import android.widget.*;
import android.graphics.Color;

import java.util.List;
import java.util.ArrayList;

import com.github.mikephil.charting.charts.LineChart;


public class ThirdFragment extends Fragment {
    private View view;
    private MainActivity mActivity;
    private Client client;

    private Toast toast;

    private TextView leftValTv;
    private TextView rightValTv;
    private TextView anglehTv, anglevTv;
    private TextView trackTv;
    private LinearLayout trackLinear;
    private TextView xposTv, yposTv, widthTv, heightTv;

    private Timer timer;

    private LineChart chart;
    private DynamicLineChart dynamicLineChart;
    private List<Integer> lineDataList = new ArrayList<>(); //数据集合
    private List<String> lineNames = new ArrayList<>(); //折线名字集合
    private List<Integer> lineColor = new ArrayList<>();//折线颜色集合


    private Handler mHandler;
    private FragHandler fHandler = new FragHandler();

    private class FragHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            // TODO: Implement this method
            switch (msg.what) {
                case 1:
                    break;
                case 15:
                    client.send(Command.MESSAGE);
                    break;
                case 22:
                    anglehTv.setText("水平角:" + client.angleH + "°");
                    anglevTv.setText("垂直角:" + client.angleV + "°");

                    leftValTv.setText("左电机:" + client.leftCtlVal);
                    rightValTv.setText("右电机:" + client.rightCtlVal);

                    addLineData();

                    if (!client.trackFlag) {
                        trackTv.setVisibility(View.VISIBLE);//visible
                        trackLinear.setVisibility(View.GONE);//gone
                    } else {
                        trackTv.setVisibility(View.GONE);
                        trackLinear.setVisibility(View.VISIBLE);
                        xposTv.setText("x坐标:" + client.xpos);
                        yposTv.setText("y坐标:" + client.ypos);
                        widthTv.setText("宽度:" + client.width);
                        heightTv.setText("高度:" + client.height);

                    }

                    break;
                default:
                    break;
            }
            super.handleMessage(msg);
        }

    }

    public static ThirdFragment newInstance() {

        return new ThirdFragment();
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        view = inflater.inflate(R.layout.tab_third, container, false);
        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        // TODO: Implement this method
        super.onActivityCreated(savedInstanceState);
        mActivity = (MainActivity)getActivity();
        client = mActivity.getClient();
        client.setHandler(fHandler, 2);
        mHandler = mActivity.getHandler();
        toast = Toast.makeText(getActivity(), "", Toast.LENGTH_SHORT);
        initView();

    }

    private void initView() {

        chart = view.findViewById(R.id.lineChart);

        lineNames.add("水平角偏移");
        lineNames.add("垂直角偏移");

        lineNames.add("目标x轴偏移");
        lineNames.add("目标y轴偏移");


        lineColor.add(Color.RED);
        lineColor.add(Color.parseColor("#ff7f00"));

        lineColor.add(Color.parseColor("#238e23"));
        lineColor.add(Color.parseColor("#3299cc"));


        dynamicLineChart = new DynamicLineChart(chart, lineNames, lineColor);

        dynamicLineChart.setYAxis(50, -50, 10);

        dynamicLineChart.setDescription("时间");

        anglehTv = view.findViewById(R.id.angleh_tv);
        anglevTv = view.findViewById(R.id.anglev_tv);

        leftValTv = view.findViewById(R.id.left_val_tv);

        rightValTv = view.findViewById(R.id.right_val_tv);


        trackTv =  view.findViewById(R.id.track_tv);

        trackLinear =  view.findViewById(R.id.track_linear);

        xposTv = view.findViewById(R.id.xpos_tv);
        yposTv = view.findViewById(R.id.ypos_tv);

        widthTv = view.findViewById(R.id.width_tv);
        heightTv = view.findViewById(R.id.height_tv);


    }

    private void addLineData() {
        lineDataList.add((int)(getAnglehOfs() / 1.8));
        lineDataList.add((int)(getAnglevOfs() / 1.8));
        lineDataList.add(getXposOfs() * 100 / client.IMAGE_W);
        lineDataList.add(getYposOfs() * 100 / client.IMAGE_H);

        dynamicLineChart.addEntry(lineDataList);
        lineDataList.clear();
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


    private class Timer extends Thread {

        @Override
        public void run() {
            while (client.connection() != null) {
                try {
                    Message msg = new Message();
                    msg.what = 15;
                    fHandler.sendMessage(msg);
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    break;
                }
            }
        }
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


    private float getAnglehOfs() {
        return client.angleHDefault - client.angleH;
    }

    private float getAnglevOfs() {
        return client.angleV - client.angleVDefault;
    }

    private int getXposOfs() {
        return client.xpos - client.IMAGE_W / 2;
    }

    private int getYposOfs() {
        return client.ypos - client.IMAGE_H / 2;
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

}
