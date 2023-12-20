/*
 * #%L
 * %%
 * Copyright (C) 2020 - 2021 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anySet;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;

import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.subscription.SubscriptionManagerImpl.SubscriptionState;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.util.MulticastWildcardRegexFactory;
import io.joynr.proxy.Future;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.MulticastSubscriptionQos;
import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.tests.testBroadcastInterface.LocationUpdateBroadcastListener;
import joynr.types.DiscoveryEntryWithMetaInfo;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionManagerTest {

    private String attributeName;
    private AttributeSubscriptionAdapter<Integer> attributeSubscriptionCallback;

    private PeriodicSubscriptionQos qos;
    private OnChangeSubscriptionQos onChangeQos;

    private PeriodicSubscriptionQos qosWithoutExpiryDate;
    private MessagingQos qosSettings;

    @Mock
    ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;

    private ConcurrentMap<String, AttributeSubscriptionListener<?>> attributeSubscriptionDirectory = spy(new ConcurrentHashMap<String, AttributeSubscriptionListener<?>>());
    private ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionDirectory = spy(new ConcurrentHashMap<String, BroadcastSubscriptionListener>());
    private ConcurrentMap<Pattern, Set<String>> multicastSubscribersDirectory = spy(new ConcurrentHashMap<Pattern, Set<String>>());
    private ConcurrentMap<String, SubscriptionState> subscriptionStates = spy(new ConcurrentHashMap<String, SubscriptionState>());
    private ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers = spy(new ConcurrentHashMap<String, MissedPublicationTimer>());
    private ConcurrentMap<String, Class<?>[]> unicastBroadcastTypes = spy(new ConcurrentHashMap<String, Class<?>[]>());
    private ConcurrentMap<Pattern, Class<?>[]> multicastBroadcastTypes = spy(new ConcurrentHashMap<Pattern, Class<?>[]>());
    private ConcurrentMap<String, Future<String>> subscriptionFutureMap = spy(new ConcurrentHashMap<String, Future<String>>());
    private ConcurrentMap<String, List<MulticastInformation>> subscriptionIdToMulticastInformationMap = spy(new ConcurrentHashMap<String, List<MulticastInformation>>());

    @Mock
    private SubscriptionState subscriptionState;

    private SubscriptionManager subscriptionManager;
    private String subscriptionId;
    private MissedPublicationTimer missedPublicationTimer;

    private String fromParticipantId;
    private String toParticipantId;
    private DiscoveryEntryWithMetaInfo toDiscoveryEntry;
    private Future<String> future;
    private Object proxy;

    @Mock
    private ConcurrentMap<String, Class<?>> subscriptionAttributeTypes;
    @Mock
    private ScheduledExecutorService cleanupScheduler;

    @Mock
    private Dispatcher dispatcher;

    @Mock
    private MulticastWildcardRegexFactory multicastWildcardRegexFactory;

    @Mock
    private MulticastReceiverRegistrar mockMulticastReceiverRegistrar;

    private ArgumentMatcher<SubscriptionState> matchesSubscriptionStateContainingProxy;

    @Before
    public void setUp() {
        subscriptionManager = new SubscriptionManagerImpl(attributeSubscriptionDirectory,
                                                          broadcastSubscriptionDirectory,
                                                          multicastSubscribersDirectory,
                                                          subscriptionStates,
                                                          missedPublicationTimers,
                                                          subscriptionEndFutures,
                                                          subscriptionAttributeTypes,
                                                          unicastBroadcastTypes,
                                                          multicastBroadcastTypes,
                                                          subscriptionFutureMap,
                                                          subscriptionIdToMulticastInformationMap,
                                                          cleanupScheduler,
                                                          dispatcher,
                                                          multicastWildcardRegexFactory,
                                                          mockMulticastReceiverRegistrar);
        subscriptionId = "testSubscription";

        attributeName = "testAttribute";
        attributeSubscriptionCallback = new AttributeSubscriptionAdapter<Integer>();
        long minInterval_ms = 100;
        long maxInterval_ms = 5000;
        long subscriptionDuration = 20000;
        long alertInterval_ms = 6000;
        long publicationTtl_ms = 1000;
        qos = new PeriodicSubscriptionQos();
        qos.setPeriodMs(maxInterval_ms);
        qos.setValidityMs(subscriptionDuration);
        qos.setAlertAfterIntervalMs(alertInterval_ms);
        qos.setPublicationTtlMs(publicationTtl_ms);

        onChangeQos = new OnChangeSubscriptionQos();
        onChangeQos.setMinIntervalMs(minInterval_ms);
        onChangeQos.setValidityMs(subscriptionDuration);
        onChangeQos.setPublicationTtlMs(publicationTtl_ms);

        qosWithoutExpiryDate = new PeriodicSubscriptionQos();
        qosWithoutExpiryDate.setPeriodMs(maxInterval_ms);
        qosWithoutExpiryDate.setValidityMs(SubscriptionQos.IGNORE_VALUE);
        qosWithoutExpiryDate.setAlertAfterIntervalMs(alertInterval_ms);
        qosWithoutExpiryDate.setPublicationTtlMs(publicationTtl_ms);

        missedPublicationTimer = new MissedPublicationTimer(System.currentTimeMillis() + subscriptionDuration,
                                                            maxInterval_ms,
                                                            alertInterval_ms,
                                                            attributeSubscriptionCallback,
                                                            subscriptionState,
                                                            subscriptionId);

        qosSettings = new MessagingQos();
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        toDiscoveryEntry.setParticipantId(toParticipantId);
        future = new Future<String>();
        proxy = new Object();

        Field proxyField;
        try {
            proxyField = SubscriptionState.class.getDeclaredField("proxy");
        } catch (NoSuchFieldException | SecurityException e) {
            throw new RuntimeException(e);
        }
        proxyField.setAccessible(true);
        matchesSubscriptionStateContainingProxy = new ArgumentMatcher<SubscriptionState>() {
            @Override
            public boolean matches(SubscriptionState argument) {
                SubscriptionState state = (SubscriptionState) argument;
                try {
                    return proxyField.get(state) == proxy;
                } catch (IllegalArgumentException | IllegalAccessException e) {
                    throw new RuntimeException(e);
                }
            }
        };
    }

    private void registerAttributeSubscription() {
        class IntegerReference extends TypeReference<Integer> {
        }

        @SuppressWarnings("unchecked")
        Future<String> future = mock(Future.class);
        AttributeSubscribeInvocation subscriptionRequest = new AttributeSubscribeInvocation(attributeName,
                                                                                            IntegerReference.class,
                                                                                            attributeSubscriptionCallback,
                                                                                            qos,
                                                                                            future,
                                                                                            proxy);
        subscriptionManager.registerAttributeSubscription(fromParticipantId,
                                                          new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                          subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();
    }

    @Test
    public void registerSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                       JsonGenerationException, JsonMappingException, IOException {
        registerAttributeSubscription();

        verify(attributeSubscriptionDirectory).put(Mockito.anyString(), Mockito.eq(attributeSubscriptionCallback));
        verify(subscriptionStates).put(Mockito.anyString(), argThat(matchesSubscriptionStateContainingProxy));

        ArgumentCaptor<Long> capturedExpiryInterval = ArgumentCaptor.forClass(Long.class);
        long remainingExpiryDateMs = onChangeQos.getExpiryDateMs() - System.currentTimeMillis();
        verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                          capturedExpiryInterval.capture(),
                                          Mockito.eq(TimeUnit.MILLISECONDS));
        assertTrue(capturedExpiryInterval.getValue() >= remainingExpiryDateMs
                && capturedExpiryInterval.getValue() <= remainingExpiryDateMs + 100);
        verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId), any());

        verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                   eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                   any(SubscriptionRequest.class),
                                                   any(MessagingQos.class));
    }

    @Test
    public void subscriptionEndRunnableDoesNotInterruptItself() throws JoynrSendBufferFullException,
                                                                JoynrMessageNotSentException, JsonGenerationException,
                                                                JsonMappingException, IOException {
        ScheduledFuture<?> future = mock(ScheduledFuture.class);
        //doReturn(future).when(subscriptionEndFutures).remove(eq(subscriptionId));
        doAnswer(new Answer<ScheduledFuture<?>>() {
            int call = 0;

            @Override
            public ScheduledFuture<?> answer(InvocationOnMock invocation) throws Throwable {
                call++;
                if (call == 2) {
                    return future;
                }
                return null;
            }

        }).when(subscriptionEndFutures).remove(anyString());

        registerAttributeSubscription();
        verify(subscriptionEndFutures).remove(eq(subscriptionId));

        ArgumentCaptor<Runnable> runnableCaptor = ArgumentCaptor.forClass(Runnable.class);
        verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId), any());
        verify(cleanupScheduler).schedule(runnableCaptor.capture(), anyLong(), Mockito.eq(TimeUnit.MILLISECONDS));

        Runnable r = runnableCaptor.getValue();
        r.run();
        verify(subscriptionEndFutures, times(2)).remove(eq(subscriptionId));
        verify(future, times(0)).cancel(anyBoolean());
    }

    @Test
    public void registerBroadcastSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                                JsonGenerationException, JsonMappingException, IOException {
        String broadcastName = "broadcastName";
        BroadcastSubscriptionListener broadcastSubscriptionListener = mock(LocationUpdateBroadcastListener.class);
        BroadcastSubscribeInvocation subscriptionRequest = new BroadcastSubscribeInvocation(broadcastName,
                                                                                            broadcastSubscriptionListener,
                                                                                            onChangeQos,
                                                                                            future,
                                                                                            proxy);
        subscriptionManager.registerBroadcastSubscription(fromParticipantId,
                                                          new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                          subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

        verify(broadcastSubscriptionDirectory).put(Mockito.anyString(), Mockito.eq(broadcastSubscriptionListener));
        verify(subscriptionStates).put(Mockito.anyString(), argThat(matchesSubscriptionStateContainingProxy));

        ArgumentCaptor<Long> capturedExpiryInterval = ArgumentCaptor.forClass(Long.class);
        long remainingExpiryDateMs = onChangeQos.getExpiryDateMs() - System.currentTimeMillis();
        verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                          capturedExpiryInterval.capture(),
                                          Mockito.eq(TimeUnit.MILLISECONDS));
        assertTrue(capturedExpiryInterval.getValue() >= remainingExpiryDateMs
                && capturedExpiryInterval.getValue() <= remainingExpiryDateMs + 100);
        verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId), any());

        verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                   eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                   any(SubscriptionRequest.class),
                                                   any(MessagingQos.class));
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
                                                                                future,
                                                                                proxy);
        subscriptionId = request.getSubscriptionId();
        subscriptionManager.registerAttributeSubscription(fromParticipantId,
                                                          new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                          request);

        verify(attributeSubscriptionDirectory).put(Mockito.anyString(), Mockito.eq(attributeSubscriptionCallback));
        verify(subscriptionStates).put(Mockito.anyString(), argThat(matchesSubscriptionStateContainingProxy));

        verify(cleanupScheduler, never()).schedule(Mockito.any(Runnable.class),
                                                   Mockito.anyLong(),
                                                   Mockito.any(TimeUnit.class));
        verify(subscriptionEndFutures, never()).put(Mockito.anyString(), Mockito.any(ScheduledFuture.class));

        verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                   eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                   any(SubscriptionRequest.class),
                                                   any(MessagingQos.class));
    }

    private static interface TestMulticastSubscriptionInterface {
        @JoynrMulticast(name = "myMulticast")
        void subscribeToMyMulticast();
    }

    @Test
    public void testRegisterMulticastSubscription() throws Exception {
        testRegisterMulticastSubscription(null);
    }

    @Test
    public void testRegisterMulticastSubscriptionWithExistingSubscriptionId() throws Exception {
        testRegisterMulticastSubscription(subscriptionId);
    }

    @Test
    public void testRegisterMulticastSubscriptionWithPartitions() throws Exception {
        testRegisterMulticastSubscription(null, "one", "two", "three");
    }

    private void testRegisterMulticastSubscription(String subscriptionId, String... partitions) throws Exception {
        Method method = TestMulticastSubscriptionInterface.class.getMethod("subscribeToMyMulticast", new Class[0]);
        BroadcastSubscriptionListener listener = spy(new BroadcastSubscriptionListener() {
            @Override
            public void onError(SubscriptionException error) {
            }

            @Override
            public void onSubscribed(String subscriptionId) {
            }

            @SuppressWarnings("unused")
            public void onReceive() {
            }
        });
        SubscriptionQos subscriptionQos = mock(MulticastSubscriptionQos.class);
        Object[] args;
        if (subscriptionId == null) {
            args = new Object[]{ listener, subscriptionQos, partitions };
        } else {
            args = new Object[]{ subscriptionId, listener, subscriptionQos, partitions };
        }
        String multicastId = MulticastIdUtil.createMulticastId(toParticipantId, "myMulticast", partitions);
        Set<String> subscriptionIdSet = new HashSet<>();
        Pattern multicastIdPattern = Pattern.compile(multicastId);
        multicastSubscribersDirectory.put(multicastIdPattern, subscriptionIdSet);
        when(multicastWildcardRegexFactory.createIdPattern(multicastId)).thenReturn(multicastIdPattern);

        MulticastSubscribeInvocation invocation = new MulticastSubscribeInvocation(method, args, future, proxy);

        DiscoveryEntryWithMetaInfo toDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        toDiscoveryEntry.setParticipantId(toParticipantId);
        subscriptionManager.registerMulticastSubscription(fromParticipantId,
                                                          new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                          invocation);

        verify(subscriptionStates).put(eq(invocation.getSubscriptionId()),
                                       argThat(matchesSubscriptionStateContainingProxy));
        verify(multicastSubscribersDirectory).put(any(Pattern.class), anySet());
        assertEquals(1, subscriptionIdSet.size());
        if (subscriptionId != null) {
            assertEquals(subscriptionId, subscriptionIdSet.iterator().next());
        }

        assertEquals(1, subscriptionIdToMulticastInformationMap.size());
        verify(mockMulticastReceiverRegistrar).addMulticastReceiver(multicastId,
                                                                    fromParticipantId,
                                                                    toDiscoveryEntry.getParticipantId());
        verify(dispatcher, never()).sendSubscriptionRequest(eq(fromParticipantId),
                                                            eq(new HashSet<>(Arrays.asList(toDiscoveryEntry))),
                                                            any(SubscriptionRequest.class),
                                                            any(MessagingQos.class));
    }

    @Test
    public void unregisterSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                         JsonGenerationException, JsonMappingException, IOException {
        ScheduledFuture<?> future = mock(ScheduledFuture.class);
        doReturn(future).when(subscriptionEndFutures).remove(eq(subscriptionId));

        when(subscriptionStates.get(subscriptionId)).thenReturn(subscriptionState);
        when(missedPublicationTimers.containsKey(subscriptionId)).thenReturn(true);
        when(missedPublicationTimers.get(subscriptionId)).thenReturn(missedPublicationTimer);
        subscriptionManager.unregisterSubscription(fromParticipantId,
                                                   new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                   subscriptionId,
                                                   qosSettings);

        verify(subscriptionStates).remove(eq(subscriptionId));
        verify(subscriptionState).stop();
        verify(subscriptionEndFutures).remove(eq(subscriptionId));
        verify(future).cancel(eq(true));

        verify(dispatcher,
               times(1)).sendSubscriptionStop(Mockito.eq(fromParticipantId),
                                              eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                              Mockito.eq(new SubscriptionStop(subscriptionId)),
                                              Mockito.any(MessagingQos.class));

    }

    @Test
    public void unregisterNonExistentSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                                    JsonGenerationException, JsonMappingException, IOException {

        when(subscriptionStates.get(subscriptionId)).thenReturn(null);
        subscriptionManager.unregisterSubscription(fromParticipantId,
                                                   new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry)),
                                                   subscriptionId,
                                                   qosSettings);

        verify(subscriptionStates, times(0)).remove(eq(subscriptionId));
        verify(subscriptionState, times(0)).stop();

        verify(dispatcher,
               times(0)).sendSubscriptionStop(Mockito.eq(fromParticipantId),
                                              eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                              Mockito.eq(new SubscriptionStop(subscriptionId)),
                                              Mockito.any(MessagingQos.class));

    }

    @Test
    public void unregisterMulticastSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                                  JsonGenerationException, JsonMappingException, IOException {

        String multicastId = "testMulticastId";
        String secondMulticastId = "secondMulticastId";
        String secondToParticipantId = "secondToParticipantId";
        DiscoveryEntryWithMetaInfo secondToDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        secondToDiscoveryEntry.setParticipantId(secondToParticipantId);
        List<MulticastInformation> multicastInformationList = new ArrayList<>();
        multicastInformationList.add(new MulticastInformation(multicastId, fromParticipantId, toParticipantId));
        multicastInformationList.add(new MulticastInformation(secondMulticastId,
                                                              fromParticipantId,
                                                              secondToParticipantId));

        String secondSubscriptionId = "secondSubscriptionId";
        String thirdMulticastId = "thirdMulticastId";
        String thirdToParticipantId = "thirdToParticipantId";
        List<MulticastInformation> secondMulticastInformationList = new ArrayList<>();
        secondMulticastInformationList.add(new MulticastInformation(thirdMulticastId,
                                                                    fromParticipantId,
                                                                    thirdToParticipantId));

        subscriptionIdToMulticastInformationMap.put(subscriptionId, multicastInformationList);
        subscriptionIdToMulticastInformationMap.put(secondSubscriptionId, secondMulticastInformationList);

        when(subscriptionStates.get(subscriptionId)).thenReturn(subscriptionState);
        when(missedPublicationTimers.containsKey(subscriptionId)).thenReturn(true);
        when(missedPublicationTimers.get(subscriptionId)).thenReturn(missedPublicationTimer);

        assertEquals(2, subscriptionIdToMulticastInformationMap.size());
        subscriptionManager.unregisterSubscription(fromParticipantId,
                                                   new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry,
                                                                                                         secondToDiscoveryEntry)),
                                                   subscriptionId,
                                                   qosSettings);

        verify(subscriptionStates).remove(eq(subscriptionId));
        verify(subscriptionState).stop();

        verify(dispatcher,
               times(0)).sendSubscriptionStop(Mockito.eq(fromParticipantId),
                                              eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry,
                                                                                                       secondToDiscoveryEntry))),
                                              Mockito.eq(new SubscriptionStop(subscriptionId)),
                                              Mockito.any(MessagingQos.class));
        verify(mockMulticastReceiverRegistrar, times(1)).removeMulticastReceiver(multicastId,
                                                                                 fromParticipantId,
                                                                                 toParticipantId);
        verify(mockMulticastReceiverRegistrar, times(1)).removeMulticastReceiver(secondMulticastId,
                                                                                 fromParticipantId,
                                                                                 secondToParticipantId);
        verify(mockMulticastReceiverRegistrar, times(0)).removeMulticastReceiver(thirdMulticastId,
                                                                                 fromParticipantId,
                                                                                 thirdToParticipantId);
        assertEquals(1, subscriptionIdToMulticastInformationMap.size());

    }

    @Test
    public void testHandleSubscriptionReplyWithError() {
        SubscriptionException subscriptionError = new SubscriptionException(subscriptionId);
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId, subscriptionError);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onFailure(eq(subscriptionError));
        verify(subscriptionStates).remove(eq(subscriptionId));
    }

    @Test
    public void testHandleSubscriptionReplyWithErrorWithSubscriptionListener() {
        SubscriptionException subscriptionError = new SubscriptionException(subscriptionId);
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId, subscriptionError);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        AttributeSubscriptionListener<?> subscriptionListener = mock(AttributeSubscriptionListener.class);
        attributeSubscriptionDirectory.put(subscriptionId, subscriptionListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onFailure(eq(subscriptionError));
        verify(subscriptionListener).onError(eq(subscriptionError));
        verify(subscriptionStates).remove(eq(subscriptionId));
    }

    @Test
    public void testHandleSubscriptionReplyWithErrorWithBroadcastListener() {
        SubscriptionException subscriptionError = new SubscriptionException(subscriptionId);
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId, subscriptionError);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        BroadcastSubscriptionListener broadcastListener = mock(BroadcastSubscriptionListener.class);
        broadcastSubscriptionDirectory.put(subscriptionId, broadcastListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onFailure(eq(subscriptionError));
        verify(broadcastListener).onError(eq(subscriptionError));
        verify(subscriptionStates).remove(eq(subscriptionId));
    }

    @Test
    public void testHandleSubscriptionReplyWithSuccess() {
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onSuccess(eq(subscriptionId));
    }

    @Test
    public void testHandleSubscriptionReplyWithSuccessWithSubscriptionListener() {
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        AttributeSubscriptionListener<?> subscriptionListener = mock(AttributeSubscriptionListener.class);
        attributeSubscriptionDirectory.put(subscriptionId, subscriptionListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onSuccess(eq(subscriptionId));
        verify(subscriptionListener).onSubscribed(eq(subscriptionId));
    }

    @Test
    public void testHandleSubscriptionReplyWithSuccessWithBroadcastListener() {
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        BroadcastSubscriptionListener broadcastListener = mock(BroadcastSubscriptionListener.class);
        broadcastSubscriptionDirectory.put(subscriptionId, broadcastListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onSuccess(eq(subscriptionId));
        verify(broadcastListener).onSubscribed(eq(subscriptionId));
    }

    private interface TestBroadcastListener extends BroadcastSubscriptionListener {
        void onReceive(String value);
    }

    @Test
    public void testHandleMulticastSubscriptionWithWildcardSubscribers() {
        Pattern subscriberOnePattern = Pattern.compile("one/[^/]+/three");
        String subscriberOneId = "one";
        multicastSubscribersDirectory.putIfAbsent(subscriberOnePattern,
                                                  new HashSet<String>(Arrays.asList(subscriberOneId)));

        Pattern subscriberTwoPattern = Pattern.compile("one/two/three");
        String subscriberTwoId = "two";
        multicastSubscribersDirectory.putIfAbsent(subscriberTwoPattern,
                                                  new HashSet<String>(Arrays.asList(subscriberTwoId)));

        Pattern subscriberThreePattern = Pattern.compile("four/five/six");
        String subscriberThreeId = "three";
        multicastSubscribersDirectory.putIfAbsent(subscriberThreePattern,
                                                  new HashSet<String>(Arrays.asList(subscriberThreeId)));

        @SuppressWarnings("rawtypes")
        Class[] types = new Class[]{ String.class };
        unicastBroadcastTypes.putIfAbsent(subscriberOneId, types);
        unicastBroadcastTypes.putIfAbsent(subscriberTwoId, types);
        unicastBroadcastTypes.putIfAbsent(subscriberThreeId, types);

        TestBroadcastListener listenerOne = mock(TestBroadcastListener.class);
        broadcastSubscriptionDirectory.putIfAbsent(subscriberOneId, listenerOne);

        TestBroadcastListener listenerTwo = mock(TestBroadcastListener.class);
        broadcastSubscriptionDirectory.putIfAbsent(subscriberTwoId, listenerTwo);

        TestBroadcastListener listenerThree = mock(TestBroadcastListener.class);
        broadcastSubscriptionDirectory.putIfAbsent(subscriberThreeId, listenerThree);

        subscriptionManager.handleMulticastPublication("one/two/three", new Object[]{ "value" });

        verify(listenerOne).onReceive(anyString());
        verify(listenerTwo).onReceive(anyString());
        verify(listenerThree, never()).onReceive(anyString());
    }

}
