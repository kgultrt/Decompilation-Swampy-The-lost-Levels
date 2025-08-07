# 设置模块的本地路径
LOCAL_PATH := $(call my-dir)

# 包含清除默认值的变量
include $(CLEAR_VARS)

# 定义目标模块名称
LOCAL_MODULE := libfuckwater

# 设置C/C++编译选项
LOCAL_CFLAGS := -w -s -Wno-error=format-security -fvisibility=hidden -fpermissive
LOCAL_CPPFLAGS := -std=c++17 \
                  -w -s -Wno-error=format-security -fvisibility=hidden -Werror \
                  -Wno-error=c++11-narrowing -fpermissive -Wall -fexceptions \
                  -I$(LOCAL_PATH)/Include -I$(LOCAL_PATH)/Include/Imgui -I$(LOCAL_PATH)/And64InlineHook

# 定义源文件列表
LOCAL_SRC_FILES := Source/Imgui/imgui.cpp \
                   Source/Imgui/imgui_draw.cpp \
                   Source/Imgui/imgui_demo.cpp \
                   Source/Imgui/imgui_tables.cpp \
                   Source/Imgui/imgui_widgets.cpp \
                   Source/Imgui/imgui_impl_android.cpp \
                   Source/Imgui/imgui_impl_opengl3.cpp \
                   Source/Imgui_Android_Input.cpp \
                   Source/EGL.cpp \
                   Source/HookGame.cpp \
                   Source/MyFile.cpp \
                   native-lib.cpp \

# 指定要链接的系统库
LOCAL_LDLIBS := -ldl -llog -lEGL -lGLESv3 -landroid

# 包含用于构建共享库的规则
include $(BUILD_SHARED_LIBRARY)