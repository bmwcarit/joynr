package io.joynr.pubsub.subscription;

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

import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;

import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.type.TypeReference;
import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class SubscriptionManagerImpl implements SubscriptionManager {

    private ConcurrentMap<String, AttributeSubscriptionListener<?>> subscriptionListenerDirectory;
    private ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionListenerDirectory;
    private ConcurrentMap<String, Class<? extends TypeReference<?>>> subscriptionAttributeTypes;
    private ConcurrentMap<String, PubSubState> subscriptionStates;
    private ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers;
    private ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures; // These futures will be needed if a
    // subscription
    // should be updated with a new end time

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionManagerImpl.class);
    private ScheduledExecutorService cleanupScheduler;

    @Inject
    public SubscriptionManagerImpl(@Named("joynr.scheduler.cleanup") ScheduledExecutorService cleanupScheduler) {
        super();
        this.cleanupScheduler = cleanupScheduler;
        this.subscriptionListenerDirectory = Maps.newConcurrentMap();
        this.broadcastSubscriptionListenerDirectory = Maps.newConcurrentMap();
        this.subscriptionStates = Maps.newConcurrentMap();
        this.missedPublicationTimers = Maps.newConcurrentMap();
        this.subscriptionEndFutures = Maps.newConcurrentMap();
        this.subscriptionAttributeTypes = Maps.newConcurrentMap();

    }

    public SubscriptionManagerImpl(ConcurrentMap<String, AttributeSubscriptionListener<?>> attributeSubscriptionDirectory,
                                   ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionDirectory,
                                   ConcurrentMap<String, PubSubState> subscriptionStates,
                                   ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers,
                                   ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures,
                                   ConcurrentMap<String, Class<? extends TypeReference<?>>> subscriptionAttributeTypes,
                                   ScheduledExecutorService cleanupScheduler) {
        super();
        this.subscriptionListenerDirectory = attributeSubscriptionDirectory;
        this.broadcastSubscriptionListenerDirectory = broadcastSubscriptionDirectory;
        this.subscriptionStates = subscriptionStates;
        this.missedPublicationTimers = missedPublicationTimers;
        this.subscriptionEndFutures = subscriptionEndFutures;
        this.subscriptionAttributeTypes = subscriptionAttributeTypes;
        this.cleanupScheduler = cleanupScheduler;
    }

    private String registerSubscription(final SubscriptionQos qos) {
        String uuid = UUID.randomUUID().toString();
        String subscriptionId = uuid;

        PubSubState subState = new PubSubState();
        subState.updateTimeOfLastPublication();
        subscriptionStates.put(subscriptionId, subState);

        long expiryDate = qos.getExpiryDate();
        logger.info("subscription: {} expiryDate: "
                            + (expiryDate == SubscriptionQos.NO_EXPIRY_DATE ? "never" : expiryDate
                                    - System.currentTimeMillis()),
                    subscriptionId);

        if (expiryDate != SubscriptionQos.NO_EXPIRY_DATE) {
            SubscriptionEndRunnable endRunnable = new SubscriptionEndRunnable(subscriptionId);
            ScheduledFuture<?> subscriptionEndFuture = cleanupScheduler.schedule(endRunnable,
                                                                                 expiryDate,
                                                                                 TimeUnit.MILLISECONDS);
            subscriptionEndFutures.put(subscriptionId, subscriptionEndFuture);
        }
        return subscriptionId;
    }

    @Override
    public String registerAttributeSubscription(final String attributeName,
                                                Class<? extends TypeReference<?>> attributeTypeReference,
                                                AttributeSubscriptionListener<?> attributeSubscriptionCallback,
                                                final SubscriptionQos qos) {

        String subscriptionId = registerSubscription(qos);
        logger.info("Attribute subscription registered with Id: " + subscriptionId);
        subscriptionAttributeTypes.put(subscriptionId, attributeTypeReference);
        subscriptionListenerDirectory.put(subscriptionId, attributeSubscriptionCallback);

        if (qos instanceof HeartbeatSubscriptionInformation) {
            HeartbeatSubscriptionInformation heartbeat = (HeartbeatSubscriptionInformation) qos;

            // alerts only if alert after interval > 0
            if (heartbeat.getAlertAfterInterval() > 0) {

                logger.info("Will notify if updates are missed.");

                missedPublicationTimers.put(subscriptionId,
                                            new MissedPublicationTimer(qos.getExpiryDate(),
                                                                       heartbeat.getHeartbeat(),
                                                                       heartbeat.getAlertAfterInterval(),
                                                                       attributeSubscriptionCallback,
                                                                       subscriptionStates.get(subscriptionId)));
            }
        }

        return subscriptionId;
    }

    @Override
    public String registerBroadcastSubscription(String broadcastName,
                                                Map<String, Object> filterParameters,
                                                BroadcastSubscriptionListener broadcastSubscriptionListener,
                                                SubscriptionQos qos) {
        String subscriptionId = registerSubscription(qos);
        logger.info("Attribute subscription registered with Id: " + subscriptionId);
        broadcastSubscriptionListenerDirectory.put(subscriptionId, broadcastSubscriptionListener);
        return subscriptionId;
    }

    @Override
    public void unregisterSubscription(final String subscriptionId) {
        PubSubState subscriptionState = subscriptionStates.get(subscriptionId);
        if (subscriptionState != null) {
            logger.info("Called unregister / unsubscribe on subscription id= " + subscriptionId);
            removeSubscription(subscriptionId);
        } else {
            logger.info("Called unregister on a non/no longer existent subscription, used id= " + subscriptionId);
        }
    }

    @Override
    public void touchSubscriptionState(final String subscriptionId) {
        logger.info("Touching subscription state for id=" + subscriptionId);
        if (!subscriptionStates.containsKey(subscriptionId)) {
            logger.debug("No subscription state found for id: " + subscriptionId);
            return;
        }
        PubSubState subscriptionState = subscriptionStates.get(subscriptionId);
        subscriptionState.updateTimeOfLastPublication();
    }

    @Override
    public AttributeSubscriptionListener<?> getSubscriptionListener(final String subscriptionId) {
        if (!subscriptionStates.containsKey(subscriptionId)
                || !subscriptionListenerDirectory.containsKey(subscriptionId)) {
            logger.error("Received publication for not existing subscription callback with id=" + subscriptionId);
        }
        return subscriptionListenerDirectory.get(subscriptionId);

    }

    @Override
    public Class<? extends TypeReference<?>> getAttributeTypeReference(final String subscriptionId) {
        return subscriptionAttributeTypes.get(subscriptionId);
    }

    protected void removeSubscription(String subscriptionId) {
        if (missedPublicationTimers.containsKey(subscriptionId)) {
            missedPublicationTimers.get(subscriptionId).cancel();
            missedPublicationTimers.remove(subscriptionId);
        }
        ScheduledFuture<?> future = subscriptionEndFutures.remove(subscriptionId);
        if (future != null) {
            future.cancel(true);
        }
        subscriptionStates.remove(subscriptionId);
        subscriptionListenerDirectory.remove(subscriptionId);
        subscriptionAttributeTypes.remove(subscriptionId);

    }

    class SubscriptionEndRunnable implements Runnable {
        private String subscriptionId;

        public SubscriptionEndRunnable(String subscriptionId) {
            super();
            this.subscriptionId = subscriptionId;
        }

        @Override
        public void run() {
            // subscription expired, stop the missed publication timer and remove it from the maps
            removeSubscription(subscriptionId);
        }

    }

    @Override
    public void shutdown() {

    }

}
