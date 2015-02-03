package io.joynr.pubsub.publication;

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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.MessagingQos;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import joynr.BroadcastFilterParameters;
import joynr.BroadcastSubscriptionRequest;
import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Maps;
import com.google.common.collect.Multimap;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class PublicationManagerImpl implements PublicationManager {
    private static final Logger logger = LoggerFactory.getLogger(PublicationManagerImpl.class);
    // Map ProviderId -> SubscriptionRequest
    private final Multimap<String, PublicationInformation> queuedSubscriptionRequests;
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

    private AttributePollInterpreter attributePollInterpreter;
    private ScheduledExecutorService cleanupScheduler;
    private RequestReplySender requestReplySender;

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

        public SubscriptionQos getQos() {
            return subscriptionRequest.getQos();
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

    @Inject
    public PublicationManagerImpl(AttributePollInterpreter attributePollInterpreter,
                                  RequestReplySender requestReplySender,
                                  @Named("joynr.scheduler.cleanup") ScheduledExecutorService cleanupScheduler) {
        super();
        this.requestReplySender = requestReplySender;
        this.cleanupScheduler = cleanupScheduler;
        this.queuedSubscriptionRequests = HashMultimap.create();
        this.subscriptionId2PublicationInformation = Maps.newConcurrentMap();
        this.publicationTimers = Maps.newConcurrentMap();
        this.subscriptionEndFutures = Maps.newConcurrentMap();
        this.unregisterAttributeListeners = Maps.newConcurrentMap();
        this.unregisterBroadcastListeners = Maps.newConcurrentMap();
        this.attributePollInterpreter = attributePollInterpreter;

    }

    private void handleSubscriptionRequest(PublicationInformation publicationInformation,
                                           SubscriptionRequest subscriptionRequest,
                                           RequestCaller requestCaller) {

        final String subscriptionId = subscriptionRequest.getSubscriptionId();

        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();

        try {
            Method method = findGetterForAttributeName(requestCaller.getClass(),
                                                       subscriptionRequest.getSubscribedToName());

            // Send initial publication
            triggerPublication(publicationInformation, requestCaller, method);

            boolean hasSubscriptionHeartBeat = subscriptionQos instanceof HeartbeatSubscriptionInformation;
            boolean isKeepAliveSubscription = subscriptionQos instanceof OnChangeWithKeepAliveSubscriptionQos;

            if (hasSubscriptionHeartBeat || isKeepAliveSubscription) {
                final PublicationTimer timer = new PublicationTimer(publicationInformation,
                                                                    method,
                                                                    requestCaller,
                                                                    requestReplySender,
                                                                    attributePollInterpreter);

                timer.startTimer();
                publicationTimers.put(subscriptionId, timer);
            }

            // Handle onChange subscriptions
            if (subscriptionQos instanceof OnChangeSubscriptionQos) {
                AttributeListener attributeListener = new AttributeListenerImpl(subscriptionId, this);
                String attributeName = subscriptionRequest.getSubscribedToName();
                requestCaller.registerAttributeListener(attributeName, attributeListener);
                unregisterAttributeListeners.put(subscriptionId, new UnregisterAttributeListener(requestCaller,
                                                                                                 attributeName,
                                                                                                 attributeListener));
            }
        } catch (NoSuchMethodException e) {
            cancelPublicationCreation(subscriptionId);
            logger.error("Error subscribing: {}. The provider does not have the requested attribute",
                         subscriptionRequest);
        } catch (IllegalArgumentException e) {
            cancelPublicationCreation(subscriptionId);
            logger.error("Error subscribing: " + subscriptionRequest, e);

        }

    }

    private void handleBroadcastSubscriptionRequest(String proxyParticipantId,
                                                    String providerParticipantId,
                                                    BroadcastSubscriptionRequest subscriptionRequest,
                                                    RequestCaller requestCaller) {
        logger.info("adding broadcast publication: " + subscriptionRequest.toString());
        BroadcastListener broadcastListener = new BroadcastListenerImpl(subscriptionRequest.getSubscriptionId(), this);
        String broadcastName = subscriptionRequest.getSubscribedToName();
        requestCaller.registerBroadcastListener(broadcastName, broadcastListener);
        unregisterBroadcastListeners.put(subscriptionRequest.getSubscriptionId(),
                                         new UnregisterBroadcastListener(requestCaller,
                                                                         broadcastName,
                                                                         broadcastListener));
    }

    @Override
    public void addSubscriptionRequest(String proxyParticipantId,
                                       String providerParticipantId,
                                       SubscriptionRequest subscriptionRequest,
                                       RequestCaller requestCaller) {

        // Check that this is a valid subscription
        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();
        long subscriptionEndDelay = subscriptionQos.getExpiryDate() == SubscriptionQos.NO_EXPIRY_DATE ? SubscriptionQos.NO_EXPIRY_DATE
                : subscriptionQos.getExpiryDate() - System.currentTimeMillis();

        if (subscriptionEndDelay < 0) {
            logger.error("Not adding subscription which ends in {} ms", subscriptionEndDelay);
            return;
        }

        // See if the publications for this subscription are already handled
        final String subscriptionId = subscriptionRequest.getSubscriptionId();
        if (publicationExists(subscriptionId)) {
            logger.info("updating publication: " + subscriptionRequest.toString());
            removePublication(subscriptionId);
        } else {
            logger.info("adding publication: " + subscriptionRequest.toString());
        }

        PublicationInformation publicationInformation = new PublicationInformation(providerParticipantId,
                                                                                   proxyParticipantId,
                                                                                   subscriptionRequest);
        subscriptionId2PublicationInformation.put(subscriptionId, publicationInformation);

        if (subscriptionRequest instanceof BroadcastSubscriptionRequest) {
            handleBroadcastSubscriptionRequest(proxyParticipantId,
                                               providerParticipantId,
                                               (BroadcastSubscriptionRequest) subscriptionRequest,
                                               requestCaller);
        } else {
            handleSubscriptionRequest(publicationInformation, subscriptionRequest, requestCaller);
        }

        if (subscriptionQos.getExpiryDate() != SubscriptionQos.NO_EXPIRY_DATE) {
            // Create a runnable to remove the publication when the subscription expires
            ScheduledFuture<?> subscriptionEndFuture = cleanupScheduler.schedule(new Runnable() {

                @Override
                public void run() {
                    logger.info("Publication with Id " + subscriptionId + " expired...");
                    removePublication(subscriptionId);
                }

            }, subscriptionEndDelay, TimeUnit.MILLISECONDS);
            subscriptionEndFutures.put(subscriptionId, subscriptionEndFuture);
        }
        logger.info("publication added: " + subscriptionRequest.toString());
    }

    private void cancelPublicationCreation(String subscriptionId) {
        subscriptionId2PublicationInformation.remove(subscriptionId);
        logger.error("Subscription request rejected. Removing publication.");
    }

    private boolean publicationExists(String subscriptionId) {
        return subscriptionId2PublicationInformation.containsKey(subscriptionId);
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
        subscriptionId2PublicationInformation.put(subscriptionRequest.getSubscriptionId(), publicationInformation);
    }

    protected void removePublication(String subscriptionId) {
        PublicationInformation publicationInformation = subscriptionId2PublicationInformation.remove(subscriptionId);

        // Remove (eventually) queued susbcriptionRequest
        queuedSubscriptionRequests.get(publicationInformation.getProviderParticipantId())
                                  .remove(publicationInformation);

        PublicationTimer publicationTimer = publicationTimers.remove(subscriptionId);
        if (publicationTimer != null) {
            publicationTimer.cancel();
        }

        ScheduledFuture<?> future = subscriptionEndFutures.remove(subscriptionId);
        if (future != null) {
            future.cancel(true);
        }

        UnregisterAttributeListener unregisterAttributeListener = unregisterAttributeListeners.remove(subscriptionId);
        if (unregisterAttributeListener != null) {
            unregisterAttributeListener.unregister();
        }
        UnregisterBroadcastListener unregisterBroadcastListener = unregisterBroadcastListeners.remove(subscriptionId);
        if (unregisterBroadcastListener != null) {
            unregisterBroadcastListener.unregister();
        }
    }

    // Class that holds information needed to unregister attribute listener
    static class UnregisterAttributeListener {
        final RequestCaller requestCaller;
        final String attributeName;
        final AttributeListener attributeListener;

        public UnregisterAttributeListener(RequestCaller requestCaller,
                                           String attributeName,
                                           AttributeListener attributeListener) {
            this.requestCaller = requestCaller;
            this.attributeName = attributeName;
            this.attributeListener = attributeListener;
        }

        public void unregister() {
            requestCaller.unregisterAttributeListener(attributeName, attributeListener);
        }
    }

    // Class that holds information needed to unregister broadcast listener
    static class UnregisterBroadcastListener {
        final RequestCaller requestCaller;
        final String broadcastName;
        final BroadcastListener broadcastListener;

        public UnregisterBroadcastListener(RequestCaller requestCaller,
                                           String broadcastName,
                                           BroadcastListener broadcastListener) {
            this.requestCaller = requestCaller;
            this.broadcastName = broadcastName;
            this.broadcastListener = broadcastListener;
        }

        public void unregister() {
            requestCaller.unregisterBroadcastListener(broadcastName, broadcastListener);
        }
    }

    @Override
    public void stopPublication(String subscriptionId) {
        removePublication(subscriptionId);
    }

    @Override
    public void stopPublicationByProviderId(String providerParticipantId) {
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
        long expiryDate = publicationInformation.subscriptionRequest.getQos().getExpiryDate();
        logger.debug("ExpiryDate - System.currentTimeMillis: " + (expiryDate - System.currentTimeMillis()));
        return (expiryDate != SubscriptionQos.NO_EXPIRY_DATE && expiryDate <= System.currentTimeMillis());
    }

    @Override
    public void restoreQueuedSubscription(String providerId, RequestCaller requestCaller) {
        Collection<PublicationInformation> queuedRequests = queuedSubscriptionRequests.get(providerId);
        Iterator<PublicationInformation> queuedRequestsIterator = queuedRequests.iterator();
        while (queuedRequestsIterator.hasNext()) {
            PublicationInformation publicInformation = queuedRequestsIterator.next();
            queuedRequestsIterator.remove();
            if (!isExpired(publicInformation)) {
                addSubscriptionRequest(publicInformation.getProxyParticipantId(),
                                       publicInformation.getProviderParticipantId(),
                                       publicInformation.subscriptionRequest,
                                       requestCaller);
            }
        }
    }

    @Override
    public void attributeValueChanged(String subscriptionId, Object value) {

        if (subscriptionId2PublicationInformation.containsKey(subscriptionId)) {
            PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);

            if (isExpired(publicationInformation)) {
                stopPublication(subscriptionId);
            } else {
                PublicationTimer publicationTimer = publicationTimers.get(subscriptionId);
                if (publicationTimer != null) {
                    // used by OnChangedWithKeepAlive
                    publicationTimer.sendPublicationNow(value);
                } else {
                    sendPublication(value, publicationInformation);
                }

                logger.info("attribute changed for subscription id: {} sending publication if delay > minInterval.",
                            subscriptionId);
            }

        } else {
            logger.error("subscription {} has expired but attributeValueChanged has been called", subscriptionId);
            return;
        }

    }

    @Override
    public void eventOccurred(String subscriptionId, List<BroadcastFilter> filters, Object... values) {
        if (subscriptionId2PublicationInformation.containsKey(subscriptionId)) {
            PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);

            if (processFilterChain(publicationInformation, filters, values)) {
                sendPublication(Arrays.asList(values), publicationInformation);
                logger.info("event occured changed for subscription id: {} sending publication: ", subscriptionId);
            }

        } else {
            logger.error("subscription {} has expired but eventOccurred has been called", subscriptionId);
            return;
        }

    }

    private boolean processFilterChain(PublicationInformation publicationInformation,
                                       List<BroadcastFilter> filters,
                                       Object[] values) {

        if (filters != null && filters.size() > 0) {
            boolean filterResult = true;
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

                    filterResult &= (Boolean) filterMethod.invoke(filter, args);
                } catch (Exception e) {
                    logger.error("processFilterChain error: {}", e.getMessage());
                    throw new IllegalStateException("processFilterChain: Error in reflection calling filters.", e);
                }
            }
            return filterResult;
        } else {
            return true;
        }
    }

    private void sendPublication(Object value, PublicationInformation publicationInformation) {
        SubscriptionPublication publication = new SubscriptionPublication(value,
                                                                          publicationInformation.getSubscriptionId());
        try {
            MessagingQos messagingQos = new MessagingQos();
            messagingQos.setTtl_ms(publicationInformation.subscriptionRequest.getQos().getPublicationTtl());
            requestReplySender.sendSubscriptionPublication(publicationInformation.providerParticipantId,
                                                           publicationInformation.proxyParticipantId,
                                                           publication,
                                                           messagingQos);
            // TODO handle exceptions during publication. See JOYNR-2113
        } catch (JoynrException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        } catch (JsonGenerationException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        } catch (JsonMappingException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        } catch (IOException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        }
    }

    private void triggerPublication(PublicationInformation publicationInformation,
                                    RequestCaller requestCaller,
                                    Method method) {
        sendPublication(attributePollInterpreter.execute(requestCaller, method), publicationInformation);
    }

    private Method findGetterForAttributeName(Class<?> clazz, String attributeName) throws NoSuchMethodException {
        String attributeGetterName = "get" + attributeName.toUpperCase().charAt(0)
                + attributeName.subSequence(1, attributeName.length());
        return ReflectionUtils.findMethodByParamTypes(clazz, attributeGetterName, new Class[]{});

    }

    @Override
    public void shutdown() {

    }
}
