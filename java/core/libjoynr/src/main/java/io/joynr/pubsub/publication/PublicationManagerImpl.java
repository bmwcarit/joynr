package io.joynr.pubsub.publication;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;

import java.util.Collection;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import joynr.OnChangeSubscriptionQos;
import joynr.SubscriptionRequest;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.Maps;
import com.google.common.collect.Multimap;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.Inject;
import com.google.inject.Singleton;

@Singleton
public class PublicationManagerImpl implements PublicationManager {
    private static final Logger logger = LoggerFactory.getLogger(PublicationManagerImpl.class);
    // Map ProviderId -> SubscriptionRequest
    private final Multimap<String, PublicationInformation> queuedSubscriptionRequests;
    // Map SubscriptionId -> SubscriptionRequest
    private final ConcurrentMap<String, PublicationInformation> subscriptionId2PublicationInformation;
    // Map SubscriptionId -> PubSubState
    private final ConcurrentMap<String, PubSubState> publicationStates;
    // Map SubscriptionId -> PublicationTimer
    private final ConcurrentMap<String, PublicationTimer> publicationTimers;
    // Map SubscriptionId -> ScheduledFuture
    private final ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;
    // Map SubscriptionId -> UnregisterOnChange
    private final ConcurrentMap<String, UnregisterOnChange> unregisterOnChange;

    ScheduledExecutorService publicationScheduler;
    private AttributePollInterpreter attributePollInterpreter;

    static class PublicationInformation {
        private String providerParticipantId;
        private String proxyParticipantId;
        private SubscriptionRequest subscriptionRequest;

        PublicationInformation(String providerParticipantId,
                               String proxyParticipantId,
                               SubscriptionRequest subscriptionRequest) {
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
    }

    @Inject
    public PublicationManagerImpl(AttributePollInterpreter attributePollInterpreter) {
        super();
        this.queuedSubscriptionRequests = HashMultimap.create();
        this.subscriptionId2PublicationInformation = Maps.newConcurrentMap();
        this.publicationStates = Maps.newConcurrentMap();
        this.publicationTimers = Maps.newConcurrentMap();
        this.subscriptionEndFutures = Maps.newConcurrentMap();
        this.unregisterOnChange = Maps.newConcurrentMap();
        ThreadFactory publicationEndThreadFactory = new ThreadFactoryBuilder().setNameFormat("Publication-%d").build();
        this.publicationScheduler = Executors.newScheduledThreadPool(5, publicationEndThreadFactory);
        this.attributePollInterpreter = attributePollInterpreter;

    }

    PublicationManagerImpl(Multimap<String, PublicationInformation> queuedSubscriptionRequests,
                           ConcurrentMap<String, PublicationInformation> subscriptionId2SubscriptionRequest,
                           ConcurrentMap<String, PubSubState> publicationStates,
                           ConcurrentMap<String, PublicationTimer> publicationTimers,
                           ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures,
                           ScheduledExecutorService publicationEndScheduler,
                           AttributePollInterpreter attributePollInterpreter) {
        super();
        this.queuedSubscriptionRequests = queuedSubscriptionRequests;
        this.subscriptionId2PublicationInformation = subscriptionId2SubscriptionRequest;
        this.publicationStates = publicationStates;
        this.publicationTimers = publicationTimers;
        this.subscriptionEndFutures = subscriptionEndFutures;
        this.publicationScheduler = publicationEndScheduler;
        this.attributePollInterpreter = attributePollInterpreter;

        this.unregisterOnChange = Maps.newConcurrentMap();
    }

