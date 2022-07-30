package pers.hw.evtrack;


import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import android.app.Dialog;
import android.app.AlertDialog;
import android.os.StrictMode;
import android.os.Message;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.WindowManager;
import android.view.ViewGroup;
import android.view.MenuItem;
import android.view.LayoutInflater;
import android.view.ViewGroupOverlay;
import android.view.View.OnClickListener;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.Color;
import android.widget.*;
import android.content.Context;

import java.net.InetSocketAddress;
import java.lang.reflect.Field;
import java.util.Timer;
import java.util.TimerTask;

import pers.hw.evtrack.net.Buffer;
import pers.hw.evtrack.net.TcpConnection;
import pers.hw.evtrack.view.DialogUtils;


public class MainActivity extends FragmentActivity {

    public static final int FRAG_MAX = 4;
    private FragmentComponent[] fc;
    private Dialog loadDialog;
    private TextView titleTv;
    private Toast toast;
    private Client client;
    private int fragIndex = FRAG_MAX;

    private PopupMenu popupMenu;
    private MainHandler mHandler = new MainHandler();

    private final InetSocketAddress DEFAULT_SERVER_ADDR = new InetSocketAddress("192.168.5.1",18825);

    private InetSocketAddress serverAddr = DEFAULT_SERVER_ADDR;

    private int userType;
    private String userName;
    private String password;
    private Timer timer;


