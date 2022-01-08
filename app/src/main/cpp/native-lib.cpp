#include <jni.h>
#include <string>
#include "xhook.h"
#include <android/log.h>

// xhook:https://github.com/iqiyi/xHook/blob/master/README.zh-CN.md
static const char *const kTag = "IOMonitor.JNI";

//保存方法地址
static int (*original_open)(const char *pathname, int flags, mode_t mode);
static int (*original_open64)(const char *pathname, int flags, mode_t mode);
static ssize_t (*original_read) (int fd, void *buf, size_t size);
static ssize_t (*original_read_chk) (int fd, void* buf, size_t count, size_t buf_size);
static ssize_t (*original_write) (int fd, const void *buf, size_t size);
static ssize_t (*original_write_chk) (int fd, const void* buf, size_t count, size_t buf_size);
static int (*original_close) (int fd);

static bool kInitSuc = false;
static JavaVM *kJvm;

static jclass kJavaBridgeClass;
static jclass kJavaContextClass;
static jmethodID kMethodIDGetJavaContext;
static jfieldID kFieldIDStack;
static jfieldID kFieldIDThreadName;

//需要hook的库
const static char *TARGET_MODULES[] = {
        "libopenjdkjvm.so",
        "libjavacore.so",
        "libopenjdk.so"
};
const static size_t TARGET_MODULE_COUNT = sizeof(TARGET_MODULES) / sizeof(char *);

char *jstringToChars(JNIEnv *env, jstring jstr) {
    if (jstr == nullptr) {
        return nullptr;
    }

    jboolean isCopy = JNI_FALSE;
    const char *str = env->GetStringUTFChars(jstr, &isCopy);
    char *ret = strdup(str);
    env->ReleaseStringUTFChars(jstr, str);
    return ret;
}

void printStack(){
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "printStack");
    JNIEnv* env = NULL;
    kJvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (env == NULL || !kInitSuc) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "ProxyOpen env null or kInitSuc:%d", kInitSuc);
    } else {

        jobject java_context_obj = env->CallStaticObjectMethod(kJavaBridgeClass, kMethodIDGetJavaContext);
        if (NULL == java_context_obj) {
            return;
        }
        __android_log_print(ANDROID_LOG_DEBUG, kTag, "CallStaticObjectMethod end");
        jstring j_stack = (jstring) env->GetObjectField(java_context_obj, kFieldIDStack);
        jstring j_thread_name = (jstring) env->GetObjectField(java_context_obj, kFieldIDThreadName);

        char* thread_name = jstringToChars(env, j_thread_name);
        char* stack = jstringToChars(env, j_stack);
        __android_log_print(ANDROID_LOG_ERROR, kTag, "printStack...");
        __android_log_print(ANDROID_LOG_ERROR, kTag, "thread_name=%s",thread_name);
        __android_log_print(ANDROID_LOG_ERROR, kTag, "stack=%s",stack);

//        JavaContext java_context(GetCurrentThreadId(), thread_name == NULL ? "" : thread_name, stack == NULL ? "" : stack);
        free(stack);
        free(thread_name);


        env->DeleteLocalRef(java_context_obj);
        env->DeleteLocalRef(j_stack);
        env->DeleteLocalRef(j_thread_name);
    }
}

static bool InitJIniEnvn(JavaVM *vm) {
    kJvm = vm;
    JNIEnv* env = NULL;
    if (kJvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK){
        __android_log_print(ANDROID_LOG_ERROR, kTag, "InitJniEnv GetEnv !JNI_OK");
        return false;
    }

    __android_log_print(ANDROID_LOG_DEBUG, kTag, "InitJniEnv1");
    jclass temp_cls = env->FindClass("com/lanshifu/iomonitordemo/IOCanaryJniBridge");
    if (temp_cls == NULL)  {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "InitJniEnv kJavaBridgeClass NULL");
        return false;
    }
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "InitJniEnv2");
    kJavaBridgeClass = reinterpret_cast<jclass>(env->NewGlobalRef(temp_cls));

    jclass temp_java_context_cls = env->FindClass("com/lanshifu/iomonitordemo/IOCanaryJniBridge$JavaContext");
    if (temp_java_context_cls == NULL)  {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "InitJniEnv kJavaBridgeClass NULL");
        return false;
    }
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "InitJniEnv3");
    kJavaContextClass = reinterpret_cast<jclass>(env->NewGlobalRef(temp_java_context_cls));
    kFieldIDStack = env->GetFieldID(kJavaContextClass, "stack", "Ljava/lang/String;");
    kFieldIDThreadName = env->GetFieldID(kJavaContextClass, "threadName", "Ljava/lang/String;");
    if (kFieldIDStack == NULL || kFieldIDThreadName == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "InitJniEnv kJavaContextClass field NULL");
        return false;
    }

    __android_log_print(ANDROID_LOG_DEBUG, kTag, "InitJniEnv4");
    kMethodIDGetJavaContext = env->GetStaticMethodID(kJavaBridgeClass, "getJavaContext", "()Lcom/lanshifu/iomonitordemo/IOCanaryJniBridge$JavaContext;");
    if (kMethodIDGetJavaContext == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "InitJniEnv kMethodIDGetJavaContext NULL");
        return false;
    }

    return true;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved){
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "JNI_OnLoad");
    kInitSuc = false;


    if (!InitJIniEnvn(vm)) {
        return -1;
    }

    kInitSuc = true;
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "JNI_OnLoad done");
    return JNI_VERSION_1_6;
}


