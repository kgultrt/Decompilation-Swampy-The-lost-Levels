#include <time.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <cctype>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <functional> // 用于std::function

class PatchManager {
public:
    // 常规补丁结构
    struct Patch {
        uintptr_t offset;
        bool applied = false;
        unsigned char original[4] = {0};  // 存储原始指令
        unsigned char patchBytes[4] = {0}; // 存储补丁指令
        bool backed_up = false;           // 标志是否已备份原始指令
    };

    // 函数劫持结构
    struct FunctionHook {
        uintptr_t targetOffset;      // 目标函数偏移
        uintptr_t baseAddress;        // 基地址
        bool hooked = false;          // 劫持状态
        unsigned char original[16];   // 原始指令备份
        unsigned char trampoline[32]; // 跳板指令
        size_t trampolineSize;        // 跳板大小
        void* hookFunction;           // 劫持函数地址
        void* returnTrampoline;       // 返回跳板地址
        bool backed_up = false;
    };

    // 补丁配置
    Patch AtoB {0x00479960, false, {0}, {0x1e, 0xff, 0x2f, 0xe1}}; // bx lr
    Patch BtoA {0x00492490, false, {0}, {0x1e, 0xff, 0x2f, 0xe1}}; // bx lr
    Patch drawAlpha {0x00479a48, false, {0}, {0x02, 0x00, 0x51, 0xe3}}; // cmp r1, 2
    Patch test {0x00479e20, false, {0}, {0x02, 0x10, 0xa0, 0xe3}}; // mov r1, 2 0210a0e3
    Patch CollectionRestriction {0x004e99a0, false, {0}, {0x3e, 0x00, 0x50, 0xe3}}; //3e0050e3
    Patch CollectionBonusLevelRestriction {0x004e9910, false, {0}, {0x14, 0x00, 0x50, 0xe3}}; //14 00 50 e3
    Patch CollectionBonusLevelRestriction1 {0x004ea31c, false, {0}, {0x14, 0x20, 0x0a, 0xe3}}; //14 20 a0 e3
    Patch CollectionBonusLevelRestriction2 {0x004ea328, false, {0}, {0x14, 0x00, 0xa0, 0xe3}}; //14 00 a0 e3
    Patch CollectionBonusLevelRestriction3 {0x004ea33c, false, {0}, {0x14, 0x60, 0xc4, 0xe5}}; //14 60 c4 e5
    Patch FixFRANKENButton {0x0057c1a4, false, {0}, {0x1e, 0xff, 0x2f, 0xe1}}; // bx lr
    Patch TEST_HOOK {0x0058e250, false, {0}, {0x00, 0x00, 0xa0, 0xe1}}; // nop 0000a0e1
    
    Patch isGameWon1 {0x0045317c, false, {0}, {0x01, 0x00, 0xa0, 0xe3}}; // 0100a0e3
    Patch isGameWon2 {0x00453180, false, {0}, {0x00, 0x00, 0xa0, 0xe1}}; // nop

    
    // 存储所有劫持
    std::vector<FunctionHook*> activeHooks;

    // 常规补丁切换
    bool TogglePatch(Patch& p, uintptr_t base, const char* hexCode = nullptr) {
        void* addr = reinterpret_cast<void*>(base + p.offset);
        
        // 处理自定义指令
        if (hexCode && !ParseHexString(hexCode, p.patchBytes)) {
            LOGE("无效的机器码: %s", hexCode);
            return false;
        }
        
        // 确保安全备份原始指令
        if (!p.backed_up && !BackupOriginal(addr, p)) {
            LOGE("备份原始指令失败!");
            return false;
        }
        
        return p.applied ? Restore(addr, p) : Apply(addr, p);
    }
    
    // 创建函数劫持
    FunctionHook* CreateFunctionHook(uintptr_t base, uintptr_t offset, void* hookFunc) {
        void* targetAddr = reinterpret_cast<void*>(base + offset);
        
        // 创建新劫持对象
        FunctionHook* hook = new FunctionHook;
        hook->targetOffset = offset;
        hook->baseAddress = base;
        hook->hookFunction = hookFunc;
        hook->returnTrampoline = nullptr;
        
        // 备份原始指令
        if (!BackupFunction(targetAddr, *hook)) {
            LOGE("函数劫持备份失败");
            delete hook;
            return nullptr;
        }
        
        // 创建跳板指令
        if (!CreateTrampoline(*hook)) {
            LOGE("跳板创建失败");
            delete hook;
            return nullptr;
        }
        
        // 应用劫持
        if (!ApplyHook(targetAddr, *hook)) {
            LOGE("应用劫持失败");
            delete hook;
            return nullptr;
        }
        
        activeHooks.push_back(hook);
        return hook;
    }
    
