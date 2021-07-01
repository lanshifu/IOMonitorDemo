#include <jni.h>
#include <string>
#include "xhook.h"
#include <android/log.h>

// xhook:https://github.com/iqiyi/xHook/blob/master/README.zh-CN.md
static const char* const kTag = "IOMonitor.JNI";

//保存方法地址
static int (*original_open) (const char *pathname, int flags, mode_t mode);
static int (*original_open64) (const char *pathname, int flags, mode_t mode);

const static char* TARGET_MODULES[] = {
        "libopenjdkjvm.so",
        "libjavacore.so",
        "libopenjdk.so"
};
const static size_t TARGET_MODULE_COUNT = sizeof(TARGET_MODULES) / sizeof(char*);


extern "C" JNIEXPORT jstring JNICALL
Java_com_lanshifu_iomonitordemo_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    return env->NewStringUTF(hello.c_str());
}


int ProxyOpen(const char *pathname, int flags, mode_t mode) {

    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen start");
    int ret = original_open(pathname, flags, mode);
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen end");

    //调用
    return ret;
}

int ProxyOpen64(const char *pathname, int flags, mode_t mode) {

    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen64 start");
    int ret = original_open64(pathname, flags, mode);
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen64 end");

    return ret;
}





extern "C" JNIEXPORT void JNICALL
Java_com_lanshifu_iomonitordemo_MainActivity_doHook(JNIEnv *env, jobject thiz) {


    //打开xhook日志
    xhook_enable_debug(1);

    //注册hook信息
    for (int i = 0; i < TARGET_MODULE_COUNT; ++i) {
        const char* so_name = TARGET_MODULES[i];
        __android_log_print(ANDROID_LOG_INFO, kTag, "try to hook function in %s.", so_name);


        xhook_register(so_name, "open", (void*)ProxyOpen, (void**)&original_open);
        xhook_register(so_name, "open64", (void*)ProxyOpen64, (void**)&original_open64);

//        xhook_hook_symbol(soinfo, "read", (void*)ProxyRead, (void**)&original_read)
    }

    //执行 hook,同步
    xhook_refresh(0);



}