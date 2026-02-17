// MemoryBrowser.cpp
#include "Hook/MemoryBrowser.h"

MemoryBrowser::MemoryBrowser(const std::string& dir, MemoryRange range)
    : saveDir(dir), safeRange(range) {
    initMemoryBrowser();
}

void MemoryBrowser::setSaveDir(const std::string& dir) {
    saveDir = dir;
}

void MemoryBrowser::initMemoryBrowser() {
    currentAddress = safeRange.start;
    LOGE("内存浏览器范围: 0x%lx - 0x%lx", safeRange.start, safeRange.end);
}

void MemoryBrowser::pushHistory(uintptr_t addr) {
    historyIndex = (historyIndex + 1) % 5;
    previousAddresses[historyIndex] = addr;
}

void MemoryBrowser::draw() {
    const int bytesPerLine = 16;
    const int displayLines = 32;
    const int totalBytes = bytesPerLine * displayLines;

    ImGui::Begin("内存浏览器 (安全模式)");

    ImGui::Text("安全范围: 0x%lX - 0x%lX", safeRange.start, safeRange.end);

    // 导航按钮
    if (ImGui::Button("<< Prev Page")) {
        if (currentAddress > safeRange.start + totalBytes) {
            pushHistory(currentAddress);
            currentAddress -= totalBytes;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Next Page >>")) {
        if (currentAddress < safeRange.end - totalBytes) {
            pushHistory(currentAddress);
            currentAddress += totalBytes;
        }
    }
    ImGui::SameLine();
    if (historyIndex >= 0 && ImGui::Button("<< Back")) {
        currentAddress = previousAddresses[historyIndex];
        historyIndex = (historyIndex + 4) % 5;
    }
    ImGui::SameLine();
    if (ImGui::Button("Base Address")) {
        pushHistory(currentAddress);
        currentAddress = safeRange.start;
    }

    // 地址滑块
    uintptr_t maxValidAddress = safeRange.end - totalBytes;
    if (maxValidAddress < safeRange.start) maxValidAddress = safeRange.start;
    ImGui::SliderScalar("导航", ImGuiDataType_U64, &currentAddress,
                        &safeRange.start, &maxValidAddress, "0x%08lX");

    // 对齐与边界保护
    currentAddress = currentAddress & ~(bytesPerLine - 1);
    if (currentAddress < safeRange.start) currentAddress = safeRange.start;
    if (currentAddress > safeRange.end - totalBytes) currentAddress = maxValidAddress;

    // 读取内存
    uint8_t memoryBuffer[totalBytes];
    size_t validBytes = 0;
    uintptr_t endAddress = currentAddress + totalBytes;
    if (endAddress > safeRange.end) {
        validBytes = safeRange.end - currentAddress;
    } else {
        validBytes = totalBytes;
    }

    if (validBytes > 0) {
        memcpy(memoryBuffer, (void*)currentAddress, validBytes);
    }

    // 显示内存表格
    ImGui::BeginChild("内存视图", ImVec2(0, 800), true);
    ImGui::Columns(3, "memoryColumns");
    ImGui::SetColumnWidth(0, 200);
    ImGui::SetColumnWidth(1, 800);
    ImGui::SetColumnWidth(2, 250);

    ImGui::Text("地址"); ImGui::NextColumn();
    ImGui::Text("十六进制"); ImGui::NextColumn();
    ImGui::Text("ASCII"); ImGui::NextColumn();
    ImGui::Separator();

    for (int i = 0; i < validBytes; i += bytesPerLine) {
        // 地址
        ImGui::Text("%08lX", currentAddress + i); ImGui::NextColumn();

        // 十六进制
        std::string hexLine;
        for (int j = 0; j < bytesPerLine; ++j) {
            if (i + j < validBytes) {
                char byteHex[4];
                snprintf(byteHex, sizeof(byteHex), "%02X ", memoryBuffer[i + j]);
                hexLine += byteHex;
            } else {
                hexLine += "   ";
            }
        }
        ImGui::Text("%s", hexLine.c_str()); ImGui::NextColumn();

        // ASCII
        std::string asciiLine;
        for (int j = 0; j < bytesPerLine; ++j) {
            if (i + j < validBytes) {
                uint8_t c = memoryBuffer[i + j];
                asciiLine += (c >= 32 && c <= 126) ? static_cast<char>(c) : '.';
            } else {
                asciiLine += ' ';
            }
        }
        ImGui::Text("%s", asciiLine.c_str()); ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Text("当前地址: 0x%lX", currentAddress);

    // 保存按钮
    if (!isSavingMemory && ImGui::Button("保存模块内存到文件")) {
        saveModuleMemoryToFile();
    }
    if (isSavingMemory) {
        ImGui::SameLine();
        ImGui::Text("保存中... %.1f%%", saveProgress * 100);
    }

    ImGui::End();
}

void MemoryBrowser::saveModuleMemoryToFile() {
    if (isSavingMemory) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char filename[128];
    std::strftime(filename, sizeof(filename), "libwmw_%Y%m%d_%H%M%S.bin", std::localtime(&time));
    std::string fullPath = saveDir + "/" + filename;

    saveProgress = 0.0f;
    isSavingMemory = true;

    std::thread([this, fullPath] {
        FILE* file = fopen(fullPath.c_str(), "wb");
        if (!file) {
            LOGE("无法创建文件: %s", fullPath.c_str());
            isSavingMemory = false;
            return;
        }

        const size_t bufferSize = 4096;
        uint8_t buffer[bufferSize];
        size_t totalSize = safeRange.end - safeRange.start;
        size_t bytesSaved = 0;

        for (uintptr_t addr = safeRange.start; addr < safeRange.end; addr += bufferSize) {
            // 可以添加 isDestroy 检查，但需要从外部传入状态，暂不处理
            size_t currentSize = (addr + bufferSize <= safeRange.end) ? bufferSize : safeRange.end - addr;
            memcpy(buffer, (void*)addr, currentSize);
            fwrite(buffer, 1, currentSize, file);
            bytesSaved += currentSize;
            saveProgress = static_cast<float>(bytesSaved) / totalSize;
        }

        fclose(file);
        LOGE("内存保存完成: %s (%.2f MB)", fullPath.c_str(), bytesSaved / (1024.0f * 1024.0f));
        isSavingMemory = false;
    }).detach();
}