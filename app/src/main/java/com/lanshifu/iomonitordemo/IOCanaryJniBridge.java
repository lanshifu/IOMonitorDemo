/*
 * Tencent is pleased to support the open source community by making wechat-matrix available.
 * Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the BSD 3-Clause License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.lanshifu.iomonitordemo;

import com.tencent.matrix.iocanary.config.IOConfig;
import com.tencent.matrix.iocanary.core.IOIssue;
import com.tencent.matrix.iocanary.core.OnJniIssuePublishListener;
import com.tencent.matrix.iocanary.util.IOCanaryUtil;
import com.tencent.matrix.util.MatrixLog;

import java.util.ArrayList;

/**
 * Created by liyongjie on 2017/12/8.
 */

public class IOCanaryJniBridge {
    private static final String TAG = "Matrix.IOCanaryJniBridge";

    /**
     * 给native层调用，获取线程堆栈
     */
    private static final class JavaContext {
        private final String stack;
        private String threadName;

        private JavaContext() {
            stack = IOCanaryUtil.getThrowableStack(new Throwable());
            if (null != Thread.currentThread()) {
                threadName = Thread.currentThread().getName();
            }
        }
    }

    /**
     * 声明为private，给c++部分调用！！！不要干掉！！！
     * @return
     */
    private static JavaContext getJavaContext() {
        try {
            return new JavaContext();
        } catch (Throwable th) {
            MatrixLog.printErrStackTrace(TAG, th, "get javacontext exception");
        }

        return null;
    }
}
