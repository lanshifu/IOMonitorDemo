# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

project("iomonitordemo")

SET(TARGET iomonitordemo)

add_library(
        native-lib

        SHARED

        native-lib.cpp
        xhook.c
        xh_core.c
        xh_elf.c
        xh_jni.c
        xh_log.c
        xh_util.c
        xh_version.c
)


find_library(
        log-lib
        log)

target_link_libraries(
        native-lib

        ${log-lib})