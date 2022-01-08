package com.lanshifu.iomonitordemo

/**
 * @author lanxiaobin
 * @date 2022/1/8
 */
class IOMonitor {

    init {
        System.loadLibrary("native-lib")
    }

    external fun doHook()

}