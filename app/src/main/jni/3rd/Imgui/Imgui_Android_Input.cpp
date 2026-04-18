#include "Imgui/Imgui_Android_Input.h"
#include <cstring>
#include <cmath>

// 静态成员定义
ImguiAndroidInput::InputState ImguiAndroidInput::s_inputState;

ImguiAndroidInput::ImguiAndroidInput() {
    g_window = nullptr;
    loopRun = false;
    Inputio = false;
    Scrollio = false;
    Activeio = false;
    ScrollX = 0.0f;
    ScrollY = 0.0f;
    f = 1.0f;
    fps = 60;
    max_fps = 60;
    winio = false;
    fullwinio = false;
    winWidth = 0.0f;
    winHeight = 0.0f;
    oldwinWidth = 0.0f;
    oldwinHeight = 0.0f;
    ItemActive = false;
    ItemHovered = false;
    ItemFocused = false;
    ItemEdited = false;
    ItemScrollio = false;
    upio = false;
    runScroll = false;
}

ImguiAndroidInput::~ImguiAndroidInput() {
    stopLongPressDetection();
}

// ---------- JNI 辅助函数 ----------
bool ImguiAndroidInput::getJniEnv(JNIEnv** env, bool* shouldDetach) const {
    if (!jni_.jvm) return false;
    *shouldDetach = false;
    jint result = jni_.jvm->GetEnv((void**)env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        if (jni_.jvm->AttachCurrentThread(env, nullptr) == JNI_OK) {
            *shouldDetach = true;
        } else {
            return false;
        }
    }
    return (*env != nullptr);
}

void ImguiAndroidInput::cleanupJniEnv(JNIEnv* env, bool shouldDetach) const {
    if (shouldDetach) {
        jni_.jvm->DetachCurrentThread();
    }
}

void ImguiAndroidInput::checkJniException(JNIEnv* env) const {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        toast("JNI异常");
    }
}

// ---------- 文本输入回调实现 ----------
void ImguiAndroidInput::CopyCallback(ImGuiInputTextCallbackData* data) {
    std::lock_guard<std::mutex> lock(s_inputState.mtx);
    int len = data->SelectionEnd - data->SelectionStart;
    if (len <= 0) {
        s_inputState.copyBuffer = nullptr;
    } else {
        s_inputState.copyBuffer = (char*)malloc(len + 1);
        if (s_inputState.copyBuffer) {
            memcpy(s_inputState.copyBuffer, data->Buf + data->SelectionStart, len);
            s_inputState.copyBuffer[len] = '\0';
        }
    }
    s_inputState.done = true;
    s_inputState.cv.notify_one();
}

void ImguiAndroidInput::PasteCallback(ImGuiInputTextCallbackData* data) {
    if (!s_inputState.pasteBuffer) return;
    if (data->HasSelection()) {
        data->DeleteChars(data->SelectionStart, data->SelectionEnd - data->SelectionStart);
    }
    data->InsertChars(data->CursorPos, s_inputState.pasteBuffer);
}

void ImguiAndroidInput::SelectAllCallback(ImGuiInputTextCallbackData* data) {
    data->SelectAll();
    s_inputState.selectSize = data->SelectionEnd - data->SelectionStart;
}

void ImguiAndroidInput::CutCallback(ImGuiInputTextCallbackData* data) {
    CopyCallback(data);
    data->DeleteChars(data->SelectionStart, data->SelectionEnd - data->SelectionStart);
}

int ImguiAndroidInput::inputCallback(ImGuiInputTextCallbackData* CallbackData) {
    std::unique_lock<std::mutex> lock(s_inputState.mtx);
    switch (s_inputState.activeAction) {
        case 0: CopyCallback(CallbackData); break;
        case 1: PasteCallback(CallbackData); break;
        case 2: SelectAllCallback(CallbackData); break;
        case 3: CutCallback(CallbackData); break;
        default: return 0;
    }
    s_inputState.activeAction = -1;
    s_inputState.done = true;
    s_inputState.cv.notify_one();
    return 0;
}

// ---------- JNI 公开方法实现 ----------
std::string ImguiAndroidInput::JNI_Copy() {
    if (!io_ || !io_->WantTextInput) return "";
    std::unique_lock<std::mutex> lock(s_inputState.mtx);
    s_inputState.activeAction = 0;
    s_inputState.done = false;
    s_inputState.cv.wait(lock, [] { return s_inputState.done; });
    std::string result = s_inputState.copyBuffer ? s_inputState.copyBuffer : "";
    free(s_inputState.copyBuffer);
    s_inputState.copyBuffer = nullptr;
    return result;
}

