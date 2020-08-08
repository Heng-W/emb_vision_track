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


public class FourthFragment extends Fragment {
    private View view;
    private MainActivity mActivity;
    private Client client;

    private Toast toast;

    private TextView cmdTv;
    private EditText cmdEdit;

    private Button cmdBtn;

    private TextView clientCntTv;

    private EditText msgEdit;


    private Handler mHandler;
    private FragHandler fHandler = new FragHandler();


    private class FragHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            // TODO: Implement this method
            switch (msg.what) {
                case 1:

                    break;
                case 5:

                    cmdTv.setText(client.cmdResult);

                    break;
                case 20:
                    clientCntTv.setText("当前用户数量: " + client.clientCnt);
                    break;
                case 22:
                    toast.setText("发送成功");
                    toast.show();

                    break;
                default:
                    break;
            }
            super.handleMessage(msg);
        }

    }

    public static FourthFragment newInstance() {

        return new FourthFragment();
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        view = inflater.inflate(R.layout.tab_fourth, container, false);
        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        // TODO: Implement this method
        super.onActivityCreated(savedInstanceState);
        mActivity = (MainActivity)getActivity();
        client = mActivity.getClient();
        client.setHandler(fHandler, 3);
        mHandler = mActivity.getHandler();
        toast = Toast.makeText(getActivity(), "", Toast.LENGTH_SHORT);
        initView();


    }

    private void initView() {

        clientCntTv = view.findViewById(R.id.client_cnt);
        clientCntTv.setText("当前用户数量: " + client.clientCnt);

        TextView loginUserIdTv = view.findViewById(R.id.login_user_id);
        loginUserIdTv.setText("用户ID: " + client.userID);

        TextView userName = view.findViewById(R.id.login_user_name);

        userName.setText("用户名: " + client.userName);


        cmdEdit = view.findViewById(R.id.cmd_edit);
        cmdBtn = view.findViewById(R.id.cmd_btn);
        cmdBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (client.userType != 0) {
                    toast.setText("用户权限不足");
                    toast.show();
                    return;
                }
                String cmd = cmdEdit.getText().toString();
                if (cmd == null || cmd.equals("")) {
                    toast.setText("消息不能为空");
                    toast.show();
                    return;
                }

                client.writeToServer(Command.system_cmd, cmd);
            }
        });

        cmdTv = view.findViewById(R.id.cmd_result_tv);

        msgEdit = view.findViewById(R.id.client_msg_edit);
        Button sendMsgBtn = view.findViewById(R.id.send_msg_btn);

        sendMsgBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String msg = msgEdit.getText().toString();
                if (msg == null || msg.equals("")) {
                    toast.setText("消息不能为空");
                    toast.show();

                    return;
                }

                client.writeToServer(Command.send_to_all_clients, msg);

            }

        });

    }

}
