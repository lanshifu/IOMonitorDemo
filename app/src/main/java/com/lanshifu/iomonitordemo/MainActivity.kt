package com.lanshifu.iomonitordemo

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        findViewById<TextView>(R.id.sample_text).text = stringFromJNI()
        findViewById<TextView>(R.id.btnGc).setOnClickListener {
            //手动触发gc，Class对象的 finalize 会被调用
            Runtime.getRuntime().gc()
        }

        IOMonitor.start()
        doHook()

        IOMonitor.testInputStreamNeverClose()

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun doHook()

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}