void ImguiAndroidInput::JNI_Paste(std::string data) {
    if (!io_ || !io_->WantTextInput) return;
    std::unique_lock<std::mutex> lock(s_inputState.mtx);
    s_inputState.pasteBuffer = data.c_str();
    s_inputState.activeAction = 1;
    s_inputState.done = false;
    s_inputState.cv.wait(lock, [] { return s_inputState.done; });
}

int ImguiAndroidInput::JNI_SelectAll() {
    if (!io_ || !io_->WantTextInput) return 0;
    std::unique_lock<std::mutex> lock(s_inputState.mtx);
    s_inputState.activeAction = 2;
    s_inputState.done = false;
    s_inputState.cv.wait(lock, [] { return s_inputState.done; });
    return s_inputState.selectSize;
}

std::string ImguiAndroidInput::JNI_Cut() {
    if (!io_ || !io_->WantTextInput) return "";
    std::unique_lock<std::mutex> lock(s_inputState.mtx);
    s_inputState.activeAction = 3;
    s_inputState.done = false;
    s_inputState.cv.wait(lock, [] { return s_inputState.done; });
    std::string result = s_inputState.copyBuffer ? s_inputState.copyBuffer : "";
    free(s_inputState.copyBuffer);
    s_inputState.copyBuffer = nullptr;
    return result;
}

void ImguiAndroidInput::addUTF8(const char* in_data) {
    if (io_) io_->AddInputCharactersUTF8(in_data);
}

void ImguiAndroidInput::InputKey(int action, int code) {
    if (!io_) return;
    bool down = (action == Action_DOWN);
    if (code == 59) io_->KeyShift = down;
    io_->KeysDown[code] = down;
    usleep(20000);
}

// ---------- 初始化 ----------
void ImguiAndroidInput::initImguiIo(ImGuiIO* io) {
    io_ = io;
    touch_ = TouchState{};
    if (max_fps == 0) max_fps = 60;
}

void ImguiAndroidInput::setwin(ImGuiWindow* window) {
    g_window = window;
}

void ImguiAndroidInput::setImguiContext(ImGuiContext* g) {
    g_ = g;
}

void ImguiAndroidInput::setMaxFPS(int MAX_FPS) {
    max_fps = MAX_FPS;
}

// ---------- 输入法控制 ----------
bool ImguiAndroidInput::openInput() {
    if (!jni_.openInput || !g_window) return false;
    JNIEnv* env = nullptr;
    bool shouldDetach = false;
    if (!getJniEnv(&env, &shouldDetach)) return false;
    jboolean ret = env->CallStaticBooleanMethod(jni_.pJclass, jni_.openInput);
    checkJniException(env);
    cleanupJniEnv(env, shouldDetach);
    return ret == JNI_TRUE;
}

bool ImguiAndroidInput::closeInput() {
    if (!jni_.closeInput) return false;
    JNIEnv* env = nullptr;
    bool shouldDetach = false;
    if (!getJniEnv(&env, &shouldDetach)) return false;
    jboolean ret = env->CallStaticBooleanMethod(jni_.pJclass, jni_.closeInput);
    checkJniException(env);
    cleanupJniEnv(env, shouldDetach);
    return ret == JNI_TRUE;
}

// ---------- Toast / IO 设置 ----------
void ImguiAndroidInput::toast(std::string str) const {
    if (!jni_.show) return;
    JNIEnv* env = nullptr;
    bool shouldDetach = false;
    if (!getJniEnv(&env, &shouldDetach)) return;
    jstring jstr = env->NewStringUTF(str.c_str());
    env->CallStaticVoidMethod(jni_.pJclass, jni_.show, jstr);
    env->DeleteLocalRef(jstr);
    checkJniException(env);
    cleanupJniEnv(env, shouldDetach);
}

void ImguiAndroidInput::ioset(jint pos, jint v) const {
    if (!jni_.io) return;
    JNIEnv* env = nullptr;
    bool shouldDetach = false;
    if (!getJniEnv(&env, &shouldDetach)) return;
    jstring jstr = env->NewStringUTF("psio");
    env->CallStaticVoidMethod(jni_.pJclass, jni_.io, jstr, pos, v);
    env->DeleteLocalRef(jstr);
    checkJniException(env);
    cleanupJniEnv(env, shouldDetach);
}

