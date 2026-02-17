#include "EGL.h"
#include "ImGuiUtils.h"

// 静态变量
static bool RunInitImgui = false;

// 工具函数：获取当前时间（秒）
double getEGLCurrentTime() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

// ========================= 构造与初始化 =========================
EGL::EGL()
    : mEglDisplay(EGL_NO_DISPLAY)
    , mEglSurface(EGL_NO_SURFACE)
    , mEglConfig(nullptr)
    , mEglContext(EGL_NO_CONTEXT)
    , SurfaceWin(nullptr)
    , SurfaceThread(nullptr)
    , input(nullptr)
    , FPS(90)
    , surfaceWidthHalf(0)
    , surfaceHighHalf(0)
    , StatusBarHeight(0)
    , ThreadIo(false)
    , isStyle(false)
    , imFont(nullptr)
    , SaveDir("/sdcard/WMW-MOD")
{}

int EGL::initEgl() {
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay error=%u", glGetError());
        return -1;
    }

    EGLint version[2];
    if (!eglInitialize(mEglDisplay, &version[0], &version[1])) {
        LOGE("eglInitialize error=%u", glGetError());
        return -1;
    }

    const EGLint attribs[] = {
        EGL_BUFFER_SIZE, 32,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLint num_config;
    if (!eglGetConfigs(mEglDisplay, nullptr, 1, &num_config)) {
        LOGE("eglGetConfigs error=%u", glGetError());
        return -1;
    }

    if (!eglChooseConfig(mEglDisplay, attribs, &mEglConfig, 1, &num_config)) {
        LOGE("eglChooseConfig error=%u", glGetError());
        return -1;
    }

    int attrib_list[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    mEglContext = eglCreateContext(mEglDisplay, mEglConfig, EGL_NO_CONTEXT, attrib_list);
    if (mEglContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext error=%u", glGetError());
        return -1;
    }

    mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, SurfaceWin, nullptr);
    if (mEglSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface error=%u", glGetError());
        return -1;
    }

    if (!eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
        LOGE("eglMakeCurrent error=%u", glGetError());
        return -1;
    }

    return 1;
}

void EGL::setupImGuiContext() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    ImGui::SetupImGuiStyle(/*bStyleDark_*/ true, /*alpha_*/ 0.5f);
    io->IniSavingRate = 10.0f;
    std::string SaveFile = SaveDir + "/save.ini";
    io->IniFilename = SaveFile.c_str();

    // 设置字体（字体数据来自 pch.h 中的 OPPOSans_H）
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    imFont = io->Fonts->AddFontFromMemoryTTF((void*)OPPOSans_H, OPPOSans_H_size, 32.0f,
                                             &font_cfg, io->Fonts->GetGlyphRangesChineseFull());
    io->MouseDoubleClickTime = 0.0001f;

    g = ImGui::GetCurrentContext();
    style = &ImGui::GetStyle();
    style->ScaleAllSizes(1.0f);
    style->FramePadding = ImVec2(10.0f, 20.0f);
}

void EGL::loadImGuiStyle() {
    std::string LoadFile = SaveDir + "/Style.dat";
    ImGuiStyle s;
    if (MyFile::ReadFile(&s, LoadFile.c_str()) == 1) {
        *style = s;
        LOGE("读取主题成功");
    }
}

int EGL::initImgui() {
    if (!RunInitImgui) {
        RunInitImgui = true;
        setupImGuiContext();
        LOGE("CreateContext成功");
    } else {
        // 已经初始化过，只需要关联上下文
        ImGui::SetCurrentContext(g);
    }

    ImGui_ImplAndroid_Init(SurfaceWin);
    LOGE("ImGui_ImplAndroid_Init成功");
    ImGui_ImplOpenGL3_Init("#version 300 es");
    LOGE("ImGui_ImplOpenGL3_Init成功");

    loadImGuiStyle();
    return 1;
}

// ========================= 生命周期管理 =========================
void EGL::onSurfaceCreate(JNIEnv* env, jobject surface, int SurfaceWidth, int SurfaceHigh) {
    LOGE("onSurfaceCreate");
    SurfaceWin = ANativeWindow_fromSurface(env, surface);
    surfaceWidth = SurfaceWidth;
    surfaceHigh = SurfaceHigh;
    surfaceWidthHalf = SurfaceWidth / 2;
    surfaceHighHalf = SurfaceHigh / 2;

    SurfaceThread = new std::thread([this] { EglThread(); });
    SurfaceThread->detach();
    LOGE("onSurfaceCreate_end");
}

void EGL::onSurfaceChange(int SurfaceWidth, int SurfaceHigh) {
    surfaceWidth = SurfaceWidth;
    surfaceHigh = SurfaceHigh;
    surfaceWidthHalf = SurfaceWidth / 2;
    surfaceHighHalf = SurfaceHigh / 2;
    isChage = true;
    LOGE("onSurfaceChange");
}

void EGL::onSurfaceDestroy() {
    LOGE("onSurfaceDestroy");
    isDestroy = true;

    std::unique_lock<std::mutex> ulo(Threadlk);
    cond.wait(ulo, [this] { return !ThreadIo; });
    delete SurfaceThread;
    SurfaceThread = nullptr;
}

