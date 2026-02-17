// MemoryBrowser.h
#ifndef MEMORYBROWSER_H
#define MEMORYBROWSER_H

#include "pch.h"
#include <string>
#include <cstdint>

class MemoryBrowser {
public:
    struct MemoryRange {
        uintptr_t start;
        uintptr_t end;
    };

    explicit MemoryBrowser(const std::string& saveDir, MemoryRange safeRange);
    ~MemoryBrowser() = default;

    void draw();  // 绘制内存浏览器窗口
    void setSaveDir(const std::string& dir);
    void setInput(ImguiAndroidInput* input) { this->input = input; } // 可选

    // 保存当前模块内存到文件
    void saveModuleMemoryToFile();

private:
    MemoryRange safeRange;
    uintptr_t currentAddress = 0;
    uintptr_t previousAddresses[5] = {0};
    int historyIndex = -1;

    bool isSavingMemory = false;
    float saveProgress = 0.0f;
    std::string saveDir;

    ImguiAndroidInput* input = nullptr; // 可能不需要，但保留

    void pushHistory(uintptr_t addr);
    void initMemoryBrowser(); // 初始化当前地址
};

#endif // MEMORYBROWSER_H