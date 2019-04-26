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
package io.joynr.android.test;

import java.util.Properties;

import android.content.Context;

import com.google.inject.Module;

import io.joynr.joynrandroidruntime.JoynrAndroidRuntime;
import io.joynr.proxy.ProxyBuilder;

public class JoynrAndroidRobolectricRuntime extends JoynrAndroidRuntime {
    public JoynrAndroidRobolectricRuntime(Context applicationContext) {
        super(applicationContext);
    }

    public JoynrAndroidRobolectricRuntime(Context applicationContext, Properties joynrConfig) {
        super(applicationContext, joynrConfig);
    }

    public JoynrAndroidRobolectricRuntime(Context applicationContext, Properties joynrConfig, Module... joynrModules) {
        super(applicationContext, joynrConfig, joynrModules);
    }

    @Override
    public <T> ProxyBuilder<T> getProxyBuilder(String domain, Class<T> interfaceClass) {
        return new AndroidRobolectricProxyBuilder<T>(runtimeInitTask, domain, interfaceClass, uiLogger);
    }
}
