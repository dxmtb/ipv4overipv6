package com.troublemaker.ipv4overipv6;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.VpnService;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.content.IntentFilter;

import java.io.Serializable;

public class MainActivity extends AppCompatActivity implements Serializable {
    final static String TAG = "MainActivity";
    private MsgReceiver msgReceiver;
    private Intent intent;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        msgReceiver = new MsgReceiver();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction("com.troublemaker.ipv4overipv6.RECEIVER");
        registerReceiver(msgReceiver, intentFilter);
        Intent intent = VpnService.prepare(this);
        if (intent != null) {
            Log.d(TAG, "Ask user to confirm VPN connection");
            startActivityForResult(intent, 0);
        } else {
            Log.d(TAG, "Already have VPN permission");
            onActivityResult(0, RESULT_OK, null);
        }
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (result == RESULT_OK) {
            intent = new Intent(this, MyVpnService.class);
           // Bundle bundle = new Bundle();
            //bundle.putSerializable("main", this);
            //intent.putExtras(bundle);
            startService(intent);
        }
    }
    @Override
    protected void onDestroy() {
        stopService(intent);
        unregisterReceiver(msgReceiver);
        super.onDestroy();
    }
    public class MsgReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent rintent) {
            //拿到进度，更新UI
            String data = rintent.getStringExtra("data");
            String[] strs = data.split(" ");
            TextView totalTime = (TextView) findViewById(R.id.timeText);
            totalTime.setText(strs[0]);
            TextView upSpeed = (TextView) findViewById(R.id.upSpeedText);
            upSpeed.setText(strs[1]);
            TextView downSpeed = (TextView) findViewById(R.id.downSpeedText);
            downSpeed.setText(strs[2]);
            TextView upBytes = (TextView) findViewById(R.id.upBytesText);
            upBytes.setText(strs[3]);
            TextView upPackets = (TextView) findViewById(R.id.upPacketsText);
            upPackets.setText(strs[4]);
            TextView downBytes = (TextView) findViewById(R.id.downBytesText);
            downBytes.setText(strs[5]);
            TextView downPacktes = (TextView) findViewById(R.id.downPacketsText);
            downPacktes.setText(strs[6]);
            TextView ipv4 = (TextView) findViewById(R.id.ipv4Text);
            ipv4.setText(strs[7]);
            TextView ipv6 = (TextView) findViewById(R.id.ipv6Text);
            ipv6.setText(strs[8]);
        }

    }
}
