package com.lanshifu.iomonitordemo

import android.app.Application
import android.content.Context
import me.weishu.reflection.Reflection

/**
 * @author lanxiaobin
 * @date 7/1/21
 */
class MainApplication : Application() {

    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        Reflection.unseal(base)
    }
}