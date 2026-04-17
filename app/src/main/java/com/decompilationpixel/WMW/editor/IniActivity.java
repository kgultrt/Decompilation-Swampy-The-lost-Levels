package com.decompilationpixel.WMW.editor;

import android.Manifest;
import android.app.AlertDialog;
import android.content.pm.PackageManager;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.decompilationpixel.WMW.utils.Ini;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.card.MaterialCardView;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class IniActivity extends AppCompatActivity {

    private static final int REQUEST_STORAGE_PERMISSION = 100;

    private String folderPath;
    private File[] iniFiles;

    private LinearLayout container;
    private View fileListView;
    private View editorView;

    private RecyclerView fileRecyclerView;
    private RecyclerView editorRecyclerView;
    private MaterialButton btnSave, btnBackToList, btnAddSection, btnAddKeyValue;

    private File currentEditingFile;
    private List<IniItem> editorItems;
    private EditorAdapter editorAdapter;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        folderPath = getIntent().getStringExtra("folder_path");
        if (TextUtils.isEmpty(folderPath)) {
            Toast.makeText(this, "未提供文件夹路径", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        if (needStoragePermission() && !hasStoragePermission()) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_STORAGE_PERMISSION);
        } else {
            initUI();
            loadFileList();
        }
    }

    private boolean needStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return !Environment.isExternalStorageManager();
        } else {
            return Build.VERSION.SDK_INT < Build.VERSION_CODES.M ||
                    ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                            != PackageManager.PERMISSION_GRANTED;
        }
    }

    private boolean hasStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED;
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_STORAGE_PERMISSION) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                initUI();
                loadFileList();
            } else {
                Toast.makeText(this, "需要存储权限才能读取 INI 文件", Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }

    private void initUI() {
        container = new LinearLayout(this);
        container.setOrientation(LinearLayout.VERTICAL);
        container.setBackgroundColor(0xFFF5F5F5);
        setContentView(container);

        fileListView = createFileListView();
        editorView = createEditorView();

        container.addView(fileListView);
        container.addView(editorView);

        showFileList();
    }

    private View createFileListView() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(24, 24, 24, 24);

        // 标题卡片
        MaterialCardView cardTitle = new MaterialCardView(this);
        cardTitle.setCardElevation(4);
        cardTitle.setRadius(16);
        cardTitle.setUseCompatPadding(true);
        LinearLayout titleLayout = new LinearLayout(this);
        titleLayout.setOrientation(LinearLayout.VERTICAL);
        titleLayout.setPadding(24, 20, 24, 20);
        TextView tvTitle = new TextView(this);
        tvTitle.setText("INI 文件管理");
        tvTitle.setTextSize(24);
        tvTitle.setTypeface(null, Typeface.BOLD);
        tvTitle.setTextColor(0xFF333333);
        titleLayout.addView(tvTitle);
        cardTitle.addView(titleLayout);
        layout.addView(cardTitle);

        // 新建文件按钮
        MaterialButton btnNewFile = new MaterialButton(this);
        btnNewFile.setText("＋ 新建 INI 文件");
        btnNewFile.setIconResource(android.R.drawable.ic_menu_add);
        btnNewFile.setIconPadding(8);
        btnNewFile.setOnClickListener(v -> showNewFileDialog());
        LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        btnParams.topMargin = 16;
        layout.addView(btnNewFile, btnParams);

        // 文件列表标题
        TextView tvFileList = new TextView(this);
        tvFileList.setText("现有文件");
        tvFileList.setTextSize(18);
        tvFileList.setTypeface(null, Typeface.BOLD);
        tvFileList.setTextColor(0xFF666666);
        tvFileList.setPadding(8, 24, 8, 8);
        layout.addView(tvFileList);

        // RecyclerView 卡片
        MaterialCardView cardList = new MaterialCardView(this);
        cardList.setCardElevation(2);
        cardList.setRadius(12);
        fileRecyclerView = new RecyclerView(this);
        fileRecyclerView.setLayoutManager(new LinearLayoutManager(this));
        fileRecyclerView.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
        cardList.addView(fileRecyclerView);
        layout.addView(cardList, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        return layout;
    }

    private View createEditorView() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(24, 24, 24, 24);

        // 标题
        MaterialCardView cardEditorTitle = new MaterialCardView(this);
        cardEditorTitle.setCardElevation(4);
        cardEditorTitle.setRadius(16);
        cardEditorTitle.setUseCompatPadding(true);
        LinearLayout titleLayout = new LinearLayout(this);
        titleLayout.setOrientation(LinearLayout.VERTICAL);
        titleLayout.setPadding(24, 20, 24, 20);
        TextView tvEditorTitle = new TextView(this);
        tvEditorTitle.setText("编辑内容");
        tvEditorTitle.setTextSize(22);
        tvEditorTitle.setTypeface(null, Typeface.BOLD);
        tvEditorTitle.setTextColor(0xFF333333);
        titleLayout.addView(tvEditorTitle);
        cardEditorTitle.addView(titleLayout);
        layout.addView(cardEditorTitle);

        // 操作按钮行（添加节、添加键值对）
        LinearLayout buttonBarTop = new LinearLayout(this);
        buttonBarTop.setOrientation(LinearLayout.HORIZONTAL);
        buttonBarTop.setPadding(0, 16, 0, 16);

        btnAddSection = new MaterialButton(this);
        btnAddSection.setText("添加节");
        btnAddSection.setIconResource(android.R.drawable.ic_menu_add);
        btnAddSection.setOnClickListener(v -> showAddSectionDialog());

        btnAddKeyValue = new MaterialButton(this);
        btnAddKeyValue.setText("添加键值对");
        btnAddKeyValue.setIconResource(android.R.drawable.ic_input_add);
        btnAddKeyValue.setOnClickListener(v -> showAddKeyValueDialog());

        buttonBarTop.addView(btnAddSection, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        buttonBarTop.addView(btnAddKeyValue, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        layout.addView(buttonBarTop);

        // 提示文字
        TextView tvHint = new TextView(this);
        tvHint.setText("长按节可改名，长按键值对可删除");
        tvHint.setTextSize(13);
        tvHint.setTextColor(0xFF888888);
        tvHint.setPadding(8, 0, 8, 8);
        layout.addView(tvHint);

        // 编辑器列表卡片
        MaterialCardView cardEditorList = new MaterialCardView(this);
        cardEditorList.setCardElevation(2);
        cardEditorList.setRadius(12);
        editorRecyclerView = new RecyclerView(this);
        editorRecyclerView.setLayoutManager(new LinearLayoutManager(this));
        editorRecyclerView.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
        cardEditorList.addView(editorRecyclerView);
        layout.addView(cardEditorList, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

        // 底部按钮栏
        LinearLayout buttonBarBottom = new LinearLayout(this);
        buttonBarBottom.setOrientation(LinearLayout.HORIZONTAL);
        buttonBarBottom.setPadding(0, 24, 0, 0);

        btnSave = new MaterialButton(this);
        btnSave.setText("保存更改");
        btnSave.setIconResource(android.R.drawable.ic_menu_save);
        btnSave.setOnClickListener(v -> saveCurrentFile());

        btnBackToList = new MaterialButton(this);
        btnBackToList.setText("返回列表");
        btnBackToList.setIconResource(android.R.drawable.ic_menu_revert);
        btnBackToList.setOnClickListener(v -> showFileList());

        buttonBarBottom.addView(btnSave, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        buttonBarBottom.addView(btnBackToList, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        layout.addView(buttonBarBottom);

        return layout;
    }

    private void showNewFileDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("新建 INI 文件");

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(48, 24, 48, 0);
        final EditText input = new EditText(this);
        input.setHint("文件名（不含 .ini）");
        layout.addView(input);
        builder.setView(layout);

        builder.setPositiveButton("创建", (dialog, which) -> {
            String name = input.getText().toString().trim();
            if (TextUtils.isEmpty(name)) {
                Toast.makeText(this, "文件名不能为空", Toast.LENGTH_SHORT).show();
                return;
            }
            if (!name.endsWith(".ini")) name += ".ini";
            File newFile = new File(folderPath, name);
            if (newFile.exists()) {
                Toast.makeText(this, "文件已存在", Toast.LENGTH_SHORT).show();
                return;
            }
            try {
                if (newFile.createNewFile()) {
                    Ini emptyIni = new Ini();
                    emptyIni.save(newFile.getAbsolutePath());
                    Toast.makeText(this, "创建成功", Toast.LENGTH_SHORT).show();
                    loadFileList();
                } else {
                    Toast.makeText(this, "创建失败", Toast.LENGTH_SHORT).show();
                }
            } catch (IOException e) {
                Toast.makeText(this, "创建失败: " + e.getMessage(), Toast.LENGTH_SHORT).show();
            }
        });
        builder.setNegativeButton("取消", null);
        builder.show();
    }

    private void showAddSectionDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("添加新节");

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(48, 24, 48, 0);
        final EditText input = new EditText(this);
        input.setHint("节名");
        layout.addView(input);
        builder.setView(layout);

        builder.setPositiveButton("添加", (dialog, which) -> {
            String sectionName = input.getText().toString().trim();
            if (TextUtils.isEmpty(sectionName)) {
                Toast.makeText(this, "节名不能为空", Toast.LENGTH_SHORT).show();
                return;
            }
            for (IniItem item : editorItems) {
                if (item.type == IniItem.TYPE_SECTION && item.displaySection.equals(sectionName)) {
                    Toast.makeText(this, "节已存在", Toast.LENGTH_SHORT).show();
                    return;
                }
            }
            editorItems.add(new IniItem(IniItem.TYPE_SECTION, sectionName, null, null, null));
            editorAdapter.notifyItemInserted(editorItems.size() - 1);
            editorRecyclerView.smoothScrollToPosition(editorItems.size() - 1);
        });
        builder.setNegativeButton("取消", null);
        builder.show();
    }

    private void showAddKeyValueDialog() {
        List<String> sectionNames = new ArrayList<>();
        for (IniItem item : editorItems) {
            if (item.type == IniItem.TYPE_SECTION) {
                sectionNames.add(item.displaySection);
            }
        }
        if (sectionNames.isEmpty()) {
            Toast.makeText(this, "请先添加一个节", Toast.LENGTH_SHORT).show();
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("选择要添加键值对的节");
        final String[] sectionsArray = sectionNames.toArray(new String[0]);
        builder.setItems(sectionsArray, (dialog, which) -> {
            String selectedSection = sectionsArray[which];
            showKeyValueInputDialog(selectedSection);
        });
        builder.setNegativeButton("取消", null);
        builder.show();
    }

    private void showKeyValueInputDialog(String section) {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(48, 24, 48, 0);

        final EditText keyInput = new EditText(this);
        keyInput.setHint("键名");
        final EditText valueInput = new EditText(this);
        valueInput.setHint("值");

        layout.addView(keyInput);
        layout.addView(valueInput);

        new AlertDialog.Builder(this)
                .setTitle("添加到节: " + section)
                .setView(layout)
                .setPositiveButton("添加", (dialog, which) -> {
                    String key = keyInput.getText().toString().trim();
                    String value = valueInput.getText().toString().trim();
                    if (TextUtils.isEmpty(key)) {
                        Toast.makeText(this, "键名不能为空", Toast.LENGTH_SHORT).show();
                        return;
                    }
                    int insertPos = -1;
                    for (int i = 0; i < editorItems.size(); i++) {
                        IniItem item = editorItems.get(i);
                        if (item.type == IniItem.TYPE_SECTION && item.displaySection.equals(section)) {
                            insertPos = i + 1;
                            while (insertPos < editorItems.size() &&
                                    editorItems.get(insertPos).type == IniItem.TYPE_KEY_VALUE) {
                                insertPos++;
                            }
                            break;
                        }
                    }
                    if (insertPos == -1) insertPos = editorItems.size();
                    IniItem newItem = new IniItem(IniItem.TYPE_KEY_VALUE, null, section, key, value);
                    editorItems.add(insertPos, newItem);
                    editorAdapter.notifyItemInserted(insertPos);
                    editorRecyclerView.smoothScrollToPosition(insertPos);
                })
                .setNegativeButton("取消", null)
                .show();
    }

    private void loadFileList() {
        File dir = new File(folderPath);
        if (!dir.exists() || !dir.isDirectory()) {
            Toast.makeText(this, "文件夹不存在或无法访问", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }
        iniFiles = dir.listFiles((d, name) -> name.toLowerCase().endsWith(".ini"));
        if (iniFiles == null || iniFiles.length == 0) {
            iniFiles = new File[0];
        }

        FileListAdapter adapter = new FileListAdapter(iniFiles, file -> {
            currentEditingFile = file;
            loadIniToEditor(file);
            showEditor();
        });
        fileRecyclerView.setAdapter(adapter);
    }

    private void loadIniToEditor(File iniFile) {
        Ini ini = new Ini();
        if (!ini.load(iniFile.getAbsolutePath())) {
            Toast.makeText(this, "加载 INI 文件失败", Toast.LENGTH_SHORT).show();
            return;
        }

        editorItems = new ArrayList<>();
        for (String section : ini.sections()) {
            String displaySection = section.isEmpty() ? "[全局]" : section;
            editorItems.add(new IniItem(IniItem.TYPE_SECTION, displaySection, null, null, null));
            for (String key : ini.keys(section)) {
                String value = ini.get(section, key);
                editorItems.add(new IniItem(IniItem.TYPE_KEY_VALUE, null, section, key, value));
            }
        }

        if (editorAdapter == null) {
            editorAdapter = new EditorAdapter(editorItems);
            editorRecyclerView.setAdapter(editorAdapter);
        } else {
            editorAdapter.updateItems(editorItems);
        }
    }

    private void saveCurrentFile() {
        if (currentEditingFile == null || editorItems == null) return;

        // 由于使用了 TextWatcher 实时同步，直接使用 currentValue 即可
        Ini ini = new Ini();
        String currentSection = null;
        for (IniItem item : editorItems) {
            if (item.type == IniItem.TYPE_SECTION) {
                String display = item.displaySection;
                if ("[全局]".equals(display)) {
                    currentSection = "";
                } else {
                    currentSection = display;
                }
            } else if (item.type == IniItem.TYPE_KEY_VALUE) {
                if (currentSection == null) {
                    currentSection = "";
                }
                String realSection = (item.section != null && !item.section.isEmpty()) ? item.section : currentSection;
                ini.set(realSection, item.key, item.currentValue);
            }
        }

        if (ini.save(currentEditingFile.getAbsolutePath())) {
            Toast.makeText(this, "保存成功", Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this, "保存失败", Toast.LENGTH_SHORT).show();
        }
    }

    private void showFileList() {
        fileListView.setVisibility(View.VISIBLE);
        editorView.setVisibility(View.GONE);
        loadFileList();
    }

    private void showEditor() {
        fileListView.setVisibility(View.GONE);
        editorView.setVisibility(View.VISIBLE);
    }

    private void showRenameSectionDialog(int position, String oldDisplayName) {
        String oldSectionName = "[全局]".equals(oldDisplayName) ? "" : oldDisplayName;

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("修改节名");

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(48, 24, 48, 0);
        final EditText input = new EditText(this);
        input.setText(oldSectionName);
        input.setSelection(input.getText().length());
        layout.addView(input);
        builder.setView(layout);

        builder.setPositiveButton("修改", (dialog, which) -> {
            String newName = input.getText().toString().trim();
            if (TextUtils.isEmpty(newName)) {
                Toast.makeText(this, "节名不能为空", Toast.LENGTH_SHORT).show();
                return;
            }
            for (int i = 0; i < editorItems.size(); i++) {
                IniItem item = editorItems.get(i);
                if (item.type == IniItem.TYPE_SECTION && i != position) {
                    String otherName = item.displaySection;
                    if ("[全局]".equals(otherName)) otherName = "";
                    if (otherName.equals(newName)) {
                        Toast.makeText(this, "节名已存在", Toast.LENGTH_SHORT).show();
                        return;
                    }
                }
            }
            IniItem sectionItem = editorItems.get(position);
            sectionItem.displaySection = newName;
            String newRealName = newName;
            for (int i = position + 1; i < editorItems.size(); i++) {
                IniItem item = editorItems.get(i);
                if (item.type == IniItem.TYPE_SECTION) break;
                if (item.type == IniItem.TYPE_KEY_VALUE) {
                    item.section = newRealName;
                }
            }
            editorAdapter.notifyItemChanged(position);
        });
        builder.setNegativeButton("取消", null);
        builder.show();
    }

    private void deleteKeyValue(int position) {
        new AlertDialog.Builder(this)
                .setTitle("确认删除")
                .setMessage("确定要删除这个键值对吗？")
                .setPositiveButton("删除", (d, w) -> {
                    editorItems.remove(position);
                    editorAdapter.notifyItemRemoved(position);
                })
                .setNegativeButton("取消", null)
                .show();
    }

    // ==================== 适配器 ====================
    static class FileListAdapter extends RecyclerView.Adapter<FileListAdapter.ViewHolder> {
        private final File[] files;
        private final OnFileClickListener listener;

        FileListAdapter(File[] files, OnFileClickListener listener) {
            this.files = files;
            this.listener = listener;
        }

        @NonNull
        @Override
        public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            TextView tv = new TextView(parent.getContext());
            tv.setPadding(32, 20, 32, 20);
            tv.setTextSize(16);
            tv.setTextColor(0xFF333333);
            tv.setBackgroundResource(android.R.drawable.list_selector_background);
            return new ViewHolder(tv);
        }

        @Override
        public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
            holder.textView.setText(files[position].getName());
            holder.textView.setOnClickListener(v -> listener.onFileClick(files[position]));
        }

        @Override
        public int getItemCount() {
            return files.length;
        }

        class ViewHolder extends RecyclerView.ViewHolder {
            TextView textView;
            ViewHolder(TextView tv) {
                super(tv);
                textView = tv;
            }
        }
    }

    interface OnFileClickListener {
        void onFileClick(File file);
    }

    class EditorAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
        private List<IniItem> items;

        EditorAdapter(List<IniItem> items) {
            this.items = items;
        }

        void updateItems(List<IniItem> newItems) {
            this.items = newItems;
            notifyDataSetChanged();
        }

        @Override
        public int getItemViewType(int position) {
            return items.get(position).type;
        }

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            if (viewType == IniItem.TYPE_SECTION) {
                TextView tv = new TextView(parent.getContext());
                tv.setPadding(24, 20, 24, 12);
                tv.setTextSize(18);
                tv.setTypeface(null, Typeface.BOLD);
                tv.setTextColor(0xFF1E88E5);
                tv.setBackgroundColor(0xFFF0F0F0);
                return new SectionViewHolder(tv);
            } else {
                LinearLayout ll = new LinearLayout(parent.getContext());
                ll.setOrientation(LinearLayout.HORIZONTAL);
                ll.setPadding(24, 12, 16, 12);

                TextView keyView = new TextView(parent.getContext());
                keyView.setLayoutParams(new LinearLayout.LayoutParams(0,
                        ViewGroup.LayoutParams.WRAP_CONTENT, 2));
                keyView.setTextSize(15);
                keyView.setTextColor(0xFF444444);

                EditText valueView = new EditText(parent.getContext());
                valueView.setLayoutParams(new LinearLayout.LayoutParams(0,
                        ViewGroup.LayoutParams.WRAP_CONTENT, 3));
                valueView.setTextSize(15);
                valueView.setBackgroundResource(android.R.drawable.edit_text);
                valueView.setPadding(16, 8, 16, 8);

                MaterialButton deleteBtn = new MaterialButton(parent.getContext());
                deleteBtn.setText("删除");
                deleteBtn.setTextSize(12);
                deleteBtn.setPadding(8, 0, 0, 0);
                deleteBtn.setBackgroundColor(0x00000000);
                deleteBtn.setTextColor(0xFFE53935);
                deleteBtn.setIconResource(android.R.drawable.ic_menu_delete);

                ll.addView(keyView);
                ll.addView(valueView);
                ll.addView(deleteBtn);
                return new KeyValueViewHolder(ll, keyView, valueView, deleteBtn);
            }
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
            IniItem item = items.get(position);
            if (holder instanceof SectionViewHolder) {
                SectionViewHolder svh = (SectionViewHolder) holder;
                svh.textView.setText(item.displaySection);
                svh.textView.setOnLongClickListener(v -> {
                    showRenameSectionDialog(position, item.displaySection);
                    return true;
                });
            } else if (holder instanceof KeyValueViewHolder) {
                KeyValueViewHolder kvh = (KeyValueViewHolder) holder;
                kvh.keyView.setText(item.key + " = ");
                kvh.valueView.setText(item.currentValue);
                
                // 移除旧的监听器避免重复添加
                if (kvh.valueView.getTag() instanceof TextWatcher) {
                    kvh.valueView.removeTextChangedListener((TextWatcher) kvh.valueView.getTag());
                }
                TextWatcher watcher = new TextWatcher() {
                    @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
                    @Override public void onTextChanged(CharSequence s, int start, int before, int count) {}
                    @Override public void afterTextChanged(Editable s) {
                        item.currentValue = s.toString();
                    }
                };
                kvh.valueView.addTextChangedListener(watcher);
                kvh.valueView.setTag(watcher);
                
                kvh.deleteBtn.setOnClickListener(v -> deleteKeyValue(position));
                kvh.itemView.setOnLongClickListener(v -> {
                    deleteKeyValue(position);
                    return true;
                });
            }
        }

        @Override
        public int getItemCount() {
            return items.size();
        }

        class SectionViewHolder extends RecyclerView.ViewHolder {
            TextView textView;
            SectionViewHolder(TextView tv) {
                super(tv);
                textView = tv;
            }
        }

        class KeyValueViewHolder extends RecyclerView.ViewHolder {
            TextView keyView;
            EditText valueView;
            MaterialButton deleteBtn;
            KeyValueViewHolder(View itemView, TextView key, EditText value, MaterialButton del) {
                super(itemView);
                keyView = key;
                valueView = value;
                deleteBtn = del;
            }
        }
    }

    static class IniItem {
        static final int TYPE_SECTION = 0;
        static final int TYPE_KEY_VALUE = 1;

        int type;
        String displaySection;
        String section;
        String key;
        String currentValue;

        IniItem(int type, String displaySection, String section, String key, String value) {
            this.type = type;
            this.displaySection = displaySection;
            this.section = section;
            this.key = key;
            this.currentValue = value;
        }
    }
}