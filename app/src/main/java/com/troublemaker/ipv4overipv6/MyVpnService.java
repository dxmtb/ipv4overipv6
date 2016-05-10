package com.troublemaker.ipv4overipv6;

import java.io.*;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.lang.String;
import  java.lang.Object;

import android.app.AlertDialog.Builder;
import android.app.PendingIntent;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.net.VpnService;
import android.os.Handler;
import android.os.Environment;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import android.view.View;
import android.app.Activity;

public class MyVpnService extends VpnService implements Runnable {
    private static final String TAG = "MyVpnService";
    private Handler mHandler;
    private Thread mThread;
    private ParcelFileDescriptor myinterface;

    private File extDir;

    private File trafficFile;
    private  FileInputStream trafficFileInputStream;
    private BufferedInputStream trafficIn;
    private long startTime;
    private long nowTime;
    private long lastTime;
    private long lastUpBytes;
    private long lastDownBytes;
    private long endTime;
    private long totalUpBytes;
    private long totalDownBytes;
    private long totalUpPackets;
    private long totalDownPackets;
    private String ipv4Addr;
    private String ipv6Addr;
    private double upSpeed;
    private double downSpeed;
    private Intent mIntent = new Intent("com.troublemaker.ipv4overipv6.RECEIVER");

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (mThread != null) {
            mThread.interrupt();
        }
        mThread = new Thread(this, "MyVpnThread");
        Log.d(TAG, "thread start");

        mThread.start();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if (mThread != null) {
            mThread.interrupt();
        }
    }

    static {
        System.loadLibrary("myvpnservice");
    }
    public native String initBackend();
    public native void startBackend(int tunFd);

    @Override
    public synchronized void run() {
        String ip_info = initBackend();
        Log.d(TAG,ip_info);
        startTime = System.currentTimeMillis();
        totalUpBytes = 0;
        totalUpPackets = 0;
        totalDownBytes = 0;
        totalDownPackets = 0;
        upSpeed = 0;
        downSpeed = 0;
        ipv6Addr = "2402:f000:1:4417::900";

        startVPN(ip_info);

        extDir = this.getFilesDir();//获取当前路径
        Log.d(TAG,extDir.toString());

        trafficFile = new File(extDir, "traffic_info_pipe");
        try {
            trafficFileInputStream = new FileInputStream(trafficFile);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return;
        }
        trafficIn = new BufferedInputStream(trafficFileInputStream);
        byte[] readTrafficBuf = new byte[4];
        lastTime = System.currentTimeMillis();
        lastUpBytes = 0;
        lastDownBytes = 0;
        while (true) {
            Log.d(TAG, "XXXXXXXXXXXXXXXXXXXXXXXXX");
            int readTrafficLen;
            try {
                readTrafficLen = trafficIn.read(readTrafficBuf);
            } catch (IOException e) {
                e.printStackTrace();
                return;
            }

            Log.d(TAG, "readTrafficLen: " + readTrafficLen);

            if (readTrafficLen > 0) {
                showTraffic(readTrafficBuf);
            }

//            try {
//                Thread.sleep(1000);
//            } catch (InterruptedException e1) {
//                e1.printStackTrace();
//            }
        }
    }

    private void startVPN(String parameters) {
        Builder builder = new Builder();
        String[] strs = parameters.split(" ");
        ipv4Addr = strs[0];
        builder.setMtu(1500);
        builder.addAddress(strs[0], 32);
        builder.addRoute(strs[1], 0);
        //builder.addDnsServer(strs[2]);
        builder.addDnsServer(strs[3]);
        builder.addDnsServer(strs[4]);
        try {
            myinterface.close();
        } catch (Exception e) {
            // ignore
        }

        myinterface = builder.establish();
        final int k = myinterface.getFd();
        Log.d(TAG, k + "");
        new Thread(new Runnable() {
            @Override
            public void run() {
                startBackend(k);
            }
        }).start();
    }
    private void showTraffic(byte[] data) {
        //handle the traffic
        byte bLoop;

        int num = 0;
        for (int i = 0; i < data.length; i++) {
            bLoop = data[i];
             num+= (bLoop & 0xFF) << (8 * i);
        }
        if (num>0){
            totalUpBytes +=num;
            lastUpBytes +=num;
            totalUpPackets ++;
        }else{
            totalDownBytes -=num;
            lastDownBytes -=num;
            totalDownPackets ++;
        }
        Log.d(TAG,lastUpBytes+"  "+lastDownBytes);
        nowTime = System.currentTimeMillis();
        if (nowTime-lastTime>1000){
            upSpeed = lastUpBytes*1000 / (nowTime-lastTime);
            downSpeed = lastDownBytes*1000 / (nowTime-lastTime);
            lastUpBytes = 0;
            lastDownBytes = 0;
            lastTime = System.currentTimeMillis();
        }

        long totalTime = nowTime - startTime;

        String str = totalTime/1000+"s "+upSpeed+"bytes/s "+downSpeed+"bytes/s "+totalUpBytes+"bytes "+totalUpPackets+" "+totalDownBytes+"bytes "+totalDownPackets+" "+ipv4Addr+" "+ipv6Addr;
        Log.d(TAG,str);
        mIntent.putExtra("data", str);
        sendBroadcast(mIntent);
    }
}