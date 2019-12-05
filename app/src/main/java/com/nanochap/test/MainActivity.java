package com.nanochap.test;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import com.nanochap.utilts.ExecuteCommandUtils;

import java.util.Timer;
import java.util.TimerTask;


/**
 * @author xiaoyi
 * @description
 * @date 2019/11/25
 */
public class MainActivity extends Activity {
    /****************************************/
    private CanFrame mcanFrame, scanFrame;
    private int CanId = 0x1EB;
    private String STAETCOMMAND="ip link set can0 up type can bitrate 50000";
    public Timer timer;

    Handler handler;

    String rxIdCode = "";
    String tag = "serial test";

    private EditText ET1;
    private Button RECV;
    private Button SEND;

    /****************************************/

    @SuppressLint("HandlerLeak")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_mains);

        /********************************************/
        ET1 = (EditText) findViewById(R.id.edit1);
        RECV = (Button) findViewById(R.id.recv1);
        SEND = (Button) findViewById(R.id.send1);

        scanFrame = new CanFrame();

        //初始化CAN启动命令
        ExecuteCommandUtils.initCommand(STAETCOMMAND, true, true);
        
        // CanControl.InitCan(50000);

        CanControl.OpenCan();//42

        RECV.setOnClickListener(new manager());

        SEND.setOnClickListener(new manager());

        timer = new Timer();
        setTimerTask();

        handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                //Log.d(tag,"send start ...");
                ET1.append("canid:0x"
                        + Integer.toHexString(mcanFrame.can_id)
                        + "  data:" + mcanFrame.data + "  length:"
                        + Integer.toString(mcanFrame.can_dlc) + "\n");
                if (msg.what == 1) {
                }
            }
        };

        /*********************************************/

    }
    
    private void setTimerTask() {
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                mcanFrame = CanControl.CanRead(scanFrame, 1);

                if (mcanFrame.can_id != 0) {
                    Message smsg = new Message();
                    smsg.what = 1;
                    handler.sendMessage(smsg);
                }
            }

        }, 100, 100);
    }


    /***********************************************/
    class manager implements View.OnClickListener {
        public void onClick(View v) {
            String rxIdCode = "";
            String str;

            int i;
            switch (v.getId()) {
                //recvive
                case R.id.recv1:

                    Log.d(tag, "recv start ...");

                    //if(RX == null)return;
                    mcanFrame = CanControl.CanRead(scanFrame, 1);
                    Log.d(tag, "display recv data ...");

                    break;

                //send
                case R.id.send1:
                    Log.d(tag, "send start ...");

                    CharSequence tx = ET1.getText();

                    char[] text = new char[tx.length()];

                    for (i = 0; i < tx.length(); i++) {
                        text[i] = tx.charAt(i);
                    }

                    CanControl.CanWrite(CanId, String.copyValueOf(text, 0, i));

                    ET1.setText("");
            }
        }
    }

    /***********************************************/

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }


    /* add by cym 20140617 */
    static {
        System.loadLibrary("canjni");
    }

}