    private class MainHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            // TODO: Implement this method
            switch (msg.what) {
                case 0:
                    refreshConnectView();
                    toast.setText("连接已关闭");
                    toast.show();
                    break;
                case 1:
                    refreshConnectView();
                    toast.setText("连接中断");
                    toast.show();
                    break;
                case 2:
                    refreshConnectView();
                    toast.setText("处理异常");
                    toast.show();
                    break;
                case 7:
                    refreshConnectView();
                    if (msg.arg1 == 1) {
                        toast.setText("账号已在别处登录");
                        toast.show();
                    }
                    break;
                case 10:
                    clearDim();
                    DialogUtils.closeDialog(loadDialog);
                    if (client.connection() != null) {
                        toast.setText("连接成功");
                        toast.show();
                        initMainView();
                    } else {
                        toast.setText("连接失败");
                        toast.show();
                        getWindow().getDecorView().setBackgroundColor(getResources().getColor(R.color.white));
                        getWindow().setStatusBarColor(getResources().getColor(R.color.primary));
                        getWindow().setNavigationBarColor(getResources().getColor(R.color.nav_bar));
                        setContentView(R.layout.activity_main);
                        titleTv = findViewById(R.id.title_text);
                        LinearLayout menuIcon = findViewById(R.id.menu_icon);
                        initPopupMenu(menuIcon);
                        menuIcon.setOnClickListener(new OnClickListener() {
                            public void onClick(View v) {
                                showPopupMenu();
                            }

                        });
                        initFragmentComponent();
                        setSelect(0);
                    }
                    break;
                case 12:
                    timer.cancel();
                    clearDim();
                    DialogUtils.closeDialog(loadDialog);
                    switch (msg.arg1) {
                        case -1:
                            toast.setText("找不到服务器");
                            toast.show();
                            break;
                        case 0:
                            toast.setText("服务器连接成功");
                            toast.show();
                            initMainView();
                            break;
                        case 1:
                            toast.setText("密码错误，请重试");
                            toast.show();
                            try {
                                client.connection().shutdown();
                            } catch (Exception e) {}
                            break;
                        case 2:
                            toast.setText("用户名不存在");
                            toast.show();
                            try {
                                client.connection().shutdown();
                            } catch (Exception e) {}
                            break;
                        case 3:
                            toast.setText("用户ID未注册");
                            toast.show();
                            try {
                                client.connection().shutdown();
                            } catch (Exception e) {}
                            break;
                        default:
                            break;
                    }
                    break;
                case 21:
                    toast.setText("摄像头复位成功");
                    toast.show();
                    break;
                case 22:
                    if (msg.arg1 == client.userID) {
                        toast.setText("状态复位成功");
                    } else {
                        toast.setText("ID为" + msg.arg1 + "的用户复位了状态");
                    }
                    toast.show();
                    break;
                case 50:
                    // toast.setText(client.noticeMsg);
                    // toast.show();
                    initNoticeDialog("消息", (String)msg.obj);
                    break;
                case 80:
                    toast.setText("image");
                    toast.show();
                    break;
                case 88:
                    setSelect(0);
                    break;
                case 100:
                    toast.setText("用户权限不足");
                    toast.show();
                    break;
                default:
                    break;
            }
            super.handleMessage(msg);
        }

    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.connect);

        StrictMode.setThreadPolicy(
            new StrictMode.ThreadPolicy.Builder().detectDiskReads().detectDiskWrites().detectNetwork().penaltyLog().build());
        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder().detectLeakedSqlLiteObjects().penaltyLog().penaltyDeath().build());

        // requestPermission();
        toast = Toast.makeText(this, "", Toast.LENGTH_SHORT);
        fc = new FragmentComponent[FRAG_MAX];
        initConnectView();
    }


    public Client getClient() {
        return this.client;
    }

    public Handler getHandler() {
        return this.mHandler;
    }

    public int dp2px(float dpValue) {
        WindowManager wm = (WindowManager) this.getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics dm = new DisplayMetrics();
        wm.getDefaultDisplay().getMetrics(dm);
        float density = dm.density;
        return (int)(dpValue * density + 0.5f);
    }

    public int px2dp(float pxValue) {
        WindowManager wm = (WindowManager) this.getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics dm = new DisplayMetrics();
        wm.getDefaultDisplay().getMetrics(dm);
        float density = dm.density;
        return (int)(pxValue / density + 0.5f);
    }

    public int getScreenWidth() {
        return getWindowManager().getDefaultDisplay().getWidth();
    }

    public int getScreenHeight() {
        return getWindowManager().getDefaultDisplay().getHeight();
    }

    public void applyDim(float dimAmount) {
        ViewGroup parent = (ViewGroup) getWindow().getDecorView().getRootView();
        Drawable dim = new ColorDrawable(Color.BLACK);
        dim.setBounds(0, 0, parent.getWidth(), parent.getHeight());
        dim.setAlpha((int)(255 * dimAmount));
        ViewGroupOverlay overlay = parent.getOverlay();
        overlay.add(dim);
    }

    public void clearDim() {
        ViewGroup parent = (ViewGroup) getWindow().getDecorView().getRootView();
        ViewGroupOverlay overlay = parent.getOverlay();
        overlay.clear();
    }

    private void refreshConnectView() {
        userType = 0;

        fragIndex = FRAG_MAX;
        for (int i = 0; i < FRAG_MAX; i++) {
            fc[i].release();
        }
        getWindow().getDecorView().setBackgroundColor(getResources().getColor(R.color.window_bg));
        getWindow().setStatusBarColor(getResources().getColor(R.color.window_bg));
        getWindow().setNavigationBarColor(getResources().getColor(R.color.window_bg));
        titleTv.setText(getResources().getString(R.string.app_name));

        setContentView(R.layout.connect);
        initConnectView();
    }

    private void initMainView() {
        client.send(Command.INIT);

        getWindow().getDecorView().setBackgroundColor(getResources().getColor(R.color.white));
        getWindow().setStatusBarColor(getResources().getColor(R.color.primary));
        getWindow().setNavigationBarColor(getResources().getColor(R.color.nav_bar));
        setContentView(R.layout.activity_main);
        titleTv = findViewById(R.id.title_text);
        LinearLayout menuIcon =  findViewById(R.id.menu_icon);
        initPopupMenu(menuIcon);
        menuIcon.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                showPopupMenu();
            }

        });
        initFragmentComponent();
    }

    private void initPopupMenu(View v) {
        popupMenu = new PopupMenu(this, v);
        getMenuInflater().inflate(R.menu.main, popupMenu.getMenu());

        popupMenu.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.action_exit: {
                        TcpConnection conn = client.connection();
                        if (conn != null) {
                            conn.shutdown();
                        }
                        Message msg = mHandler.obtainMessage(0);
                        mHandler.sendMessage(msg);
                        break;
                    }
                    case R.id.action_reset: {
                        if (userType != 0) {
                            Message msg = mHandler.obtainMessage(100);
                            mHandler.sendMessage(msg);
                            return true;
                        }
                        Buffer buf = Client.createBuffer(Command.RESET);
                        buf.appendInt32(0x7);
                        client.send(buf);
                        break;
                    }
                    case R.id.action_camera_reset: {
                        if (userType != 0) {
                            Message msg = mHandler.obtainMessage(100);
                            mHandler.sendMessage(msg);
                            return true;
                        }
                        Buffer buf = Client.createBuffer(Command.RESET);
                        buf.appendInt32(0x2);
                        client.send(buf);
                        break;
                    }
                    case R.id.action_track_mode:
                        if (userType == 2) {
                            Message msg = new Message();
                            msg.what = 100;
                            mHandler.sendMessage(msg);
                            return true;
                        }
                        if (client.useMultiScale) {
                            client.send(Command.DISABLE_MULTI_SCALE);
                        } else {
                            client.send(Command.ENABLE_MULTI_SCALE);
                        }
                        break;
                    case R.id.action_halt:
                        if (client.userID != 0) {
                            Message msg = new Message();
                            msg.what = 100;
                            mHandler.sendMessage(msg);
                            return true;
                        }
                        client.send(Command.HALT);
                        break;
                    case R.id.action_reboot:
                        if (client.userID != 0) {
                            Message msg = new Message();
                            msg.what = 100;
                            mHandler.sendMessage(msg);
                            return true;
                        }
                        client.send(Command.REBOOT);
                        break;
                    default:
                        break;
                }
                return true;
            }
        });
        popupMenu.setOnDismissListener(new PopupMenu.OnDismissListener() {
            @Override
            public void onDismiss(PopupMenu menu) {
                clearDim();
            }
        });
        try {
            Field field = popupMenu.getClass().getDeclaredField("mPopup");
            field.setAccessible(true);
            /*   MenuPopupHelper helper = (MenuPopupHelper) field.get(popupMenu);
             helper.setForceShowIcon(true);*/
        } catch (Exception e) {
        }
    }


    private void initNoticeDialog(String title, String msg) {
        AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
        builder.setIcon(R.mipmap.icon_launcher_round);
        builder.setTitle(title);
        //    通过LayoutInflater来加载一个xml的布局文件作为一个View对象
        View view = LayoutInflater.from(MainActivity.this).inflate(R.layout.dialog_notice, null);
        //    设置我们自己定义的布局文件作为弹出框的Content
        builder.setView(view);
        builder.setPositiveButton("确定", null);

        final TextView noticeTv = view.findViewById(R.id.dialog_notice_tv);

        noticeTv.setText(msg);
        // builder.create();
        final AlertDialog dialog = builder.create();
        dialog.show();
    }

    private boolean checkAddress(String s) {
        return s.matches("((25[0-5]|2[0-4]\\d|((1\\d{2})|([1-9]?\\d)))\\.){3}(25[0-5]|2[0-4]\\d|((1\\d{2})|([1-9]?\\d)))") ||
               s.matches("[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(\\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})+\\.?");
    }

    private boolean checkPort(String s) {
        return s.matches("[0-9]|[1-9]\\d{1,3}|[1-5]\\d{4}|6[0-5]{2}[0-3][0-5]");
    }

    private void initServerAddrDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
        builder.setIcon(R.mipmap.icon_launcher_round);
        builder.setTitle("更改服务器地址");
        //    通过LayoutInflater来加载一个xml的布局文件作为一个View对象
        View view = LayoutInflater.from(MainActivity.this).inflate(R.layout.dialog_server_addr, null);
        //    设置我们自己定义的布局文件作为弹出框的Content
        builder.setView(view);
        builder.setPositiveButton("确定", null);
        builder.setNegativeButton("取消", null);
        builder.setNeutralButton("重置", null);

        final EditText serverIpEdit = view.findViewById(R.id.server_addr_edt);
        final EditText serverPortEdit = view.findViewById(R.id.server_port_edt);

        serverIpEdit.setText(serverAddr.getHostString());
        serverPortEdit.setText(String.valueOf(serverAddr.getPort()));

        // builder.create();
        final AlertDialog dialog = builder.create();
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(
        new OnClickListener() {

            @Override
            public void onClick(View v) {
                String serverIp = serverIpEdit.getText().toString();
                String serverPort = serverPortEdit.getText().toString();

                if (!checkAddress(serverIp)) {
                    toast.setText("IP格式错误");
                    toast.show();
                    return;
                }
                if (!checkPort(serverPort)) {
                    toast.setText("端口格式错误");
                    toast.show();
                    return;
                }

                serverAddr = new InetSocketAddress(serverIp, Integer.parseInt(serverPort));

                toast.setText("设置成功");
                toast.show();
                dialog.dismiss();

            }
        });
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(
        new OnClickListener() {

            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });

        dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(
        new OnClickListener() {

            @Override
            public void onClick(View v) {
                serverIpEdit.setText(DEFAULT_SERVER_ADDR.getHostString());
                serverPortEdit.setText(String.valueOf(DEFAULT_SERVER_ADDR.getPort()));
            }
        });
    }

    private void showPopupMenu() {

        switch (fragIndex) {
            case 0:
                //tstItem.setVisible(false);
                break;
            default:
                break;
        }
        popupMenu.show();
        applyDim(0.15f);
    }


    private void initConnectView() {
        final EditText userEdt = findViewById(R.id.user);
        final EditText pwdEdt = findViewById(R.id.pwd);

        final RadioGroup userGroup = findViewById(R.id.user_group);

        userGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {

            public void onCheckedChanged(RadioGroup group, int i) {
                switch (i) {
                    case R.id.root_user:
                        userEdt.setText("root");
                        pwdEdt.setText("");
                        pwdEdt.setVisibility(View.VISIBLE);
                        userType = 0;
                        break;
                    case R.id.normal_user:
                        userEdt.setText("");
                        pwdEdt.setText("");
                        pwdEdt.setVisibility(View.VISIBLE);
                        userType = 1;
                        break;
                    case R.id.guest_user:
                        userEdt.setText("guest");
                        pwdEdt.setVisibility(View.INVISIBLE);
                        userType = 2;
                        break;
                }
            }
        });

        Button connectBtn = findViewById(R.id.connect_button);
        connectBtn.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                userName = userEdt.getText().toString();
                password = pwdEdt.getText().toString();

                if (userType == 2) {
                    userName = "guest";
                    password = "";
                } else {
                    if (userName.equals("")) {
                        toast.setText("请输入用户名");
                        toast.show();
                        return;
                    }
                    if (password.equals("")) {
                        toast.setText("请输入密码");
                        toast.show();
                        return;
                    }
                    if (!userName.matches("[A-Za-z_][A-Za-z0-9_]*")) {
                        toast.setText("用户名格式不正确");
                        toast.show();
                        return;
                    }
                }
                // start to connect
                loadDialog = DialogUtils.createLoadingDialog(MainActivity.this, "连接中...");
                applyDim(0.2f);
                new Thread(new Runnable() {
                    public void run() {
                        client = new Client(serverAddr, userName, password);
                        client.setMainActivity(MainActivity.this);
                        client.setMainHandler(mHandler);
                        timer = new Timer("Timer");
                        timer.schedule(new TimerTask() {
                            public void run() {
                                if (client.connection() == null) {
                                    client.stop();
                                    Message msg = mHandler.obtainMessage(12, -1, 0);
                                    mHandler.sendMessage(msg);
                                }
                            }
                        }, 5000);

                        client.start();
                    }
                }).start();
            }
        });

        TextView serverIpTv = findViewById(R.id.server_ip);
        serverIpTv.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                initServerAddrDialog();
            }

        });
    }

    private void initFragmentComponent() {
        fc[0] = new FragmentComponent(0, R.id.first_sel, R.id.first_tv);
        fc[1] = new FragmentComponent(1, R.id.second_sel, R.id.second_tv);
        fc[2] = new FragmentComponent(2, R.id.third_sel, R.id.third_tv);
        fc[3] = new FragmentComponent(3, R.id.fourth_sel, R.id.fourth_tv);
    }

    private void setSelect(int i) {
        FragmentManager fm = getSupportFragmentManager();
        FragmentTransaction transaction = fm.beginTransaction(); // 创建一个事务
        if (fragIndex != FRAG_MAX) {
            fc[fragIndex].dismiss(transaction);
        }
        fragIndex = i;
        if (fragIndex != FRAG_MAX) {
            fc[fragIndex].display(transaction);
        }
        transaction.commit();//提交事务
    }

    private class FragmentComponent {

        private Fragment fragTab;
        private LinearLayout fragSel;
        private TextView tv;
        // private ImageView img;
        // private int primaryImgSrc, selectedImgSrc;
        private String label;
        private final int index;

        public FragmentComponent(int i, int fragSelId, int tvId) {
            index = i;
            fragSel = findViewById(fragSelId);
            tv = findViewById(tvId);
            // img = (ImageView)findViewById(imgId);
            label = (String) tv.getText();
            // primaryImgSrc = pImgSrc;
            // selectedImgSrc = sImgSrc;
            fragSel.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    setSelect(index);
                }
            });
        }

        public void dismiss(FragmentTransaction transaction) {
            if (fragTab != null) {
                transaction.hide(fragTab);
                tv.setTextColor(getResources().getColor(R.color.dark_grey));
                // img.setImageDrawable(getResources().getDrawable(primaryImgSrc));
            }
        }

        public void display(FragmentTransaction transaction) {
            if (fragTab == null) {
                createFragment();
                transaction.add(R.id.frag_content, fragTab); // 添加到Activity
            } else {
                transaction.show(fragTab);
            }
            tv.setTextColor(getResources().getColor(R.color.deep_blue));
            // img.setImageDrawable(getResources().getDrawable(selectedImgSrc));
            titleTv.setText(label);
        };

        public void release() {
            if (fragTab == null)
                return;
            FragmentManager fm = getSupportFragmentManager();
            FragmentTransaction transaction = fm.beginTransaction(); // 创建一个事务
            transaction.remove(fragTab);
            transaction.commit(); // 提交事务
            fragTab = null;
        }

        private void createFragment() {
            switch (index) {
                case 0:
                    fragTab = FirstFragment.newInstance();
                    break;
                case 1:
                    fragTab = SecondFragment.newInstance();
                    break;
                case 2:
                    fragTab = ThirdFragment.newInstance();
                    break;
                case 3:
                    fragTab = FourthFragment.newInstance();
                    break;
                default:
                    break;
            }
        }

    }


}
