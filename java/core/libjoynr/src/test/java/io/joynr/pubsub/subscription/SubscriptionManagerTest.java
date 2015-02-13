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

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.core.type.TypeReference;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionManagerTest {

    private String attributeName;
    private AttributeSubscriptionListener<?> attributeSubscriptionCallback;

    private SubscriptionQos qos;
    private OnChangeSubscriptionQos onChangeQos;

    private SubscriptionQos qosWithoutExpiryDate;

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

    @Mock
    private ConcurrentMap<String, Class<?>> subscriptionAttributeTypes;
    @Mock
    private ScheduledExecutorService cleanupScheduler;

    @Before
    public void setUp() {
        subscriptionManager = new SubscriptionManagerImpl(attributeSubscriptionDirectory,
                                                          broadcastSubscriptionDirectory,
                                                          subscriptionStates,
                                                          missedPublicationTimers,
                                                          subscriptionEndFutures,
                                                          subscriptionAttributeTypes,
                                                          cleanupScheduler);
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
    }

    @Test
    public void registerSubscription() {
        class IntegerReference extends TypeReference<Integer> {
        }

        AttributeSubscribeInvocation subscriptionRequest = new AttributeSubscribeInvocation(attributeName,
                                                                                            IntegerReference.class,
                                                                                            attributeSubscriptionCallback,
                                                                                            qos,
                                                                                            null);
        subscriptionManager.registerAttributeSubscription(subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

        Mockito.verify(attributeSubscriptionDirectory).put(Mockito.anyString(),
                                                           Mockito.eq(attributeSubscriptionCallback));
        Mockito.verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        Mockito.verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                                  Mockito.eq(qos.getExpiryDate()),
                                                  Mockito.eq(TimeUnit.MILLISECONDS));
        Mockito.verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId),
                                                                     Mockito.any(ScheduledFuture.class));
    }

    @Test
    public void registerBroadcastSubscription() {
        String broadcastName = "broadcastName";
        BroadcastSubscriptionListener broadcastSubscriptionCallback = mock(BroadcastSubscriptionListener.class);
        BroadcastSubscribeInvocation subscriptionRequest = new BroadcastSubscribeInvocation(broadcastName,
                                                                                            broadcastSubscriptionCallback,
                                                                                            onChangeQos,
                                                                                            null);
        subscriptionManager.registerBroadcastSubscription(subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

        Mockito.verify(broadcastSubscriptionDirectory).put(Mockito.anyString(),
                                                           Mockito.eq(broadcastSubscriptionCallback));
        Mockito.verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        Mockito.verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                                  Mockito.eq(qos.getExpiryDate()),
                                                  Mockito.eq(TimeUnit.MILLISECONDS));
        Mockito.verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId),
                                                                     Mockito.any(ScheduledFuture.class));
    }

    @Test
    public void registerSubscriptionWithoutExpiryDate() {
        class IntegerReference extends TypeReference<Integer> {
        }

        AttributeSubscribeInvocation request = new AttributeSubscribeInvocation(attributeName,
                                                                                IntegerReference.class,
                                                                                attributeSubscriptionCallback,
                                                                                qosWithoutExpiryDate,
                                                                                null);
        subscriptionId = request.getSubscriptionId();
        subscriptionManager.registerAttributeSubscription(request);

        Mockito.verify(attributeSubscriptionDirectory).put(Mockito.anyString(),
                                                           Mockito.eq(attributeSubscriptionCallback));
        Mockito.verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        Mockito.verify(cleanupScheduler, never()).schedule(Mockito.any(Runnable.class),
                                                           Mockito.anyLong(),
                                                           Mockito.any(TimeUnit.class));
        Mockito.verify(subscriptionEndFutures, never()).put(Mockito.anyString(), Mockito.any(ScheduledFuture.class));
    }

    @Test
    public void unregisterSubscription() {

        Mockito.when(subscriptionStates.get(subscriptionId)).thenReturn(subscriptionState);
        Mockito.when(missedPublicationTimers.containsKey(subscriptionId)).thenReturn(true);
        Mockito.when(missedPublicationTimers.get(subscriptionId)).thenReturn(missedPublicationTimer);
        subscriptionManager.unregisterSubscription(subscriptionId);

        Mockito.verify(subscriptionStates).get(Mockito.eq(subscriptionId));
        Mockito.verify(subscriptionState).stop();
    }

    @Test
    public void touchSubscriptionState() {
        Mockito.when(subscriptionStates.containsKey(subscriptionId)).thenReturn(true);
        Mockito.when(subscriptionStates.get(subscriptionId)).thenReturn(subscriptionState);
        subscriptionManager.touchSubscriptionState(subscriptionId);

        Mockito.verify(subscriptionStates).containsKey(subscriptionId);
        Mockito.verify(subscriptionStates).get(subscriptionId);
        Mockito.verify(subscriptionState).updateTimeOfLastPublication();

    }
}
