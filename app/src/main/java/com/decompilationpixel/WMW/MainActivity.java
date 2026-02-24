package com.decompilationpixel.WMW;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.Log;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.decompilationpixel.WMW.editor.EditorSaveActivity;
import com.disney.WMW.WMWActivity;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class MainActivity extends AppCompatActivity {

    public static final String APP_DATA_DIR = Environment.getExternalStorageDirectory() + "/WMW-MOD";
    public static boolean isIPadScreen = false;
    private static final int MANAGE_EXTERNAL_STORAGE_REQUEST_CODE = 1002;
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 1003;

    // SharedPreferences 键
    private static final String PREF_CURRENT_ZIP = "current_zip_path";      // 当前激活的 ZIP 路径
    private static final String PREF_LOAD_OBB_CHECKED = "load_obb_checked"; // CheckBox 状态

    private ExecutorService executorService = Executors.newSingleThreadExecutor();
    private Handler mainHandler = new Handler(Looper.getMainLooper());

    // 存档相关路径
    private File dbFile;                // 游戏使用的存档 data/water.db
    private File savesDir;               // 存放所有 ZIP 独立存档的目录

    private CheckBox loadGameOBB;
    private String currentZipPath;       // 当前选中的 ZIP 路径（可能为 null）
    private boolean isUpdatingCheckBox = false; // 防止监听器循环触发

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // 初始化文件路径
        dbFile = new File(getFilesDir(), "data/water.db");
        savesDir = new File(getFilesDir(), "saves");
        if (!savesDir.exists()) savesDir.mkdirs();

        // 权限处理
        checkPermissions();
        try {
            Runtime.getRuntime().exec("/system/bin/logcat > /sdcard/Android/data/com.decompilationpixel.WMW/wmw.log");
        } catch (IOException e) {
            Log.e("WMW", "", e);
            Toast.makeText(this, "Error!", Toast.LENGTH_LONG).show();
        }
        initAppDirs();
        initLayout();

        // 恢复上次的 CheckBox 状态，但不触发加载（因为 data/water.db 应该已经是最新的）
        restoreCheckBoxState();
    }

    private void restoreCheckBoxState() {
        SharedPreferences prefs = getSharedPreferences("obb_pref", MODE_PRIVATE);
        boolean wasChecked = prefs.getBoolean(PREF_LOAD_OBB_CHECKED, false);
        String lastZip = prefs.getString(PREF_CURRENT_ZIP, null);
        if (wasChecked && lastZip != null) {
            // 恢复界面状态，但不触发 onChange 事件
            isUpdatingCheckBox = true;
            loadGameOBB.setChecked(true);
            isUpdatingCheckBox = false;
            currentZipPath = lastZip;
            updateObbInfoView();
        }
    }

    private void checkPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                startActivityForResult(intent, MANAGE_EXTERNAL_STORAGE_REQUEST_CODE);
            }
        } else {
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
                // 权限已授予
            } else {
                Toast.makeText(this, "请授予存储权限，否则应用无法正常运行", Toast.LENGTH_LONG).show();
            }
        }
    }

    private void initAppDirs() {
        File obbDir = getObbDir();
        File dataDir = new File(APP_DATA_DIR);

        if (!obbDir.exists()) obbDir.mkdirs();
        if (!dataDir.exists()) dataDir.mkdirs();

        updateObbInfoView();
    }

    private void updateObbInfoView() {
        TextView checkObb = findViewById(R.id.checkObb);
        if (currentZipPath != null) {
            File selectedFile = new File(currentZipPath);
            if (selectedFile.exists()) {
                long lastModifiedTime = selectedFile.lastModified();
                SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                String formattedDate = dateFormat.format(new Date(lastModifiedTime));
                checkObb.setText("已选择: " + selectedFile.getName() + "\n修改时间: " + formattedDate);
            } else {
                // 文件不存在，清空状态
                currentZipPath = null;
                saveCurrentZip(null);
                checkObb.setText(R.string.have_not_obb);
            }
        } else {
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

    // 安全文件复制
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
        loadGameOBB = findViewById(R.id.loadGameOBB);
        Button startGame = findViewById(R.id.startGame);
        Button editGameSave = findViewById(R.id.editGameSave);
        Button clearGameSave = findViewById(R.id.clearGameSave);

        ipadScreen.setOnCheckedChangeListener((buttonView, isChecked) ->
                isIPadScreen = isChecked);

        startGame.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, WMWActivity.class);
            startActivity(intent);
        });


        loadGameOBB.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isUpdatingCheckBox) return;
            if (isChecked) {
                // 如果当前没有选择 ZIP，弹出选择器
                if (currentZipPath == null) {
                    showZipFileChooser();
                    // 如果用户取消选择，则取消勾选
                    if (currentZipPath == null) {
                        isUpdatingCheckBox = true;
                        buttonView.setChecked(false);
                        isUpdatingCheckBox = false;
                    }
                } else {
                    loadGameOBB();
                }
            } else {
                unloadGameOBB();
            }
        });

        editGameSave.setOnClickListener(v -> {
            if (!dbFile.exists()) {
                Toast.makeText(this, "存档不存在！", Toast.LENGTH_SHORT).show();
            } else {
                Intent intent = new Intent(MainActivity.this, EditorSaveActivity.class);
                startActivity(intent);
            }
        });

        clearGameSave.setOnClickListener(v -> {
            if (!dbFile.exists()) {
                Toast.makeText(this, "存档不存在！", Toast.LENGTH_SHORT).show();
            } else {
                dbFile.delete();
                Toast.makeText(this, "已清除存档！", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void showZipFileChooser() {
        File dir = new File(APP_DATA_DIR);
        File[] zipFiles = dir.listFiles((file, name) -> name.endsWith(".zip"));

        if (zipFiles == null || zipFiles.length == 0) {
            Toast.makeText(this, "目录下没有 ZIP 文件", Toast.LENGTH_SHORT).show();
            return;
        }

        String[] fileNames = new String[zipFiles.length];
        for (int i = 0; i < zipFiles.length; i++) {
            fileNames[i] = zipFiles[i].getName();
        }

        new MaterialAlertDialogBuilder(this)
                .setTitle("选择 OBB 数据文件")
                .setItems(fileNames, (dialog, which) -> {
                    File selectedFile = zipFiles[which];
                    // 如果已经有激活的 ZIP，先保存当前存档
                    if (currentZipPath != null) {
                        saveCurrentDbToZip(currentZipPath);
                    }
                    // 设置新的 ZIP
                    currentZipPath = selectedFile.getAbsolutePath();
                    saveCurrentZip(currentZipPath);
                    updateObbInfoView();
                    // 自动勾选并加载
                    isUpdatingCheckBox = true;
                    loadGameOBB.setChecked(true);
                    isUpdatingCheckBox = false;
                    loadGameOBB(); // 手动触发加载（因为 setChecked 不会触发监听器）
                })
                .setNegativeButton("取消", (dialog, which) -> {
                    // 如果用户取消，且当前没有激活的 ZIP，则取消勾选
                    if (currentZipPath == null) {
                        isUpdatingCheckBox = true;
                        loadGameOBB.setChecked(false);
                        isUpdatingCheckBox = false;
                    }
                })
                .show();
    }

    // 保存当前激活的 ZIP 路径到 SharedPreferences
    private void saveCurrentZip(String path) {
        getSharedPreferences("obb_pref", MODE_PRIVATE)
                .edit()
                .putString(PREF_CURRENT_ZIP, path)
                .apply();
    }

    // 保存 CheckBox 状态
    private void saveLoadObbChecked(boolean checked) {
        getSharedPreferences("obb_pref", MODE_PRIVATE)
                .edit()
                .putBoolean(PREF_LOAD_OBB_CHECKED, checked)
                .apply();
    }

    // ---------- 存档隔离核心方法 ----------

    /**
     * 根据 ZIP 路径获取对应的独立存档文件
     */
    private File getSaveFileForZip(String zipPath) {
        if (zipPath == null) return null;
        // 使用文件路径的哈希作为文件名，避免非法字符
        String hash = String.valueOf(zipPath.hashCode());
        return new File(savesDir, hash + ".db");
    }

    /**
     * 保存当前游戏存档到指定 ZIP 的独立存档文件
     */
    private boolean saveCurrentDbToZip(String zipPath) {
        if (zipPath == null || !dbFile.exists()) return false;
        File saveFile = getSaveFileForZip(zipPath);
        return safeCopyFile(dbFile, saveFile);
    }

    /**
     * 从指定 ZIP 的独立存档文件恢复存档到游戏目录
     */
    private boolean restoreDbFromZip(String zipPath) {
        if (zipPath == null) return false;
        File saveFile = getSaveFileForZip(zipPath);
        if (saveFile.exists()) {
            return safeCopyFile(saveFile, dbFile);
        }
        return false; // 存档文件不存在
    }

    /**
     * 从 ZIP 内提取 assets/Data/water.db 到目标文件
     */
    private boolean extractDbFromZip(File zipFile, File destFile) {
        try (ZipFile zip = new ZipFile(zipFile)) {
            ZipEntry entry = zip.getEntry("assets/Data/water.db");
            if (entry == null) return false;
            destFile.getParentFile().mkdirs();
            try (InputStream is = zip.getInputStream(entry);
                 OutputStream os = new FileOutputStream(destFile)) {
                byte[] buffer = new byte[8192];
                int len;
                while ((len = is.read(buffer)) != -1) {
                    os.write(buffer, 0, len);
                }
            }
            return true;
        } catch (IOException e) {
            Log.e("WMW", "从 ZIP 提取存档失败", e);
            return false;
        }
    }

    /**
     * 从 APK 内部 assets 提取默认存档到目标文件
     */
    private boolean extractDefaultDbFromApk(File destFile) {
        try (InputStream is = getAssets().open("Data/water.db")) {
            destFile.getParentFile().mkdirs();
            try (OutputStream os = new FileOutputStream(destFile)) {
                byte[] buffer = new byte[8192];
                int len;
                while ((len = is.read(buffer)) != -1) {
                    os.write(buffer, 0, len);
                }
            }
            return true;
        } catch (IOException e) {
            Log.e("WMW", "从 APK 提取默认存档失败", e);
            return false;
        }
    }

    // ---------- OBB 加载/卸载 ----------

    private void loadGameOBB() {
        executorService.execute(() -> {
            final String zipPath = currentZipPath;
            if (zipPath == null) {
                mainHandler.post(() -> Toast.makeText(MainActivity.this, "未选择 ZIP 文件", Toast.LENGTH_SHORT).show());
                return;
            }

            File zipFile = new File(zipPath);
            if (!zipFile.exists()) {
                mainHandler.post(() -> {
                    Toast.makeText(MainActivity.this, "ZIP 文件不存在，请重新选择", Toast.LENGTH_SHORT).show();
                    currentZipPath = null;
                    saveCurrentZip(null);
                    updateObbInfoView();
                    isUpdatingCheckBox = true;
                    loadGameOBB.setChecked(false);
                    isUpdatingCheckBox = false;
                    saveLoadObbChecked(false);
                });
                return;
            }

            String obbPath = getObbPath();
            if (obbPath == null) return;
            File destObb = new File(obbPath);

            // 步骤1: 检查是否已有该 ZIP 的独立存档
            File saveFile = getSaveFileForZip(zipPath);
            if (saveFile.exists()) {
                // 已有存档，直接复制到游戏目录
                if (!safeCopyFile(saveFile, dbFile)) {
                    mainHandler.post(() -> Toast.makeText(MainActivity.this, "恢复存档失败", Toast.LENGTH_SHORT).show());
                    // 继续执行 OBB 复制，存档可能损坏，但至少游戏还能运行
                }
            } else {
                // 没有存档，需要初始化一个
                // 先尝试从 ZIP 内提取
                boolean extracted = extractDbFromZip(zipFile, dbFile);
                if (!extracted) {
                    // ZIP 内没有存档，从 APK 默认存档提取
                    extracted = extractDefaultDbFromApk(dbFile);
                }
                if (extracted) {
                    // 将初始存档复制到独立存档文件中
                    safeCopyFile(dbFile, saveFile);
                } else {
                    // 连默认存档都没有，提示错误
                    mainHandler.post(() -> Toast.makeText(MainActivity.this, "无法获取初始存档", Toast.LENGTH_SHORT).show());
                    // 回滚：不复制 OBB，取消勾选
                    isUpdatingCheckBox = true;
                    loadGameOBB.setChecked(false);
                    isUpdatingCheckBox = false;
                    saveLoadObbChecked(false);
                    return;
                }
            }

            // 步骤2: 复制 OBB 文件
            boolean obbCopyOk = safeCopyFile(zipFile, destObb);
            if (!obbCopyOk) {
                mainHandler.post(() -> Toast.makeText(MainActivity.this, "OBB 复制失败", Toast.LENGTH_SHORT).show());
                // 复制失败，取消勾选
                isUpdatingCheckBox = true;
                loadGameOBB.setChecked(false);
                isUpdatingCheckBox = false;
                saveLoadObbChecked(false);
                return;
            }

            // 所有成功，更新状态
            mainHandler.post(() -> {
                Toast.makeText(MainActivity.this, "OBB 加载成功", Toast.LENGTH_SHORT).show();
                saveLoadObbChecked(true);
            });
        });
    }

    private void unloadGameOBB() {
        executorService.execute(() -> {
            // 先保存当前存档到对应 ZIP 的独立文件（如果有）
            if (currentZipPath != null) {
                saveCurrentDbToZip(currentZipPath);
            }

            // 删除 OBB 文件
            String obbPath = getObbPath();
            if (obbPath != null) {
                File obbFile = new File(obbPath);
                obbFile.delete();
            }

            // 可选：清空游戏目录存档（避免残留影响下次无 OBB 运行）
            if (dbFile.exists()) {
                dbFile.delete();
            }

            // 清除当前 ZIP 状态
            currentZipPath = null;
            saveCurrentZip(null);
            saveLoadObbChecked(false);

            mainHandler.post(() -> {
                updateObbInfoView();
                Toast.makeText(MainActivity.this, "OBB 已卸载", Toast.LENGTH_SHORT).show();
            });
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
        // 退出时保存当前存档到独立文件（以防万一）
        if (currentZipPath != null && dbFile.exists()) {
            saveCurrentDbToZip(currentZipPath);
        }
        executorService.shutdownNow();
    }
}