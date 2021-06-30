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

import android.util.Log;


import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;


public class IOCloseLeakDetector implements InvocationHandler {
    private static final String TAG = "IOCloseLeakDetector";
    private static final int DEFAULT_MAX_STACK_LAYER = 10;

    private final Object originalReporter;

    public IOCloseLeakDetector(Object originalReporter) {
        this.originalReporter = originalReporter;
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        Log.i(TAG, "invoke method: " + method.getName());
        if (method.getName().equals("report")) {
            if (args.length != 2) {
                Log.e(TAG, "invoke: args.length != 2" );
                return null;
            }
            if (!(args[1] instanceof Throwable)) {
                Log.e(TAG, "closeGuard report args 1 should be throwable" );
                return null;
            }
            Throwable throwable = (Throwable) args[1];

            String stack = stackTraceToString(throwable.getStackTrace());
            Log.e(TAG, "io流没关闭，stack= \n\r" + stack );
            return null;
        }
        return method.invoke(originalReporter, args);
    }

    public static String stackTraceToString(final StackTraceElement[] arr) {
        if (arr == null) {
            return "";
        }

        StringBuffer sb = new StringBuffer();
        for (StackTraceElement stackTraceElement : arr) {
            String className = stackTraceElement.getClassName();
            sb.append(stackTraceElement).append('\n');
        }
        return sb.toString();
    }

}
