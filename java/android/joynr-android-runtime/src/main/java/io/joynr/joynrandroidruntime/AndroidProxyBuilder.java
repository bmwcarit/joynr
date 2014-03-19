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

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;

import java.util.concurrent.TimeUnit;

import android.os.AsyncTask;
import android.util.Log;

public class AndroidProxyBuilder<T extends JoynrInterface> extends AsyncTask<Object, String, T> implements
        ProxyBuilder<T> {

    private JoynrRuntime runtime;
    private String providerDomain;
    private MessagingQos messagingQos;
    private DiscoveryQos discoveryQos;
    private UILogger uiLogger;
    private ProxyCreatedCallback<T> callback = null;
    Class<T> proxyInterface;
    private String participantId = null;
    private ProxyBuilder<T> builder = null;
    private InitRuntimeTask runtimeInitTask;

    public AndroidProxyBuilder(InitRuntimeTask runtimeInitTask,
                               String providerDomain,
                               Class<T> proxyInterface,
                               UILogger uiLogger) {
        super();
        this.runtimeInitTask = runtimeInitTask;

        this.providerDomain = providerDomain;
        this.proxyInterface = proxyInterface;
        this.uiLogger = uiLogger;
    }

    @Override
    protected T doInBackground(Object... arg0) {

        Log.d("JAS", "starting CreateProxy");
        return buildProxy();
    }

    private T buildProxy() {
        T proxy = null;
        try {
            this.runtime = runtimeInitTask.get(discoveryQos.getDiscoveryTimeout(), TimeUnit.MILLISECONDS);
            builder = runtime.getProxyBuilder(providerDomain, proxyInterface);
            if (participantId != null) {
                builder.setParticipantId(participantId);
            }
            proxy = builder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        } catch (JoynrIllegalStateException e) {
            Log.e("JAS", e.toString());
            publishProgress(e.getMessage());
            e.printStackTrace();
        } catch (JoynrArbitrationException e) {
            Log.e("JAS", e.toString());
            publishProgress(e.getMessage());
            e.printStackTrace();
        } catch (InterruptedException e) {
            Log.e("JAS", e.toString());
            publishProgress(e.getMessage());
            e.printStackTrace();
        } catch (Exception e) {
            if (e.getMessage() != null) {
                Log.e("JAS", e.toString());
                publishProgress(e.getMessage());
            }
            e.printStackTrace();
        }
        Log.d("JAS", "Returning Proxy");
        return proxy;
    }

    protected void onProgressUpdate(String... progress) {
        uiLogger.logText(progress);
    }

    @Override
    protected void onPostExecute(T result) {
        Log.d("JAS", "calling onProxyCreated Callback");
        callback.onProxyCreated(result);
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
    public ProxyBuilder<T> setDiscoveryQos(DiscoveryQos discoveryQos) throws JoynrArbitrationException {
        this.discoveryQos = discoveryQos;
        return this;
    }

    @Override
    public ProxyBuilder<T> setMessagingQos(MessagingQos messagingQos) {
        this.messagingQos = messagingQos;
        return this;
    }

    @Override
    public T build() {
        // TODO test if this works
        return buildProxy();
    }

    @Override
    public void build(ProxyCreatedCallback<T> newCallback) {
        this.callback = newCallback;
        this.execute();

    }
}