int64_t GetTickCountMicros() {
    struct timespec ts;
    int result = clock_gettime(CLOCK_BOOTTIME, &ts);

    if (result != 0) {
        // XXX: there was an error, probably because the driver didn't
        // exist ... this should return
        // a real error, like an exception!
        return 0;
    }
    return (int64_t)ts.tv_sec*1000000 + (int64_t)ts.tv_nsec/1000;
}

int ProxyOpen(const char *pathname, int flags, mode_t mode) {
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen start,name=%s",pathname);
    int ret = original_open(pathname, flags, mode);
    if (ret != -1) {
        //todo 获取线程堆栈
        printStack();
    }
    //调用
    return ret;
}

int ProxyOpen64(const char *pathname, int flags, mode_t mode) {
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen64 start,name=%s",pathname);
    int ret = original_open64(pathname, flags, mode);
    //todo 获取线程堆栈
    if (ret != -1) {
        printStack();
    }
    return ret;
}

/**
 *  Proxy for read: callback to the java layer
 */
ssize_t ProxyRead(int fd, void *buf, size_t size) {
    int64_t start = GetTickCountMicros();
    size_t ret = original_read(fd, buf, size);
    int64_t read_cost_us = GetTickCountMicros() - start;
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyRead fd:%d,size=%d,read_cost_us=%d",fd,size,read_cost_us);
    return ret;
}


ssize_t ProxyReadChk(int fd, void* buf, size_t count, size_t buf_size) {
    int64_t start = GetTickCountMicros();
    ssize_t ret = original_read_chk(fd, buf, count, buf_size);
    int64_t read_cost_us = GetTickCountMicros() - start;
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyRead fd:%d buf:%p buf_size:%d ret:%d cost:%d", fd, buf, buf_size, ret, read_cost_us);
    return ret;
}


ssize_t ProxyWriteChk(int fd, const void* buf, size_t count, size_t buf_size) {
    int64_t start = GetTickCountMicros();
    ssize_t ret = original_write_chk(fd, buf, count, buf_size);
    int64_t write_cost_us = GetTickCountMicros() - start;
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyWrite fd:%d buf:%p buf_size:%d ret:%d cost:%d", fd, buf, buf_size, ret, write_cost_us);
    return ret;
}

/**
 *  Proxy for write: callback to the java layer
 */
ssize_t ProxyWrite(int fd, const void *buf, size_t size) {
    int64_t start = GetTickCountMicros();
    size_t ret = original_write(fd, buf, size);
    long write_cost_us = GetTickCountMicros() - start;
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyWrite fd:%d buf:%p size:%d ret:%d cost:%d", fd, buf, size, ret, write_cost_us);
    return ret;
}

int ProxyClose(int fd) {
    int ret = original_close(fd);
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyClose fd:%d ret:%d", fd, ret);
    return ret;
}


extern "C" JNIEXPORT void JNICALL
Java_com_lanshifu_iomonitordemo_IOMonitor_doHook(JNIEnv *env, jobject thiz) {


    //1、打开xhook日志
    xhook_enable_debug(1);

    //2、注册hook信息
    for (int i = 0; i < TARGET_MODULE_COUNT; ++i) {
        const char *so_name = TARGET_MODULES[i];
        __android_log_print(ANDROID_LOG_INFO, kTag, "try to hook function in %s.", so_name);

        xhook_register(so_name, "open", (void *) ProxyOpen, (void **) &original_open);
        xhook_register(so_name, "open64", (void *) ProxyOpen64, (void **) &original_open64);

        xhook_register(so_name, "read", (void*)ProxyRead, (void**)&original_read);
        xhook_register(so_name, "__read_chk", (void*)ProxyReadChk, (void**)&original_read_chk);

        xhook_register(so_name, "write", (void*)ProxyWrite, (void**)&original_write);
        xhook_register(so_name, "__write_chk", (void*)ProxyWriteChk, (void**)&original_write_chk);

        xhook_register(so_name, "close", (void*)ProxyClose, (void**)&original_close);
    }

    //3、执行 hook,同步
    xhook_refresh(0);


}