    @Override
    public void addSubscriptionRequest(String proxyParticipantId,
                                       String providerParticipantId,
                                       SubscriptionRequest subscriptionRequest,
                                       RequestCaller requestCaller,
                                       RequestReplySender requestReplySender) {

        // Check that this is a valid subscription
        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();
        long subscriptionEndDelay = subscriptionQos.getExpiryDate() - System.currentTimeMillis();

        if (subscriptionEndDelay < 0) {
            logger.error("Not adding subscription which ends in {} ms", subscriptionEndDelay);
            return;
        }

        // See if the publications for this subscription are already handled
        String subscriptionId = subscriptionRequest.getSubscriptionId();
        if (publicationExists(subscriptionId)) {
            logger.info("Publication with id: " + subscriptionId + " already exists.");
            // TODO update subscription
            return;
        }

        PubSubState pubState = new PubSubState();
        try {
            logger.info("adding publication: " + subscriptionRequest.toString());
            PublicationInformation existingSubscriptionRequest = subscriptionId2PublicationInformation.putIfAbsent(subscriptionId,
                                                                                                                   new PublicationInformation(providerParticipantId,
                                                                                                                                              proxyParticipantId,
                                                                                                                                              subscriptionRequest));
            if (existingSubscriptionRequest != null) {
                // we only use putIfAbsent instead of .put, because putIfAbsent is threadsafe
                logger.debug("there already was a SubscriptionRequest with that subscriptionId in the map");
            }
            PubSubState existingPubSubState = publicationStates.putIfAbsent(subscriptionId, pubState);
            if (existingPubSubState != null) {
                // we only use putIfAbsent instead of .put, because putIfAbsent is threadsafe
                logger.debug("there already was a pubState with that subscriptionId in the map");
            }

            //TODO only use timer for periodic subscriptions
            final PublicationTimer timer = new PublicationTimer(providerParticipantId,
                                                                proxyParticipantId,
                                                                pubState,
                                                                subscriptionRequest,
                                                                requestCaller,
                                                                requestReplySender,
                                                                attributePollInterpreter);

            if (subscriptionQos instanceof HeartbeatSubscriptionInformation) {
                timer.startTimer();
            } else {
                // send initial value for onChange subscription in a new Thread, because we are still in the
                // MessageReceiver Thread at this point
                Runnable initalPublicationRunnable = new Runnable() {
                    @Override
                    public void run() {
                        logger.trace("SendingInitialPublication");
                        timer.sendInitialPublication();
                        logger.trace("finished sending InitialPublication");
                    }
                };
                publicationScheduler.execute(initalPublicationRunnable);
            }

            publicationTimers.putIfAbsent(subscriptionId, timer);

            // Handle onChange subscriptions
            if (subscriptionQos instanceof OnChangeSubscriptionQos) {
                AttributeListener attributeListener = new AttributeListenerImpl(subscriptionId, this);
                String attributeName = subscriptionRequest.getAttributeName();
                requestCaller.registerAttributeListener(attributeName, attributeListener);
                unregisterOnChange.putIfAbsent(subscriptionId, new UnregisterOnChange(requestCaller,
                                                                                      attributeName,
                                                                                      attributeListener));
            }

            // Create a runnable to remove the publication when the subscription expires
            PublicationEndRunnable endRunnable = new PublicationEndRunnable(subscriptionId);
            ScheduledFuture<?> subscriptionEndFuture = publicationScheduler.schedule(endRunnable,
                                                                                     subscriptionEndDelay,
                                                                                     TimeUnit.MILLISECONDS);
            subscriptionEndFutures.putIfAbsent(subscriptionId, subscriptionEndFuture);
            logger.info("publication added: " + subscriptionRequest.toString());

        } catch (NoSuchMethodException e) {
            cancelPublicationCreation(subscriptionId);
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            cancelPublicationCreation(subscriptionId);
            e.printStackTrace();
        }

    }

    private void cancelPublicationCreation(String subscriptionId) {
        subscriptionId2PublicationInformation.remove(subscriptionId);
        publicationStates.remove(subscriptionId);
        logger.error("Subscription request rejected. Removing publication.");
    }

    private boolean publicationExists(String subscriptionId) {
        return publicationStates.containsKey(subscriptionId);
    }

