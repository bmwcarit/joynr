package io.joynr.joynrandroidruntime;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;

import java.util.Arrays;
import java.util.Collections;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ExecutionException;

import joynr.types.ProviderQos;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Messenger;

import com.google.common.collect.Sets;
import com.google.inject.Module;

public class JoynrAndroidRuntime implements JoynrRuntime {

    protected InitRuntimeTask runtimeInitTask;
    protected static UILogger uiLogger = new UILogger();

    public JoynrAndroidRuntime(Context applicationContext) {
        runtimeInitTask = new InitRuntimeTask(applicationContext, uiLogger);
        runtimeInitTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public JoynrAndroidRuntime(Context applicationContext, Properties joynrConfig) {
        runtimeInitTask = new InitRuntimeTask(joynrConfig,
                                              applicationContext,
                                              uiLogger,
                                              Collections.<Module> emptyList());
        runtimeInitTask.execute();
    }

    public JoynrAndroidRuntime(Context applicationContext, Properties joynrConfig, Module... joynrModules) {
        runtimeInitTask = new InitRuntimeTask(joynrConfig, applicationContext, uiLogger, Arrays.asList(joynrModules));
        runtimeInitTask.execute();
    }

    private JoynrRuntime getJoynrRuntime() {
        // this will block until the runtime is created successfully
        // TODO since the caller expects the register call to be async, we need to check if
        // this will not block to long
        JoynrRuntime runtime;
        try {
            runtime = runtimeInitTask.get();
        } catch (InterruptedException e) {
            e.printStackTrace();
            uiLogger.logText(e.getMessage());
            return null;
        } catch (ExecutionException e) {
            e.printStackTrace();
            uiLogger.logText(e.getMessage());
            return null;
        }
        return runtime;
    }

    /**
     * Registers an Android provider in the joynr framework
     *
     * @deprecated Will be removed by end of the year 2016. Use {@link io.joynr.joynrandroidruntime.JoynrAndroidRuntime#
     * registerProvider(String, JoynrProvider, ProviderQos)} instead.
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @return Returns a Future which can be used to check the registration status.
     */
    @Deprecated
    @Override
    public Future<Void> registerProvider(String domain, AbstractJoynrProvider provider) {
        // this will block until the runtime is created successfully
        // TODO since the caller expects the register call to be async, we need to check if
        // this will not block to long
        JoynrRuntime runtime = getJoynrRuntime();

        // registration of providers is asynchronously
        return runtime.registerProvider(domain, provider, provider.getProviderQos());
    }

    @Override
    public Future<Void> registerProvider(String domain, Object provider, ProviderQos providerQos) {
        // this will block until the runtime is created successfully
        // TODO since the caller expects the register call to be async, we need to check if
        // this will not block to long
        JoynrRuntime runtime = getJoynrRuntime();

        // registration of providers is asynchronously
        return runtime.registerProvider(domain, provider, providerQos);
    }

    @Override
    public void unregisterProvider(String domain, Object provider) {
        // this will block until the runtime is created successfully
        // TODO since the caller expects the unregister call to be async, we need to check if
        // this will not block to long
        JoynrRuntime runtime = getJoynrRuntime();

        runtime.unregisterProvider(domain, provider);
    }

    @Override
    public <T> ProxyBuilder<T> getProxyBuilder(String domain, Class<T> interfaceClass) {
        return new AndroidProxyBuilder<T>(runtimeInitTask, Sets.newHashSet(domain), interfaceClass, uiLogger);
    }

    public void addLogListener(Messenger clientMessenger) {
        uiLogger.addLogListener(clientMessenger);
    }

    public void removeLogListener(Messenger clientMessanger) {
        uiLogger.removeLogListener(clientMessanger);
    }

    @Override
    public void shutdown(boolean clear) {
        JoynrRuntime runtime = getJoynrRuntime();
        runtime.shutdown(clear);
    }

    @Override
    public <T> ProxyBuilder<T> getProxyBuilder(Set<String> domains, Class<T> interfaceClass) {
        return new AndroidProxyBuilder<T>(runtimeInitTask, domains, interfaceClass, uiLogger);
    }

}
