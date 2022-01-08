package com.lanshifu.iomonitordemo.bhook

import com.bytedance.android.bytehook.ByteHook




/**
 * @author lanxiaobin
 * @date 2021/9/10
 */
object BHookSdk {

    fun init() {
        ByteHook.init()

        dohook()

    }

    init {
        System.loadLibrary("bhook-lib")
    }

    external fun dohook()
}