    // 删除函数劫持
    bool RemoveFunctionHook(FunctionHook* hook) {
        if (!hook || !hook->hooked) return false;
        
        void* targetAddr = reinterpret_cast<void*>(hook->baseAddress + hook->targetOffset);
        
        if (!RestoreFunction(targetAddr, *hook)) {
            LOGE("恢复函数失败");
            return false;
        }
        
        // 释放跳板内存
        if (hook->returnTrampoline) {
            FreeExecutableMemory(hook->returnTrampoline, hook->trampolineSize);
        }
        
        // 从活动列表移除
        auto it = std::find(activeHooks.begin(), activeHooks.end(), hook);
        if (it != activeHooks.end()) activeHooks.erase(it);
        
        delete hook;
        return true;
    }
    
private:
    // 解析十六进制字符串
    bool ParseHexString(const char* hex, unsigned char* output) {
        size_t len = strlen(hex);
        if (len != 8) {
            LOGE("机器码必须是8个字符 (4字节)");
            return false;
        }
        
        char byteStr[3] = {0};
        for (size_t i = 0; i < 4; ++i) {
            byteStr[0] = hex[i*2];
            byteStr[1] = hex[i*2+1];
            
            std::transform(byteStr, byteStr+2, byteStr, ::tolower);
            
            if (!isxdigit(byteStr[0]) || !isxdigit(byteStr[1])) {
                LOGE("无效的十六进制字符: %c%c", byteStr[0], byteStr[1]);
                return false;
            }
            
            output[i] = std::strtoul(byteStr, nullptr, 16);
        }
        return true;
    }
    
    // 备份原始指令
    bool BackupOriginal(void* addr, Patch& p) {
        if (!SetMemoryProtection(addr)) {
            LOGE("备份: 内存保护设置失败");
            return false;
        }
        
        memcpy(p.original, addr, 4);
        __builtin___clear_cache(reinterpret_cast<char*>(addr), 
                               reinterpret_cast<char*>(addr) + 4);
        
        // 简单验证
        bool allZeros = true;
        for (int i = 0; i < 4; ++i) {
            if (p.original[i] != 0) {
                allZeros = false;
                break;
            }
        }
        if (allZeros) {
            LOGE("警告：目标地址可能是未初始化内存!");
            return false;
        }
        
        p.backed_up = true;
        LOGE("原始指令备份成功: %02X %02X %02X %02X", 
             p.original[0], p.original[1], p.original[2], p.original[3]);
        return true;
    }
    
    // 应用补丁
    bool Apply(void* addr, Patch& p) {
        if (!SetMemoryProtection(addr)) return false;
        
        memcpy(addr, p.patchBytes, 4);
        __builtin___clear_cache(reinterpret_cast<char*>(addr), 
                               reinterpret_cast<char*>(addr) + 4);
        
        // 验证写入
        unsigned char verify[4];
        memcpy(verify, addr, 4);
        if (memcmp(p.patchBytes, verify, 4) != 0) {
            LOGE("应用失败: 写入验证不匹配");
            return false;
        }
        
        usleep(10000);  // 10ms等待
        p.applied = true;
        LOGE("补丁应用成功: %02X %02X %02X %02X", 
             p.patchBytes[0], p.patchBytes[1], p.patchBytes[2], p.patchBytes[3]);
        return true;
    }
    
    // 恢复原始指令
    bool Restore(void* addr, Patch& p) {
        if (!SetMemoryProtection(addr)) return false;
        
        usleep(50000);  // 50ms等待
        memcpy(addr, p.original, 4);
        __builtin___clear_cache(reinterpret_cast<char*>(addr), 
                               reinterpret_cast<char*>(addr) + 4);
        
        // 验证恢复
        unsigned char verify[4];
        memcpy(verify, addr, 4);
        if (memcmp(p.original, verify, 4) != 0) {
            LOGE("恢复失败: 验证不匹配!");
            return false;
        }
        
        p.applied = false;
        LOGE("原始指令恢复成功");
        return true;
    }
    
    // 设置内存保护
    bool SetMemoryProtection(void* addr) {
        return SetMemoryProtection(addr, 4);
    }
    
    // 设置内存保护（重载）
    bool SetMemoryProtection(void* addr, size_t size) {
        uintptr_t pageStart = (uintptr_t)addr & ~(getpagesize() - 1);
        size_t len = ((uintptr_t)addr + size - pageStart + getpagesize() - 1) / getpagesize() * getpagesize();
        if (mprotect((void*)pageStart, len, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            LOGE("mprotect失败: %s", strerror(errno));
            return false;
        }
        return true;
    }
    
    // 分配可执行内存
    void* AllocateExecutableMemory(size_t size) {
        void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED) {
            LOGE("分配可执行内存失败: %s", strerror(errno));
            return nullptr;
        }
        return mem;
    }
    
