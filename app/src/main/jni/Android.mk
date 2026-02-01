# 设置模块的本地路径
LOCAL_PATH := $(call my-dir)

# 包含清除默认值的变量
include $(CLEAR_VARS)

# 定义目标模块名称
LOCAL_MODULE := libhook

# 设置C/C++编译选项
LOCAL_CFLAGS := -w -s -Wno-error=format-security -fvisibility=hidden -fpermissive
LOCAL_CPPFLAGS := -std=c++17 \
                  -w -s -Wno-error=format-security -fvisibility=hidden -Werror \
                  -Wno-error=c++11-narrowing -fpermissive -Wall -fexceptions \
                  -I$(LOCAL_PATH)/include -I$(LOCAL_PATH)/3rd/Imgui -I$(LOCAL_PATH)/3rd/Substrate

# 定义源文件列表
LOCAL_SRC_FILES := 3rd/Imgui/imgui.cpp \
                   3rd/Imgui/imgui_draw.cpp \
                   3rd/Imgui/imgui_demo.cpp \
                   3rd/Imgui/imgui_tables.cpp \
                   3rd/Imgui/imgui_widgets.cpp \
                   3rd/Imgui/imgui_impl_android.cpp \
                   3rd/Imgui/imgui_impl_opengl3.cpp \
                   3rd/Imgui/Imgui_Android_Input.cpp \
                   3rd/Substrate/hde64.c \
                   3rd/Substrate/SubstrateDebug.cpp \
                   3rd/Substrate/SubstrateHook.cpp \
                   3rd/Substrate/SubstratePosixMemory.cpp \
                   3rd/Substrate/SymbolFinder.cpp \
                   src/EGL.cpp \
                   src/HookGame.cpp \
                   src/MyFile.cpp \
                   native-lib.cpp \

# 指定要链接的系统库
LOCAL_LDLIBS := -ldl -llog -lEGL -lGLESv3 -landroid

# 包含用于构建共享库的规则
include $(BUILD_SHARED_LIBRARY)