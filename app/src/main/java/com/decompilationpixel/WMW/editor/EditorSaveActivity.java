package com.decompilationpixel.WMW.editor;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.SparseArray;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.ScrollView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SearchView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager.widget.ViewPager;

import com.decompilationpixel.WMW.R;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class EditorSaveActivity extends AppCompatActivity {

    private SQLiteDatabase database;
    private Map<String, List<String>> tableSchemas = new HashMap<>();
    private List<String> tableNames = new ArrayList<>();
    private TextView tvTableInfo;
    private DatabasePagerAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_save_editor);

        // 打开数据库
        File dbFile = new File(getFilesDir(), "data/water.db");
        
        database = SQLiteDatabase.openDatabase(dbFile.getPath(), null, SQLiteDatabase.OPEN_READWRITE);
        
        // 获取所有表名
        getTableNames();
        
        // 设置UI组件
        setupUI();
    }

    private void setupUI() {
        TabLayout tabLayout = findViewById(R.id.tabLayout);
        ViewPager viewPager = findViewById(R.id.viewPager);
        tvTableInfo = findViewById(R.id.tvTableInfo);
        Button btnSearch = findViewById(R.id.btnSearch);
        
        // 更新表格信息显示
        updateTableInfo();
        
        // 设置Tab导航
        adapter = new DatabasePagerAdapter(getSupportFragmentManager());
        viewPager.setAdapter(adapter);
        tabLayout.setupWithViewPager(viewPager);
        
        // 搜索按钮点击事件
        btnSearch.setOnClickListener(v -> showSearchDialog());
        
        // 添加页面切换监听
        viewPager.addOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener() {
            @Override
            public void onPageSelected(int position) {
                // 刷新当前显示的Fragment
                adapter.refreshFragment(position);
            }
        });
    }
    
    private void updateTableInfo() {
        tvTableInfo.setText(String.format(Locale.getDefault(), 
            "共 %d 张表 | 已加载 %d 张", tableSchemas.size(), tableNames.size()));
    }

    private void getTableNames() {
        try (Cursor cursor = database.rawQuery("SELECT name FROM sqlite_master WHERE type='table'", null)) {
            if (cursor.moveToFirst()) {
                do {
                    String tableName = cursor.getString(0);
                    if (!tableName.equals("android_metadata") && !tableName.equals("sqlite_sequence")) {
                        tableNames.add(tableName);
                        // 获取表结构
                        getTableSchema(tableName);
                    }
                } while (cursor.moveToNext());
            }
        }
    }

    private void getTableSchema(String tableName) {
        List<String> columns = new ArrayList<>();
        try (Cursor cursor = database.rawQuery("PRAGMA table_info(" + tableName + ")", null)) {
            if (cursor.moveToFirst()) {
                do {
                    columns.add(cursor.getString(1)); // 获取列名
                } while (cursor.moveToNext());
            }
        }
        tableSchemas.put(tableName, columns);
    }
    
    private void showSearchDialog() {
        MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(this);
        builder.setTitle("搜索表数据");
        
        // 创建搜索视图
        SearchView searchView = new SearchView(this);
        searchView.setQueryHint("输入表名或关键词搜索...");
        
        // 监听搜索
        searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
            @Override
            public boolean onQueryTextSubmit(String query) {
                searchTables(query);
                return true;
            }

            @Override
            public boolean onQueryTextChange(String newText) {
                searchTables(newText);
                return true;
            }
        });
        
        builder.setView(searchView);
        builder.setNegativeButton("关闭", null);
        builder.show();
    }
    
    private void searchTables(String query) {
        if (database == null || query == null || query.isEmpty()) return;
        
        // TODO: 实现跨表搜索逻辑
        Toast.makeText(this, "搜索: " + query, Toast.LENGTH_SHORT).show();
    }

    private class DatabasePagerAdapter extends FragmentPagerAdapter {
        
        private final SparseArray<TableFragment> fragmentReferences = new SparseArray<>();

        public DatabasePagerAdapter(FragmentManager fm) {
            super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        }

        @Override
        public Fragment getItem(int position) {
            TableFragment fragment = TableFragment.newInstance(tableNames.get(position));
            fragmentReferences.put(position, fragment);
            return fragment;
        }

        @Override
        public int getCount() {
            return tableNames.size();
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return tableNames.get(position);
        }
        
        // 刷新指定位置的Fragment数据
        public void refreshFragment(int position) {
            TableFragment fragment = fragmentReferences.get(position);
            if (fragment != null) {
                fragment.reloadTable();
            }
        }
        
        // 处理Fragment销毁
        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {
            fragmentReferences.remove(position);
            super.destroyItem(container, position, object);
        }
    }

    public static class TableFragment extends Fragment {
        
        private String tableName;
        private List<Map<String, String>> tableData = new ArrayList<>();
        private RecyclerView recyclerView;
        private TableAdapter adapter;
        private TextView tvHeader;
        private Button btnAdd;
        private boolean isDataLoaded = false;

        static TableFragment newInstance(String tableName) {
            TableFragment fragment = new TableFragment();
            Bundle args = new Bundle();
            args.putString("table_name", tableName);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            if (getArguments() != null) {
                tableName = getArguments().getString("table_name");
            }
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            View view = inflater.inflate(R.layout.fragment_table, container, false);
            
            // 初始化视图
            tvHeader = view.findViewById(R.id.tvHeader);
            btnAdd = view.findViewById(R.id.btnAdd);
            recyclerView = view.findViewById(R.id.recyclerView);
            
            // 设置表头
            EditorSaveActivity activity = (EditorSaveActivity) getActivity();
            if (activity != null) {
                List<String> columns = activity.tableSchemas.get(tableName);
                if (columns != null) {
                    tvHeader.setText(String.format(Locale.getDefault(), 
                        "%s  (%d列)", tableName, columns.size()));
                }
            }
            
            // 设置列表
            recyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
            recyclerView.addItemDecoration(new DividerItemDecoration(getContext(), DividerItemDecoration.VERTICAL));
            
            // 初始化适配器
            adapter = new TableAdapter(tableData);
            recyclerView.setAdapter(adapter);
            
            // 添加记录按钮
            btnAdd.setOnClickListener(v -> showEditDialog(null));
            
            // 只在首次创建或数据未加载时加载数据
            if (!isDataLoaded) {
                loadTableData();
            }
            
            return view;
        }

        @Override
        public void setUserVisibleHint(boolean isVisibleToUser) {
            super.setUserVisibleHint(isVisibleToUser);
            if (isVisibleToUser && isResumed()) {
                // Fragment对用户可见时刷新数据
                loadTableData();
            }
        }

        @Override
        public void onResume() {
            super.onResume();
            if (getUserVisibleHint()) {
                // Fragment可见时的处理
                loadTableData();
            }
        }

        private void loadTableData() {
            EditorSaveActivity activity = (EditorSaveActivity) getActivity();
            if (activity == null || activity.database == null) return;
            
            List<String> columns = activity.tableSchemas.get(tableName);
            if (columns == null) return;

            // 确定排序字段（优先使用 _id，其次 id）
            String orderBy = null;
            if (columns.contains("_id")) {
                orderBy = "_id DESC";
            } else if (columns.contains("id")) {
                orderBy = "id DESC";
            }

            try (Cursor cursor = activity.database.query(tableName, null, null, null, null, null, orderBy)) {
                tableData.clear();
                if (cursor.moveToFirst()) {
                    do {
                        Map<String, String> row = new HashMap<>();
                        for (String column : columns) {
                            int index = cursor.getColumnIndexOrThrow(column);
                            row.put(column, index >= 0 ? cursor.getString(index) : "NULL");
                        }
                        tableData.add(row);
                    } while (cursor.moveToNext());
                }
            }
            
            // 更新适配器
            if (adapter != null) {
                adapter.updateData(tableData);
            }
            
            isDataLoaded = true;
        }
        
        private class TableAdapter extends RecyclerView.Adapter<TableAdapter.ViewHolder> {
            
            private List<Map<String, String>> data;
            
            public TableAdapter(List<Map<String, String>> data) {
                this.data = data;
            }
            
            public void updateData(List<Map<String, String>> newData) {
                this.data = newData;
                notifyDataSetChanged();
            }
            
            @NonNull
            @Override
            public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
                View view = LayoutInflater.from(parent.getContext())
                    .inflate(R.layout.item_table_row, parent, false);
                return new ViewHolder(view);
            }
            
            @Override
            public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
                Map<String, String> row = data.get(position);
                
                // 显示唯一ID（优先使用_id，其次id）
                String idValue = "ID: ";
                if (row.containsKey("_id")) {
                    idValue += row.get("_id");
                } else if (row.containsKey("id")) {
                    idValue += row.get("id");
                } else {
                    idValue += "-";
                }
                holder.tvId.setText(idValue);
                
                // 创建详情预览（排除ID字段）
                StringBuilder details = new StringBuilder();
                for (Map.Entry<String, String> entry : row.entrySet()) {
                    String key = entry.getKey();
                    if (!key.equals("_id") && !key.equals("id")) {
                        if (details.length() > 0) details.append(" • ");
                        if (entry.getValue() != null && entry.getValue().length() > 20) {
                            details.append(entry.getValue().substring(0, 20)).append("...");
                        } else {
                            details.append(entry.getValue());
                        }
                    }
                }
                
                holder.tvDetails.setText(details.toString());
                
                // 行点击事件
                holder.itemView.setOnClickListener(v -> {
                    showRowDetail(row);
                });
                
                // 更多操作按钮
                holder.btnMore.setOnClickListener(v -> {
                    showRowMenu(v, position, row);
                });
            }
            
            @Override
            public int getItemCount() {
                return data.size();
            }
            
            public class ViewHolder extends RecyclerView.ViewHolder {
                TextView tvId;
                TextView tvDetails;
                Button btnMore;
                
                public ViewHolder(@NonNull View itemView) {
                    super(itemView);
                    tvId = itemView.findViewById(R.id.tvRowId);
                    tvDetails = itemView.findViewById(R.id.tvRowDetails);
                    btnMore = itemView.findViewById(R.id.btnMore);
                }
            }
        }
        
        private void showRowDetail(Map<String, String> row) {
            MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireContext());
            builder.setTitle("记录详情");
            
            // 创建垂直布局
            LinearLayout layout = new LinearLayout(requireContext());
            layout.setOrientation(LinearLayout.VERTICAL);
            int padding = dpToPx(16);
            layout.setPadding(padding, padding, padding, padding);
            
            // 添加所有列值
            for (Map.Entry<String, String> entry : row.entrySet()) {
                TextView label = new TextView(requireContext());
                label.setText(String.format(Locale.getDefault(), "%s:", entry.getKey()));
                label.setTextSize(16);
                label.setTypeface(null, Typeface.BOLD);
                label.setPadding(0, dpToPx(8), 0, dpToPx(2));
                
                TextView value = new TextView(requireContext());
                value.setText(entry.getValue());
                value.setTextSize(16);
                value.setPadding(dpToPx(12), 0, 0, dpToPx(10));
                
                layout.addView(label);
                layout.addView(value);
            }
            
            // 添加滚动支持
            ScrollView scrollView = new ScrollView(requireContext());
            scrollView.addView(layout);
            
            builder.setView(scrollView);
            builder.setPositiveButton("关闭", null);
            builder.setNeutralButton("编辑", (dialog, which) -> {
                // 获取记录的索引
                showEditDialog(tableData.indexOf(row));
            });
            builder.show();
        }
        
        private int dpToPx(int dp) {
            return (int) TypedValue.applyDimension(
                    TypedValue.COMPLEX_UNIT_DIP, 
                    dp, 
                    getResources().getDisplayMetrics());
        }
        
        private void showRowMenu(View anchor, int position, Map<String, String> row) {
            PopupMenu popup = new PopupMenu(requireContext(), anchor);
            popup.getMenuInflater().inflate(R.menu.row_menu, popup.getMenu());
            
            popup.setOnMenuItemClickListener(item -> {
                int itemId = item.getItemId();
                if (itemId == R.id.menu_edit) {
                    showEditDialog(position);
                    return true;
                } else if (itemId == R.id.menu_delete) {
                    deleteRow(position, row);
                    return true;
                } else if (itemId == R.id.menu_duplicate) {
                    duplicateRow(row);
                    return true;
                }
                return false;
            });
            
            popup.show();
        }
        
        private void deleteRow(int position, Map<String, String> row) {
            EditorSaveActivity activity = (EditorSaveActivity) getActivity();
            if (activity == null) return;
            
            MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireContext());
            builder.setTitle("确认删除");
            builder.setMessage("确定要删除此行数据吗？该操作无法撤销。");
            
            builder.setPositiveButton("删除", (dialog, which) -> {
                String where = "";
                List<String> whereArgs = new ArrayList<>();
                int count = 0;
                
                for (Map.Entry<String, String> entry : row.entrySet()) {
                    if (count > 0) where += " AND ";
                    where += entry.getKey() + " = ?";
                    whereArgs.add(entry.getValue());
                    count++;
                }
                
                activity.database.delete(tableName, where, whereArgs.toArray(new String[0]));
                Toast.makeText(activity, "已删除!", Toast.LENGTH_SHORT).show();
                reloadTable();
            });
            
            builder.setNegativeButton("取消", null);
            builder.show();
        }
        
        private void duplicateRow(Map<String, String> row) {
            EditorSaveActivity activity = (EditorSaveActivity) getActivity();
            if (activity == null) return;
            
            ContentValues values = new ContentValues();
            for (Map.Entry<String, String> entry : row.entrySet()) {
                values.put(entry.getKey(), entry.getValue());
            }
            
            activity.database.insert(tableName, null, values);
            Toast.makeText(activity, "已复制!", Toast.LENGTH_SHORT).show();
            reloadTable();
        }

        private void showEditDialog(Integer position) {
            EditorSaveActivity activity = (EditorSaveActivity) getActivity();
            if (activity == null) return;

            MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireContext());
            builder.setTitle(position == null ? "添加记录" : "编辑记录");
            
            // 动态创建编辑表单
            LinearLayout layout = new LinearLayout(requireContext());
            layout.setOrientation(LinearLayout.VERTICAL);
            int padding = dpToPx(16);
            layout.setPadding(padding, padding, padding, padding);
            
            Map<String, String> currentRow = position == null ? 
                new HashMap<>() : tableData.get(position);
            
            List<TextInputLayout> inputLayouts = new ArrayList<>();
            List<TextInputEditText> editTexts = new ArrayList<>();
            List<String> columns = activity.tableSchemas.get(tableName);
            
            for (String column : columns) {
                // 创建带标签的输入框
                TextInputLayout inputLayout = new TextInputLayout(requireContext(), null);
                inputLayout.setHint(column);
                inputLayout.setPadding(0, 0, 0, dpToPx(8));
                
                TextInputEditText editText = new TextInputEditText(inputLayout.getContext());
                editText.setText(currentRow.get(column));
                editText.setMinWidth(dpToPx(200));
                editText.setSingleLine(true);
                inputLayout.addView(editText);
                
                layout.addView(inputLayout);
                inputLayouts.add(inputLayout);
                editTexts.add(editText);
            }
            
            // 添加滚动支持
            ScrollView scrollView = new ScrollView(requireContext());
            scrollView.addView(layout);
            builder.setView(scrollView);
            
            builder.setPositiveButton("保存", (dialog, which) -> {
                ContentValues values = new ContentValues();
                
                for (int i = 0; i < columns.size(); i++) {
                    String colName = columns.get(i);
                    String value = editTexts.get(i).getText().toString();
                    values.put(colName, value);
                }
                
                if (position == null) {
                    // 插入新行
                    activity.database.insert(tableName, null, values);
                    Toast.makeText(activity, "已添加新记录!", Toast.LENGTH_SHORT).show();
                } else {
                    // 更新行
                    String where = "";
                    List<String> whereArgs = new ArrayList<>();
                    int count = 0;
                    
                    for (Map.Entry<String, String> entry : tableData.get(position).entrySet()) {
                        if (count > 0) where += " AND ";
                        where += entry.getKey() + " = ?";
                        whereArgs.add(entry.getValue());
                        count++;
                    }
                    
                    activity.database.update(tableName, values, where, whereArgs.toArray(new String[0]));
                    Toast.makeText(activity, "已更新记录!", Toast.LENGTH_SHORT).show();
                }
                
                reloadTable();
            });
            
            if (position != null) {
                builder.setNeutralButton("删除", (dialog, which) -> {
                    deleteRow(position, tableData.get(position));
                });
            }
            
            builder.setNegativeButton("取消", null);
            builder.show();
        }

        private void reloadTable() {
            loadTableData();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (database != null) {
            database.close();
        }
    }
}
