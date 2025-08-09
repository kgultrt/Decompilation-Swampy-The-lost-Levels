//
// Created by admin on 2022/6/14.
//

#ifndef PCH_H
#define PCH_H
#define LOGE(fmt, ...) \
    do { \
        char buf[256]; \
        snprintf(buf, sizeof(buf), "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
        printf("%s\n", buf); \
    } while (0)

#include "EGL/egl.h"
#include "GLES2/gl2.h"
#include "GLES3/gl3.h"
#include <string>
#include <sched.h>
#include <unistd.h>
#include "Imgui_Android_Input.h"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <jni.h>
#include <unordered_map>
//#include "../timer.h"
//#include "myStruct.h"
//using namespace std;
#include "OPPOSans-H.h"

#include <random>
#include "android/native_window_jni.h"
#include "log.h"
#include "MyFile.h"
#include "PatchManager.h"


#endif //IMGUITESTMENU_PCH_H
