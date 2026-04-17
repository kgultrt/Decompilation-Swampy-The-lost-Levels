// ini.h
// 极简INI解析器 - 单头文件，开箱即用
// 支持节(Section)、键值对、注释(; 或 #)、全局节(空字符串节名)
// 提供字符串、整数、布尔值、浮点数的读取与写入

#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// 去除字符串前导空白
static inline std::string ltrim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start);
}

// 去除字符串尾部空白
static inline std::string rtrim(const std::string& s) {
    size_t end = s.find_last_not_of(" \t\r\n");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

// 去除字符串两端空白
static inline std::string trim(const std::string& s) {
    return rtrim(ltrim(s));
}

class Ini {
public:
    using ValueMap = std::unordered_map<std::string, std::string>;
    using SectionMap = std::unordered_map<std::string, ValueMap>;

    // 从文件加载INI配置
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        return load(file);
    }

    // 从输入流加载INI配置
    bool load(std::istream& is) {
        data_.clear();
        // 跳过可能的UTF-8 BOM头
        if (is.peek() == '\xEF') {
            char bom[3];
            is.read(bom, 3);
            if (bom[0] != '\xEF' || bom[1] != '\xBB' || bom[2] != '\xBF') {
                // 不是有效BOM，回退
                is.clear();
                is.seekg(0);
            }
        }
        std::string line;
        std::string current_section;
        while (std::getline(is, line)) {
            std::string trimmed = trim(line);
            // 跳过空行和注释行
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#')
                continue;

            // 处理节：[section]
            if (trimmed[0] == '[') {
                auto end = trimmed.find(']');
                if (end != std::string::npos) {
                    current_section = trim(trimmed.substr(1, end - 1));
                }
                continue;
            }

            // 处理键值对
            size_t delim_pos = trimmed.find('=');
            if (delim_pos == std::string::npos) {
                delim_pos = trimmed.find(':');
            }
            if (delim_pos != std::string::npos) {
                std::string key = trim(trimmed.substr(0, delim_pos));
                std::string value = trim(trimmed.substr(delim_pos + 1));
                if (!key.empty()) {
                    data_[current_section][key] = value;
                }
            }
        }
        return true;
    }

    // 从字符串内容解析INI
    bool parse(const std::string& content) {
        std::istringstream iss(content);
        return load(iss);
    }

    // 保存到文件
    bool save(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        return save(file);
    }

    // 保存到输出流
    bool save(std::ostream& os) const {
        // 收集所有节名并排序（保证输出稳定）
        std::vector<std::string> sections_vec;
        for (const auto& pair : data_) {
            sections_vec.push_back(pair.first);
        }
        std::sort(sections_vec.begin(), sections_vec.end());

        // 先输出全局节（空字符串节名）
        auto global_it = data_.find("");
        if (global_it != data_.end() && !global_it->second.empty()) {
            std::vector<std::string> keys_vec;
            for (const auto& kv : global_it->second) keys_vec.push_back(kv.first);
            std::sort(keys_vec.begin(), keys_vec.end());
            for (const auto& key : keys_vec) {
                os << key << "=" << global_it->second.at(key) << "\n";
            }
            if (!sections_vec.empty() && sections_vec[0] != "") os << "\n";
        }

        // 输出其他节
        for (const auto& section : sections_vec) {
            if (section.empty()) continue;
            const auto& section_data = data_.at(section);
            if (section_data.empty()) continue;

            os << "[" << section << "]\n";
            std::vector<std::string> keys_vec;
            for (const auto& kv : section_data) keys_vec.push_back(kv.first);
            std::sort(keys_vec.begin(), keys_vec.end());
            for (const auto& key : keys_vec) {
                os << key << "=" << section_data.at(key) << "\n";
            }
            os << "\n";
        }
        return true;
    }

    // 获取字符串值（可指定默认值）
    std::string get(const std::string& section, const std::string& key,
                    const std::string& default_value = "") const {
        auto sec_it = data_.find(section);
        if (sec_it == data_.end()) return default_value;
        auto val_it = sec_it->second.find(key);
        if (val_it == sec_it->second.end()) return default_value;
        return val_it->second;
    }

    // 设置字符串值
    void set(const std::string& section, const std::string& key, const std::string& value) {
        data_[section][key] = value;
    }

    // 检查节是否存在
    bool hasSection(const std::string& section) const {
        return data_.find(section) != data_.end();
    }

    // 检查节中的键是否存在
    bool hasKey(const std::string& section, const std::string& key) const {
        auto sec_it = data_.find(section);
        if (sec_it == data_.end()) return false;
        return sec_it->second.find(key) != sec_it->second.end();
    }

    // 移除整个节
    void removeSection(const std::string& section) {
        data_.erase(section);
    }

    // 移除节中的某个键
    void removeKey(const std::string& section, const std::string& key) {
        auto sec_it = data_.find(section);
        if (sec_it != data_.end()) {
            sec_it->second.erase(key);
            if (sec_it->second.empty()) data_.erase(sec_it);
        }
    }

    // 获取所有节名（包括空字符串表示的全局节）
    std::vector<std::string> sections() const {
        std::vector<std::string> result;
        for (const auto& pair : data_) result.push_back(pair.first);
        return result;
    }

    // 获取节中所有键名
    std::vector<std::string> keys(const std::string& section) const {
        std::vector<std::string> result;
        auto sec_it = data_.find(section);
        if (sec_it != data_.end()) {
            for (const auto& kv : sec_it->second) result.push_back(kv.first);
        }
        return result;
    }

    // 清空所有数据
    void clear() {
        data_.clear();
    }

    // 辅助：获取整数值
    int getInt(const std::string& section, const std::string& key, int default_value = 0) const {
        std::string val = get(section, key);
        if (val.empty()) return default_value;
        try {
            return std::stoi(val);
        } catch (...) {
            return default_value;
        }
    }

    // 辅助：获取布尔值 (true: true/1/yes/on, 忽略大小写)
    bool getBool(const std::string& section, const std::string& key, bool default_value = false) const {
        std::string val = get(section, key);
        if (val.empty()) return default_value;
        std::string lower = val;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
            return true;
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
            return false;
        return default_value;
    }

    // 辅助：获取浮点数值
    double getDouble(const std::string& section, const std::string& key, double default_value = 0.0) const {
        std::string val = get(section, key);
        if (val.empty()) return default_value;
        try {
            return std::stod(val);
        } catch (...) {
            return default_value;
        }
    }

private:
    SectionMap data_;
};