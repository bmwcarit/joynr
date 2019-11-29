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

import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.apache.commons.lang.NotImplementedException;

import android.os.AsyncTask;
import android.util.Log;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;

public class AndroidProxyBuilder<T> extends AsyncTask<Object, String, T> implements ProxyBuilder<T> {

    private JoynrRuntime runtime;
    private Set<String> providerDomains;
    private MessagingQos messagingQos;
    private DiscoveryQos discoveryQos;
    private UILogger uiLogger;
    private ProxyCreatedCallback<T> callback = null;
    Class<T> proxyInterface;
    private String participantId = null;
    private ProxyBuilder<T> builder = null;
    private InitRuntimeTask runtimeInitTask;
    private String statelessAsyncCallbackUseCase;

    public AndroidProxyBuilder(InitRuntimeTask runtimeInitTask,
                               Set<String> domains,
                               Class<T> proxyInterface,
                               UILogger uiLogger) {
        super();
        this.runtimeInitTask = runtimeInitTask;

        this.providerDomains = domains;
        this.proxyInterface = proxyInterface;
        this.uiLogger = uiLogger;
    }

    @Override
    protected T doInBackground(Object... arg0) {

        Log.d("JAS", "starting CreateProxy");
        try {
            return buildProxy();
        } catch (InterruptedException | ExecutionException | TimeoutException e) {
            if (callback != null) {
                callback.onProxyCreationError(new JoynrRuntimeException(e));
            }
            return null;
        }
    }

    protected T buildProxy() throws InterruptedException, ExecutionException, TimeoutException {
        this.runtime = runtimeInitTask.get(discoveryQos.getDiscoveryTimeoutMs(), TimeUnit.MILLISECONDS);
        builder = runtime.getProxyBuilder(providerDomains, proxyInterface);
        if (participantId != null) {
            builder.setParticipantId(participantId);
        }
        T proxy = builder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build(callback);
        Log.d("JAS", "Returning Proxy");
        return proxy;
    }

    @Override
    protected void onProgressUpdate(String... progress) {
        uiLogger.logText(progress);
    }

    @Override
    protected void onPostExecute(T result) {
        if (result != null && callback != null) {
            Log.d("JAS", "calling onProxyCreated Callback");
            callback.onProxyCreationFinished(result);
        }
    }

    @Override
    public String getParticipantId() {
        if (builder != null) {
            participantId = builder.getParticipantId();
        }
        return participantId;
    }

    @Override
    public void setParticipantId(String participantId) {
        this.participantId = participantId;
        if (builder != null) {
            builder.setParticipantId(participantId);
        }

    }

    @Override
    public ProxyBuilder<T> setDiscoveryQos(DiscoveryQos discoveryQos) throws DiscoveryException {
        this.discoveryQos = discoveryQos;
        return this;
    }

    @Override
    public ProxyBuilder<T> setMessagingQos(MessagingQos messagingQos) {
        this.messagingQos = messagingQos;
        return this;
    }

    @Override
    public ProxyBuilder<T> setStatelessAsyncCallbackUseCase(String useCase) {
        this.statelessAsyncCallbackUseCase = statelessAsyncCallbackUseCase;
        return this;
    }

    @Override
    public T build() {
        throw new UnsupportedOperationException("On Android, only method signature public void build(ProxyCreatedCallback<T> newCallback) is supported");
    }

    @Override
    public T build(ProxyCreatedCallback<T> newCallback) {
        this.callback = newCallback;
        this.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        return null;

    }

    @Override
    public T build(ArbitrationResult arbitrationResult) {
        throw new NotImplementedException();
    }
}
