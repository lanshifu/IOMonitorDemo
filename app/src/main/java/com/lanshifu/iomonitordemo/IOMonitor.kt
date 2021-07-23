package com.lanshifu.iomonitordemo

import android.annotation.SuppressLint
import android.util.Log
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.FileReader
import java.lang.reflect.Proxy

/**
 * @author lanxiaobin
 * @date 6/28/21
 */
object IOMonitor {

    val TAG = "IOMonitor"

    /**
     * 模拟器文件没关闭
     */
    fun testInputStreamNeverClose() {

        val dir: String = MainApplication.context?.externalCacheDir?.absolutePath ?: ""
        val f = File(dir, "log.txt")
        if (!f.exists()) {
            f.createNewFile()
        }
        f.writeText("text1")
        f.writeText(this.javaClass.name)
//        this.javaClass.declaredMethods.forEach {
//            f.writeText(it.toString())
//        }

        val r = FileReader(f)
        val read = r.read()
        Log.d(TAG, "read=$read")
//        r.close()


    }

    fun testFileInputStream() {

        val startTime = System.currentTimeMillis()
        val dir: String = MainApplication.context?.externalCacheDir?.absolutePath ?: ""
        val fromFile = "$dir/log.txt"
        val toFile = "$dir/log_copy2.txt"

        val fis = FileInputStream(fromFile)
        val fos = FileOutputStream(toFile)
        val arr = ByteArray(1024 * 4)
        var len: Int = fis.read(arr)
        while ((fis.read(arr).also { len = it }) != -1) {
            fos.write(arr, 0, len)
            fos.write(arr)
            len = fis.read(arr)
        }
//        fis.close()
//        fos.close()
        Log.i(TAG, "testFileInputStream: const:${System.currentTimeMillis() - startTime} ms")
    }

    fun start() {
        tryHookCloseGuard()
    }

    /**
     *
     * Use a way of dynamic proxy to hook
     *
     * @return
     */
    @SuppressLint("SoonBlockedPrivateApi")
    private fun tryHookCloseGuard(): Boolean {
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