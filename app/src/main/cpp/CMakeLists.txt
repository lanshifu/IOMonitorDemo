# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

project("iomonitordemo")

SET(TARGET iomonitordemo)

# import bytehook from maven
find_package(bytehook REQUIRED CONFIG)

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

add_library(
        mmap-lib

        SHARED

        mmap_demo.cpp
)

add_library(
        bhook-lib

        SHARED

        bhook_demo.cpp
)

find_library(
        log-lib
        log)

target_link_libraries(
        native-lib

        ${log-lib})

target_link_libraries(
        mmap-lib

        ${log-lib})

#target_link_libraries(
#        bhook-lib
#        bytehook
#        ${log-lib})
target_link_libraries(bhook-lib bytehook::bytehook ${log-lib})