#include <jni.h>
#include <string>
#include "xhook.h"
#include "../../../../../../../../Library/Android/sdk/ndk/21.1.6352462/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/sys/stat.h"
#include <android/log.h>
#include <unordered_map>
#include <unistd.h>

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

// 获取系统的当前时间，单位微秒(us)
int64_t GetSysTimeMicros() {
#ifdef _WIN32
    // 从1601年1月1日0:0:0:000到1970年1月1日0:0:0:000的时间(单位100ns)
#define EPOCHFILETIME   (116444736000000000UL)
		FILETIME ft;
		LARGE_INTEGER li;
		int64_t tt = 0;
		GetSystemTimeAsFileTime(&ft);
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;
		// 从1970年1月1日0:0:0:000到现在的微秒数(UTC时间)
		tt = (li.QuadPart - EPOCHFILETIME) / 10;
		return tt;
#else
    timeval tv;
    gettimeofday(&tv, 0);
    return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
#endif // _WIN32
}

class JavaContext {
public:
    JavaContext(intmax_t thread_id , const std::string& thread_name, const std::string& stack)
            : thread_id_(thread_id), thread_name_(thread_name), stack_(stack) {
    }

    const intmax_t thread_id_;
    const std::string thread_name_;
    const std::string stack_;
};

//rw short for read/write operation
class IOInfo {
public:
    IOInfo(const std::string path, const JavaContext java_context)
            : start_time_μs_(GetSysTimeMicros())
            , path_(path), java_context_(java_context) {
    }

    const std::string path_;
    const JavaContext java_context_;

    int64_t start_time_μs_;
    int op_cnt_ = 0;
    long buffer_size_ = 0;
    long op_size_ = 0;
    long rw_cost_us_ = 0;
    long max_continual_rw_cost_time_μs_ = 0;
    long max_once_rw_cost_time_μs_ = 0;
    long current_continual_rw_time_μs_ = 0;
    int64_t last_rw_time_μs_ = 0;
    long file_size_ = 0;

    long total_cost_μs_ = 0;
};

//constexpr static const char* kTag = "IOCanary.native.FileIOInfoCollector";
constexpr static const int kContinualThreshold = 8*1000;//in μs， half of 16.6667

intmax_t GetCurrentThreadId() {
    return gettid();
}

int GetFileSize(const char* file_path) {
    struct stat stat_buf;
    if (-1 == stat(file_path, &stat_buf)) {
        return -1;
    }
    return stat_buf.st_size;
}

std::unordered_map<int, std::shared_ptr<IOInfo>> info_map_;

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

const JavaContext& getJavaContext(){
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "getJavaContext");
    JNIEnv* env = NULL;
    kJvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jobject java_context_obj = env->CallStaticObjectMethod(kJavaBridgeClass, kMethodIDGetJavaContext);
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "CallStaticObjectMethod end");
    jstring j_stack = (jstring) env->GetObjectField(java_context_obj, kFieldIDStack);
    jstring j_thread_name = (jstring) env->GetObjectField(java_context_obj, kFieldIDThreadName);

    char* thread_name = jstringToChars(env, j_thread_name);
    char* stack = jstringToChars(env, j_stack);
    __android_log_print(ANDROID_LOG_ERROR, kTag, "getJavaContext...");
    __android_log_print(ANDROID_LOG_ERROR, kTag, "thread_name=%s",thread_name);
    __android_log_print(ANDROID_LOG_ERROR, kTag, "stack=%s",stack);

    JavaContext java_context(GetCurrentThreadId(), thread_name == NULL ? "" : thread_name, stack == NULL ? "" : stack);
    free(stack);
    free(thread_name);

    env->DeleteLocalRef(java_context_obj);
    env->DeleteLocalRef(j_stack);
    env->DeleteLocalRef(j_thread_name);
    return java_context;
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

void CountRWInfo(int fd,  long op_size, long rw_cost) {
    if (info_map_.find(fd) == info_map_.end()) {
        return;
    }

    const int64_t now = GetSysTimeMicros();

    info_map_[fd]->op_cnt_ ++;
    info_map_[fd]->op_size_ += op_size;
    info_map_[fd]->rw_cost_us_ += rw_cost;

    if (rw_cost > info_map_[fd]->max_once_rw_cost_time_μs_) {
        info_map_[fd]->max_once_rw_cost_time_μs_ = rw_cost;
    }

    //__android_log_print(ANDROID_LOG_DEBUG, kTag, "CountRWInfo rw_cost:%d max_once_rw_cost_time_:%d current_continual_rw_time_:%d;max_continual_rw_cost_time_:%d; now:%lld;last:%lld",
    //      rw_cost, info_map_[fd]->max_once_rw_cost_time_μs_, info_map_[fd]->current_continual_rw_time_μs_, info_map_[fd]->max_continual_rw_cost_time_μs_, now, info_map_[fd]->last_rw_time_ms_);

    if (info_map_[fd]->last_rw_time_μs_ > 0 && (now - info_map_[fd]->last_rw_time_μs_) < kContinualThreshold) {
        info_map_[fd]->current_continual_rw_time_μs_ += rw_cost;

    } else {
        info_map_[fd]->current_continual_rw_time_μs_ = rw_cost;
    }
    if (info_map_[fd]->current_continual_rw_time_μs_ > info_map_[fd]->max_continual_rw_cost_time_μs_) {
        info_map_[fd]->max_continual_rw_cost_time_μs_ = info_map_[fd]->current_continual_rw_time_μs_;
    }
    info_map_[fd]->last_rw_time_μs_ = now;

    if (info_map_[fd]->buffer_size_ < op_size) {
        info_map_[fd]->buffer_size_ = op_size;
    }

}

