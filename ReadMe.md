## 这是什么？
这是 Dec WMW: The Lost Levels

基于:
https://github.com/StArrayJaN/Where-is-My-Water
修改

## 怎么构建？
首先你要有一份完整的 libwmw.so 和资源文件

把它们统统压缩并交给prebuild.py处理

zip file tree:
assets
libwmw.so
libfmodex.so

so 的 sha256:
2cbbacf36e09b001fc0af46556246d2a74c3ea21b118dc1d46bc474dbf637495
4c849b928c23301b287fb4fecdcaf82c1888578e0a97727a42baf3f8053d73bc

没了！

afl.txt:
所有函数的函数表

str.txt:
所有字符串在so中的位置