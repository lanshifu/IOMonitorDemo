package com.lanshifu.iomonitordemo.matrix;

import android.app.Application;

import com.tencent.matrix.Matrix;
import com.tencent.matrix.iocanary.IOCanaryPlugin;
import com.tencent.matrix.iocanary.config.IOConfig;

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
        //add to matrix
        builder.plugin(ioCanaryPlugin);

        //init matrix
        Matrix.init(builder.build());

        // start plugin
        ioCanaryPlugin.start();
    }
}
