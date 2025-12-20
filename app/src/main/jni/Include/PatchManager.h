#include "SubstrateHook.h"
#include "CydiaSubstrate.h"
#include <time.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <set>
#include <mutex>
#include <sys/sysconf.h>
#include <errno.h>
#include <dlfcn.h>

#define LOGE(fmt, ...) \
    do { \
        char buf[256]; \
        snprintf(buf, sizeof(buf), "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
        printf("%s\n", buf); \
    } while (0)

class PatchManager {
public:
    // ARM寄存器枚举
    enum ARMRegisters {
        A$r0 = 0, A$r1, A$r2, A$r3, A$r4, A$r5, A$r6, A$r7,
        A$r8, A$r9, A$r10, A$r11, A$r12, A$sp, A$lr, A$pc
    };

    // Thumb指令常量
    #define T$nop 0x46c0
    #define T$bx(rm) (0x4700 | (rm << 3))
    #define A$ldr_rd_$rn_im$(rd, rn, im) (0xe5900000 | (rn << 16) | (rd << 12) | (im & 0xfff))
    
    // 补丁结构体
    struct Patch {
        uintptr_t offset;
        bool applied = false;
        unsigned char original[16] = {0};
        unsigned char patchBytes[16] = {0};
        bool backed_up = false;
        size_t size = 4;
        const char* name = nullptr;
    };

    // 简化后的函数劫持结构体
    struct FunctionHook {
        uintptr_t baseAddress = 0;
        uintptr_t offset = 0;
        void* hookFunction = nullptr;
        void* originalFunction = nullptr;
        bool hooked = false;
        const char* name = nullptr;
    };

    // 构造函数
    PatchManager() : pageSize(sysconf(_SC_PAGESIZE)) {
        initCommonPatches();
    }
    
    ~PatchManager() {
        removeAllHooks();
    }
    
    // 补丁开关
    bool TogglePatch(Patch& p, uintptr_t base, const char* hexCode = nullptr) {
        std::lock_guard<std::mutex> lock(hookMutex);
        void* addr = reinterpret_cast<void*>(base + p.offset);
        
        if (hexCode && !ParseHexString(hexCode, p.patchBytes, p.size)) {
            LOGE("Invalid machine code: %s (Patch: %s)", hexCode, p.name ? p.name : "unnamed");
            return false;
        }
        
        if (!p.backed_up && !BackupOriginal(addr, p)) {
            LOGE("Failed to backup original instructions! (Patch: %s)", p.name ? p.name : "unnamed");
            return false;
        }
        
        bool result = p.applied ? Restore(addr, p) : Apply(addr, p);
        
        if (result && p.name) {
            LOGE("%s: %s", p.name, p.applied ? "Restored" : "Applied");
        }
        
        return result;
    }

    // 使用Substrate的函数劫持
    bool HookFunction(uintptr_t base, uintptr_t offset, void* hookFunc, FunctionHook& hook, const char* name = nullptr) {
        std::lock_guard<std::mutex> lock(hookMutex);
        
        if (hook.hooked) {
            LOGE("Function already hooked (Hook: %s)", hook.name ? hook.name : "unnamed");
            return false;
        }
        
        void* symbol = reinterpret_cast<void*>(base + offset);
        
        if (name) hook.name = name;
        
        LOGE("Hooking with Substrate: %s @ %p", 
             hook.name ? hook.name : "unnamed", symbol);
        
        // 使用Substrate进行hook
        void* original = nullptr;
        MSHookFunction(symbol, hookFunc, &original);
        
        hook.baseAddress = base;
        hook.offset = offset;
        hook.hookFunction = hookFunc;
        hook.originalFunction = original;
        hook.hooked = true;
        activeHooks.push_back(&hook);
        
        LOGE("Hook successful: %s -> %p (Original: %p)", 
             hook.name ? hook.name : "unnamed", hookFunc, original);
        return true;
    }
    
    // 使用Substrate移除劫持
    bool UnhookFunction(FunctionHook& hook) {
        std::lock_guard<std::mutex> lock(hookMutex);
        
        if (!hook.hooked) {
            LOGE("Function not hooked (Hook: %s)", hook.name ? hook.name : "unnamed");
            return false;
        }
        
        void* symbol = reinterpret_cast<void*>(hook.baseAddress + hook.offset);
        
        LOGE("Removing hook: %s @ %p", hook.name ? hook.name : "unnamed", symbol);
        
        // 使用Substrate恢复原函数
        void* dummy;
        MSHookFunction(symbol, hook.originalFunction, &dummy);
        
        hook.hooked = false;
        
        // 从活动列表中移除
        auto it = std::find(activeHooks.begin(), activeHooks.end(), &hook);
        if (it != activeHooks.end()) {
            activeHooks.erase(it);
        }
        
        LOGE("Hook removed: %s", hook.name ? hook.name : "unnamed");
        return true;
    }
    
    // 示例补丁
    Patch AtoB;
    Patch BtoA;
    Patch drawAlpha;
    Patch test;
    Patch CollectionRestriction;
    Patch CollectionBonusLevelRestriction;
    Patch CollectionBonusLevelRestriction1;
    Patch CollectionBonusLevelRestriction2;
    Patch CollectionBonusLevelRestriction3;
    Patch FixFRANKENButton;
    Patch TEST_HOOK;
    Patch isGameWon1;
    Patch isGameWon2;
    Patch XML_HOOK;
    
private:
    std::mutex hookMutex;
    const size_t pageSize;
    std::vector<FunctionHook*> activeHooks;
    
    void initCommonPatches() {
        // 初始化常用补丁
        initPatch(AtoB, "AtoB", 0x00479960, "1EFF2FE1");
        initPatch(BtoA, "BtoA", 0x00492490, "1EFF2FE1");
        initPatch(drawAlpha, "drawAlpha", 0x00479a48, "020051E3");
        initPatch(test, "test", 0x00479e20, "0210A0E3");
        initPatch(CollectionRestriction, "CollectionRestriction", 0x004e99a0, "3E0050E3");
        initPatch(CollectionBonusLevelRestriction, "CollectionBonusLevelRestriction", 0x004e9910, "140050E3");
        initPatch(CollectionBonusLevelRestriction1, "CollectionBonusLevelRestriction1", 0x004ea31c, "14200AE3", 4);
        initPatch(CollectionBonusLevelRestriction2, "CollectionBonusLevelRestriction2", 0x004ea328, "1400A0E3");
        initPatch(CollectionBonusLevelRestriction3, "CollectionBonusLevelRestriction3", 0x004ea33c, "1460C4E5");
        initPatch(FixFRANKENButton, "FixFRANKENButton", 0x0057c1a4, "1EFF2FE1");
        initPatch(TEST_HOOK, "TEST_HOOK", 0x0058e250, "0000A0E1");
        initPatch(isGameWon1, "isGameWon1", 0x0045317c, "0100A0E3");
        initPatch(isGameWon2, "isGameWon2", 0x00453180, "0000A0E1");
        initPatch(XML_HOOK, "XML_HOOK", 0x004835f8, "010050e3");
    }
    
    void removeAllHooks() {
        std::lock_guard<std::mutex> lock(hookMutex);
        for (auto hook : activeHooks) {
            UnhookFunction(*hook);
        }
        activeHooks.clear();
    }
    
    void initPatch(Patch& p, const char* name, uintptr_t offset, const char* hexCode, size_t size = 4) {
        p.offset = offset;
        p.size = size;
        p.name = name;
        if (!ParseHexString(hexCode, p.patchBytes, size)) {
            LOGE("Failed to initialize patch: %s (Hex: %s)", name, hexCode);
        }
    }
    
    bool ParseHexString(const char* hex, unsigned char* output, size_t size) {
        size_t len = strlen(hex);
        if (len != size * 2) {
            LOGE("Hex string length mismatch: expected %zu chars, got %zu", size * 2, len);
            return false;
        }
        
        for (size_t i = 0; i < size; ++i) {
            char byteStr[3] = {hex[i*2], hex[i*2+1], '\0'};
            if (!isxdigit(byteStr[0]) || !isxdigit(byteStr[1])) {
                LOGE("Invalid hex character: %s", byteStr);
                return false;
            }
            output[i] = static_cast<unsigned char>(strtoul(byteStr, nullptr, 16));
        }
        return true;
    }
    
    bool BackupOriginal(void* addr, Patch& p) {
        if (!SetMemoryProtection(addr, p.size, PROT_READ | PROT_WRITE | PROT_EXEC)) {
            return false;
        }
        
        memcpy(p.original, addr, p.size);
        ClearCache(addr, p.size);
        p.backed_up = true;
        return true;
    }
    
    bool Apply(void* addr, Patch& p) {
        if (!SetMemoryProtection(addr, p.size, PROT_READ | PROT_WRITE | PROT_EXEC)) {
            return false;
        }
        
        memcpy(addr, p.patchBytes, p.size);
        ClearCache(addr, p.size);
        p.applied = true;
        return true;
    }
    
    bool Restore(void* addr, Patch& p) {
        if (!SetMemoryProtection(addr, p.size, PROT_READ | PROT_WRITE | PROT_EXEC)) {
            return false;
        }
        
        memcpy(addr, p.original, p.size);
        ClearCache(addr, p.size);
        p.applied = false;
        return true;
    }
    
    bool SetMemoryProtection(void* addr, size_t size, int prot) {
        uintptr_t pageStart = reinterpret_cast<uintptr_t>(addr) & ~(pageSize - 1);
        size_t protectSize = ((reinterpret_cast<uintptr_t>(addr) + size - pageStart + pageSize - 1) / pageSize) * pageSize;
        
        if (mprotect(reinterpret_cast<void*>(pageStart), protectSize, prot) != 0) {
            LOGE("mprotect failed (Address: %p, Size: %zu): %s", addr, size, strerror(errno));
            return false;
        }
        return true;
    }
    
    void ClearCache(void* addr, size_t size) {
        __builtin___clear_cache(
            reinterpret_cast<char*>(addr),
            reinterpret_cast<char*>(addr) + size);
    }
};