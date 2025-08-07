#include "HookGame.h"
#include <time.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

void HookGame::initGamePatch() {
    patchMgr.TogglePatch(patchMgr.CollectionRestriction, base);
    patchMgr.TogglePatch(patchMgr.CollectionBonusLevelRestriction, base);
    patchMgr.TogglePatch(patchMgr.CollectionBonusLevelRestriction1, base);
    patchMgr.TogglePatch(patchMgr.CollectionBonusLevelRestriction2, base);
    patchMgr.TogglePatch(patchMgr.CollectionBonusLevelRestriction3, base);
    patchMgr.TogglePatch(patchMgr.FixFRANKENButton, base);
    patchMgr.TogglePatch(patchMgr.TEST_HOOK, base);
}

bool HookGame::AtoB_Hook() {
    bool success = patchMgr.TogglePatch(patchMgr.AtoB, base);
    
    return success;
}

bool HookGame::BtoA_Hook() {
    bool success = patchMgr.TogglePatch(patchMgr.BtoA, base);
    
    return success;
}

uintptr_t HookGame::getBaseAddress(const char* moduleName) {
    uintptr_t baseAddress = 0;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        LOGE("打开 /proc/self/maps 失败\n");
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, moduleName)) {
            uintptr_t startAddr = 0;
            sscanf(line, "%lx-%*lx %*s %*s %*s %*d", &startAddr);
            baseAddress = startAddr;
            break;
        }
    }
    fclose(fp);

    if (baseAddress == 0) {
        LOGE("未找到模块 %s 的基址\n", moduleName);
    } else {
        LOGE("模块 %s 基址: 0x%lx\n", moduleName, baseAddress);
    }

    return baseAddress;
}