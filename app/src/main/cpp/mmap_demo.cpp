
#include <jni.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <android/log.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define TAG    "mmap_demo" // 这个是自定义的LOG的标识
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__) // 定义LOGD类型

char *write_ptr = NULL;

static char *openMMap(int buffer_fd, size_t buffer_size);

extern "C"  jlong
Java_com_lanshifu_iomonitordemo_MmapDemo_initNative(JNIEnv *env, jobject instance, jstring buffer_path_, jint capacity, jstring log_path_) {
    const char *buffer_path = env->GetStringUTFChars(buffer_path_, NULL);
    const char *log_path = env->GetStringUTFChars(log_path_, NULL);
    size_t buffer_size = static_cast<size_t>(capacity);
    //1.打开文件，获得句柄
    int buffer_fd = open(buffer_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    //2.调用mmap，
    char *buffer_ptr = openMMap(buffer_fd, buffer_size);
    env->ReleaseStringUTFChars(buffer_path_, buffer_path);
    env->ReleaseStringUTFChars(log_path_, log_path);
    write_ptr = buffer_ptr;
    LOGD("initNative，write_ptr=%s",write_ptr);
    return reinterpret_cast<long>(buffer_ptr);
}

static char *openMMap(int buffer_fd, size_t buffer_size) {
    char *map_ptr = NULL;
    if (buffer_fd != -1) {
        // 根据 buffer size 调整 buffer 文件大小，ftruncate会将参数fd指定的文件大小改为参数length指定的大小
        ftruncate(buffer_fd, static_cast<int>(buffer_size));
        //lseek是一个用于改变读写一个文件时读写指针位置的一个系统调用,这里就是定位到文件开始位置
        lseek(buffer_fd, 0, SEEK_SET);
        map_ptr = (char *) mmap(0, buffer_size, PROT_WRITE | PROT_READ, MAP_SHARED, buffer_fd, 0);
        if (map_ptr == MAP_FAILED) {
            map_ptr = NULL;
        }
    }
    return map_ptr;
}


extern "C" JNIEXPORT jlong JNICALL
Java_com_lanshifu_iomonitordemo_MmapDemo_writeNative(JNIEnv *env, jobject instance, jlong ptr, jstring log_) {
    const char *log = env->GetStringUTFChars(log_, 0);
    jsize log_len = env->GetStringUTFLength(log_);
    //将log拷贝到write_ptr地址
    memcpy(reinterpret_cast<void *>(write_ptr), log, log_len);
    write_ptr = write_ptr + log_len;

    LOGD("writeNative，write_ptr=%s,log=%s,log_len=%d",write_ptr,log,log_len);
    env->ReleaseStringUTFChars(log_, log);
    return 0;
}
