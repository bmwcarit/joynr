package io.joynr.android.test;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

import com.google.common.collect.Sets;

import android.util.Log;

import io.joynr.joynrandroidruntime.AndroidProxyBuilder;
import io.joynr.joynrandroidruntime.InitRuntimeTask;
import io.joynr.joynrandroidruntime.UILogger;

public class AndroidRobolectricProxyBuilder<T> extends AndroidProxyBuilder<T> {

    public AndroidRobolectricProxyBuilder(InitRuntimeTask runtimeInitTask,
                                          String providerDomain,
                                          Class<T> proxyInterface,
                                          UILogger uiLogger) {
        super(runtimeInitTask, Sets.newHashSet(providerDomain), proxyInterface, uiLogger);
    }

    @Override
    public T build() {
        T proxy = null;
        try {
            proxy = buildProxy();
        } catch (InterruptedException | ExecutionException | TimeoutException e) {
            Log.e("JAS", e.getMessage(), e);
            publishProgress(e.getMessage());
        }
        return proxy;
    }
}