// ========================= EGL 渲染线程 =========================
void EGL::EglThread() {
    if (initEgl() != 1) return;
    if (initImgui() != 1) return;

    ThreadIo = true;
    input->initImguiIo(io);
    input->setImguiContext(g);
    input->setwin(g_window);

    // 初始化功能模块
    runtimeStats = std::make_unique<RuntimeStats>(SaveDir);
    gamePatch = std::make_unique<GamePatchManager>(SaveDir);
    memoryBrowser = std::make_unique<MemoryBrowser>(SaveDir,
        MemoryBrowser::MemoryRange{gamePatch->getModuleInfo("libwmw.so").base,
                                   gamePatch->getModuleInfo("libwmw.so").end});

    // 应用游戏补丁（如果需要）
    if (!gamePatch->isPatched()) {
        gamePatch->applyPatches();
    }

    mainRenderLoop();  // 主渲染循环
}

void EGL::mainRenderLoop() {
    static bool showMemoryBrowser = false;

    while (true) {
        if (isChage) {
            glViewport(0, 0, surfaceWidth, surfaceHigh);
            isChage = false;
        }

        if (isDestroy) {
            isDestroy = false;
            ThreadIo = false;
            cond.notify_all();
            return;
        }

        clearBuffers();
        if (!ActivityState) continue;

        // 更新运行时统计
        double currentTime = getEGLCurrentTime();
        if (runtimeStats) {
            runtimeStats->update(currentTime);
        }

        renderImGuiWindow();

        swapBuffers();
    }
}

void EGL::renderImGuiWindow() {
    imguiMainWinStart();

    // 内存浏览器开关
    static bool showMemoryBrowser = false;

    ImGui::SetNextWindowBgAlpha(0.7f);
    style->WindowTitleAlign = ImVec2(0.5f, 0.5f);

    if (input->Scrollio && !input->Activeio) {
        input->funScroll(g->WheelingWindow ? g->WheelingWindow : g->HoveredWindow);
    }

    ImGui::Begin("WMW Mod Tool v1.0");

    input->g_window = g_window = ImGui::GetCurrentWindow();
    ImGui::SetWindowPos({0, 200}, ImGuiCond_FirstUseEver);

    // 运行时信息
    if (runtimeStats) {
        int h, m, s;
        runtimeStats->getElapsedTime(h, m, s);
        ImGui::Text("累计运行时长: %02d:%02d:%02d", h, m, s);
    }

    // 游戏补丁信息
    if (gamePatch) {
        uintptr_t base = gamePatch->getBaseAddress();
        ImGui::Text("目标库基址 (动态): 0x%lx", base);
        ImGui::Text("LevelDone调用次数: %u", gamePatch->getCollisionCount());

        if (ImGui::Button("重置碰撞计数器")) {
            gamePatch->resetCollisionCounter();
            LOGE("流体碰撞计数器已重置");
        }
        ImGui::SameLine();
        if (ImGui::Button("退出游戏(exit)")) {
            exit(0);
        }

        if (ImGui::Button(hookGame.patchMgr.AtoB.applied ? "还原AtoB" : "应用AtoB")) {
            bool success = hookGame.AtoB_Hook();
            LOGE("%s %s", hookGame.patchMgr.AtoB.applied ? "应用" : "还原",
                 success ? "成功" : "失败");
        }
        if (ImGui::Button(hookGame.patchMgr.BtoA.applied ? "还原BtoA" : "应用BtoA")) {
            bool success = hookGame.BtoA_Hook();
            LOGE("%s %s", hookGame.patchMgr.BtoA.applied ? "应用" : "还原",
                 success ? "成功" : "失败");
        }
        if (ImGui::Button(hookGame.patchMgr.stopWater.applied ? "还原stopWater" : "应用stopWater")) {
            bool success = hookGame.stopWater_Hook();
            LOGE("%s %s", hookGame.patchMgr.stopWater.applied ? "应用" : "还原",
                 success ? "成功" : "失败");
        }
        if (ImGui::Button(hookGame.patchMgr.hookWater.applied ? "还原500水上限 (常量, 无用)" : "应用100水上限 (常量, 无用)")) {
            bool success = hookGame.hookWater_Hook();
            LOGE("%s %s", hookGame.patchMgr.hookWater.applied ? "应用" : "还原",
                 success ? "成功" : "失败");
        }
        if (ImGui::Button(patchMgr.isGameWon1.applied ? "直接胜利 开" : "直接胜利 关")) {
            bool success = patchMgr.TogglePatch(patchMgr.isGameWon1, base) &&
                           patchMgr.TogglePatch(patchMgr.isGameWon2, base);
            LOGE("%s %s", patchMgr.isGameWon1.applied ? "应用" : "还原",
                 success ? "成功" : "失败");
        }

        if (ImGui::Button(showMemoryBrowser ? "关闭内存查看器 (Beta)" : "打开内存查看器 (Beta)")) {
            showMemoryBrowser = !showMemoryBrowser;
        }
    }

    ImGui::End();

    // 绘制内存浏览器（独立窗口）
    if (showMemoryBrowser && memoryBrowser) {
        memoryBrowser->draw();
    }

    imguiMainWinEnd();
}

// ========================= OpenGL 辅助 =========================
void EGL::clearBuffers() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int EGL::swapBuffers() {
    if (eglSwapBuffers(mEglDisplay, mEglSurface)) {
        return 1;
    }
    LOGE("eglSwapBuffers error=%u", glGetError());
    return 0;
}

void EGL::imguiMainWinStart() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
}

void EGL::imguiMainWinEnd() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ========================= 设置接口 =========================
void EGL::setSaveSettingsdir(std::string& dir) {
    SaveDir = dir;
    LOGE("保存路径=%s", SaveDir.c_str());
}

void EGL::setinput(ImguiAndroidInput* input_) {
    input = input_;
}