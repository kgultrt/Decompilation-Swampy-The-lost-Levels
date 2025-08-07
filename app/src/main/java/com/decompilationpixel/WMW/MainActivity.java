package com.decompilationpixel.WMW;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import com.decompilationpixel.WMW.R;
import com.decompilationpixel.WMW.editor.EditorSaveActivity;
import com.disney.WMW.WMWActivity;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends AppCompatActivity {

    public static final String APP_DATA_DIR = Environment.getExternalStorageDirectory() + "/WMW-MOD";
    public static boolean isIPadScreen = false;
    private static final int MANAGE_EXTERNAL_STORAGE_REQUEST_CODE = 1002;
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 1003;
    
    private ExecutorService executorService = Executors.newSingleThreadExecutor();
    private Handler mainHandler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // 权限处理
        checkPermissions();
        initAppDirs();
        initLayout();
    }

    private void checkPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11+ 跳转系统设置页申请 MANAGE_EXTERNAL_STORAGE
            if (!Environment.isExternalStorageManager()) {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                startActivityForResult(intent, MANAGE_EXTERNAL_STORAGE_REQUEST_CODE);
            }
        } else {
            // 旧版本Android请求存储权限
            if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE) 
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, 
                    new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 
                    STORAGE_PERMISSION_REQUEST_CODE);
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == MANAGE_EXTERNAL_STORAGE_REQUEST_CODE) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && 
                Environment.isExternalStorageManager()) {
                // 权限已授予，继续初始化
            } else {
                Toast.makeText(this, "请授予存储权限，否则应用无法正常运行", Toast.LENGTH_LONG).show();
            }
        }
    }

    // 优化目录初始化
    private void initAppDirs() {
        File obbDir = getObbDir();
        File dataDir = new File(APP_DATA_DIR);
        File extraDataFile = new File(APP_DATA_DIR, "wmw-extra.zip");

        // 使用更安全的创建方式
        if (!obbDir.exists()) obbDir.mkdirs();
        if (!dataDir.exists()) dataDir.mkdirs();

        TextView checkObb = findViewById(R.id.checkObb);
        if (extraDataFile.exists()) {
            // 设置时间格式并显示
            long lastModifiedTime = extraDataFile.lastModified();
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            String formattedDate = dateFormat.format(new Date(lastModifiedTime));
            checkObb.setText(getString(R.string.have_obb) + formattedDate);
        } else {
            // 当文件不存在时显示默认文本
            checkObb.setText(R.string.have_not_obb);
        }
    }

    @Override
    public void onBackPressed() {
        Log.d("WMW", "Back button pressed");
        showExitDialog();
    }

    private void showExitDialog() {
        new MaterialAlertDialogBuilder(this)
            .setTitle(R.string.exit_str_title)
            .setMessage(R.string.exit_str)
            .setPositiveButton(R.string.ok, (dialog, which) -> finish())
            .setNegativeButton(R.string.cancel, null)
            .show();
    }

    // 替换危险命令的安全文件复制方法
    private boolean safeCopyFile(File source, File dest) {
        try (FileChannel inChannel = new FileInputStream(source).getChannel();
             FileChannel outChannel = new FileOutputStream(dest).getChannel()) {
            
            outChannel.transferFrom(inChannel, 0, inChannel.size());
            return true;
            
        } catch (IOException e) {
            Log.e("WMW", "File copy failed", e);
            return false;
        }
    }
    
    // 安全删除文件
    private boolean safeDeleteFile(File file) {
        return file.delete();
    }

    private void initLayout() {
        final CheckBox ipadScreen = findViewById(R.id.resetSize);
        final CheckBox loadGameOBB = findViewById(R.id.loadGameOBB);
        Button startGame = findViewById(R.id.startGame);
        Button editGameSave = findViewById(R.id.editGameSave);

        ipadScreen.setOnCheckedChangeListener((buttonView, isChecked) -> 
            isIPadScreen = isChecked);

        startGame.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, WMWActivity.class);
            startActivity(intent);
        });

        loadGameOBB.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                loadGameOBB();
            } else {
                unloadGameOBB();
            }
        });
        
        editGameSave.setOnClickListener(v -> {
            
            File dbFile = new File(getFilesDir(), "data/water.db");
        
            if (!dbFile.exists()) {
                Toast.makeText(this, "存档不存在！", Toast.LENGTH_SHORT).show();
                return;
            } else {
                Intent intent = new Intent(MainActivity.this, EditorSaveActivity.class);
                startActivity(intent);
            }
        });
    }
    
    // 异步处理文件操作
    private void loadGameOBB() {
        executorService.execute(() -> {
            File extraDataFile = new File(APP_DATA_DIR, "wmw-extra.zip");
            String obbPath = getObbPath();
            
            // 确保资源文件存在
            if (!extraDataFile.exists()) {
                AppUtils.ExportAssets(this, APP_DATA_DIR, "wmw-extra.zip");
            }
            
            boolean success = false;
            if (extraDataFile.exists() && obbPath != null) {
                File dest = new File(obbPath);
                success = safeCopyFile(extraDataFile, dest);
            }
        });
    }
    
    private void unloadGameOBB() {
        executorService.execute(() -> {
            String obbPath = getObbPath();
            boolean success = false;
            
            if (obbPath != null) {
                File obbFile = new File(obbPath);
                success = safeDeleteFile(obbFile);
            }
        });
    }

    // 优化的OBB路径获取
    private String getObbPath() {
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            try {
                String pn = getPackageName();
                PackageInfo pkgInfo = getPackageManager().getPackageInfo(pn, 0);
                int versionCode = pkgInfo.versionCode;
                return getObbDir() + File.separator + "main." + versionCode + "." + pn + ".obb";
            } catch (PackageManager.NameNotFoundException e) {
                Log.e("WMW", "Package not found", e);
            }
        }
        return null;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        executorService.shutdownNow();
    }
}