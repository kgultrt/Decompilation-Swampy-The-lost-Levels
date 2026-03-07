// RuntimeStats.cpp
#include "Hook/RuntimeStats.h"

RuntimeStats::RuntimeStats(const std::string& saveDir) 
    : totalRunTime(0.0), lastSaveTime(0.0) {   // 确保成员变量初始化
    runTimeFilePath = saveDir + "/time.ini";
    load();
}

void RuntimeStats::load() {
    FILE* f = fopen(runTimeFilePath.c_str(), "r");
    if (f) {
        int matched = fscanf(f, "[Runtime]\nTotalSeconds=%lf", &totalRunTime);
        fclose(f);
        if (matched == 1) {
            LOGE("读取运行时长成功: %.2f 秒", totalRunTime);
        } else {
            totalRunTime = 0.0;
            LOGE("运行时长文件格式错误，重置为 0");
        }
    } else {
        totalRunTime = 0.0;
        LOGE("运行时长文件不存在，初始化为 0");
    }
}

void RuntimeStats::setSaveDir(const std::string& dir) {
    runTimeFilePath = dir + "/time.ini";
}

void RuntimeStats::save() {
    FILE* f = fopen(runTimeFilePath.c_str(), "w");
    if (f) {
        fprintf(f, "[Runtime]\nTotalSeconds=%.2f\n", totalRunTime);
        fclose(f);
    }
}

void RuntimeStats::update(double currentTime) {
    if (lastSaveTime == 0.0) {          // 首次调用，只记录起始时间，不累加
        lastSaveTime = currentTime;
        return;
    }
    double timeSinceLastSave = currentTime - lastSaveTime;
    if (timeSinceLastSave >= 1.0) {
        lastSaveTime = currentTime;
        totalRunTime += timeSinceLastSave;
        save();                         // 每秒保存一次，避免丢失
    }
}

void RuntimeStats::getElapsedTime(int& hours, int& minutes, int& seconds) const {
    int totalSec = static_cast<int>(totalRunTime);
    hours = totalSec / 3600;
    minutes = (totalSec % 3600) / 60;
    seconds = totalSec % 60;
}