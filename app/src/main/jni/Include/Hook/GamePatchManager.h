// GamePatchManager.h
#ifndef GAMEPATCHMANAGER_H
#define GAMEPATCHMANAGER_H

#include "pch.h"
#include "Hook/HookGame.h"

class GamePatchManager {
public:
    explicit GamePatchManager(const std::string& saveDir);
    ~GamePatchManager() = default;
    
    PatchManager patchMgr;

    // 获取模块基址
    uintptr_t getBaseAddress() const;

    // 获取模块范围
    struct ModuleInfo {
        uintptr_t base;
        uintptr_t end;
    };
    ModuleInfo getModuleInfo(const char* moduleName) const;

    // 碰撞计数
    uint32_t getCollisionCount() const;
    void resetCollisionCounter();

    // 补丁操作
    bool applyAtoB();
    bool applyBtoA();
    bool toggleInstantWin();

    // 是否已打补丁
    bool isPatched() const { return isGamePatched; }
    void applyPatches(); // 应用所有需要的补丁（如初始化时）

    // 设置保存目录（如果需要）
    void setSaveDir(const std::string& dir);

private:
    HookGame hookGame;
    bool isGamePatched = false;
    std::string saveDir;

    uintptr_t baseAddress;
    ModuleInfo moduleInfo;

    void updateModuleInfo();
};

#endif // GAMEPATCHMANAGER_H