package com.decompilationpixel.WMW;

import android.app.AlertDialog;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.content.Intent;
import android.provider.Settings;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.disney.WMW.WMWActivity;
import com.decompilationpixel.WMW.R;
import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.io.FileInputStream;
import java.io.FileOutputStream;
    

public class MainActivity extends AppCompatActivity {

    public static String appDataDir = Environment.getExternalStorageDirectory().toString() + "/WMW-MOD";
    public static boolean isIPadScreen = false;
    private static final int MANAGE_EXTERNAL_STORAGE_REQUEST_CODE = 1002;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
		//申请权限
		//吐槽一下原作者的命名，你这直接上一个中文名，你看看这个，简洁
		Permission.getPermission(this);
		
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ 跳转系统设置页申请 MANAGE_EXTERNAL_STORAGE
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + getPackageName()));
            startActivityForResult(intent, MANAGE_EXTERNAL_STORAGE_REQUEST_CODE);
        }
		
		//添加数据目录的File对象
		File obbdir = getObbDir();
		File dataDir = new File(appDataDir);
		File extraDataFile = new File(appDataDir + "/wmw-extra.zip");
		
		//检测是否存在，不存在则创建，存在则用文本显示更新时间
		if (!obbdir.exists() || !dataDir.exists()) {
			obbdir.mkdirs();
			dataDir.mkdirs();
		} else{
			TextView checkObb = findViewById(R.id.checkObb);
			
			//设置时间格式
			long lastModifiedTime = extraDataFile.lastModified();
			Date lastModifiedDate = new Date(lastModifiedTime);
			SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
			String formattedDate = dateFormat.format(lastModifiedDate);
			
			//显示更新时间
			checkObb.setText(getString(R.string.have_obb) + formattedDate);
		}
		
		initLayout();
		extractLibFilesToPrivateDir(this);
    }

	@Override
	public void onBackPressed() {
        Log.e("WMW","返回被按下");
        showExitDialog();
	}
	

    private void showExitDialog() {
        MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(this);
        builder.setTitle(getString(R.string.exit_str_title))
               .setMessage(getString(R.string.exit_str))
               .setPositiveButton(R.string.ok, (dialog, which) -> finish())
               .setNegativeButton(R.string.cancel, null)
               .show();
    }
	
	// 将应用中的 .so 文件解压到私有目录
    private void extractLibFilesToPrivateDir(Context context) {
        File libDir = new File(context.getApplicationInfo().nativeLibraryDir);
        File privateDir = context.getDir("libs", Context.MODE_PRIVATE);

        if (!privateDir.exists()) {
            privateDir.mkdirs();
        }

        File[] libFiles = libDir.listFiles((dir, name) -> name.endsWith(".so"));
        if (libFiles != null) {
            for (File libFile : libFiles) {
                try {
                    File destFile = new File(privateDir, libFile.getName());
                    copyFile(libFile, destFile);
                } catch (IOException e) {
                    Log.e("WMW", "Failed to copy " + libFile.getName(), e);
                }
            }
        }
    }

    // 复制文件方法
    private void copyFile(File src, File dst) throws IOException {
        try (FileInputStream in = new FileInputStream(src);
             FileOutputStream out = new FileOutputStream(dst)) {
            byte[] buffer = new byte[1024];
            int len;
            while ((len = in.read(buffer)) > 0) {
                out.write(buffer, 0, len);
            }
        }
    }
	
	public static String getObbPath(Context context) {
		PackageManager packageManager = context.getPackageManager();
		PackageInfo packageInfo = null;
		try {
			packageInfo = packageManager.getPackageInfo(context.getPackageName(), 0);
		} catch (PackageManager.NameNotFoundException e) {}
		String pn = context.getPackageName();
		if (Environment.getExternalStorageState().equals("mounted")) { 
			File file = new File(context.getObbDir().toString()); 
			if (packageInfo.versionCode > 0) { 
				String str = file + File.separator + "main." + packageInfo.versionCode + "." + pn + ".obb";
				Log.e("WMW", "obbFilePath: " + str);
				return str;
			} 
		}  
		return null;
	}

	private void initLayout() {
		final CheckBox ipadScreen = findViewById(R.id.resetSize);
		final CheckBox loadGameOBB = findViewById(R.id.loadGameOBB);
		Button startGame = findViewById(R.id.startGame);
		
		ipadScreen.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				if (ipadScreen.isChecked()) {
					isIPadScreen = true;
				} else {
				    isIPadScreen = false;
				}
			}
		});
		
		startGame.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				Intent intent = new Intent(MainActivity.this, WMWActivity.class);
				startActivity(intent);
			}
		});
		
		loadGameOBB.setOnClickListener(new View.OnClickListener() {
		    @Override
			public void onClick(View view) {
				if (loadGameOBB.isChecked()) {
					loadGameOBB();
				} else {
				    unloadGameOBB();
				}
			}
		});
	}
	
	private void loadGameOBB() {
		File obbdir = getObbDir();
		File dataDir = new File(appDataDir);
		File extraDataFile = new File(appDataDir + "/wmw-extra.zip");
		String obbPath = getObbPath(this);
		
		if (!extraDataFile.exists()) {
			AppUtils.ExportAssets(this, appDataDir, "wmw-extra.zip");
		}
		
		Process process = null;
		try {
			process = Runtime.getRuntime().exec("cp " + extraDataFile.toString() + " " + obbPath);
			if (process.waitFor() != 0)
			{
				Toast.makeText(this, "OBB复制失败", 0).show();
			}
			//记录日志
			Runtime.getRuntime().exec("logcat >" + appDataDir + "/wmw.log");
		} catch (IOException|InterruptedException e) {
			Log.e("WMW","",e);
		}
	}
	
	
	private void unloadGameOBB() {
		File obbdir = getObbDir();
		File dataDir = new File(appDataDir);
		File extraDataFile = new File(appDataDir + "/wmw-extra.zip");
		String obbPath = getObbPath(this);
		
		Process process = null;
		try {
			process = Runtime.getRuntime().exec("rm -rf " + obbPath);
			if (process.waitFor() != 0)
			{
				Toast.makeText(this, "OBB删除失败", 0).show();
			}
			//记录日志
			Runtime.getRuntime().exec("logcat >" + appDataDir + "/wmw.log");
		} catch (IOException|InterruptedException e) {
			Log.e("WMW","",e);
		}
	}
}
