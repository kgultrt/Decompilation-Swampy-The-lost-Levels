#!/bin/bash

# 设置脚本失败时退出
set -e

# 定位到项目根目录
cd "$(dirname "$0")" || exit 1

# 构建 APK
echo "正在构建 Debug 版本..."
bash gradlew app:assembleDebug || { echo "构建失败"; exit 1; }

# 切换到构建输出目录
echo "进入目录..."
cd app/build/outputs/apk/debug || { echo "无法进入构建输出目录"; exit 1; }

echo "复制文件..."
cp app-debug.apk /sdcard || { echo "复制文件失败"; exit 1; }

echo "构建完成！"

adb -s 0.0.0.0 install /sdcard/app-debug.apk

echo "打开应用吧！"