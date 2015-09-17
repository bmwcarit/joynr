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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import io.joynr.dispatcher.Dispatcher;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;

import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.tests.testBroadcastInterface.LocationUpdateBroadcastListener;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.Maps;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionManagerTest {

    private String attributeName;
    private AttributeSubscriptionListener<?> attributeSubscriptionCallback;

    private SubscriptionQos qos;
    private OnChangeSubscriptionQos onChangeQos;

    private SubscriptionQos qosWithoutExpiryDate;
    private MessagingQos qosSettings;

    @Mock
    ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;

    private ConcurrentMap<String, AttributeSubscriptionListener<?>> attributeSubscriptionDirectory = spy(new ConcurrentHashMap<String, AttributeSubscriptionListener<?>>());
    private ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionDirectory = spy(new ConcurrentHashMap<String, BroadcastSubscriptionListener>());
    private ConcurrentMap<String, PubSubState> subscriptionStates = spy(new ConcurrentHashMap<String, PubSubState>());
    private ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers = spy(new ConcurrentHashMap<String, MissedPublicationTimer>());

    @Mock
    private PubSubState subscriptionState;

    private SubscriptionManager subscriptionManager;
    private String subscriptionId;
    private MissedPublicationTimer missedPublicationTimer;

    private String fromParticipantId;
    private String toParticipantId;

    @Mock
    private ConcurrentMap<String, Class<?>> subscriptionAttributeTypes;
    @Mock
    private ScheduledExecutorService cleanupScheduler;

    @Mock
    private Dispatcher dispatcher;

    @Before
    public void setUp() {
        subscriptionManager = new SubscriptionManagerImpl(attributeSubscriptionDirectory,
                                                          broadcastSubscriptionDirectory,
                                                          subscriptionStates,
                                                          missedPublicationTimers,
                                                          subscriptionEndFutures,
                                                          subscriptionAttributeTypes,
                                                          Maps.<String, Class<?>[]> newConcurrentMap(),
                                                          cleanupScheduler,
                                                          dispatcher);
        subscriptionId = "testSubscription";

        attributeName = "testAttribute";
        attributeSubscriptionCallback = new AttributeSubscriptionListener<Integer>() {
            @Override
            public void onError() {
                // TODO Auto-generated method stub
            }

            @Override
            public void onReceive(Integer value) {
                // TODO Auto-generated method stub

            }
        };
        long minInterval_ms = 100;
        long maxInterval_ms = 5000;
        long endDate_ms = System.currentTimeMillis() + 20000;
        long alertInterval_ms = 6000;
        long publicationTtl_ms = 1000;
        qos = new PeriodicSubscriptionQos(maxInterval_ms, endDate_ms, alertInterval_ms, publicationTtl_ms);

        onChangeQos = new OnChangeSubscriptionQos(minInterval_ms, endDate_ms, publicationTtl_ms);
        qosWithoutExpiryDate = new PeriodicSubscriptionQos(maxInterval_ms,
                                                           SubscriptionQos.NO_EXPIRY_DATE,
                                                           alertInterval_ms,
                                                           publicationTtl_ms);
        missedPublicationTimer = new MissedPublicationTimer(endDate_ms,
                                                            maxInterval_ms,
                                                            alertInterval_ms,
                                                            attributeSubscriptionCallback,
                                                            subscriptionState);

        qosSettings = new MessagingQos();
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
    }

    @Test
    public void registerSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                      JsonGenerationException, JsonMappingException, IOException {
        class IntegerReference extends TypeReference<Integer> {
        }

        AttributeSubscribeInvocation subscriptionRequest = new AttributeSubscribeInvocation(attributeName,
                                                                                            IntegerReference.class,
                                                                                            attributeSubscriptionCallback,
                                                                                            qos,
                                                                                            null);
        subscriptionManager.registerAttributeSubscription(fromParticipantId, toParticipantId, subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

        Mockito.verify(attributeSubscriptionDirectory).put(Mockito.anyString(),
                                                           Mockito.eq(attributeSubscriptionCallback));
        Mockito.verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        Mockito.verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                                  Mockito.eq(qos.getExpiryDate()),
                                                  Mockito.eq(TimeUnit.MILLISECONDS));
        Mockito.verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId),
                                                                     Mockito.any(ScheduledFuture.class));

        Mockito.verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                           eq(toParticipantId),
                                                           any(SubscriptionRequest.class),
                                                           any(MessagingQos.class),
                                                           eq(false));
    }

    @Test
    public void registerBroadcastSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                               JsonGenerationException, JsonMappingException, IOException {
        String broadcastName = "broadcastName";
        BroadcastSubscriptionListener broadcastSubscriptionListener = mock(LocationUpdateBroadcastListener.class);
        BroadcastSubscribeInvocation subscriptionRequest = new BroadcastSubscribeInvocation(broadcastName,
                                                                                            broadcastSubscriptionListener,
                                                                                            onChangeQos,
                                                                                            null);
        subscriptionManager.registerBroadcastSubscription(fromParticipantId, toParticipantId, subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

        Mockito.verify(broadcastSubscriptionDirectory).put(Mockito.anyString(),
                                                           Mockito.eq(broadcastSubscriptionListener));
        Mockito.verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        Mockito.verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                                  Mockito.eq(qos.getExpiryDate()),
                                                  Mockito.eq(TimeUnit.MILLISECONDS));
        Mockito.verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId),
                                                                     Mockito.any(ScheduledFuture.class));

        Mockito.verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                           eq(toParticipantId),
                                                           any(SubscriptionRequest.class),
                                                           any(MessagingQos.class),
                                                           eq(true));
    }

    @Test
    public void registerSubscriptionWithoutExpiryDate() throws JoynrSendBufferFullException,
                                                       JoynrMessageNotSentException, JsonGenerationException,
                                                       JsonMappingException, IOException {
        class IntegerReference extends TypeReference<Integer> {
        }

        AttributeSubscribeInvocation request = new AttributeSubscribeInvocation(attributeName,
                                                                                IntegerReference.class,
                                                                                attributeSubscriptionCallback,
                                                                                qosWithoutExpiryDate,
                                                                                null);
        subscriptionId = request.getSubscriptionId();
        subscriptionManager.registerAttributeSubscription(fromParticipantId, toParticipantId, request);

        Mockito.verify(attributeSubscriptionDirectory).put(Mockito.anyString(),
                                                           Mockito.eq(attributeSubscriptionCallback));
        Mockito.verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        Mockito.verify(cleanupScheduler, never()).schedule(Mockito.any(Runnable.class),
                                                           Mockito.anyLong(),
                                                           Mockito.any(TimeUnit.class));
        Mockito.verify(subscriptionEndFutures, never()).put(Mockito.anyString(), Mockito.any(ScheduledFuture.class));

        Mockito.verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                           eq(toParticipantId),
                                                           any(SubscriptionRequest.class),
                                                           any(MessagingQos.class),
                                                           eq(false));
    }

    @Test
    public void unregisterSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                        JsonGenerationException, JsonMappingException, IOException {

        Mockito.when(subscriptionStates.get(subscriptionId)).thenReturn(subscriptionState);
        Mockito.when(missedPublicationTimers.containsKey(subscriptionId)).thenReturn(true);
        Mockito.when(missedPublicationTimers.get(subscriptionId)).thenReturn(missedPublicationTimer);
        subscriptionManager.unregisterSubscription(fromParticipantId, toParticipantId, subscriptionId, qosSettings);

        Mockito.verify(subscriptionStates).get(Mockito.eq(subscriptionId));
        Mockito.verify(subscriptionState).stop();

        Mockito.verify(dispatcher, times(1)).sendSubscriptionStop(Mockito.eq(fromParticipantId),
                                                                  Mockito.eq(toParticipantId),
                                                                  Mockito.eq(new SubscriptionStop(subscriptionId)),
                                                                  Mockito.any(MessagingQos.class));

    }
}
