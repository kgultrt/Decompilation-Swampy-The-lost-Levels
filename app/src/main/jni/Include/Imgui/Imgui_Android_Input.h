#ifndef FIND_ROODS_IMGUI_ANDROID_INPUT_H
#define FIND_ROODS_IMGUI_ANDROID_INPUT_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include "timer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <jni.h>
#include "log.h"
#include <unistd.h>

class ImguiAndroidInput {
public:
    ImguiAndroidInput();
    ~ImguiAndroidInput();

    // 公共 API（完全不变）
    void initImguiIo(ImGuiIO* io);
    void setwin(ImGuiWindow* g_window_);
    void setImguiContext(ImGuiContext* g_);
    void toast(std::string str) const;
    void ioset(jint pos, jint v) const;
    bool openInput();
    bool closeInput();
    void isLongTouch(int x, int y);
    void funMshowinit(jclass thiz, JNIEnv* env);
    void setMaxFPS(int MAX_FPS);
    static int inputCallback(ImGuiInputTextCallbackData* CallbackData);
    std::string JNI_Cut();
    int JNI_SelectAll();
    void JNI_Paste(std::string data);
    std::string JNI_Copy();
    void addUTF8(const char* in_data);
    void InputKey(int action, int code);
    bool InputTouchEvent(int event_get_action, float x, float y);
    float funScroll();
    float funScroll(ImGuiWindow* Window);

    // 公开成员变量（保持兼容）
    ImGuiWindow* g_window;
    bool loopRun;
    bool Inputio;
    bool Scrollio;
    bool Activeio;
    float ScrollX;
    float ScrollY;
    float f;
    int fps;
    int max_fps;
    bool winio;
    bool fullwinio;
    float winWidth;
    float winHeight;
    float oldwinWidth;
    float oldwinHeight;
    bool ItemActive;
    bool ItemHovered;
    bool ItemFocused;
    bool ItemEdited;
    bool ItemScrollio;
    bool upio;
    bool runScroll;

private:
    // 触摸事件类型
    enum eTouchEvent {
        TOUCH_DOWN,
        TOUCH_UP,
        TOUCH_MOVE,
        TOUCH_CANCEL,
        TOUCH_OUTSIDE
    };

    // 输入动作
    enum InputAction {
        Action_DOWN,
        Action_UP
    };

    // JNI 相关信息
    struct JniContext {
        JavaVM* jvm = nullptr;
        jobject obj = nullptr;
        jclass pJclass = nullptr;
        jmethodID show = nullptr;
        jmethodID io = nullptr;
        jmethodID openInput = nullptr;
        jmethodID closeInput = nullptr;
        jmethodID isLongTouch = nullptr;
    } jni_;

    // 文本输入回调状态管理
    struct InputState {
        std::mutex mtx;
        std::condition_variable cv;
        int activeAction = -1;      // -1: idle, 0:copy, 1:paste, 2:selectAll, 3:cut
        bool done = false;
        char* copyBuffer = nullptr;
        const char* pasteBuffer = nullptr;
        int selectSize = 0;
    };
    static InputState s_inputState; // 静态成员，跨实例共享

    // 触摸/滚动状态
    struct TouchState {
        bool isMouseMove = false;
        timer touchTimer;
        float downX = 0.0f, downY = 0.0f;
        float setScrollX = 0.0f, setScrollY = 0.0f;
        float scrollXMax = 0.0f, scrollYMax = 0.0f;
        float touchDuration = 0.0f;
        float scrollVelocityX = 0.0f, scrollVelocityY = 0.0f;
        float friction = 0.95f;     // 惯性衰减系数
        bool moveWindow = false;
    } touch_;

    ImGuiIO* io_ = nullptr;
    ImGuiContext* g_ = nullptr;

    std::atomic<bool> longPressActive_{false};
    std::thread longPressThread_;

    // 私有辅助函数
    bool getJniEnv(JNIEnv** env, bool* shouldDetach) const;
    void cleanupJniEnv(JNIEnv* env, bool shouldDetach) const;
    void checkJniException(JNIEnv* env) const;
    void startLongPressDetection();
    void stopLongPressDetection();
    void resetScrollInertia();
    void updateScrollInertia(ImGuiWindow* window = nullptr);
    
    // 静态回调辅助
    static void CopyCallback(ImGuiInputTextCallbackData* data);
    static void PasteCallback(ImGuiInputTextCallbackData* data);
    static void SelectAllCallback(ImGuiInputTextCallbackData* data);
    static void CutCallback(ImGuiInputTextCallbackData* data);
};

#endif // FIND_ROODS_IMGUI_ANDROID_INPUT_H