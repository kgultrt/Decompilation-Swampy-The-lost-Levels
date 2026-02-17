#ifndef IMGUIMENU_IMGUIEGL_H
#define IMGUIMENU_IMGUIEGL_H

#include "pch.h"
#include "Hook/RuntimeStats.h"
#include "Hook/GamePatchManager.h"
#include "Hook/HookGame.h"
#include "Hook/MemoryBrowser.h"

class EGL {
    std::condition_variable cond;
    std::mutex              Threadlk;

    EGLDisplay              mEglDisplay;
    EGLSurface              mEglSurface;
    EGLConfig               mEglConfig;
    EGLContext              mEglContext;
    EGLNativeWindowType     SurfaceWin;
    std::thread*            SurfaceThread = nullptr;
    ImguiAndroidInput*      input = nullptr;
    int                     FPS = 90;

    int surfaceWidthHalf = 0;
    int surfaceHighHalf  = 0;
    int StatusBarHeight  = 0;
    bool ThreadIo = false;
    bool isStyle = false;

    ImFont* imFont;
    std::string SaveDir;

    // 功能模块
    std::unique_ptr<RuntimeStats> runtimeStats;
    std::unique_ptr<GamePatchManager> gamePatch;
    std::unique_ptr<MemoryBrowser> memoryBrowser;
    
    PatchManager patchMgr;
    HookGame hookGame;

    // EGL/OpenGL 辅助
    int initEgl();
    void clearBuffers();
    int swapBuffers();

    // ImGui 辅助
    int initImgui();
    void setupImGuiContext();
    void loadImGuiStyle();
    void imguiMainWinStart();
    void imguiMainWinEnd();

    // 渲染线程主循环
    void EglThread();
    void mainRenderLoop();
    void renderImGuiWindow();

public:
    ImGuiIO*      io = nullptr;
    ImGuiStyle*   style = nullptr;
    ImGuiWindow*  g_window = nullptr;
    ImGuiContext* g = nullptr;
    int           surfaceWidth = 0;
    int           surfaceHigh = 0;
    bool          ActivityState = true;
    bool          isChage = false;
    bool          isDestroy = false;

    EGL();
    ~EGL() = default;

    // Surface 生命周期（JNI 调用）
    void onSurfaceCreate(JNIEnv* env, jobject surface, int SurfaceWidth, int SurfaceHigh);
    void onSurfaceChange(int surfaceWidth, int SurfaceHigh);
    void onSurfaceDestroy();

    // 设置
    void setSaveSettingsdir(std::string& dir);
    void setinput(ImguiAndroidInput* input_);

    // 保留旧版本中的声明（空实现）
    void ShowStyleEditor(ImGuiStyle* ref) {}
    void Dialog(int type) {}
};

#endif // IMGUIMENU_IMGUIEGL_H