    @Override
    public void addSubscriptionRequest(String proxyParticipantId,
                                       String providerParticipantId,
                                       SubscriptionRequest subscriptionRequest) {
        logger.info("Adding subscription request for non existing provider to queue.");
        PublicationInformation publicationInformation = new PublicationInformation(providerParticipantId,
                                                                                   proxyParticipantId,
                                                                                   subscriptionRequest);
        queuedSubscriptionRequests.put(providerParticipantId, publicationInformation);
        subscriptionId2PublicationInformation.putIfAbsent(subscriptionRequest.getSubscriptionId(),
                                                          publicationInformation);
    }

    protected void removePublication(String subscriptionId) {

        if (subscriptionId2PublicationInformation.containsKey(subscriptionId)) {
            PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);
            String providerParticipantId = publicationInformation.getProviderParticipantId();
            if (providerParticipantId != null && queuedSubscriptionRequests.containsKey(providerParticipantId)) {
                queuedSubscriptionRequests.removeAll(providerParticipantId);
            }
        }
        subscriptionId2PublicationInformation.remove(subscriptionId);
        if (publicationTimers.containsKey(subscriptionId)) {
            publicationTimers.get(subscriptionId).cancel();
            publicationTimers.remove(subscriptionId);
        }
        publicationStates.remove(subscriptionId);

        ScheduledFuture<?> future = subscriptionEndFutures.remove(subscriptionId);
        if (future != null) {
            future.cancel(true);
        }

        UnregisterOnChange onChange = unregisterOnChange.remove(subscriptionId);
        if (onChange != null) {
            onChange.unregister();
        }
    }

    class PublicationEndRunnable implements Runnable {

        private final String subscriptionId;

        public PublicationEndRunnable(String subscriptionId) {
            this.subscriptionId = subscriptionId;
        }

        @Override
        public void run() {
            logger.info("Publication expired...");
            removePublication(subscriptionId);
        }

    }

    // Class that holds information needed to unregister an onChange subscription
    static class UnregisterOnChange {
        final RequestCaller requestCaller;
        final String attributeName;
        final AttributeListener attributeListener;

        public UnregisterOnChange(RequestCaller requestCaller, String attributeName, AttributeListener attributeListener) {
            this.requestCaller = requestCaller;
            this.attributeName = attributeName;
            this.attributeListener = attributeListener;
        }

        public void unregister() {
            requestCaller.unregisterAttributeListener(attributeName, attributeListener);
        }
    }

    @Override
    public void stopPublication(String subscriptionId) {
        removePublication(subscriptionId);
    }

    @Override
    public void stopPublicationByProviderId(String providerId) {
        for (PublicationInformation publcationInformation : subscriptionId2PublicationInformation.values()) {
            if (publcationInformation.getProviderParticipantId().equals(providerId)) {
                removePublication(publcationInformation.getSubscriptionId());
            }
        }
    }

    @Override
    public void restoreQueuedSubscription(String providerId,
                                          RequestCaller requestCaller,
                                          RequestReplySender requestReplySender) {
        Collection<PublicationInformation> queuedRequests = queuedSubscriptionRequests.get(providerId);
        for (PublicationInformation publicInformation : queuedRequests) {
            if (System.currentTimeMillis() < publicInformation.subscriptionRequest.getQos().getExpiryDate()) {
                addSubscriptionRequest(publicInformation.getProxyParticipantId(),
                                       publicInformation.getProviderParticipantId(),
                                       publicInformation.subscriptionRequest,
                                       requestCaller,
                                       requestReplySender);
            }
            queuedSubscriptionRequests.remove(providerId, publicInformation);
        }
    }

    @Override
    public void attributeValueChanged(String subscriptionId, Object value) {
        PublicationTimer publicationTimer = publicationTimers.get(subscriptionId);

        if (publicationTimer == null) {
            logger.error("subscription {} has expired but attributeValueChanged has been called", subscriptionId);
            return;
        }

        logger.info("attribute changed for subscription id: {} sending publication if delay > minInterval.",
                    subscriptionId);
        publicationTimer.sendPublicationNow(value);
    }

    @Override
    public void shutdown() {
        publicationScheduler.shutdownNow();

    }
}
