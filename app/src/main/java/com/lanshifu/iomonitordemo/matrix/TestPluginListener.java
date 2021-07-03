package com.lanshifu.iomonitordemo.matrix;

import android.content.Context;
import android.util.Log;

import com.tencent.matrix.plugin.DefaultPluginListener;
import com.tencent.matrix.report.Issue;
import com.tencent.matrix.util.MatrixLog;

/**
 * @author lanxiaobin
 * @date 7/4/21
 */
class TestPluginListener extends DefaultPluginListener {

    private String TAG = "TestPluginListener";

    public TestPluginListener(Context context) {
        super(context);
    }

    @Override
    public void onReportIssue(Issue issue) {
        super.onReportIssue(issue);
        MatrixLog.e(TAG, issue.toString());
        Log.e(TAG, "onReportIssue: " + issue.toString() );

        //add your code to process data
    }
}
