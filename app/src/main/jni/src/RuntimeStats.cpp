// RuntimeStats.cpp
#include "Hook/RuntimeStats.h"

RuntimeStats::RuntimeStats(const std::string& saveDir) {
    runTimeFilePath = saveDir + "/time.ini";
    load();
    lastSaveTime = 0.0; // 注意：getCurrentTime 需要定义，或者改用外部传入
}

void RuntimeStats::setSaveDir(const std::string& dir) {
    runTimeFilePath = dir + "/runtime.ini";
}

void RuntimeStats::load() {
    FILE* f = fopen(runTimeFilePath.c_str(), "r");
    if (f) {
        fscanf(f, "[Runtime]\nTotalSeconds=%lf", &totalRunTime);
        fclose(f);
        LOGE("读取运行时长成功: %.2f 秒", totalRunTime);
    } else {
        totalRunTime = 0.0;
        LOGE("运行时长文件不存在，初始化为 0");
    }
}

void RuntimeStats::save() {
    FILE* f = fopen(runTimeFilePath.c_str(), "w");
    if (f) {
        fprintf(f, "[Runtime]\nTotalSeconds=%.2f\n", totalRunTime);
        fclose(f);
    }
}

void RuntimeStats::update(double currentTime) {
    double timeSinceLastSave = currentTime - lastSaveTime;
    if (timeSinceLastSave >= 1.0) {
        lastSaveTime = currentTime;
        totalRunTime += timeSinceLastSave;
        save();
    }
}

void RuntimeStats::getElapsedTime(int& hours, int& minutes, int& seconds) const {
    int totalSec = static_cast<int>(totalRunTime);
    hours = totalSec / 3600;
    minutes = (totalSec % 3600) / 60;
    seconds = totalSec % 60;
}