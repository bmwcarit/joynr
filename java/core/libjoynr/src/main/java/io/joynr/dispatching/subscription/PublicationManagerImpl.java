/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.dispatching.subscription;

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.dispatching.DirectoryListener;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.SubscriptionPublisherObservable;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.publication.AttributeListener;
import io.joynr.pubsub.publication.BroadcastFilter;
import io.joynr.pubsub.publication.BroadcastListener;
import io.joynr.pubsub.publication.MulticastListener;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.MultiMap;
import io.joynr.util.ReflectionUtils;
import joynr.BroadcastFilterParameters;
import joynr.BroadcastSubscriptionRequest;
import joynr.MulticastPublication;
import joynr.MulticastSubscriptionRequest;
import joynr.OnChangeSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.UnicastSubscriptionQos;
import joynr.exceptions.ProviderRuntimeException;

@Singleton
public class PublicationManagerImpl
        implements PublicationManager, DirectoryListener<ProviderContainer>, ShutdownListener {
    private static final Logger logger = LoggerFactory.getLogger(PublicationManagerImpl.class);
    // Map ProviderId -> SubscriptionRequest
    private final MultiMap<String, PublicationInformation> queuedSubscriptionRequests;
    // Map SubscriptionId -> SubscriptionRequest
    private final ConcurrentMap<String, PublicationInformation> subscriptionId2PublicationInformation;
    // Map SubscriptionId -> PublicationTimer
    private final ConcurrentMap<String, PublicationTimer> publicationTimers;
    // Map SubscriptionId -> ScheduledFuture
    private final ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;
    // Map SubscriptionId -> UnregisterAttributeListener
    private final ConcurrentMap<String, UnregisterAttributeListener> unregisterAttributeListeners;
    // Map SubscriptionId -> UnregisterBroadcastListener
    private final ConcurrentMap<String, UnregisterBroadcastListener> unregisterBroadcastListeners;
    // Map provider participant ID -> MulticastListener
    private final ConcurrentMap<String, MulticastListener> multicastListeners;

    private AttributePollInterpreter attributePollInterpreter;
    private ScheduledExecutorService cleanupScheduler;
    private Dispatcher dispatcher;
    private ProviderDirectory providerDirectory;
    private RoutingTable routingTable;

    // protect the maps (see members above) and providerDirectory during add and remove of subscriptions or providers
    // to avoid race conditions, e.g. race conditions between providerDirectory and queuedSubscriptionRequests
    // protected members: subscriptionId2PublicationInformation, queuedSubscriptionRequests, publicationTimers,
    // subscriptionEndFutures, unregisterAttributeListeners, unregisterBroadcastListeners
    private Object addRemoveLock;

    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS)
    private long ttlUpliftMs = 0;

    private SubscriptionRequestStorage subscriptionRequestStorage;
    private boolean subscriptionRequestPersistency;

    static class PublicationInformation {
        private String providerParticipantId;
        private String proxyParticipantId;
        private SubscriptionRequest subscriptionRequest;
        private PubSubState pubState;

        PublicationInformation(String providerParticipantId,
                               String proxyParticipantId,
                               SubscriptionRequest subscriptionRequest) {
            pubState = new PubSubState();
            this.setProviderParticipantId(providerParticipantId);
            this.subscriptionRequest = subscriptionRequest;
            this.setProxyParticipantId(proxyParticipantId);
        }

        public String getProviderParticipantId() {
            return providerParticipantId;
        }

        public void setProviderParticipantId(String providerParticipantId) {
            this.providerParticipantId = providerParticipantId;
        }

        public String getProxyParticipantId() {
            return proxyParticipantId;
        }

        public void setProxyParticipantId(String proxyParticipantId) {
            this.proxyParticipantId = proxyParticipantId;
        }

        public String getSubscriptionId() {
            return subscriptionRequest.getSubscriptionId();
        }

        public PubSubState getState() {
            return pubState;
        }

        public UnicastSubscriptionQos getQos() {
            if (subscriptionRequest.getQos() instanceof UnicastSubscriptionQos) {
                return (UnicastSubscriptionQos) subscriptionRequest.getQos();
            } else {
                throw new IllegalArgumentException("Publication information should only be stored for unicast subscription requests");
            }
        }

        public SubscriptionRequest getSubscriptionRequest() {
            return subscriptionRequest;
        }

        @Override
        public boolean equals(Object arg0) {
            if (!(arg0 instanceof PublicationInformation)) {
                return false;
            }
            PublicationInformation pi = (PublicationInformation) arg0;
            return proxyParticipantId.equals(pi.proxyParticipantId)
                    && providerParticipantId.equals(pi.providerParticipantId)
                    && subscriptionRequest.equals(pi.subscriptionRequest);
        }

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + ((proxyParticipantId == null) ? 0 : proxyParticipantId.hashCode());
            result = prime * result + ((providerParticipantId == null) ? 0 : providerParticipantId.hashCode());
            result = prime * result + ((subscriptionRequest == null) ? 0 : subscriptionRequest.hashCode());
            return result;
        }

        public String getSubscribedToName() {
            return subscriptionRequest.getSubscribedToName();
        }
    }

    // CHECKSTYLE:OFF
    @Inject
    public PublicationManagerImpl(AttributePollInterpreter attributePollInterpreter,
                                  Dispatcher dispatcher,
                                  ProviderDirectory providerDirectory,
                                  RoutingTable routingTable,
                                  @Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler,
                                  SubscriptionRequestStorage subscriptionRequestStorage,
                                  ShutdownNotifier shutdownNotifier,
                                  @Named(ConfigurableMessagingSettings.PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCY) boolean subscriptionRequestPersistency) {
        super();
        this.dispatcher = dispatcher;
        this.providerDirectory = providerDirectory;
        this.routingTable = routingTable;
        this.addRemoveLock = new Object();
        this.cleanupScheduler = cleanupScheduler;
        this.subscriptionRequestStorage = subscriptionRequestStorage;
        this.queuedSubscriptionRequests = new MultiMap<>();
        this.subscriptionId2PublicationInformation = new ConcurrentHashMap<>();
        this.publicationTimers = new ConcurrentHashMap<>();
        this.subscriptionEndFutures = new ConcurrentHashMap<>();
        this.unregisterAttributeListeners = new ConcurrentHashMap<>();
        this.unregisterBroadcastListeners = new ConcurrentHashMap<>();
        this.multicastListeners = new ConcurrentHashMap<>();
        this.attributePollInterpreter = attributePollInterpreter;
        this.subscriptionRequestPersistency = subscriptionRequestPersistency;
        providerDirectory.addListener(this);
        providerDirectory.forEach(this::entryAdded);
        if (subscriptionRequestPersistency) {
            queueSavedSubscriptionRequests();
        }
        shutdownNotifier.registerForShutdown(this);
    }
    // CHECKSTYLE:ON

    private void queueSavedSubscriptionRequests() {

        MultiMap<String, PersistedSubscriptionRequest> persistedSubscriptionRequests = subscriptionRequestStorage.getSavedSubscriptionRequests();
        if (persistedSubscriptionRequests == null || persistedSubscriptionRequests.isEmpty()) {
            return;
        }

        try {
            for (String providerId : persistedSubscriptionRequests.keySet()) {
                for (PersistedSubscriptionRequest persistedSubscriptionRequest : persistedSubscriptionRequests.get(providerId)) {
                    addSubscriptionRequest(persistedSubscriptionRequest.getProxyParticipantId(),
                                           providerId,
                                           persistedSubscriptionRequest.getSubscriptonRequest());
                    subscriptionRequestStorage.removeSubscriptionRequest(providerId, persistedSubscriptionRequest);
                }
            }
        } catch (Exception e) {
            logger.error("Unable to queue saved subscription requests: ", e);
        }
    }

    // requires addRemoveLock: publicationTimers, handleOnChangeSubscription
    private void handleSubscriptionRequest(PublicationInformation publicationInformation,
                                           SubscriptionRequest subscriptionRequest,
                                           ProviderContainer providerContainer) {
        final String subscriptionId = subscriptionRequest.getSubscriptionId();

        try {
            Method method = findGetterForAttributeName(providerContainer.getProviderProxy().getClass(),
                                                       subscriptionRequest.getSubscribedToName());

            triggerPublication(publicationInformation, providerContainer, method);

            SubscriptionQos subscriptionQos = subscriptionRequest.getQos();
            boolean hasSubscriptionHeartBeat = subscriptionQos instanceof HeartbeatSubscriptionInformation;
            boolean isOnChangeSubscription = subscriptionQos instanceof OnChangeSubscriptionQos;

            if (hasSubscriptionHeartBeat || isOnChangeSubscription) {
                // TODO: send error subscription reply is periodMs < MIN_PERIOD_MS or periodMs > MAX_PERIOD_MS?
                final PublicationTimer timer = new PublicationTimer(publicationInformation,
                                                                    method,
                                                                    providerContainer,
                                                                    this,
                                                                    attributePollInterpreter);

                timer.startTimer();
                publicationTimers.put(subscriptionId, timer);
            }

            if (subscriptionQos instanceof OnChangeSubscriptionQos) {
                handleOnChangeSubscription(subscriptionRequest, providerContainer, subscriptionId);
            }

            MessagingQos messagingQos = createMessagingQos(subscriptionQos);
            dispatcher.sendSubscriptionReply(publicationInformation.providerParticipantId,
                                             publicationInformation.proxyParticipantId,
                                             new SubscriptionReply(subscriptionId),
                                             messagingQos);
        } catch (NoSuchMethodException e) {
            logger.error("Error subscribing: {}. The provider does not have the requested attribute",
                         subscriptionRequest);
            throw new SubscriptionException(subscriptionId, e.getMessage());
        }
    }

    private MessagingQos createMessagingQos(SubscriptionQos subscriptionQos) {
        MessagingQos messagingQos = new MessagingQos();
        if (subscriptionQos.getExpiryDateMs() == SubscriptionQos.NO_EXPIRY_DATE) {
            messagingQos.setTtl_ms(SubscriptionQos.INFINITE_SUBSCRIPTION);
        } else {
            // TTL uplift will be done in JoynrMessageFactory
            messagingQos.setTtl_ms(subscriptionQos.getExpiryDateMs() - System.currentTimeMillis());
        }
        return messagingQos;
    }

    // requires addRemoveLock: unregisterAttributeListeners
    private void handleOnChangeSubscription(SubscriptionRequest subscriptionRequest,
                                            ProviderContainer providerContainer,
                                            String subscriptionId) {
        AttributeListener attributeListener = new AttributeListenerImpl(subscriptionId, this);
        String attributeName = subscriptionRequest.getSubscribedToName();
        SubscriptionPublisherObservable subscriptionPublisher = providerContainer.getSubscriptionPublisher();
        subscriptionPublisher.registerAttributeListener(attributeName, attributeListener);
        unregisterAttributeListeners.put(subscriptionId,
                                         new UnregisterAttributeListener(subscriptionPublisher,
                                                                         attributeName,
                                                                         attributeListener));
    }

    // requires addRemoveLock: unregisterBroadcastListeners
    private void handleBroadcastSubscriptionRequest(String proxyParticipantId,
                                                    String providerParticipantId,
                                                    BroadcastSubscriptionRequest subscriptionRequest,
                                                    ProviderContainer providerContainer) {
        logger.trace("Adding broadcast publication: {}", subscriptionRequest);

        BroadcastListener broadcastListener = new BroadcastListenerImpl(subscriptionRequest.getSubscriptionId(), this);
        String broadcastName = subscriptionRequest.getSubscribedToName();
        providerContainer.getSubscriptionPublisher().registerBroadcastListener(broadcastName, broadcastListener);
        unregisterBroadcastListeners.put(subscriptionRequest.getSubscriptionId(),
                                         new UnregisterBroadcastListener(providerContainer.getSubscriptionPublisher(),
                                                                         broadcastName,
                                                                         broadcastListener));

        final String subscriptionId = subscriptionRequest.getSubscriptionId();

        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();

        MessagingQos messagingQos = createMessagingQos(subscriptionQos);

        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId);

        dispatcher.sendSubscriptionReply(providerParticipantId, proxyParticipantId, subscriptionReply, messagingQos);
    }

    private void handleMulticastSubscriptionRequest(String proxyParticipantId,
                                                    String providerParticipantId,
                                                    MulticastSubscriptionRequest subscriptionRequest,
                                                    ProviderContainer providerContainer) {
        logger.trace("Received multicast subscription request {} for provider with participant ID {}",
                     subscriptionRequest,
                     providerParticipantId);
        dispatcher.sendSubscriptionReply(providerParticipantId,
                                         proxyParticipantId,
                                         new SubscriptionReply(subscriptionRequest.getSubscriptionId()),
                                         createMessagingQos(subscriptionRequest.getQos()));
    }

    // requires addRemoveLock: handleSubscriptionRequest, handleBroadcastSubscriptionRequest,
    // subscriptionId2PublicationInformation, addSubscriptionCleanupIfNecessary
    private void addSubscriptionRequestInternal(PublicationInformation publicationInformation,
                                                ProviderContainer providerContainer) {
        final SubscriptionRequest subscriptionRequest = publicationInformation.subscriptionRequest;
        if (cancelQueuedAndOngoingPublication(subscriptionRequest.getSubscriptionId())) {
            logger.trace("Updating publication: {}", subscriptionRequest);
        } else {
            logger.trace("Adding publication: {}", subscriptionRequest);
        }

        if (subscriptionRequest instanceof BroadcastSubscriptionRequest) {
            handleBroadcastSubscriptionRequest(publicationInformation.getProxyParticipantId(),
                                               publicationInformation.getProviderParticipantId(),
                                               (BroadcastSubscriptionRequest) subscriptionRequest,
                                               providerContainer);
        } else if (subscriptionRequest instanceof MulticastSubscriptionRequest) {
            handleMulticastSubscriptionRequest(publicationInformation.getProxyParticipantId(),
                                               publicationInformation.getProviderParticipantId(),
                                               (MulticastSubscriptionRequest) subscriptionRequest,
                                               providerContainer);
        } else {
            handleSubscriptionRequest(publicationInformation, subscriptionRequest, providerContainer);
        }
    }

    private void sendSubscriptionReplyWithError(SubscriptionException e,
                                                PublicationInformation publicationInformation,
                                                SubscriptionRequest subscriptionRequest) {
        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();
        MessagingQos messagingQos = createMessagingQos(subscriptionQos);

        SubscriptionReply subscriptionReply = new SubscriptionReply(publicationInformation.getSubscriptionId(), e);
        dispatcher.sendSubscriptionReply(publicationInformation.providerParticipantId,
                                         publicationInformation.proxyParticipantId,
                                         subscriptionReply,
                                         messagingQos);
    }

    // requires addRemoveLock: subscriptionEndFutures
    private void updateSubscriptionCleanupIfNecessary(SubscriptionRequest subscriptionRequest,
                                                      long subscriptionEndDelay) {
        ScheduledFuture<?> future = subscriptionEndFutures.remove(subscriptionRequest.getSubscriptionId());
        if (future != null) {
            future.cancel(true);
        }
        // cleanup is only necessary if publication information is stored and the subscription expires
        // no information is stored for non queued multicast subscriptions
        if (subscriptionId2PublicationInformation.containsKey(subscriptionRequest.getSubscriptionId())
                && subscriptionRequest.getQos().getExpiryDateMs() != SubscriptionQos.NO_EXPIRY_DATE) {
            final String subscriptionId = subscriptionRequest.getSubscriptionId();
            ScheduledFuture<?> subscriptionEndFuture = cleanupScheduler.schedule(new Runnable() {

                @Override
                public void run() {
                    logger.trace("Publication with Id {} expired...", subscriptionId);
                    synchronized (addRemoveLock) {
                        removePublication(subscriptionId);
                    }
                }

            }, subscriptionEndDelay, TimeUnit.MILLISECONDS);
            subscriptionEndFutures.put(subscriptionId, subscriptionEndFuture);
        }
    }

    private long validateAndGetSubscriptionEndDelay(SubscriptionRequest subscriptionRequest) {
        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();
        long subscriptionEndDelay = getSubscriptionEndDelay(subscriptionQos);
        if (subscriptionEndDelay < 0) {
            throw new SubscriptionException(subscriptionRequest.getSubscriptionId(), "Subscription expired.");
        }
        return subscriptionEndDelay;
    }

    private long getSubscriptionEndDelay(SubscriptionQos subscriptionQos) {
        long subscriptionEndDelay;
        if (subscriptionQos.getExpiryDateMs() == SubscriptionQos.NO_EXPIRY_DATE) {
            subscriptionEndDelay = SubscriptionQos.NO_EXPIRY_DATE;
        } else if (subscriptionQos.getExpiryDateMs() > (Long.MAX_VALUE - ttlUpliftMs)) {
            subscriptionEndDelay = Long.MAX_VALUE - System.currentTimeMillis();
        } else {
            subscriptionEndDelay = subscriptionQos.getExpiryDateMs() + ttlUpliftMs - System.currentTimeMillis();
        }
        return subscriptionEndDelay;
    }

    @Override
    public void addSubscriptionRequest(String proxyParticipantId,
                                       String providerParticipantId,
                                       SubscriptionRequest subscriptionRequest) {

        if (subscriptionRequestPersistency && (!(subscriptionRequest instanceof MulticastSubscriptionRequest))) {
            // only requests for attribute and selective/filtered broadcast subscriptions get persisted
            subscriptionRequestStorage.persistSubscriptionRequest(proxyParticipantId,
                                                                  providerParticipantId,
                                                                  subscriptionRequest);
        }

        synchronized (providerDirectory) {
            synchronized (addRemoveLock) {
                PublicationInformation publicationInformation = new PublicationInformation(providerParticipantId,
                                                                                           proxyParticipantId,
                                                                                           subscriptionRequest);
                try {
                    final boolean isMulticastSubscriptionRequest = subscriptionRequest instanceof MulticastSubscriptionRequest;
                    boolean isMulticastQueued = false;
                    if (!isMulticastSubscriptionRequest) {
                        routingTable.incrementReferenceCount(proxyParticipantId);
                    }

                    long subscriptionEndDelay = validateAndGetSubscriptionEndDelay(subscriptionRequest);
                    ProviderContainer providerContainer = providerDirectory.get(providerParticipantId);
                    if (providerContainer != null) {
                        addSubscriptionRequestInternal(publicationInformation, providerContainer);
                        logger.trace("Publication added: {}", subscriptionRequest.toString());
                    } else {
                        queuedSubscriptionRequests.put(providerParticipantId, publicationInformation);
                        if (isMulticastSubscriptionRequest) {
                            isMulticastQueued = true;
                        }
                        logger.trace("Added subscription request for non existing provider to queue.");
                    }

                    PublicationInformation oldEntry = subscriptionId2PublicationInformation.remove(subscriptionRequest.getSubscriptionId());
                    if (oldEntry != null && !(oldEntry.subscriptionRequest instanceof MulticastSubscriptionRequest)) {
                        routingTable.remove(proxyParticipantId);
                    }

                    if (!isMulticastSubscriptionRequest || isMulticastQueued) {
                        subscriptionId2PublicationInformation.put(subscriptionRequest.getSubscriptionId(),
                                                                  publicationInformation);
                    }
                    updateSubscriptionCleanupIfNecessary(subscriptionRequest, subscriptionEndDelay);

                } catch (SubscriptionException e) {
                    sendSubscriptionReplyWithError(e, publicationInformation, subscriptionRequest);
                } catch (JoynrIllegalStateException e) {
                    logger.error("Proxy participant ID {} unknown to routing table.", proxyParticipantId);
                    sendSubscriptionReplyWithError(new SubscriptionException(subscriptionRequest.getSubscriptionId(),
                                                                             e.getMessage()),
                                                   publicationInformation,
                                                   subscriptionRequest);
                }
            }
        }
    }

    // requires requestQueueLock: subscriptionId2PublicationInformation, queuedSubscriptionRequests, publicationTimers,
    // subscriptionEndFutures, unregisterAttributeListeners, unregisterBroadcastListeners
    private void removePublication(String subscriptionId) {
        if (!cancelQueuedAndOngoingPublication(subscriptionId)) {
            return;
        }
        ScheduledFuture<?> future = subscriptionEndFutures.remove(subscriptionId);
        if (future != null) {
            future.cancel(true);
        }
        PublicationInformation publicationInformation = subscriptionId2PublicationInformation.remove(subscriptionId);
        if (!(publicationInformation.getSubscriptionRequest() instanceof MulticastSubscriptionRequest)) {
            routingTable.remove(publicationInformation.getProxyParticipantId());
        }
    }

    // requires addRemoveLock
    private boolean cancelQueuedAndOngoingPublication(String subscriptionId) {
        PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);
        if (publicationInformation == null) {
            return false;
        }

        // Remove (eventually) queued subcriptionRequest
        queuedSubscriptionRequests.remove(publicationInformation.getProviderParticipantId(), publicationInformation);

        PublicationTimer publicationTimer = publicationTimers.remove(subscriptionId);
        if (publicationTimer != null) {
            publicationTimer.cancel();
        }

        UnregisterAttributeListener unregisterAttributeListener = unregisterAttributeListeners.remove(subscriptionId);
        if (unregisterAttributeListener != null) {
            unregisterAttributeListener.unregister();
        }
        UnregisterBroadcastListener unregisterBroadcastListener = unregisterBroadcastListeners.remove(subscriptionId);
        if (unregisterBroadcastListener != null) {
            unregisterBroadcastListener.unregister();
        }

        return true;
    }

    // Class that holds information needed to unregister attribute listener
    static class UnregisterAttributeListener {
        private final String attributeName;
        private final AttributeListener attributeListener;
        private final SubscriptionPublisherObservable subscriptionPublisher;

        public UnregisterAttributeListener(SubscriptionPublisherObservable subscriptionPublisher,
                                           String attributeName,
                                           AttributeListener attributeListener) {
            this.subscriptionPublisher = subscriptionPublisher;
            this.attributeName = attributeName;
            this.attributeListener = attributeListener;
        }

        public void unregister() {
            subscriptionPublisher.unregisterAttributeListener(attributeName, attributeListener);
        }
    }

    // Class that holds information needed to unregister broadcast listener
    static class UnregisterBroadcastListener {
        private final String broadcastName;
        private final BroadcastListener broadcastListener;
        private final SubscriptionPublisherObservable subscriptionPublisher;

        public UnregisterBroadcastListener(SubscriptionPublisherObservable subscriptionPublisher,
                                           String broadcastName,
                                           BroadcastListener broadcastListener) {
            this.subscriptionPublisher = subscriptionPublisher;
            this.broadcastName = broadcastName;
            this.broadcastListener = broadcastListener;
        }

        public void unregister() {
            subscriptionPublisher.unregisterBroadcastListener(broadcastName, broadcastListener);
        }
    }

    @Override
    public void stopPublication(String subscriptionId) {
        PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);
        if (publicationInformation == null) {
            return;
        }
        try {
            synchronized (addRemoveLock) {
                removePublication(subscriptionId);
            }
        } catch (Exception e) {
            JoynrRuntimeException error = new JoynrRuntimeException("Error stopping subscription " + subscriptionId
                    + ": " + e);
            sendPublicationError(error, publicationInformation);
        }
    }

    // requires addRemoveLock: subscriptionId2PublicationInformation, removePublication, queuedSubscriptionRequests
    /**
     * Stops all publications for a provider
     *
     * @param providerParticipantId provider for which all publication should be stopped
     */
    private void stopPublicationByProviderId(String providerParticipantId) {
        for (PublicationInformation publicationInformation : subscriptionId2PublicationInformation.values()) {
            if (publicationInformation.getProviderParticipantId().equals(providerParticipantId)) {
                removePublication(publicationInformation.getSubscriptionId());
            }
        }

        if (providerParticipantId != null && queuedSubscriptionRequests.containsKey(providerParticipantId)) {
            queuedSubscriptionRequests.removeAll(providerParticipantId);
        }
    }

    private boolean isExpired(PublicationInformation publicationInformation) {
        SubscriptionQos subscriptionQos = publicationInformation.subscriptionRequest.getQos();
        long subscriptionEndDelay = getSubscriptionEndDelay(subscriptionQos);
        logger.trace("ExpiryDate - System.currentTimeMillis: {}", subscriptionEndDelay);
        return (subscriptionEndDelay != SubscriptionQos.NO_EXPIRY_DATE && subscriptionEndDelay <= 0);
    }

    /**
     * Called every time a provider is registered to check whether there are already
     * subscriptionRequests waiting.
     *
     * @param providerId provider id
     * @param providerContainer provider container
     */
    private void restoreQueuedSubscription(String providerId, ProviderContainer providerContainer) {
        synchronized (addRemoveLock) {
            // After the provider is registered, the queued requests must be subsequently removed
            Collection<PublicationInformation> queuedRequests = queuedSubscriptionRequests.getAndRemoveAll(providerId);
            Iterator<PublicationInformation> queuedRequestsIterator = queuedRequests.iterator();
            while (queuedRequestsIterator.hasNext()) {
                PublicationInformation publicationInformation = queuedRequestsIterator.next();
                if (!isExpired(publicationInformation)) {
                    try {
                        addSubscriptionRequestInternal(publicationInformation, providerContainer);
                        if (publicationInformation.getSubscriptionRequest() instanceof MulticastSubscriptionRequest) {
                            subscriptionId2PublicationInformation.remove(publicationInformation.getSubscriptionId());
                        }
                    } catch (SubscriptionException e) {
                        removePublication(publicationInformation.getSubscriptionId());
                        sendSubscriptionReplyWithError(e,
                                                       publicationInformation,
                                                       publicationInformation.subscriptionRequest);
                    }

                }
            }
        }
    }

    @Override
    public void attributeValueChanged(String subscriptionId, Object value) {
        // no lock for subscriptionId2PublicationInformation
        PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);
        if (publicationInformation != null) {
            if (isExpired(publicationInformation)) {
                synchronized (addRemoveLock) {
                    removePublication(subscriptionId);
                }
            } else {
                // no lock for publicationTimers
                PublicationTimer publicationTimer = publicationTimers.get(subscriptionId);
                SubscriptionPublication publication = prepareAttributePublication(value, subscriptionId);
                if (publicationTimer != null) {
                    // used by OnChangedWithKeepAlive
                    publicationTimer.sendPublicationNow(publication);
                } else {
                    sendPublication(publication, publicationInformation);
                }

                logger.trace("Attribute changed for subscription id: {} sending publication if delay > minInterval.",
                             subscriptionId);
            }

        } else {
            logger.trace("Subscription {} has expired but attributeValueChanged has been called", subscriptionId);
        }
    }

    @Override
    public void broadcastOccurred(String subscriptionId, List<BroadcastFilter> filters, Object... values) {
        // no lock for subscriptionId2PublicationInformation
        PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);
        if (publicationInformation != null) {

            if (processFilterChain(publicationInformation, filters, values)) {
                long minInterval = ((OnChangeSubscriptionQos) publicationInformation.getQos()).getMinIntervalMs();
                if (minInterval <= System.currentTimeMillis()
                        - publicationInformation.getState().getTimeOfLastPublication()) {
                    sendPublication(prepareBroadcastPublication(Arrays.asList(values), subscriptionId),
                                    publicationInformation);
                    logger.trace("Event occured changed for subscription id: {} sending publication: ", subscriptionId);
                } else {
                    logger.trace("Two subsequent broadcasts of event {} occured within minInterval of subscription with id {}. Event will not be sent to the subscribing client.",
                                 publicationInformation.getSubscribedToName(),
                                 publicationInformation.getSubscriptionId());
                }
            }

        } else {
            logger.trace("Subscription {} has expired but eventOccurred has been called", subscriptionId);
        }
    }

    private boolean processFilterChain(PublicationInformation publicationInformation,
                                       List<BroadcastFilter> filters,
                                       Object[] values) {

        if (filters != null && filters.size() > 0) {
            BroadcastSubscriptionRequest subscriptionRequest = (BroadcastSubscriptionRequest) publicationInformation.subscriptionRequest;
            BroadcastFilterParameters filterParameters = subscriptionRequest.getFilterParameters();

            for (BroadcastFilter filter : filters) {
                Method filterMethod = null;
                try {
                    Method[] methodsOfFilterClass = filter.getClass().getMethods();
                    for (Method method : methodsOfFilterClass) {
                        if (method.getName().equals("filter")) {
                            filterMethod = method;
                            break;
                        }
                    }
                    if (filterMethod == null) {
                        // no filtering
                        return true;
                    }

                    if (!filterMethod.isAccessible()) {
                        filterMethod.setAccessible(true);
                    }

                    Class<?> filterParametersType = filterMethod.getParameterTypes()[values.length];
                    BroadcastFilterParameters filterParametersDerived = (BroadcastFilterParameters) filterParametersType.newInstance();
                    filterParametersDerived.setFilterParameters(filterParameters.getFilterParameters());
                    Object[] args = Arrays.copyOf(values, values.length + 1);
                    args[args.length - 1] = filterParametersDerived;

                    if ((Boolean) filterMethod.invoke(filter, args) == false) {
                        return false;
                    }
                } catch (Exception e) {
                    logger.error("ProcessFilterChain error:", e);
                    throw new IllegalStateException("processFilterChain: Error in reflection calling filters.", e);
                }
            }
        }
        return true;
    }

    private SubscriptionPublication prepareAttributePublication(Object value, String subscriptionId) {
        return new SubscriptionPublication(Arrays.asList(value), subscriptionId);
    }

    private SubscriptionPublication prepareBroadcastPublication(List<Object> values, String subscriptionId) {
        return new SubscriptionPublication(values, subscriptionId);
    }

    private void sendPublication(SubscriptionPublication publication, PublicationInformation publicationInformation) {
        try {
            sendSubscriptionPublication(publication, publicationInformation);
            // TODO handle exceptions during publication. See JOYNR-2113
        } catch (JoynrRuntimeException | IOException e) {
            logger.error("SendPublication error: ", e);
        }
    }

    private void sendPublicationError(JoynrRuntimeException error, PublicationInformation publicationInformation) {
        SubscriptionPublication publication = new SubscriptionPublication(error,
                                                                          publicationInformation.getSubscriptionId());
        sendPublication(publication, publicationInformation);
    }

    private void triggerPublication(final PublicationInformation publicationInformation,
                                    ProviderContainer providerContainer,
                                    Method method) {
        try {
            Optional<Promise<?>> optionalPromise = attributePollInterpreter.execute(providerContainer, method);
            Promise<?> attributeGetterPromise = optionalPromise.isPresent() ? optionalPromise.get() : null;
            if (attributeGetterPromise == null) {
                throw new ProviderRuntimeException("Unexpected exception while calling getter for attribute "
                        + publicationInformation.getSubscribedToName());
            }
            attributeGetterPromise.then(new PromiseListener() {

                @Override
                public void onRejection(JoynrException error) {
                    if (error instanceof JoynrRuntimeException) {
                        sendPublicationError((JoynrRuntimeException) error, publicationInformation);
                    } else {
                        sendPublicationError(new ProviderRuntimeException("Unexpected exception while calling getter for attribute "
                                + publicationInformation.getSubscribedToName()), publicationInformation);
                    }

                }

                @Override
                public void onFulfillment(Object... values) {
                    // attribute getters only return a single value
                    sendPublication(prepareAttributePublication(values[0], publicationInformation.getSubscriptionId()),
                                    publicationInformation);
                }
            });
        } catch (JoynrRuntimeException error) {
            sendPublicationError(error, publicationInformation);
        }
    }

    private Method findGetterForAttributeName(Class<?> clazz, String attributeName) throws NoSuchMethodException {
        String attributeGetterName = "get" + attributeName.toUpperCase().charAt(0)
                + attributeName.subSequence(1, attributeName.length());
        return ReflectionUtils.findMethodByParamTypes(clazz, attributeGetterName, new Class[]{});

    }

    @Override
    public void sendSubscriptionPublication(SubscriptionPublication publication,
                                            PublicationInformation publicationInformation) throws JoynrSendBufferFullException,
                                                                                           JoynrMessageNotSentException,
                                                                                           JsonGenerationException,
                                                                                           JsonMappingException,
                                                                                           IOException {
        MessagingQos messagingQos = new MessagingQos();
        // TTL uplift will be done in JoynrMessageFactory
        messagingQos.setTtl_ms(publicationInformation.getQos().getPublicationTtlMs());
        Set<String> toParticipantIds = new HashSet<>();
        toParticipantIds.add(publicationInformation.proxyParticipantId);
        dispatcher.sendSubscriptionPublication(publicationInformation.providerParticipantId,
                                               toParticipantIds,
                                               publication,
                                               messagingQos);
        publicationInformation.getState().updateTimeOfLastPublication();
    }

    @Override
    public void multicastOccurred(String providerParticipantId,
                                  String multicastName,
                                  String[] partitions,
                                  Object... values) {
        logger.trace("Multicast occurred for {} / {} / {} / {}",
                     providerParticipantId,
                     multicastName,
                     Arrays.toString(partitions),
                     Arrays.toString(values));
        String multicastId = MulticastIdUtil.createMulticastId(providerParticipantId, multicastName, partitions);
        MulticastPublication multicastPublication = new MulticastPublication(Arrays.asList(values), multicastId);
        MessagingQos messagingQos = new MessagingQos();
        dispatcher.sendMulticast(providerParticipantId, multicastPublication, messagingQos);
    }

    @Override
    public void entryAdded(final String providerParticipantId, ProviderContainer providerContainer) {
        restoreQueuedSubscription(providerParticipantId, providerContainer);
        MulticastListener multicastListener = new MulticastListener() {
            @Override
            public void multicastOccurred(String multicastName, String[] partitions, Object[] values) {
                PublicationManagerImpl.this.multicastOccurred(providerParticipantId, multicastName, partitions, values);
            }
        };
        multicastListeners.putIfAbsent(providerParticipantId, multicastListener);
        providerContainer.getSubscriptionPublisher()
                         .registerMulticastListener(multicastListeners.get(providerParticipantId));
    }

    @Override
    public void entryRemoved(String providerParticipantId) {
        synchronized (addRemoveLock) {
            stopPublicationByProviderId(providerParticipantId);
            ProviderContainer providerContainer = providerDirectory.get(providerParticipantId);
            if (providerContainer != null) {
                providerContainer.getSubscriptionPublisher()
                                 .unregisterMulticastListener(multicastListeners.remove(providerParticipantId));
            }
        }
    }

    @Override
    public void shutdown() {
        synchronized (subscriptionEndFutures.values()) {
            for (ScheduledFuture<?> future : subscriptionEndFutures.values()) {
                if (future != null) {
                    future.cancel(false);
                }
            }
        }
        providerDirectory.removeListener(this);
    }
}
