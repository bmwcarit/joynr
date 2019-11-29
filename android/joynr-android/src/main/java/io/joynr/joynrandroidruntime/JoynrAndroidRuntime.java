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
package io.joynr.joynrandroidruntime;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ExecutionException;

import org.apache.commons.lang.NotImplementedException;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Messenger;
import android.util.Log;

import com.google.inject.Module;

import io.joynr.proxy.Future;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.runtime.JoynrRuntime;
import joynr.types.ProviderQos;

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
        } catch (ExecutionException | InterruptedException e) {
            Log.e("JAS", "joynr runtime not started", e);
            uiLogger.logText(e.getMessage());
            return null;
        }
        return runtime;
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
    public Future<Void> registerProvider(String domain, Object provider, ProviderQos providerQos, boolean awaitGlobalRegistration) {
        // this will block until the runtime is created successfully
        // TODO since the caller expects the register call to be async, we need to check if
        // this will not block to long
        JoynrRuntime runtime = getJoynrRuntime();

        // registration of providers is asynchronously
        return runtime.registerProvider(domain, provider, providerQos, awaitGlobalRegistration);
    }

    @Override
    public Future<Void> registerProvider(String domain,
            Object provider,
            ProviderQos providerQos,
            boolean awaitGlobalRegistration,
            final Class<?> interfaceClass) {
        JoynrRuntime runtime = getJoynrRuntime();
        return runtime.registerProvider(domain, provider, providerQos, awaitGlobalRegistration, interfaceClass);
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
        return new AndroidProxyBuilder<T>(runtimeInitTask, new HashSet(Arrays.asList(domain)), interfaceClass, uiLogger);
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

    @Override
    public void registerStatelessAsyncCallback(StatelessAsyncCallback statelessAsyncCallback) {
        JoynrRuntime runtime = getJoynrRuntime();
        runtime.registerStatelessAsyncCallback(statelessAsyncCallback);
    }

    @Override
    public void prepareForShutdown() {
        JoynrRuntime runtime = getJoynrRuntime();
        runtime.prepareForShutdown();
    }

    @Override
    public GuidedProxyBuilder getGuidedProxyBuilder(Set<String> domains, Class<?> interfaceClass) {
        throw new NotImplementedException();
    }

}
