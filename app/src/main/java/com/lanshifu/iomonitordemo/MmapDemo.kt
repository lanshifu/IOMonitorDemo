package com.lanshifu.iomonitordemo

import android.content.Context
import android.os.Environment
import java.io.File


/**
 * @author lanxiaobin
 * @date 2021/8/2.
 */
class MmapDemo {

    init {
        System.loadLibrary("mmap-lib")
    }

    private val FOLDER_NAME = "log_folder"
    private val LOG_FILE_NAME = "log.txt"

    private var defaultBufferPath: String? = null
    private var mContext: Context? = null


    fun init(context:Context){
        mContext = context
        defaultBufferPath = getDefaultBufferPath(mContext!!);
        initNative(defaultBufferPath!!, 4096, defaultBufferPath!!);
    }

    fun mmapMapWrite(text:String){
        writeNative(0L,text)
    }


    private fun getDefaultBufferPath(context: Context): String? {
        val bufferFile: File? = context.getExternalFilesDir(FOLDER_NAME)
        if (bufferFile != null && !bufferFile.exists()) {
            bufferFile.mkdirs()
        }
        return File(bufferFile, LOG_FILE_NAME).getAbsolutePath()
    }

    external fun initNative( bufferPath:String,  capacity:Int,  logPath:String) : Long
    external fun writeNative( ptr:Long,  log:String) : Long
}