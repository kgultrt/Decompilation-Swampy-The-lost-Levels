#ifndef IMGUIMENU_IMGUIEGL_H
#define IMGUIMENU_IMGUIEGL_H

#include "pch.h"
#include "PatchManager.h"

class EGL {
    std::condition_variable cond;
    std::mutex              Threadlk;

    FILE                    *numSave       = nullptr;
    FILE                    *ColorSave     = nullptr;
    FILE                    *KeySave       = nullptr;
    EGLDisplay              mEglDisplay;
    EGLSurface              mEglSurface;
    EGLConfig               mEglConfig;
    EGLContext              mEglContext;
    EGLNativeWindowType     SurfaceWin;
    std::thread             *SurfaceThread = nullptr;
    ImguiAndroidInput       *input;
    int                     FPS = 90;

    int surfaceWidthHalf = 0;
    int surfaceHighHalf  = 0;
    int StatusBarHeight  = 0;
    int initEgl();
    void clearBuffers();
    int swapBuffers();
    bool ThreadIo;
    void EglThread();
    void Dialog(int type);
    bool isStyle = false;
    int initImgui();
    void imguiMainWinStart();
    void imguiMainWinEnd();
    void drawPatchMonitorWindow(void* targetFunc);
    void initMemoryBrowser();
    void pushHistory(uintptr_t address);
    void drawMemoryBrowser();
    
    bool patch(void* addr, const unsigned char* patchBytes, size_t size);
    
    ImFont *imFont;
    std::string SaveDir;

    // 运行时间追踪
    double getCurrentTime();
    double appStartTime = 0.0;
    double totalRunTime = 0.0;
    double lastSaveTime = 0.0;
    std::string runTimeFilePath;
    
    uintptr_t getModuleBaseAddress(const char* moduleName);
    
    PatchManager patchMgr;
    
    struct MemoryRange {
        uintptr_t start;
        uintptr_t end;
    };
    MemoryRange safeRange;
    
    struct ModuleInfo {
        uintptr_t base;
        uintptr_t end;
    };
    ModuleInfo getModuleInfo(const char* moduleName);
    
    uintptr_t currentAddress = 0;
    uintptr_t previousAddresses[5] = {0}; // 历史地址栈
    int historyIndex = -1;

public:
    ImGuiIO      *io;
    ImGuiStyle   *style;
    ImGuiWindow  *g_window = nullptr;
    ImGuiContext *g        = nullptr;
    int surfaceWidth       = 0;
    int surfaceHigh        = 0;
    bool ActivityState     = true;

    EGL();

    void onSurfaceCreate(JNIEnv *env, jobject surface, int SurfaceWidth, int SurfaceHigh);
    bool isChage = false;
    void onSurfaceChange(int surfaceWidth, int SurfaceHigh);
    bool isDestroy = false;
    void onSurfaceDestroy();
    void ShowStyleEditor(ImGuiStyle* ref);
    void setSaveSettingsdir(std::string &dir);
    void setinput(ImguiAndroidInput *input_);
    
    bool isGamePatched = false;

    ~EGL() {}
};
#endif