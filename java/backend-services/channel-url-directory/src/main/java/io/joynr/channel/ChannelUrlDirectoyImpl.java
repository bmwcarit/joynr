package io.joynr.channel;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.exceptions.JoynrException;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;

import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;

import javax.annotation.CheckForNull;

import joynr.infrastructure.ChannelUrlDirectoryProviderAsync;
import joynr.types.ChannelUrlInformation;
import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * The channelurldirectory stores channelIds mapped to channelUrls.
 *
 *
 * channelurls are stored in a concurrentHashMap. Using a in memory database could be possible optimization.
 */
// TODO Evaluate pro /cons of a in memory database

@Singleton
public class ChannelUrlDirectoyImpl extends AbstractJoynrProvider implements ChannelUrlDirectoryProviderAsync {
    private static final Logger logger = LoggerFactory.getLogger(ChannelUrlDirectoyImpl.class);

    protected ProviderQos providerQos = new ProviderQos();

    public static final String CHANNELURL_INACTIVE_TIME_IN_MS = "joynr.channel.channelurlinactivetime";

    final long channelurInactiveTimeInMS;

    private ConcurrentHashMap<String, ChannelUrlInformation> registeredChannels = new ConcurrentHashMap<String, ChannelUrlInformation>();

    Map<String, Long> inactiveChannelIds = new ConcurrentHashMap<String, Long>();

    private Thread cleanupThread;

    private Map<String, List<GetUrlsForChannelDeferred>> pendingDeferredsMap;

    ConcurrentHashMap<String, ChannelUrlInformation> getRegisteredChannels() {
        return registeredChannels;
    }

    @Inject
    public ChannelUrlDirectoyImpl(@Named(CHANNELURL_INACTIVE_TIME_IN_MS) long inactiveTimeInMS) {
        channelurInactiveTimeInMS = inactiveTimeInMS;
        pendingDeferredsMap = Maps.newConcurrentMap();
        cleanupThread = new Thread(new Runnable() {
            @Override
            public void run() {
                cleanupRunnable();
            }
        });
        cleanupThread.start();
    }

    private void cleanupRunnable() {
        while (true) {
            synchronized (cleanupThread) {
                if (inactiveChannelIds.size() == 0) {
                    try {
                        cleanupThread.wait();
                    } catch (InterruptedException e) {
                    }
                } else {
                    long currentTime = System.currentTimeMillis();
                    long timeToSleep = -1;
                    for (Entry<String, Long> inactiveChannelId : inactiveChannelIds.entrySet()) {
                        String channelId = inactiveChannelId.getKey();
                        long passedTime = currentTime - inactiveChannelId.getValue();
                        if (passedTime >= channelurInactiveTimeInMS) {
                            logger.debug("GLOBAL unregisterChannelUrls channelId: {}", channelId);
                            registeredChannels.remove(channelId);
                            inactiveChannelIds.remove(channelId);
                        } else {
                            if (timeToSleep == -1 || timeToSleep > channelurInactiveTimeInMS - passedTime) {
                                timeToSleep = channelurInactiveTimeInMS - passedTime;
                            }
                        }
                    }
                    if (timeToSleep != -1) {
                        try {
                            cleanupThread.wait(timeToSleep);
                        } catch (InterruptedException e) {
                        }
                    }
                }
            }
        }
    }

    // TODO: remove begin
    @Override
    public void getUrlsForChannel(final Callback<ChannelUrlInformation> callback, String channelId) {
        getUrlsForChannel(channelId).then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException error) {
                callback.onFailure(error);
            }

            @Override
            public void onFulfillment(Object... values) {
                callback.onSuccess((ChannelUrlInformation) values[0]);
            }
        });
    }

    // TODO: remove end

    @Override
    public Promise<GetUrlsForChannelDeferred> getUrlsForChannel(String channelId) {
        GetUrlsForChannelDeferred deferred = new GetUrlsForChannelDeferred();

        ChannelUrlInformation channelUrlInformation = registeredChannels.get(channelId);
        if (channelUrlInformation == null) {
            addPendingCallback(deferred, channelId);
            logger.warn("GLOBAL getUrlsForChannel for Channel: {} found nothing. Invoke callbacks once ChannelUrlInformation becomes available",
                        channelId);
        } else {
            logger.debug("GLOBAL getUrlsForChannel ChannelUrls for channelId {} found: {}",
                         channelId,
                         channelUrlInformation);
            deferred.resolve(channelUrlInformation);
        }
        return new Promise<GetUrlsForChannelDeferred>(deferred);
    }

    private synchronized void addPendingCallback(GetUrlsForChannelDeferred deferred, String channelId) {
        if (pendingDeferredsMap.get(channelId) == null) {
            pendingDeferredsMap.put(channelId, Lists.<GetUrlsForChannelDeferred> newArrayList());
        }
        pendingDeferredsMap.get(channelId).add(deferred);
        // TODO drop the newly added callback from the pendingCallbackMap after a while, avoiding a continuously growing
        // map
    }

    @Override
    @CheckForNull
    public ProviderQos getProviderQos() {
        return providerQos;
    }

    // TODO: remove begin
    @Override
    public synchronized void registerChannelUrls(@JoynrRpcCallback(deserialisationType = VoidToken.class) final Callback<Void> callback,
                                                 @JoynrRpcParam("channelId") String channelId,
                                                 @JoynrRpcParam("channelUrlInformation") ChannelUrlInformation channelUrlInformation) {
        registerChannelUrls(channelId, channelUrlInformation).then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException error) {
                callback.onFailure(error);
            }

            @Override
            public void onFulfillment(Object... values) {
                callback.onSuccess(null);
            }
        });
    }

    // TODO: remove end

    @Override
    public Promise<DeferredVoid> registerChannelUrls(String channelId, ChannelUrlInformation channelUrlInformation) {
        logger.debug("GLOBAL registerChannelUrls channelId: {} channelUrls: {}", channelId, channelUrlInformation);
        DeferredVoid deferred = new DeferredVoid();
        registeredChannels.put(channelId, channelUrlInformation);
        inactiveChannelIds.remove(channelId);
        if (deferred != null) {
            deferred.resolve();
        }
        List<GetUrlsForChannelDeferred> pendingDeferreds = pendingDeferredsMap.remove(channelId);
        if (pendingDeferreds != null) {
            for (GetUrlsForChannelDeferred pendingDeferred : pendingDeferreds) {
                pendingDeferred.resolve(channelUrlInformation);
            }
        }
        return new Promise<DeferredVoid>(deferred);
    }

    // TODO: remove begin
    @Override
    public synchronized void unregisterChannelUrls(@JoynrRpcCallback(deserialisationType = VoidToken.class) final Callback<Void> callback,
                                                   @JoynrRpcParam("channelId") String channelId) {
        unregisterChannelUrls(channelId).then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException error) {
                callback.onFailure(error);
            }

            @Override
            public void onFulfillment(Object... values) {
                callback.onSuccess(null);
            }
        });
    }

    // TODO: remove end

    @Override
    public Promise<DeferredVoid> unregisterChannelUrls(String channelId) {
        DeferredVoid deferred = new DeferredVoid();
        inactiveChannelIds.put(channelId, System.currentTimeMillis());
        synchronized (cleanupThread) {
            cleanupThread.notify();
        }

        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }
}
