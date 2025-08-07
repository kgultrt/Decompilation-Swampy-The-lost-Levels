#ifndef HOOKGAME_H
#define HOOKGAME_H

#include "pch.h"

class HookGame {
public:
    PatchManager patchMgr;
    
    void initGamePatch();
    bool AtoB_Hook();
    bool BtoA_Hook();

private:
    uintptr_t getBaseAddress(const char* moduleName);
    uintptr_t base = getBaseAddress("libwmw.so");
};

#endif