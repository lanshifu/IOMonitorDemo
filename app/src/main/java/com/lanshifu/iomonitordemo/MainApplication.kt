package com.lanshifu.iomonitordemo

import android.app.Application
import android.content.Context
import com.lanshifu.iomonitordemo.matrix.MatrixInit
import me.weishu.reflection.Reflection

/**
 * @author lanxiaobin
 * @date 7/1/21
 */
class MainApplication : Application() {

    companion object {
        var context:Context? = null
    }
    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        Reflection.unseal(base)

        context = base
    }

    override fun onCreate() {
        super.onCreate()
//        initMatrix()

//        BHookSdk.init()
    }

    private fun initMatrix() {
        MatrixInit.init(this)
    }


}