int ProxyOpen(const char *pathname, int flags, mode_t mode) {
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen start,name=%s",pathname);
    int ret = original_open(pathname, flags, mode);
    if (ret != -1) {
        //获取线程堆栈
        if (info_map_.find(ret) == info_map_.end()) {
            std::shared_ptr<IOInfo> info = std::make_shared<IOInfo>(pathname, getJavaContext());
            info_map_.insert(std::make_pair(ret, info));
        }
    }
    //调用
    return ret;
}

int ProxyOpen64(const char *pathname, int flags, mode_t mode) {
    __android_log_print(ANDROID_LOG_INFO, kTag, "ProxyOpen64 start,name=%s",pathname);
    int ret = original_open64(pathname, flags, mode);
    //获取线程堆栈
    if (ret != -1) {
        //获取线程堆栈
        if (info_map_.find(ret) == info_map_.end()) {
            std::shared_ptr<IOInfo> info = std::make_shared<IOInfo>(pathname, getJavaContext());
        }
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

    if (ret == -1 || ret < 0) {
        return ret;
    }

    if (info_map_.find(fd) == info_map_.end()) {
        //__android_log_print(ANDROID_LOG_DEBUG, kTag, "OnRead fd:%d not in info_map_", fd);
        return ret;
    }
    CountRWInfo(fd, size, read_cost_us);

    return ret;
}


ssize_t ProxyReadChk(int fd, void* buf, size_t count, size_t size) {
    int64_t start = GetTickCountMicros();
    ssize_t ret = original_read_chk(fd, buf, count, size);
    int64_t read_cost_us = GetTickCountMicros() - start;
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyRead fd:%d buf:%p buf_size:%d ret:%d cost:%d", fd, buf, size, ret, read_cost_us);

    if (ret == -1 || ret < 0) {
        return ret;
    }

    if (info_map_.find(fd) == info_map_.end()) {
        //__android_log_print(ANDROID_LOG_DEBUG, kTag, "OnRead fd:%d not in info_map_", fd);
        return ret;
    }
    CountRWInfo(fd, size, read_cost_us);

    return ret;
}


ssize_t ProxyWriteChk(int fd, const void* buf, size_t count, size_t size) {
    int64_t start = GetTickCountMicros();
    ssize_t ret = original_write_chk(fd, buf, count, size);
    int64_t write_cost_us = GetTickCountMicros() - start;
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyWrite fd:%d buf:%p buf_size:%d ret:%d cost:%d", fd, buf, size, ret, write_cost_us);
    if (ret == -1 || ret < 0) {
        return ret;
    }

    if (info_map_.find(fd) == info_map_.end()) {
        //__android_log_print(ANDROID_LOG_DEBUG, kTag, "OnRead fd:%d not in info_map_", fd);
        return ret;
    }
    CountRWInfo(fd, size, write_cost_us);
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
    if (ret == -1 || ret < 0) {
        return ret;
    }
    if (info_map_.find(fd) == info_map_.end()) {
        //__android_log_print(ANDROID_LOG_DEBUG, kTag, "OnRead fd:%d not in info_map_", fd);
        return ret;
    }
    CountRWInfo(fd, size, write_cost_us);
    return ret;
}

int ProxyClose(int fd) {
    int ret = original_close(fd);
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyClose fd:%d ret:%d", fd, ret);


    if (info_map_.find(fd) == info_map_.end()) {
        __android_log_print(ANDROID_LOG_DEBUG, kTag, "OnClose fd:%d not in info_map_", fd);
        return ret;
    }

    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyClose collecting...");
    info_map_[fd]->total_cost_μs_ = GetSysTimeMicros() - info_map_[fd]->start_time_μs_;
    info_map_[fd]->file_size_ = GetFileSize(info_map_[fd]->path_.c_str());
    std::shared_ptr<IOInfo> info = info_map_[fd];

    __android_log_print(ANDROID_LOG_DEBUG, kTag, "ProxyClose fd:%d ret:%d,total_cost_μs_=%d,file_size_=%d,buffer_size_=%d",
                        fd, ret,info_map_[fd]->total_cost_μs_,info_map_[fd]->file_size_,info_map_[fd]->buffer_size_);
    info_map_.erase(fd);
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