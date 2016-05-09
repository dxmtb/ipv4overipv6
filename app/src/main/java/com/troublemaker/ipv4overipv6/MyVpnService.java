package com.troublemaker.ipv4overipv6;

import java.io.*;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.lang.String;

import android.app.AlertDialog.Builder;
import android.app.PendingIntent;
import android.content.Intent;
import android.net.VpnService;
import android.os.Handler;
import android.os.Environment;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.widget.Toast;

public class MyVpnService extends VpnService implements Handler.Callback,Runnable {
    private static final String TAG = "MyVpnService";
    private Handler mHandler;
    private Thread mThread;
    private String mServerAddress;
    private String mServerPort;
    private PendingIntent mConfigureIntent;
    private ParcelFileDescriptor myinterface;
    private String myParameters;
    private byte[] readIPBuf;
    private byte[] readTrafficBuf;
    private boolean ipFlag;

    private File extDir;
    private File ipFile;
    private FileOutputStream ipFileOutputStream;
    private BufferedOutputStream ipOut;
    private FileInputStream ipFileInputStream;
    private BufferedInputStream ipIn;

    private File trafficFile;
    private FileOutputStream trafficFileOutputStream;
    private  BufferedOutputStream trafficOut;
    private  FileInputStream trafficFileInputStream;
    private BufferedInputStream trafficIn;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (mHandler == null) {
            mHandler = new Handler(this);
        }
        if (mThread != null) {
            mThread.interrupt();
        }
        ipFlag = false;
        mThread = new Thread(this, "MyVpnThread");
        Log.d(TAG,"thread start");
        mThread.start();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if (mThread != null) {
            mThread.interrupt();
        }
    }

    @Override
    public boolean handleMessage(Message message) {
        String showText = "none";
        if (message != null) {
            if (message.what == 0){
                showText = "0";
            }else if (message.what == 1){
                showText = "1";
            }else if (message.what == 2){
                showText = "2";
            }
            Toast.makeText(getApplicationContext(), showText,
                    Toast.LENGTH_SHORT).show();
            Log.d(TAG, message.toString());
        }
        return true;
    }

    static {
        System.loadLibrary("myvpnservice");
    }
    public native String initBackend();
    public native void startBackend(int tunFd);

    @Override
    public synchronized void run() {
        String ip_info = initBackend();

        // TODO: implement handle ip info

        try {
            extDir = Environment.getExternalStorageDirectory();//获取当前路径
            ipFile = new File(extDir,"ip_pipe");
            ipFileOutputStream = new FileOutputStream(ipFile);
            ipOut = new BufferedOutputStream(ipFileOutputStream);
            ipFileInputStream = new FileInputStream(ipFile);
            ipIn = new BufferedInputStream(ipFileInputStream);

            trafficFile = new File(extDir,"traffic_pipe");
            trafficFileOutputStream = new FileOutputStream(trafficFile);
            trafficOut = new BufferedOutputStream(trafficFileOutputStream);
            trafficFileInputStream = new FileInputStream(trafficFile);
            trafficIn = new BufferedInputStream(trafficFileInputStream);

            while (true){
                if (!ipFlag){
                    int readIPLen = ipIn.read(readIPBuf);
                    if (readIPLen>0){
                        startVPN(readIPBuf.toString());
                        ipFlag = true;
                        ipIn.close();
                    }
                }else{
                    int readTrafficLen = trafficIn.read(readTrafficBuf);
                    if (readTrafficLen>0){
                        showTraffic(readTrafficBuf);
                        Toast.makeText(getApplicationContext(), readTrafficBuf.toString(),
                                Toast.LENGTH_SHORT).show();
                    }
                }
                Thread.sleep(2000);
            }
        } catch (Exception e) {
            Log.e(TAG, "Got " + e.toString());
        } finally {
            try {
                myinterface.close();
                trafficIn.close();
                trafficOut.close();
            } catch (Exception e) {
                // ignore
            }
            myinterface = null;
            myParameters = null;

            mHandler.sendEmptyMessage(0);
            Log.i(TAG, "Exiting");
        }
    }

    private void startVPN(String parameters) throws Exception {
        Builder builder = new Builder();
        for (String parameter : parameters.split(" ")) {
            String[] fields = parameter.split(",");
            try {
                switch (fields[0].charAt(0)) {
                    case 'm':
                        builder.setMtu(Short.parseShort(fields[1]));
                        break;
                    case 'a':
                        builder.addAddress(fields[1], Integer.parseInt(fields[2]));
                        break;
                    case 'r':
                        builder.addRoute(fields[1], Integer.parseInt(fields[2]));
                        break;
                    case 'd':
                        builder.addDnsServer(fields[1]);
                        break;
                    case 's':
                        builder.addSearchDomain(fields[1]);
                        break;
                }
            } catch (Exception e) {
                throw new IllegalArgumentException("Bad parameter: " + parameter);
            }
        }
        try {
            myinterface.close();
        } catch (Exception e) {
            // ignore
        }

        myinterface = builder.setSession(mServerAddress)
                .setConfigureIntent(mConfigureIntent)
                .establish();
        myParameters = parameters;
        int k = myinterface.getFd();
        ipOut.write((byte)k);
        ipOut.flush();
        ipOut.close();
    }
    private void showTraffic(byte[] data) throws Exception {
        //handle the traffic
    }
}