    // 释放可执行内存
    bool FreeExecutableMemory(void* addr, size_t size) {
        return munmap(addr, size) == 0;
    }
    
    // 备份函数指令
    bool BackupFunction(void* addr, FunctionHook& hook) {
        if (!SetMemoryProtection(addr, 16)) {
            LOGE("函数劫持: 内存保护设置失败");
            return false;
        }
        
        // 备份前16字节（可根据平台调整）
        memcpy(hook.original, addr, 16);
        __builtin___clear_cache(reinterpret_cast<char*>(addr),
                               reinterpret_cast<char*>(addr) + 16);
        
        hook.backed_up = true;
        return true;
    }
    
    // 创建跳板指令
    bool CreateTrampoline(FunctionHook& hook) {
        /**
         * 跳板结构：
         * 1. 保存原始指令
         * 2. 跳回原函数（原始指令之后）
         *
         * 为简化处理，实际实现应根据目标平台生成相应的跳转指令
         * 本例使用通用跳转模板
         */
        
        // 计算所需空间
        size_t trampolineSize = 16 + 8; // 16字节原始指令 + 8字节跳回指令
        
        // 分配内存
        hook.returnTrampoline = AllocateExecutableMemory(trampolineSize);
        if (!hook.returnTrampoline) return false;
        
        // 创建跳板
        uintptr_t returnAddr = hook.baseAddress + hook.targetOffset + 16; // 被覆盖代码的下一指令
        unsigned char* tramp = reinterpret_cast<unsigned char*>(hook.returnTrampoline);
        
        // 复制被覆盖的原始指令
        memcpy(tramp, hook.original, 16);
        
        // 添加跳回指令 (BLX)
        tramp[16] = 0x02; // BLX指令的机器码
        tramp[17] = 0xF0;
        tramp[18] = 0x21;
        tramp[19] = 0xE1;
        
        // 跳转地址（小端存储）
        uint32_t retAddr = static_cast<uint32_t>(returnAddr);
        tramp[20] = retAddr & 0xFF;
        tramp[21] = (retAddr >> 8) & 0xFF;
        tramp[22] = (retAddr >> 16) & 0xFF;
        tramp[23] = (retAddr >> 24) & 0xFF;
        
        __builtin___clear_cache(reinterpret_cast<char*>(tramp), 
                                reinterpret_cast<char*>(tramp) + trampolineSize);
        
        hook.trampolineSize = trampolineSize;
        return true;
    }
    
    // 应用函数劫持
    bool ApplyHook(void* addr, FunctionHook& hook) {
        if (!SetMemoryProtection(addr, 16)) return false;
        
        // 创建跳转指令到钩子函数
        unsigned char jumpCode[12];
        
        // 长跳转指令模板：LDR PC, [PC, #-4]
        jumpCode[0] = 0x04; 
        jumpCode[1] = 0xF0;
        jumpCode[2] = 0x1F;
        jumpCode[3] = 0xE5;
        
        // 跳转地址（小端）
        uint32_t hookAddr = reinterpret_cast<uint32_t>(hook.hookFunction);
        jumpCode[4] = hookAddr & 0xFF;
        jumpCode[5] = (hookAddr >> 8) & 0xFF;
        jumpCode[6] = (hookAddr >> 16) & 0xFF;
        jumpCode[7] = (hookAddr >> 24) & 0xFF;
        
        // 附加nop指令填充（根据实际需要）
        jumpCode[8] = 0x00; 
        jumpCode[9] = 0x00; 
        jumpCode[10] = 0xA0; 
        jumpCode[11] = 0xE1;
        
        // 应用劫持指令
        memcpy(addr, jumpCode, 12);
        __builtin___clear_cache(reinterpret_cast<char*>(addr),
                               reinterpret_cast<char*>(addr) + 12);
        
        hook.hooked = true;
        return true;
    }
    
    // 恢复函数
    bool RestoreFunction(void* addr, FunctionHook& hook) {
        if (!SetMemoryProtection(addr, 16)) return false;
        
        usleep(50000);  // 等待确保安全
        
        // 恢复原始指令
        memcpy(addr, hook.original, 16);
        __builtin___clear_cache(reinterpret_cast<char*>(addr),
                               reinterpret_cast<char*>(addr) + 16);
        
        hook.hooked = false;
        return true;
    }
};