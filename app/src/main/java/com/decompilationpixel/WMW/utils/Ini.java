package com.decompilationpixel.WMW.utils;

// Ini.java
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.stream.Collectors;

/** 极简 INI 解析器 支持节 [section]、键值对 key=value 或 key:value、注释行（; 或 #） 支持全局节 支持从文件/字符串加载，修改后保存 */
public class Ini {
    private final Map<String, Map<String, String>> data = new LinkedHashMap<>();

    // ==================== 加载与保存 ====================

    /**
     * 从文件加载 INI 配置
     *
     * @param filename 文件路径
     * @return 是否成功
     */
    public boolean load(String filename) {
        try (BufferedReader reader = Files.newBufferedReader(Paths.get(filename), StandardCharsets.UTF_8)) {
            return load(reader);
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * 从输入流加载 INI 配置
     *
     * @param reader 字符输入流
     * @return 是否成功
     */
    public boolean load(BufferedReader reader) {
        data.clear();
        String currentSection = "";
        try {
            String line;
            while ((line = reader.readLine()) != null) {
                line = line.strip();
                // 跳过空行和注释
                if (line.isEmpty() || line.startsWith(";") || line.startsWith("#"))
                    continue;
                // 处理节
                if (line.startsWith("[") && line.endsWith("]")) {
                    currentSection = line.substring(1, line.length() - 1).strip();
                    continue;
                }
                // 处理键值对
                int delimIdx = line.indexOf('=');
                if (delimIdx < 0) delimIdx = line.indexOf(':');
                if (delimIdx > 0) {
                    String key = line.substring(0, delimIdx).strip();
                    String value = line.substring(delimIdx + 1).strip();
                    if (!key.isEmpty())
                        data.computeIfAbsent(currentSection, k -> new LinkedHashMap<>()).put(key, value);
                }
            }
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * 从字符串内容解析 INI
     *
     * @param content INI 格式字符串
     * @return 是否成功
     */
    public boolean parse(String content) {
        try (BufferedReader reader = new BufferedReader(new StringReader(content))) {
            return load(reader);
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * 保存到文件
     *
     * @param filename 文件路径
     * @return 是否成功
     */
    public boolean save(String filename) {
        try (BufferedWriter writer = Files.newBufferedWriter(Paths.get(filename), StandardCharsets.UTF_8)) {
            return save(writer);
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * 保存到输出流
     *
     * @param writer 字符输出流
     * @return 是否成功
     */
    public boolean save(BufferedWriter writer) {
        try {
            // 全局节（空字符串）最先输出
            if (data.containsKey("") && !data.get("").isEmpty()) {
                for (Map.Entry<String, String> entry : data.get("").entrySet()) {
                    writer.write(entry.getKey() + "=" + entry.getValue());
                    writer.newLine();
                }
                writer.newLine();
            }
            // 其他节
            for (Map.Entry<String, Map<String, String>> sectionEntry : data.entrySet()) {
                String section = sectionEntry.getKey();
                if (section.isEmpty()) continue;
                Map<String, String> kvMap = sectionEntry.getValue();
                if (kvMap.isEmpty()) continue;
                writer.write("[" + section + "]");
                writer.newLine();
                for (Map.Entry<String, String> kv : kvMap.entrySet()) {
                    writer.write(kv.getKey() + "=" + kv.getValue());
                    writer.newLine();
                }
                writer.newLine();
            }
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    // ==================== 读取操作 ====================

    /**
     * 获取字符串值
     *
     * @param section 节名（空字符串表示全局节）
     * @param key 键名
     * @param defaultValue 默认值
     * @return 值或默认值
     */
    public String get(String section, String key, String defaultValue) {
        Map<String, String> sec = data.get(section);
        if (sec == null) return defaultValue;
        return sec.getOrDefault(key, defaultValue);
    }

    public String get(String section, String key) {
        return get(section, key, "");
    }

    /** 获取整数值 */
    public int getInt(String section, String key, int defaultValue) {
        String val = get(section, key);
        try {
            return Integer.parseInt(val);
        } catch (NumberFormatException e) {
            return defaultValue;
        }
    }

    public int getInt(String section, String key) {
        return getInt(section, key, 0);
    }

    /** 获取布尔值（true/1/yes/on 为 true，false/0/no/off 为 false，忽略大小写） */
    public boolean getBool(String section, String key, boolean defaultValue) {
        String val = get(section, key);
        if (val.isEmpty()) return defaultValue;
        String lower = val.toLowerCase();
        if (lower.equals("true") || lower.equals("1") || lower.equals("yes") || lower.equals("on"))
            return true;
        if (lower.equals("false") || lower.equals("0") || lower.equals("no") || lower.equals("off"))
            return false;
        return defaultValue;
    }

    public boolean getBool(String section, String key) {
        return getBool(section, key, false);
    }

    /** 获取浮点数值 */
    public double getDouble(String section, String key, double defaultValue) {
        String val = get(section, key);
        try {
            return Double.parseDouble(val);
        } catch (NumberFormatException e) {
            return defaultValue;
        }
    }

    public double getDouble(String section, String key) {
        return getDouble(section, key, 0.0);
    }

    // ==================== 写入操作 ====================

    /**
     * 设置字符串值（会覆盖已有值）
     *
     * @param section 节名（空字符串表示全局节）
     * @param key 键名
     * @param value 值
     */
    public void set(String section, String key, String value) {
        data.computeIfAbsent(section, k -> new LinkedHashMap<>()).put(key, value);
    }

    /** 移除整个节 */
    public void removeSection(String section) {
        data.remove(section);
    }

    /** 移除节中的某个键 */
    public void removeKey(String section, String key) {
        Map<String, String> sec = data.get(section);
        if (sec != null) {
            sec.remove(key);
            if (sec.isEmpty()) data.remove(section);
        }
    }

    /** 检查节是否存在 */
    public boolean hasSection(String section) {
        return data.containsKey(section);
    }

    /** 检查节中的键是否存在 */
    public boolean hasKey(String section, String key) {
        Map<String, String> sec = data.get(section);
        return sec != null && sec.containsKey(key);
    }

    /** 获取所有节名（包括空字符串的全局节） */
    public Set<String> sections() {
        return data.keySet();
    }

    /** 获取节中所有键名 */
    public Set<String> keys(String section) {
        Map<String, String> sec = data.get(section);
        return sec == null ? Collections.emptySet() : sec.keySet();
    }

    /** 清空所有数据 */
    public void clear() {
        data.clear();
    }
}