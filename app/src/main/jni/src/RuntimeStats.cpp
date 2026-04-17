// RuntimeStats.cpp
#include "Hook/RuntimeStats.h"
#include "ini.h"   // 单头文件解析器

RuntimeStats::RuntimeStats(const std::string& saveDir) {
    iniFilePath = saveDir + "/time.ini";
    load();
}

void RuntimeStats::load() {
    Ini ini;
    if (!ini.load(iniFilePath)) {
        // 文件不存在或无法打开 -> 保持 totalRunTime = 0.0
        LOGE("运行时长文件不存在或无法打开，初始化为 0");
        return;
    }

    // 读取 TotalSeconds，默认 0.0
    totalRunTime = ini.getDouble("Runtime", "TotalSeconds", 0.0);
    LOGE("读取运行时长成功: %.2f 秒", totalRunTime);
}

void RuntimeStats::save() {
    Ini ini;

    // 如果原文件存在，先加载以保留其他可能的手动编辑内容（如注释）
    // 但为了简单，我们直接重建整个 Runtime 节
    ini.set("Runtime", "TotalSeconds", std::to_string(totalRunTime));

    if (!ini.save(iniFilePath)) {
        LOGE("保存运行时长文件失败: %s", iniFilePath.c_str());
    }
}

void RuntimeStats::setSaveDir(const std::string& dir) {
    iniFilePath = dir + "/time.ini";
}

void RuntimeStats::update(double currentTime) {
    if (lastSaveTime == 0.0) {
        lastSaveTime = currentTime;
        return;
    }
    double delta = currentTime - lastSaveTime;
    if (delta >= 1.0) {
        lastSaveTime = currentTime;
        totalRunTime += delta;
        save();
    }
}

void RuntimeStats::getElapsedTime(int& hours, int& minutes, int& seconds) const {
    int totalSec = static_cast<int>(totalRunTime);
    hours = totalSec / 3600;
    minutes = (totalSec % 3600) / 60;
    seconds = totalSec % 60;
}