/*
 * #%L
 * %%
 * Copyright (C) 2021-2023 BMW Car IT GmbH
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
package io.joynr.messaging.routing;

import static io.joynr.messaging.routing.MessageRouter.SCHEDULEDTHREADPOOL;

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;
import com.google.inject.Singleton;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.runtime.PrepareForShutdownListener;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;

@Singleton
public class GarbageCollectionHandler implements ShutdownListener {
    private static final Logger logger = LoggerFactory.getLogger(GarbageCollectionHandler.class);

    static class ProxyInformation {
        public String participantId;
        public PrepareForShutdownListener prepareForShutdownListener;
        public ShutdownListener shutdownListener;
        public final Set<String> providerParticipantIds;

        public ProxyInformation(String participantId,
                                PrepareForShutdownListener prepareForShutdownListener,
                                ShutdownListener shutdownListener) {
            this.participantId = participantId;
            this.prepareForShutdownListener = prepareForShutdownListener;
            this.shutdownListener = shutdownListener;
            this.providerParticipantIds = new HashSet<String>();
        }
    }

    // Map weak reference to proxy object -> {proxyParticipantId, shutdownListener}
    private final ConcurrentHashMap<WeakReference<Object>, ProxyInformation> proxyMap;
    private final ConcurrentHashMap<String, ProxyInformation> proxyParticipantIdToProxyInformationMap;
    private final ReferenceQueue<Object> garbageCollectedProxiesQueue;

    private MessageRouter messageRouter;
    private ScheduledFuture<?> cleanupScheduledFuture;
    private ShutdownNotifier shutdownNotifier;

    @Inject
    public GarbageCollectionHandler(MessageRouter messageRouter,
                                    ShutdownNotifier shutdownNotifier,
                                    @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs) {
        this.proxyMap = new ConcurrentHashMap<WeakReference<Object>, ProxyInformation>();
        this.proxyParticipantIdToProxyInformationMap = new ConcurrentHashMap<String, ProxyInformation>();
        this.garbageCollectedProxiesQueue = new ReferenceQueue<Object>();
        this.messageRouter = messageRouter;
        this.shutdownNotifier = shutdownNotifier;
        startCleanupThread(scheduler, routingTableCleanupIntervalMs);
        shutdownNotifier.registerForShutdown(this);
    }

    private void startCleanupThread(ScheduledExecutorService scheduler, long cleanupIntervalMs) {
        cleanupScheduledFuture = scheduler.scheduleWithFixedDelay(new Runnable() {
            @Override
            public void run() {
                // remove Routing table entries for proxies which have been garbage collected
                Reference<? extends Object> r;
                synchronized (garbageCollectedProxiesQueue) {
                    r = garbageCollectedProxiesQueue.poll();
                }
                while (r != null) {
                    ProxyInformation proxyInformation = proxyMap.get(r);
                    logger.debug("Removing garbage collected proxy participantId {}", proxyInformation.participantId);
                    messageRouter.removeNextHop(proxyInformation.participantId);
                    for (String providerParticipantId : proxyInformation.providerParticipantIds) {
                        messageRouter.removeNextHop(providerParticipantId);
                    }
                    shutdownNotifier.unregister(proxyInformation.prepareForShutdownListener);
                    shutdownNotifier.unregister(proxyInformation.shutdownListener);
                    proxyMap.remove(r);
                    proxyParticipantIdToProxyInformationMap.remove(proxyInformation.participantId);
                    synchronized (garbageCollectedProxiesQueue) {
                        r = garbageCollectedProxiesQueue.poll();
                    }
                }
            }
        }, cleanupIntervalMs, cleanupIntervalMs, TimeUnit.MILLISECONDS);
    }

    @Override
    public void shutdown() {
        if (cleanupScheduledFuture != null) {
            cleanupScheduledFuture.cancel(false);
        }
    }

    public void registerProxy(Object proxy,
                              String proxyParticipantId,
                              PrepareForShutdownListener prepareForShutdownListener,
                              ShutdownListener shutdownListener) {
        synchronized (garbageCollectedProxiesQueue) {
            ProxyInformation proxyInformation = new ProxyInformation(proxyParticipantId,
                                                                     prepareForShutdownListener,
                                                                     shutdownListener);
            if (proxyParticipantIdToProxyInformationMap.putIfAbsent(proxyParticipantId, proxyInformation) == null) {
                logger.debug("registerProxy called for {}", proxyParticipantId);
                proxyMap.put(new WeakReference<Object>(proxy, garbageCollectedProxiesQueue), proxyInformation);
            } else {
                throw new JoynrIllegalStateException("The proxy with " + proxyParticipantId
                        + " has already been registered.");
            }
        }
    }

    public void registerProxyProviderParticipantIds(String proxyParticipantId, Set<String> providerParticipantIds) {
        if (proxyParticipantId == null || proxyParticipantId.isEmpty()) {
            throw new JoynrIllegalStateException("Proxy participant id is null or has an empty value."
                    + "Registration of proxy's provider participant ids failed.");
        }

        if (providerParticipantIds == null || providerParticipantIds.isEmpty()) {
            throw new JoynrIllegalStateException("Set of the provider participant ids is null or empty."
                    + "Registration of proxy's provider participant ids failed.");
        } else {
            if (providerParticipantIds.contains(null) || providerParticipantIds.contains("")) {
                throw new JoynrIllegalStateException("Set of the provider participant ids has an entry with an empty or null value."
                        + "Registration of proxy's provider participant ids failed.");
            }
        }

        proxyParticipantIdToProxyInformationMap.computeIfPresent(proxyParticipantId, (key, oldVal) -> {
            if (oldVal.providerParticipantIds.isEmpty()) {
                oldVal.providerParticipantIds.addAll(providerParticipantIds);
                return oldVal;
            } else {
                throw new JoynrIllegalStateException("The proxy with " + proxyParticipantId
                        + " already has registered providers. Registration of proxy's provider participant ids failed.");
            }
        });
    }

}