void ImguiAndroidInput::isLongTouch(int x, int y) {
    if (!jni_.isLongTouch) return;
    JNIEnv* env = nullptr;
    bool shouldDetach = false;
    if (!getJniEnv(&env, &shouldDetach)) return;
    env->CallStaticVoidMethod(jni_.pJclass, jni_.isLongTouch, x, y);
    checkJniException(env);
    cleanupJniEnv(env, shouldDetach);
}

// ---------- JNI 初始化 ----------
void ImguiAndroidInput::funMshowinit(jclass thiz, JNIEnv* env) {
    if (jni_.show != nullptr) return;
    env->GetJavaVM(&jni_.jvm);
    jni_.pJclass = (jclass)env->NewGlobalRef(thiz);
    jni_.show = env->GetStaticMethodID(jni_.pJclass, "mShow", "(Ljava/lang/String;)V");
    jni_.io = env->GetStaticMethodID(jni_.pJclass, "mIO", "(Ljava/lang/String;II)V");
    jni_.openInput = env->GetStaticMethodID(jni_.pJclass, "openInput", "()Z");
    jni_.closeInput = env->GetStaticMethodID(jni_.pJclass, "closeInput", "()Z");
    jni_.isLongTouch = env->GetStaticMethodID(jni_.pJclass, "isLongTouch", "(II)V");
}

// ---------- 长按检测线程 ----------
void ImguiAndroidInput::startLongPressDetection() {
    if (longPressActive_) return;
    longPressActive_ = true;
    loopRun = true;
    longPressThread_ = std::thread([this]() {
        while (longPressActive_ && loopRun) {
            // 注意：islooptimestart 是成员变量，不是函数
            if (touch_.touchTimer.islooptimestart) {
                if (touch_.touchTimer.getlooptime() > 150000000 && !touch_.isMouseMove && io_ && io_->WantTextInput) {
                    ioset(3, 0);
                    longPressActive_ = false;
                    loopRun = false;
                    break;
                }
                if (touch_.isMouseMove || !io_ || !io_->WantTextInput || !io_->MouseDown[0]) {
                    loopRun = false;
                    break;
                }
            }
            usleep(10000);
        }
        longPressActive_ = false;
    });
}

void ImguiAndroidInput::stopLongPressDetection() {
    longPressActive_ = false;
    loopRun = false;
    if (longPressThread_.joinable()) {
        longPressThread_.join();
    }
}

// ---------- 滚动惯性辅助 ----------
void ImguiAndroidInput::resetScrollInertia() {
    touch_.setScrollX = touch_.setScrollY = 0.0f;
    touch_.scrollVelocityX = touch_.scrollVelocityY = 0.0f;
    Scrollio = false;
    runScroll = false;
}

void ImguiAndroidInput::updateScrollInertia(ImGuiWindow* window) {
    if (!window) window = g_window;
    if (!window) return;

    float nowX = window->Scroll.x;
    float nowY = window->Scroll.y;
    float maxX = window->ScrollMax.x;
    float maxY = window->ScrollMax.y;

    if (!upio) {
        if (touch_.touchDuration > 0.3f) {
            resetScrollInertia();
            return;
        }
        float dt = touch_.touchDuration / 1000.0f;
        if (dt > 0.0f) {
            touch_.scrollVelocityX = touch_.setScrollX / dt;
            touch_.scrollVelocityY = touch_.setScrollY / dt;
        }
        upio = true;
        runScroll = true;
        Scrollio = true;
    }

    touch_.scrollVelocityX *= touch_.friction;
    touch_.scrollVelocityY *= touch_.friction;

    float deltaX = touch_.scrollVelocityX / (float)std::max(fps, 1);
    float deltaY = touch_.scrollVelocityY / (float)std::max(fps, 1);

    float newX = nowX - deltaX;
    float newY = nowY - deltaY;

    if (newX < 0.0f) newX = 0.0f;
    if (newX > maxX) newX = maxX;
    if (newY < 0.0f) newY = 0.0f;
    if (newY > maxY) newY = maxY;

    ImGui::SetScrollX(window, newX);
    ImGui::SetScrollY(window, newY);

    if (fabs(touch_.scrollVelocityX) < 5.0f && fabs(touch_.scrollVelocityY) < 5.0f) {
        resetScrollInertia();
    }
}

