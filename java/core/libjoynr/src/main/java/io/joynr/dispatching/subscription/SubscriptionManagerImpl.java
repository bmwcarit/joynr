package io.joynr.dispatching.subscription;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import com.google.common.collect.Maps;
import com.google.common.collect.Sets;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Future;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.proxy.invocation.SubscriptionInvocation;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.BroadcastSubscriptionRequest;
import joynr.MulticastSubscriptionRequest;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Singleton
public class SubscriptionManagerImpl implements SubscriptionManager {

    private ConcurrentMap<String, AttributeSubscriptionListener<?>> subscriptionListenerDirectory;
    private ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionListenerDirectory;
    private ConcurrentMap<String, Set<String>> multicastSubscribersDirectory;
    private ConcurrentMap<String, Future<String>> subscriptionFutureMap;
    private ConcurrentMap<String, Class<?>> subscriptionTypes;
    private ConcurrentMap<String, Class<?>[]> subscriptionBroadcastTypes;
    private ConcurrentMap<String, PubSubState> subscriptionStates;
    private ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers;
    private ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures; // These futures will be needed if a
    // subscription
    // should be updated with a new end time

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionManagerImpl.class);
    private ScheduledExecutorService cleanupScheduler;
    private Dispatcher dispatcher;

    @Inject
    public SubscriptionManagerImpl(@Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler,
                                   Dispatcher dispatcher) {
        super();
        this.cleanupScheduler = cleanupScheduler;
        this.dispatcher = dispatcher;
        this.subscriptionListenerDirectory = Maps.newConcurrentMap();
        this.broadcastSubscriptionListenerDirectory = Maps.newConcurrentMap();
        this.multicastSubscribersDirectory = Maps.newConcurrentMap();
        this.subscriptionStates = Maps.newConcurrentMap();
        this.missedPublicationTimers = Maps.newConcurrentMap();
        this.subscriptionEndFutures = Maps.newConcurrentMap();
        this.subscriptionTypes = Maps.newConcurrentMap();
        this.subscriptionBroadcastTypes = Maps.newConcurrentMap();
        this.subscriptionFutureMap = Maps.newConcurrentMap();
    }

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public SubscriptionManagerImpl(ConcurrentMap<String, AttributeSubscriptionListener<?>> attributeSubscriptionDirectory,
                                   ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionDirectory,
                                   ConcurrentMap<String, Set<String>> multicastSubscribersDirectory,
                                   ConcurrentMap<String, PubSubState> subscriptionStates,
                                   ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers,
                                   ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures,
                                   ConcurrentMap<String, Class<?>> subscriptionAttributeTypes,
                                   ConcurrentMap<String, Class<?>[]> subscriptionBroadcastTypes,
                                   ConcurrentMap<String, Future<String>> subscriptionFutureMap,
                                   ScheduledExecutorService cleanupScheduler,
                                   Dispatcher dispatcher) {
        super();
        this.subscriptionListenerDirectory = attributeSubscriptionDirectory;
        this.broadcastSubscriptionListenerDirectory = broadcastSubscriptionDirectory;
        this.multicastSubscribersDirectory = multicastSubscribersDirectory;
        this.subscriptionStates = subscriptionStates;
        this.missedPublicationTimers = missedPublicationTimers;
        this.subscriptionEndFutures = subscriptionEndFutures;
        this.subscriptionTypes = subscriptionAttributeTypes;
        this.subscriptionBroadcastTypes = subscriptionBroadcastTypes;
        this.cleanupScheduler = cleanupScheduler;
        this.dispatcher = dispatcher;
        this.subscriptionFutureMap = subscriptionFutureMap;
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

        long expiryDate = qos.getExpiryDateMs();
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
    public void registerAttributeSubscription(String fromParticipantId,
                                              Set<String> toParticipantIds,
                                              final AttributeSubscribeInvocation request) {
        registerSubscription(fromParticipantId,
                             toParticipantIds,
                             request,
                             new RegisterDataAndCreateSubscriptionRequest() {
                                 @Override
                                 public SubscriptionRequest execute() {
                                     SubscriptionQos qos = request.getQos();
                                     logger.debug("Attribute subscription registered with Id: "
                                             + request.getSubscriptionId());
                                     subscriptionTypes.put(request.getSubscriptionId(),
                                                           request.getAttributeTypeReference());
                                     subscriptionListenerDirectory.put(request.getSubscriptionId(),
                                                                       request.getAttributeSubscriptionListener());
                                     subscriptionFutureMap.put(request.getSubscriptionId(), request.getFuture());

                                     if (qos instanceof HeartbeatSubscriptionInformation) {
                                         HeartbeatSubscriptionInformation heartbeat = (HeartbeatSubscriptionInformation) qos;

                                         // alerts only if alert after interval > 0
                                         if (heartbeat.getAlertAfterIntervalMs() > 0) {

                                             logger.info("Will notify if updates are missed.");

                                             missedPublicationTimers.put(request.getSubscriptionId(),
                                                                         new MissedPublicationTimer(qos.getExpiryDateMs(),
                                                                                                    heartbeat.getPeriodMs(),
                                                                                                    heartbeat.getAlertAfterIntervalMs(),
                                                                                                    request.getAttributeSubscriptionListener(),
                                                                                                    subscriptionStates.get(request.getSubscriptionId()),
                                                                                                    request.getSubscriptionId()));
                                         }
                                     }

                                     return new SubscriptionRequest(request.getSubscriptionId(),
                                                                    request.getAttributeName(),
                                                                    request.getQos());
                                 }
                             });

    }

    @Override
    public void registerBroadcastSubscription(String fromParticipantId,
                                              Set<String> toParticipantIds,
                                              final BroadcastSubscribeInvocation request) {
        registerSubscription(fromParticipantId,
                             toParticipantIds,
                             request,
                             new RegisterDataAndCreateSubscriptionRequest() {
                                 @Override
                                 public SubscriptionRequest execute() {
                                     String subscriptionId = request.getSubscriptionId();
                                     logger.debug("Broadcast subscription registered with Id: " + subscriptionId);
                                     subscriptionBroadcastTypes.put(subscriptionId, request.getOutParameterTypes());
                                     broadcastSubscriptionListenerDirectory.put(subscriptionId,
                                                                                request.getBroadcastSubscriptionListener());
                                     return new BroadcastSubscriptionRequest(request.getSubscriptionId(),
                                                                             request.getBroadcastName(),
                                                                             request.getFilterParameters(),
                                                                             request.getQos());
                                 }
                             });
    }

    @Override
    public void registerMulticastSubscription(String fromParticipantId,
                                              Set<String> toParticipantIds,
                                              final MulticastSubscribeInvocation multicastSubscribeInvocation) {
        for (String toParticipantId : toParticipantIds) {
            final String multicastId = MulticastIdUtil.createMulticastId(toParticipantId,
                                                                         multicastSubscribeInvocation.getSubscriptionName());
            registerSubscription(fromParticipantId,
                                 toParticipantIds,
                                 multicastSubscribeInvocation,
                                 new RegisterDataAndCreateSubscriptionRequest() {
                                     @Override
                                     public SubscriptionRequest execute() {
                                         String subscriptionId = multicastSubscribeInvocation.getSubscriptionId();
                                         logger.debug("Multicast subscription registered with Id: " + subscriptionId);
                                         if (!multicastSubscribersDirectory.containsKey(multicastId)) {
                                             multicastSubscribersDirectory.putIfAbsent(multicastId,
                                                                                       Sets.<String> newHashSet());
                                         }
                                         multicastSubscribersDirectory.get(multicastId).add(subscriptionId);
                                         subscriptionBroadcastTypes.put(multicastId,
                                                                        multicastSubscribeInvocation.getOutParameterTypes());
                                         broadcastSubscriptionListenerDirectory.put(subscriptionId,
                                                                                    multicastSubscribeInvocation.getListener());
                                         return new MulticastSubscriptionRequest(multicastId,
                                                                                 multicastSubscribeInvocation.getSubscriptionId(),
                                                                                 multicastSubscribeInvocation.getSubscriptionName(),
                                                                                 multicastSubscribeInvocation.getQos());
                                     }
                                 });
        }
    }

    private static interface RegisterDataAndCreateSubscriptionRequest {
        SubscriptionRequest execute();
    }

    private void registerSubscription(String fromParticipantId,
                                      Set<String> toParticipantIds,
                                      SubscriptionInvocation subscriptionInvocation,
                                      RegisterDataAndCreateSubscriptionRequest registerDataAndCreateSubscriptionRequest) {
        if (!subscriptionInvocation.hasSubscriptionId()) {
            subscriptionInvocation.setSubscriptionId(UUID.randomUUID().toString());
        }
        String subscriptionId = subscriptionInvocation.getSubscriptionId();
        subscriptionFutureMap.put(subscriptionId, subscriptionInvocation.getFuture());
        registerSubscription(subscriptionInvocation.getQos(), subscriptionId);

        SubscriptionRequest subscriptionRequest = registerDataAndCreateSubscriptionRequest.execute();

        MessagingQos messagingQos = new MessagingQos();
        SubscriptionQos qos = subscriptionRequest.getQos();
        if (qos.getExpiryDateMs() == SubscriptionQos.NO_EXPIRY_DATE) {
            messagingQos.setTtl_ms(SubscriptionQos.INFINITE_SUBSCRIPTION);
        } else {
            messagingQos.setTtl_ms(qos.getExpiryDateMs() - System.currentTimeMillis());
        }

        dispatcher.sendSubscriptionRequest(fromParticipantId, toParticipantIds, subscriptionRequest, messagingQos);
    }

    @Override
    public void unregisterSubscription(String fromParticipantId,
                                       Set<String> toParticipantIds,
                                       String subscriptionId,
                                       MessagingQos qosSettings) {
        PubSubState subscriptionState = subscriptionStates.get(subscriptionId);
        if (subscriptionState != null) {
            logger.info("Called unregister / unsubscribe on subscription id= " + subscriptionId);
            removeSubscription(subscriptionId);
        } else {
            logger.info("Called unregister on a non/no longer existent subscription, used id= " + subscriptionId);
        }

        SubscriptionStop subscriptionStop = new SubscriptionStop(subscriptionId);

        dispatcher.sendSubscriptionStop(fromParticipantId,
                                        toParticipantIds,
                                        subscriptionStop,
                                        new MessagingQos(qosSettings));
    }

    @Override
    public void handleBroadcastPublication(String subscriptionId, Object[] broadcastValues) {
        BroadcastSubscriptionListener broadcastSubscriptionListener = getBroadcastSubscriptionListener(subscriptionId);

        try {
            Class<?>[] broadcastTypes = getParameterTypesForBroadcastPublication(broadcastValues);
            Method receive = broadcastSubscriptionListener.getClass().getDeclaredMethod("onReceive", broadcastTypes);
            if (!receive.isAccessible()) {
                receive.setAccessible(true);
            }
            receive.invoke(broadcastSubscriptionListener, broadcastValues);

        } catch (IllegalAccessException | InvocationTargetException | NoSuchMethodException | SecurityException e) {
            logger.error("Broadcast publication could not be processed", e);
        }
    }

    @Override
    public void handleMulticastPublication(String multicastId, Object[] publicizedValues) {
        for (String subscriptionId : multicastSubscribersDirectory.get(multicastId)) {
            handleBroadcastPublication(subscriptionId, publicizedValues);
        }
    }

    @Override
    public <T> void handleAttributePublication(String subscriptionId, T attributeValue) {
        touchSubscriptionState(subscriptionId);
        AttributeSubscriptionListener<T> listener = getSubscriptionListener(subscriptionId);
        if (listener == null) {
            logger.error("No subscription listener found for incoming publication!");
        } else {
            listener.onReceive(attributeValue);
        }
    }

    @Override
    public <T> void handleAttributePublicationError(String subscriptionId, JoynrRuntimeException error) {
        touchSubscriptionState(subscriptionId);
        AttributeSubscriptionListener<T> listener = getSubscriptionListener(subscriptionId);
        if (listener == null) {
            logger.error("No subscription listener found for incoming publication!");
        } else {
            listener.onError(error);
        }
    }

    @Override
    public void handleSubscriptionReply(final SubscriptionReply subscriptionReply) {
        String subscriptionId = subscriptionReply.getSubscriptionId();

        if (subscriptionReply.getError() == null) {
            if (subscriptionFutureMap.containsKey(subscriptionId)) {
                subscriptionFutureMap.remove(subscriptionId).onSuccess(subscriptionId);
            }

            if (subscriptionListenerDirectory.containsKey(subscriptionId)) {
                subscriptionListenerDirectory.get(subscriptionId).onSubscribed(subscriptionId);
            } else if (broadcastSubscriptionListenerDirectory.containsKey(subscriptionId)) {
                broadcastSubscriptionListenerDirectory.get(subscriptionId).onSubscribed(subscriptionId);
            } else {
                logger.warn("No subscription listener found for incoming subscription reply for subscription ID {}!",
                            subscriptionId);
            }
        } else {
            logger.debug("Handling subscription reply with error: {}", subscriptionReply.getError());
            if (subscriptionFutureMap.containsKey(subscriptionId)) {
                subscriptionFutureMap.remove(subscriptionId).onFailure(subscriptionReply.getError());
            }

            if (subscriptionListenerDirectory.containsKey(subscriptionId)) {
                subscriptionListenerDirectory.remove(subscriptionId).onError(subscriptionReply.getError());
            } else if (broadcastSubscriptionListenerDirectory.containsKey(subscriptionId)) {
                broadcastSubscriptionListenerDirectory.remove(subscriptionId).onError(subscriptionReply.getError());
            } else {
                logger.warn("No subscription listener found for incoming subscription reply for subscription ID {}! Error message: {}",
                            subscriptionId,
                            subscriptionReply.getError().getMessage());
            }
            subscriptionTypes.remove(subscriptionId);
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

    private void removeSubscription(String subscriptionId) {
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
        subscriptionBroadcastTypes.remove(subscriptionId);
        broadcastSubscriptionListenerDirectory.remove(subscriptionId);
        for (Set<String> subscriptionIds : multicastSubscribersDirectory.values()) {
            subscriptionIds.remove(subscriptionId);
        }
        subscriptionTypes.remove(subscriptionId);
    }

    private Class<?>[] getParameterTypesForBroadcastPublication(Object[] broadcastValues) {
        List<Class<?>> parameterTypes = new ArrayList<Class<?>>(broadcastValues.length);
        for (int i = 0; i < broadcastValues.length; i++) {
            parameterTypes.add(broadcastValues[i].getClass());
        }
        return parameterTypes.toArray(new Class<?>[parameterTypes.size()]);
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
