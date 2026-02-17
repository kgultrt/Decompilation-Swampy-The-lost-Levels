// RuntimeStats.h
#ifndef RUNTIMESTATS_H
#define RUNTIMESTATS_H

#include "pch.h"
#include <string>
#include <chrono>

class RuntimeStats {
public:
    explicit RuntimeStats(const std::string& saveDir);
    ~RuntimeStats() = default;

    // 更新运行时间（需在主循环中每帧调用）
    void update(double currentTime);

    // 获取格式化的运行时间（时、分、秒）
    void getElapsedTime(int& hours, int& minutes, int& seconds) const;

    // 设置保存目录（用于文件路径）
    void setSaveDir(const std::string& dir);

private:
    double totalRunTime = 0.0;
    double lastSaveTime = 0.0;
    std::string runTimeFilePath;

    // 从文件加载已保存的运行时间
    void load();
    // 保存运行时间到文件
    void save();
};

#endif // RUNTIMESTATS_H