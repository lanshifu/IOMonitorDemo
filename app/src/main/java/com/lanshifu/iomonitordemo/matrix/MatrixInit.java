package com.lanshifu.iomonitordemo.matrix;

import android.app.Application;

import com.tencent.matrix.Matrix;
import com.tencent.matrix.iocanary.IOCanaryPlugin;
import com.tencent.matrix.iocanary.config.IOConfig;
import com.tencent.matrix.trace.TracePlugin;
import com.tencent.matrix.trace.config.TraceConfig;

/**
 * @author lanxiaobin
 * @date 7/4/21
 */
public class MatrixInit {

    public static void init(Application application) {
        Matrix.Builder builder = new Matrix.Builder(application); // build matrix
        builder.patchListener(new TestPluginListener(application)); // add general pluginListener
        DynamicConfigImplDemo dynamicConfig = new DynamicConfigImplDemo(); // dynamic config

        // init plugin
        IOCanaryPlugin ioCanaryPlugin = new IOCanaryPlugin(new IOConfig.Builder()
                .dynamicConfig(dynamicConfig)
                .build());

        //trace
        TraceConfig traceConfig = new TraceConfig.Builder()
                .dynamicConfig(dynamicConfig)
                .enableFPS(true)
                .enableEvilMethodTrace(true)
                .enableAnrTrace(true)
                .enableStartup(true)
                .isDebug(true)
                .isDevEnv(true)
                .build();
        TracePlugin tracePlugin = new TracePlugin(traceConfig);
        //add to matrix
        builder.plugin(ioCanaryPlugin);
        builder.plugin(tracePlugin);

        //init matrix
        Matrix.init(builder.build());

        // start plugin
        ioCanaryPlugin.start();
        tracePlugin.start();
    }
}
