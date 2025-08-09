#include "HookGame.h"
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <unistd.h>

// static
std::atomic<uint32_t> HookGame::fluidCollisionCounter(0);
HookGame* HookGame::instance = nullptr;

static void (*original_loadLevelDone)(void* _this) = nullptr;

// hook函数，和原函数签名一样，this指针作为参数
void HookGame::hook_loadLevelDone(void* _this) {
    fluidCollisionCounter.fetch_add(1, std::memory_order_relaxed);
    printf("[hook_loadLevelDone] called");

    // 直接调用原函数
    if (original_loadLevelDone) {
        original_loadLevelDone(_this);  // 只传递this指针
    }
}

void HookGame::install_hook(uintptr_t base) {
    uintptr_t targetAddr = base + 0x00475f7c; // 目标函数偏移

    MSHookFunction(
        (void*)targetAddr,
        (void*)hook_loadLevelDone,
        (void**)&original_loadLevelDone
    );

    printf("[install_hook] hook loadLevelDone success at %p\n", (void*)targetAddr);
}


void HookGame::initGamePatch() {
    if (!base) {
        LOGE("基础地址无效，无法初始化补丁!");
        return;
    }

    // 你的其他补丁（保持不变）
    const std::initializer_list<std::reference_wrapper<PatchManager::Patch>> patches = {
        patchMgr.CollectionRestriction,
        patchMgr.CollectionBonusLevelRestriction,
        patchMgr.CollectionBonusLevelRestriction1,
        patchMgr.CollectionBonusLevelRestriction2,
        patchMgr.CollectionBonusLevelRestriction3,
        patchMgr.FixFRANKENButton,
        patchMgr.TEST_HOOK
    };

    for (auto& patch : patches) {
        patchMgr.TogglePatch(patch, base);
    }

    // 安装计数器
    install_hook(base);
}

// 你原来的 getBaseAddress 保留（略）
uintptr_t HookGame::getBaseAddress(const char* moduleName) {
    uintptr_t baseAddress = 0;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        LOGE("打开 /proc/self/maps 失败: %s", strerror(errno));
        return 0;
    }

    char line[512];
    bool found = false;

    while (fgets(line, sizeof(line), fp)) {
        char* modulePos = strstr(line, moduleName);
        if (modulePos) {
            bool isExecutable = strstr(line, "r-xp") || strstr(line, "rwxp");
            if (isExecutable) {
                uintptr_t start, end;
                if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                    baseAddress = start;
                    found = true;
                    break;
                }
            }
        }
    }
    fclose(fp);

    if (!found) {
        LOGE("未找到可执行模块 %s", moduleName);
    } else {
        LOGE("模块 %s 基址: 0x%lx", moduleName, baseAddress);
    }

    return baseAddress;
}

bool HookGame::AtoB_Hook() {
    return patchMgr.TogglePatch(patchMgr.AtoB, base);
}

bool HookGame::BtoA_Hook() {
    return patchMgr.TogglePatch(patchMgr.BtoA, base);
}