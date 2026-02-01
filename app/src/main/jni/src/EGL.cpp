#include "EGL.h"
#include "Imgui/imgui.h"
#include <time.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

static bool RunInitImgui = false;

double EGL::getCurrentTime() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

EGL::EGL() {
    mEglDisplay = EGL_NO_DISPLAY;
    mEglSurface = EGL_NO_SURFACE;
    mEglConfig  = nullptr;
    mEglContext = EGL_NO_CONTEXT;
    
    SaveDir = "/sdcard/Android/data/com.decompilationpixel.WMW";
}

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
        EGL_BUFFER_SIZE, 32, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 8,
        EGL_STENCIL_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE
    };

    EGLint num_config;
    if (!eglGetConfigs(mEglDisplay, NULL, 1, &num_config)) {
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

    mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, SurfaceWin, NULL);
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

int EGL::initImgui() {
    if (RunInitImgui) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplAndroid_Init(this->SurfaceWin);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        return 1;
    }

    RunInitImgui = true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    LOGE("CreateContext成功");
    io = &ImGui::GetIO();
    io->IniSavingRate = 10.0f;
    std::string SaveFile = this->SaveDir + "/save.ini";
    io->IniFilename = SaveFile.c_str();

    ImGui_ImplAndroid_Init(this->SurfaceWin);
    LOGE("ImGui_ImplAndroid_Init成功");
    ImGui_ImplOpenGL3_Init("#version 300 es");
    LOGE("ImGui_ImplOpenGL3_Init成功");

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    imFont = io->Fonts->AddFontFromMemoryTTF((void *)OPPOSans_H, OPPOSans_H_size, 32.0f, &font_cfg, io->Fonts->GetGlyphRangesChineseFull());

    io->MouseDoubleClickTime = 0.0001f;
    g = ImGui::GetCurrentContext();

    style = &ImGui::GetStyle();
    style->ScaleAllSizes(1.0f);
    style->FramePadding = ImVec2(10.0f, 20.0f);

    std::string LoadFile = this->SaveDir + "/Style.dat";
    ImGuiStyle s;
    if (MyFile::ReadFile(&s, LoadFile.c_str()) == 1) {
        *style = s;
        LOGE("读取主题成功");
    }

    // 初始化运行时长
    runTimeFilePath = SaveDir + "/runtime.ini";
    FILE* f = fopen(runTimeFilePath.c_str(), "r");
    if (f) {
        fscanf(f, "[Runtime]\nTotalSeconds=%lf", &totalRunTime);
        fclose(f);
        LOGE("读取运行时长成功: %.2f 秒", totalRunTime);
    } else {
        totalRunTime = 0.0;
        LOGE("运行时长文件不存在，初始化为 0");
    }

    appStartTime = getCurrentTime();
    lastSaveTime = appStartTime;

    return 1;
}

void EGL::onSurfaceCreate(JNIEnv *env, jobject surface, int SurfaceWidth, int SurfaceHigh) {
    LOGE("onSurfaceCreate");
    this->SurfaceWin = ANativeWindow_fromSurface(env, surface);
    this->surfaceWidth = SurfaceWidth;
    this->surfaceHigh = SurfaceHigh;
    this->surfaceWidthHalf = SurfaceWidth / 2;
    this->surfaceHighHalf = SurfaceHigh / 2;

    SurfaceThread = new std::thread([this] { EglThread(); });
    SurfaceThread->detach();
    LOGE("onSurfaceCreate_end");
}

void EGL::onSurfaceChange(int SurfaceWidth, int SurfaceHigh) {
    this->surfaceWidth = SurfaceWidth;
    this->surfaceHigh = SurfaceHigh;
    this->surfaceWidthHalf = SurfaceWidth / 2;
    this->surfaceHighHalf = SurfaceHigh / 2;
    this->isChage = true;
    LOGE("onSurfaceChange");
}

void EGL::onSurfaceDestroy() {
    LOGE("onSurfaceDestroy");
    this->isDestroy = true;

    std::unique_lock<std::mutex> ulo(Threadlk);
    cond.wait(ulo, [this] { return !this->ThreadIo; });
    delete SurfaceThread;
    SurfaceThread = nullptr;
}