// ---------- 触摸事件处理（核心）----------
bool ImguiAndroidInput::InputTouchEvent(int event_action, float x, float y) {
    if (!io_) return false;

    // 无窗口时的安全处理
    if (!g_window) {
        switch (event_action) {
            case eTouchEvent::TOUCH_DOWN:
                io_->AddMousePosEvent(x, y);
                io_->AddMouseButtonEvent(0, true);
                break;
            case eTouchEvent::TOUCH_UP:
                io_->AddMouseButtonEvent(0, false);
                break;
            case eTouchEvent::TOUCH_MOVE:
                io_->AddMousePosEvent(x, y);
                break;
            default: break;
        }
        return io_->WantCaptureMouse;
    }

    // 输入法自动管理
    if (io_->WantTextInput) {
        if (!Inputio && event_action != eTouchEvent::TOUCH_OUTSIDE) {
            Inputio = openInput();
        }
    } else {
        if (Inputio) {
            Inputio = closeInput();
        }
    }

    if (ItemHovered) Activeio = true;

    switch (event_action) {
        case eTouchEvent::TOUCH_OUTSIDE:
            io_->AddMouseButtonEvent(0, false);
            touch_.moveWindow = false;
            Activeio = false;
            stopLongPressDetection();
            if (Inputio) Inputio = closeInput();
            break;

        case eTouchEvent::TOUCH_DOWN:
            touch_.downX = x;
            touch_.downY = y;
            io_->AddMousePosEvent(x, y);
            io_->AddMouseButtonEvent(0, true);
            Activeio = ItemHovered;
            touch_.isMouseMove = false;
            touch_.setScrollX = touch_.setScrollY = 0.0f;
            touch_.touchDuration = 0.0f;
            upio = false;
            runScroll = false;
            Scrollio = false;
            touch_.touchTimer.start();
            touch_.touchTimer.looptimestart();
            startLongPressDetection();
            if (y <= g_window->TitleBarHeight()) {
                touch_.moveWindow = true;
            }
            break;

        case eTouchEvent::TOUCH_UP:
            io_->AddMouseButtonEvent(0, false);
            touch_.setScrollX = x - touch_.downX;
            touch_.setScrollY = y - touch_.downY;
            touch_.touchDuration = touch_.touchTimer.stop(1);
            touch_.moveWindow = false;
            Activeio = false;
            stopLongPressDetection();
            if (!touch_.isMouseMove) {
                updateScrollInertia();
            }
            break;

        case eTouchEvent::TOUCH_MOVE:
            io_->AddMousePosEvent(x, y);
            stopLongPressDetection();
            if (!touch_.isMouseMove) {
                float dx = fabs(x - touch_.downX);
                float dy = fabs(y - touch_.downY);
                if (dx > 3.0f || dy > 3.0f) {
                    touch_.isMouseMove = true;
                }
            }
            break;
    }

    return io_->WantCaptureMouse;
}

// ---------- 滚动 API ----------
float ImguiAndroidInput::funScroll() {
    if (!g_window) return 0.0f;
    if (g_) g_->WheelingWindow = g_window;
    if (touch_.isMouseMove && io_->MouseDown[0]) {
        ImGui::SetScrollX(g_window, g_window->Scroll.x - io_->MouseDelta.x);
        ImGui::SetScrollY(g_window, g_window->Scroll.y - io_->MouseDelta.y);
        upio = false;
        runScroll = true;
    } else {
        if (runScroll) {
            updateScrollInertia(g_window);
        }
    }
    return g_window->Scroll.x;
}

float ImguiAndroidInput::funScroll(ImGuiWindow* window) {
    if (!window) return 0.0f;
    if (g_) {
        g_->WheelingWindow = window;
        g_->WheelingWindowRefMousePos = io_->MousePos;
    }
    if (touch_.isMouseMove && io_->MouseDown[0]) {
        ImGui::SetScrollX(window, window->Scroll.x - io_->MouseDelta.x);
        ImGui::SetScrollY(window, window->Scroll.y - io_->MouseDelta.y);
        upio = false;
        runScroll = true;
    } else {
        if (runScroll) {
            updateScrollInertia(window);
        }
    }
    return window->Scroll.x;
}