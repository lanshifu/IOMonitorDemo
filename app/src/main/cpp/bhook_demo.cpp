#include <jni.h>
#include <string>
#include "xhook.h"
#include <android/log.h>

#include "bytehook.h"
static const char *const kTag = "IOMonitor.JNI";

static int (*original_open)(const char *pathname, int flags, mode_t mode);
static int (*original_open64)(const char *pathname, int flags, mode_t mode);


static int open_proxy_manual(const char* pathname, int flags, mode_t modes)
{
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen start,name=%s",pathname);
    int ret = original_open(pathname, flags, modes);
    if (ret != -1) {

    }
    return ret;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_lanshifu_iomonitordemo_bhook_BHookSdk_dohook(JNIEnv *env, jobject thiz) {



//    open_stub = bytehook_hook_partial(allow_filter, NULL, NULL, "open", (void *)open_proxy_manual, open_hooked_callback, NULL);


}