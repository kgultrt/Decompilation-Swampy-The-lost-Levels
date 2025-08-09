#ifndef HOOKGAME_H
#define HOOKGAME_H

#include "pch.h"
#include <atomic>

class HookGame {
public:
    PatchManager patchMgr;
    
    void initGamePatch();
    bool AtoB_Hook();
    bool BtoA_Hook();
    
    // 获取AtoB碰撞回调计数
    uint32_t GetCollisionCount() const { 
        return fluidCollisionCounter.load(std::memory_order_relaxed); 
    }
    
    // 重置计数器
    void ResetCollisionCounter() { 
        fluidCollisionCounter = 0; 
    }

private:
    uintptr_t getBaseAddress(const char* moduleName);
    uintptr_t base = getBaseAddress("libwmw.so");
    
    // 静态函数原型和钩子变量
    static void HookedFluidCollisionCallback(void* arg);
    PatchManager::FunctionHook fluidCollisionHook;
    
    // 原子计数器确保线程安全
    static std::atomic<uint32_t> fluidCollisionCounter;
    
    static void hook_loadLevelDone(void* _this);
    void install_hook(uintptr_t base);
    
    static HookGame* instance;
};

#endif