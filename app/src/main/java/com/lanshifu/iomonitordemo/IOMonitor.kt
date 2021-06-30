package com.lanshifu.iomonitordemo

import android.annotation.SuppressLint
import android.util.Log
import java.io.*
import java.lang.reflect.Proxy

/**
 * @author lanxiaobin
 * @date 6/28/21
 */
object IOMonitor {

    /**
     * 模拟器文件没关闭
     */
    fun testInputStreamNeverClose() {

        val f = File("/data/data/com.lanshifu.iomonitordemo/hello.text")
        if (!f.exists()) {
            f.createNewFile()
        }

        val r = FileReader(f)
//        r.close()

    }

    fun start() {
        tryHook()
    }

    /**
     *
     * Use a way of dynamic proxy to hook
     *
     * @return
     */
    @SuppressLint("SoonBlockedPrivateApi")
    private fun tryHook(): Boolean {
        try {
            val closeGuardCls = Class.forName("dalvik.system.CloseGuard")
            val closeGuardReporterCls = Class.forName("dalvik.system.CloseGuard\$Reporter")
            closeGuardCls.declaredMethods.forEach {
                Log.d("TAG", "tryHook: ${it.name}")
            }
            // 非公开API
            val methodGetReporter = closeGuardCls.getDeclaredMethod("getReporter")
            val methodSetReporter =
                closeGuardCls.getDeclaredMethod("setReporter", closeGuardReporterCls)
            val methodSetEnabled =
                closeGuardCls.getDeclaredMethod("setEnabled", Boolean::class.java)
            val sOriginalReporter =
                methodGetReporter.invoke(null)
            methodSetEnabled.invoke(null, true)

            val classLoader = closeGuardReporterCls.classLoader ?: return false
            methodSetReporter.invoke(
                null, Proxy.newProxyInstance(
                    classLoader, arrayOf(closeGuardReporterCls),
                    IOCloseLeakDetector(
                        sOriginalReporter
                    )
                )
            )
            return true
        } catch (e: Throwable) {
            e.printStackTrace()
        }
        return false
    }

}