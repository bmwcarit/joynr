package io.joynr.pubsub.subscription;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;

import java.util.UUID;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class SubscriptionManagerImpl implements SubscriptionManager {

    private ConcurrentMap<String, AttributeSubscriptionListener<?>> subscriptionListenerDirectory;
    private ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionListenerDirectory;
    private ConcurrentMap<String, Class<?>> subscriptionTypes;
    private ConcurrentMap<String, Class<?>[]> subscriptionBroadcastTypes;
    private ConcurrentMap<String, PubSubState> subscriptionStates;
    private ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers;
    private ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures; // These futures will be needed if a
    // subscription
    // should be updated with a new end time

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionManagerImpl.class);
    private ScheduledExecutorService cleanupScheduler;

    @Inject
    public SubscriptionManagerImpl(@Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler) {
        super();
        this.cleanupScheduler = cleanupScheduler;
        this.subscriptionListenerDirectory = Maps.newConcurrentMap();
        this.broadcastSubscriptionListenerDirectory = Maps.newConcurrentMap();
        this.subscriptionStates = Maps.newConcurrentMap();
        this.missedPublicationTimers = Maps.newConcurrentMap();
        this.subscriptionEndFutures = Maps.newConcurrentMap();
        this.subscriptionTypes = Maps.newConcurrentMap();
        this.subscriptionBroadcastTypes = Maps.newConcurrentMap();

    }

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public SubscriptionManagerImpl(ConcurrentMap<String, AttributeSubscriptionListener<?>> attributeSubscriptionDirectory,
                                   ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionDirectory,
                                   ConcurrentMap<String, PubSubState> subscriptionStates,
                                   ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers,
                                   ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures,
                                   ConcurrentMap<String, Class<?>> subscriptionAttributeTypes,
                                   ConcurrentMap<String, Class<?>[]> subscriptionBroadcastTypes,
                                   ScheduledExecutorService cleanupScheduler) {
        super();
        this.subscriptionListenerDirectory = attributeSubscriptionDirectory;
        this.broadcastSubscriptionListenerDirectory = broadcastSubscriptionDirectory;
        this.subscriptionStates = subscriptionStates;
        this.missedPublicationTimers = missedPublicationTimers;
        this.subscriptionEndFutures = subscriptionEndFutures;
        this.subscriptionTypes = subscriptionAttributeTypes;
        this.subscriptionBroadcastTypes = subscriptionBroadcastTypes;
        this.cleanupScheduler = cleanupScheduler;
    }

    private void cancelExistingSubscriptionEndRunnable(String subscriptionId) {
        ScheduledFuture<?> scheduledFuture = subscriptionEndFutures.get(subscriptionId);
        if (scheduledFuture != null) {
            scheduledFuture.cancel(false);
        }
    }

    private void registerSubscription(final SubscriptionQos qos, String subscriptionId) {

        cancelExistingSubscriptionEndRunnable(subscriptionId);

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
    }

    @Override
    public void registerAttributeSubscription(AttributeSubscribeInvocation request) {
        if (!request.hasSubscriptionId()) {
            request.setSubscriptionId(UUID.randomUUID().toString());
        }
        SubscriptionQos qos = request.getQos();
        registerSubscription(qos, request.getSubscriptionId());
        logger.info("Attribute subscription registered with Id: " + request.getSubscriptionId());
        subscriptionTypes.put(request.getSubscriptionId(), request.getAttributeTypeReference());
        subscriptionListenerDirectory.put(request.getSubscriptionId(), request.getAttributeSubscriptionListener());

        if (qos instanceof HeartbeatSubscriptionInformation) {
            HeartbeatSubscriptionInformation heartbeat = (HeartbeatSubscriptionInformation) qos;

            // alerts only if alert after interval > 0
            if (heartbeat.getAlertAfterInterval() > 0) {

                logger.info("Will notify if updates are missed.");

                missedPublicationTimers.put(request.getSubscriptionId(),
                                            new MissedPublicationTimer(qos.getExpiryDate(),
                                                                       heartbeat.getHeartbeat(),
                                                                       heartbeat.getAlertAfterInterval(),
                                                                       request.getAttributeSubscriptionListener(),
                                                                       subscriptionStates.get(request.getSubscriptionId())));
            }
        }
    }

    @Override
    public void registerBroadcastSubscription(BroadcastSubscribeInvocation subscriptionRequest) {
        if (!subscriptionRequest.hasSubscriptionId()) {
            subscriptionRequest.setSubscriptionId(UUID.randomUUID().toString());
        }
        String subscriptionId = subscriptionRequest.getSubscriptionId();
        registerSubscription(subscriptionRequest.getQos(), subscriptionId);
        logger.info("Attribute subscription registered with Id: " + subscriptionId);
        subscriptionBroadcastTypes.put(subscriptionId, subscriptionRequest.getOutParameterTypes());
        broadcastSubscriptionListenerDirectory.put(subscriptionId,
                                                   subscriptionRequest.getBroadcastSubscriptionListener());
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

    @SuppressWarnings("unchecked")
    @Override
    public <T> AttributeSubscriptionListener<T> getSubscriptionListener(final String subscriptionId) {
        if (!subscriptionStates.containsKey(subscriptionId)
                || !subscriptionListenerDirectory.containsKey(subscriptionId)) {
            logger.error("Received publication for not existing subscription callback with id=" + subscriptionId);
        }
        return (AttributeSubscriptionListener<T>) subscriptionListenerDirectory.get(subscriptionId);

    }

    @Override
    public BroadcastSubscriptionListener getBroadcastSubscriptionListener(String subscriptionId) {
        if (!subscriptionStates.containsKey(subscriptionId)
                || !broadcastSubscriptionListenerDirectory.containsKey(subscriptionId)) {
            logger.error("Received publication for not existing subscription callback with id=" + subscriptionId);
        }
        return broadcastSubscriptionListenerDirectory.get(subscriptionId);

    }

    @Override
    public boolean isBroadcast(String subscriptionId) {
        return broadcastSubscriptionListenerDirectory.containsKey(subscriptionId);
    }

    @Override
    public Class<?> getAttributeType(final String subscriptionId) {
        return subscriptionTypes.get(subscriptionId);
    }

    @Override
    public Class<?>[] getBroadcastOutParameterTypes(String subscriptionId) {
        return subscriptionBroadcastTypes.get(subscriptionId);
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
        subscriptionTypes.remove(subscriptionId);

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
}
