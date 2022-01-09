package com.lanshifu.iomonitordemo

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    var mmapDemo = MmapDemo()
    var ioMonitor = IOMonitor()
    var i= 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        findViewById<TextView>(R.id.btnGc).setOnClickListener {
            //手动触发gc，Class对象的 finalize 会被调用
            Runtime.getRuntime().gc()
        }
        findViewById<TextView>(R.id.btnBlock).setOnClickListener {
            //手动触发gc，Class对象的 finalize 会被调用
            testBlock()
        }

        findViewById<TextView>(R.id.btnRead).setOnClickListener {
            CloseGuardHooker.testInputStreamNeverClose()
//            IOMonitor.testFileInputStream()
        }

        findViewById<TextView>(R.id.btnmmapWrite).setOnClickListener {
            mmapDemo.mmapMapWrite("${i++}")
        }

        CloseGuardHooker.start()
        ioMonitor.doHook()

//        IOMonitor.testInputStreamNeverClose()


        mmapDemo.init(this)

    }

    private fun testBlock() {
        Thread.sleep(10000)
    }

}