void EGL::EglThread() {
    this->initEgl();
    this->initImgui();
    ThreadIo = true;
    input->initImguiIo(io);
    input->setImguiContext(g);
    input->setwin(this->g_window);
    
    // 在EglThread初始化部分调用
    initMemoryBrowser();
    
    bool ActiveMEMReader = false;
    
    uintptr_t base = getModuleBaseAddress("libwmw.so");
    
    if (!isGamePatched) {
        
        hookGame.initGamePatch();
        
        isGamePatched = true;
    }
    
    while (true) {
        if (this->isChage) {
            glViewport(0, 0, this->surfaceWidth, this->surfaceHigh);
            this->isChage = false;
        }

        if (this->isDestroy) {
            this->isDestroy = false;
            ThreadIo = false;
            cond.notify_all();
            return;
        }

        this->clearBuffers();
        if (!ActivityState) continue;

        double currentTime = getCurrentTime();
        double timeSinceLastSave = currentTime - lastSaveTime;

        if (timeSinceLastSave >= 1.0) {
            lastSaveTime = currentTime;
            totalRunTime += timeSinceLastSave;

            FILE* f = fopen(runTimeFilePath.c_str(), "w");
            if (f) {
                fprintf(f, "[Runtime]\nTotalSeconds=%.2f\n", totalRunTime);
                fclose(f);
            }
        }

        imguiMainWinStart();

        ImGui::SetNextWindowBgAlpha(0.7);
        style->WindowTitleAlign = ImVec2(0.5, 0.5);

        if (input->Scrollio && !input->Activeio) {
            input->funScroll(g->WheelingWindow ? g->WheelingWindow : g->HoveredWindow);
        }
        
        ImGui::Begin("WMW Mod Tool v1.0");
        
        input->g_window = g_window = ImGui::GetCurrentWindow();
        ImGui::SetWindowPos({0, 200}, ImGuiCond_FirstUseEver);

        int hours = static_cast<int>(totalRunTime) / 3600;
        int minutes = (static_cast<int>(totalRunTime) % 3600) / 60;
        int seconds = static_cast<int>(totalRunTime) % 60;
        // 碰撞回调计数
        uint32_t collisionCount = hookGame.GetCollisionCount();
        
        ImGui::Text("累计运行时长: %02d:%02d:%02d", hours, minutes, seconds);
        ImGui::Text("目标库基址 (动态): 0x%lx", base);
        ImGui::Text("LevelDone调用次数: %u", collisionCount);

        // 添加重置按钮
        if (ImGui::Button("重置碰撞计数器")) {
            hookGame.ResetCollisionCounter();
            LOGE("流体碰撞计数器已重置");
        }
        
        if (ImGui::Button("退出游戏(exit)")) {
            exit(0);
        }
        
        if (ImGui::Button(ActiveMEMReader ? "关闭内存查看器 (Bata)" : "打开内存查看器 (Bata)")) {
            ActiveMEMReader = !ActiveMEMReader;
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
        
        if (ImGui::Button(patchMgr.isGameWon1.applied ? "直接胜利 开" : "直接胜利 关")) {
            bool success = patchMgr.TogglePatch(patchMgr.isGameWon1, base) && patchMgr.TogglePatch(patchMgr.isGameWon2, base);
            LOGE("%s %s", patchMgr.isGameWon1.applied ? "应用" : "还原", 
                 success ? "成功" : "失败");
        }
        
        if (ActiveMEMReader) {
            drawMemoryBrowser();
        }
        
        
        ImGui::End();
        imguiMainWinEnd();
        this->swapBuffers();
    }
}

bool EGL::patch(void* addr, const unsigned char* patchBytes, size_t size) {
    uintptr_t pageStart = (uintptr_t)addr & ~(getpagesize() - 1);
    if (mprotect((void*)pageStart, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LOGE("mprotect failed\n");
        return false;
    }
    memcpy(addr, patchBytes, size);
    __builtin___clear_cache((char*)addr, (char*)addr + size);

    // 读回验证
    if (memcmp(addr, patchBytes, size) != 0) {
        LOGE("Patch写入后验证失败\n");
        return false;
    }

    return true;
}

void EGL::drawPatchMonitorWindow(void* targetFunc) {
    static unsigned char currentBytes[4] = {0};
    memcpy(currentBytes, targetFunc, sizeof(currentBytes));

    ImGui::Begin("Patch Monitor");

    ImGui::Text("目标函数地址: %p", targetFunc);
    ImGui::Text("函数前4字节:");
    ImGui::Text("%02X %02X %02X %02X", currentBytes[0], currentBytes[1], currentBytes[2], currentBytes[3]);

    ImGui::End();
}

uintptr_t EGL::getModuleBaseAddress(const char* moduleName) {
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

int EGL::swapBuffers() {
    if (eglSwapBuffers(mEglDisplay, mEglSurface)) {
        return 1;
    }
    LOGE("eglSwapBuffers error=%u", glGetError());
    return 0;
}

void EGL::clearBuffers() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

void EGL::setSaveSettingsdir(std::string &dir) {
    this->SaveDir = dir;
    LOGE("保存路径=%s", SaveDir.c_str());
}

void EGL::setinput(ImguiAndroidInput *input_) {
    this->input = input_;
}

// 获取模块的安全内存范围
void EGL::initMemoryBrowser() {
    ModuleInfo module = getModuleInfo("libwmw.so");
    safeRange = {module.base, module.end};
    
    // 初始化当前地址为首个函数的入口点
    currentAddress = module.base;
    
    LOGE("内存浏览器范围: 0x%lx - 0x%lx", safeRange.start, safeRange.end);
}

// 更新历史地址栈
void EGL::pushHistory(uintptr_t address) {
    historyIndex = (historyIndex + 1) % 5;
    previousAddresses[historyIndex] = address;
}

// 安全的内存浏览器实现
void EGL::drawMemoryBrowser() {
    static const int bytesPerLine = 16;
    static const int displayLines = 32;  // 一次显示32行
    static const int totalBytes = bytesPerLine * displayLines;
    
    ImGui::Begin("内存浏览器 (安全模式)");
    
    // 显示内存范围信息
    ImGui::Text("安全范围: 0x%lX - 0x%lX", safeRange.start, safeRange.end);
    
    // 导航按钮组 - 代替输入框
    if (ImGui::Button("<< Prev Page")) {
        if (currentAddress > safeRange.start + totalBytes) {
            pushHistory(currentAddress);
            currentAddress -= totalBytes;
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Next Page >>")) {
        if (currentAddress < safeRange.end - totalBytes) {
            pushHistory(currentAddress);
            currentAddress += totalBytes;
        }
    }
    
    ImGui::SameLine();
    if (historyIndex >= 0 && ImGui::Button("<< Back")) {
        currentAddress = previousAddresses[historyIndex];
        historyIndex = (historyIndex + 4) % 5; // 后退历史索引
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Base Address")) {
        pushHistory(currentAddress);
        currentAddress = safeRange.start;
    }
    
    // 地址选择滑块 - 替代直接的地址输入
    uintptr_t maxValidAddress = safeRange.end - totalBytes;
    if (maxValidAddress < safeRange.start) maxValidAddress = safeRange.start;
    
    ImGui::SliderScalar("导航", ImGuiDataType_U64, &currentAddress, 
                        &safeRange.start, &maxValidAddress, "0x%08lX");
    
    // 确保地址对齐和边界保护
    currentAddress = currentAddress & ~(bytesPerLine - 1); // 对齐16字节边界
    if (currentAddress < safeRange.start) currentAddress = safeRange.start;
    if (currentAddress > safeRange.end - totalBytes) currentAddress = maxValidAddress;
    
    // 读取内存并显示
    uint8_t memoryBuffer[totalBytes];
    size_t validBytes = 0;
    
    // 计算可安全读取的字节数
    uintptr_t endAddress = currentAddress + totalBytes;
    if (endAddress > safeRange.end) {
        validBytes = safeRange.end - currentAddress;
    } else {
        validBytes = totalBytes;
    }
    
    // 安全拷贝内存
    if (validBytes > 0) {
        memcpy(memoryBuffer, (void*)currentAddress, validBytes);
    }
    
    // 显示内存内容表格
    ImGui::BeginChild("内存视图", ImVec2(0, 800), true);
    
    ImGui::Columns(3, "memoryColumns");
    ImGui::SetColumnWidth(0, 200); // 地址列
    ImGui::SetColumnWidth(1, 800); // 十六进制列
    ImGui::SetColumnWidth(2, 250); // ASCII列
    
    ImGui::Text("地址");
    ImGui::NextColumn();
    ImGui::Text("十六进制");
    ImGui::NextColumn();
    ImGui::Text("ASCII");
    ImGui::NextColumn();
    
    ImGui::Separator();
    
    for (int i = 0; i < validBytes; i += bytesPerLine) {
        // 显示地址
        ImGui::Text("%08lX", currentAddress + i);
        ImGui::NextColumn();
        
        // 显示十六进制
        std::string hexLine;
        for (int j = 0; j < bytesPerLine; j++) {
            if (i + j < validBytes) {
                char byteHex[4];
                snprintf(byteHex, sizeof(byteHex), "%02X ", memoryBuffer[i + j]);
                hexLine += byteHex;
            } else {
                hexLine += "   "; // 空白填充
            }
        }
        ImGui::Text("%s", hexLine.c_str());
        ImGui::NextColumn();
        
        // 显示ASCII
        std::string asciiLine;
        for (int j = 0; j < bytesPerLine; j++) {
            if (i + j < validBytes) {
                uint8_t c = memoryBuffer[i + j];
                asciiLine += (c >= 32 && c <= 126) ? static_cast<char>(c) : '.';
            } else {
                asciiLine += ' '; // 空白填充
            }
        }
        ImGui::Text("%s", asciiLine.c_str());
        ImGui::NextColumn();
    }
    
    ImGui::Columns(1);
    ImGui::EndChild();
    
    ImGui::Text("当前地址: 0x%lX", currentAddress);
    
    ImGui::End();
}

EGL::ModuleInfo EGL::getModuleInfo(const char* moduleName) {
    ModuleInfo info = {0, 0};
    FILE* fp = fopen("/proc/self/maps", "r");
    
    if (!fp) {
        LOGE("打开 /proc/self/maps 失败: %s", strerror(errno));
        return info;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, moduleName)) {
            uintptr_t start, end;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                info = {start, end};
                break;
            }
        }
    }
    
    fclose(fp);
    LOGE("模块 %s 范围: 0x%lx-0x%lx", moduleName, info.base, info.end);
    return info;
}

void EGL::saveModuleMemoryToFile() {
    // 保存目录
    std::string saveDir = "/sdcard/Android/data/com.decompilationpixel.WMW";
    
    // 生成文件名
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char filename[128];
    std::strftime(filename, sizeof(filename), "libwmw_%Y%m%d_%H%M%S.bin", std::localtime(&time));
    std::string fullPath = saveDir + "/" + filename;
    
    // 显示初始状态
    saveProgress = 0.0f;
    isSavingMemory = true;
    
    // 在新线程中执行保存操作
    std::thread([this, fullPath] {
        FILE* file = fopen(fullPath.c_str(), "wb");
        if (!file) {
            LOGE("无法创建文件: %s", fullPath.c_str());
            isSavingMemory = false;
            return;
        }
        
        const size_t bufferSize = 4096; // 4KB分块保存
        uint8_t buffer[bufferSize];
        size_t totalSize = safeRange.end - safeRange.start;
        size_t bytesSaved = 0;
        
        for (uintptr_t addr = safeRange.start; addr < safeRange.end; addr += bufferSize) {
            if (this->isDestroy) break; // 如果正在销毁则终止
            
            size_t currentSize = (addr + bufferSize <= safeRange.end) 
                                ? bufferSize 
                                : safeRange.end - addr;
            
            memcpy(buffer, (void*)addr, currentSize);
            fwrite(buffer, 1, currentSize, file);
            bytesSaved += currentSize;
            
            // 更新进度 (0.0f - 1.0f)
            saveProgress = static_cast<float>(bytesSaved) / totalSize;
        }
        
        fclose(file);
        
        LOGE("内存保存完成: %s (%.2f MB)", 
             fullPath.c_str(), bytesSaved / (1024.0f * 1024.0f));
        
        isSavingMemory = false;
    }).detach();
}
