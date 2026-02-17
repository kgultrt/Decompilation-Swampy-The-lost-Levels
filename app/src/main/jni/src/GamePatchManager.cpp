// GamePatchManager.cpp
#include "Hook/GamePatchManager.h"

GamePatchManager::GamePatchManager(const std::string& saveDir)
    : saveDir(saveDir) {
    updateModuleInfo();
}

void GamePatchManager::setSaveDir(const std::string& dir) {
    saveDir = dir;
}

void GamePatchManager::updateModuleInfo() {
    moduleInfo = getModuleInfo("libwmw.so");
    baseAddress = moduleInfo.base;
    LOGE("模块 libwmw.so 范围: 0x%lx-0x%lx", moduleInfo.base, moduleInfo.end);
}

uintptr_t GamePatchManager::getBaseAddress() const {
    return baseAddress;
}

GamePatchManager::ModuleInfo GamePatchManager::getModuleInfo(const char* moduleName) const {
    ModuleInfo info = {0, 0};
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        LOGE("打开 /proc/self/maps 失败: %s", strerror(errno));
        return info;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, moduleName)) {
            uintptr_t start, end;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                info = {start, end};
                break;
            }
        }
    }
    fclose(fp);
    return info;
}

uint32_t GamePatchManager::getCollisionCount() const {
    return hookGame.GetCollisionCount();
}

void GamePatchManager::resetCollisionCounter() {
    hookGame.ResetCollisionCounter();
}

bool GamePatchManager::applyAtoB() {
    return hookGame.AtoB_Hook();
}

bool GamePatchManager::applyBtoA() {
    return hookGame.BtoA_Hook();
}

bool GamePatchManager::toggleInstantWin() {
    bool success = patchMgr.TogglePatch(patchMgr.isGameWon1, baseAddress) &&
                   patchMgr.TogglePatch(patchMgr.isGameWon2, baseAddress);
    return success;
}

void GamePatchManager::applyPatches() {
    if (!isGamePatched) {
        hookGame.initGamePatch();
        isGamePatched = true;
    }
}