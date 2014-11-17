package io.joynr.pubsub.publication;

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

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.publication.PublicationManagerImpl.PublicationInformation;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import joynr.BroadcastSubscriptionRequest;
import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.tests.testProvider;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.AdditionalMatchers;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.Multimap;

@SuppressWarnings("unchecked")
@RunWith(MockitoJUnitRunner.class)
public class PublicationManagerTest {

    private static final String PROVIDER_PARTICIPANT_ID = "providerParticipantId";
    private static final String PROXY_PARTICIPANT_ID = "proxyParticipantId";
    private static final long NOW_MS = System.currentTimeMillis();
    private static final long DURATION_MS = 5000;
    private static final long END_DATE_MS = NOW_MS + DURATION_MS;

    private static final String SUBSCRIPTION_ID = "PublicationTest_id";

    PublicationManager publicationManager;

    @Mock
    Multimap<String, PublicationInformation> queuedSubscriptionRequests;
    @Mock
    ConcurrentMap<String, PublicationInformation> subscriptionId2SubscriptionRequest;
    @Mock
    ConcurrentMap<String, PubSubState> publicationStates;
    @Mock
    ConcurrentMap<String, PublicationTimer> publicationTimers;
    @Mock
    ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;
    @Mock
    ScheduledExecutorService cleanupScheduler;
    @Mock
    AttributePollInterpreter attributePollInterpreter;

    private RequestCaller requestCaller;

    @Mock
    private SubscriptionRequest subscriptionRequest;

    private PublicationInformation publicationInformation;

    @Mock
    private RequestReplySender messageSender;

    private SubscriptionQos subscriptionQos;

    private PeriodicSubscriptionQos subscriptionQosWithoutExpiryDate;

    @Mock
    private JoynrProvider provider;

    @Mock
    private PublicationTimer publicationTimer;

    @Mock
    private ScheduledFuture scheduledFuture;

    @Mock
    private PublicationInformation subscriptionRequest1;

    @Mock
    private PublicationInformation subscriptionRequest2;

    @Before
    public void setUp() {
        publicationInformation = new PublicationInformation(PROVIDER_PARTICIPANT_ID,
                                                            PROXY_PARTICIPANT_ID,
                                                            subscriptionRequest);
        publicationManager = new PublicationManagerImpl(queuedSubscriptionRequests,
                                                        subscriptionId2SubscriptionRequest,
                                                        publicationStates,
                                                        publicationTimers,
                                                        subscriptionEndFutures,
                                                        attributePollInterpreter,
                                                        messageSender,
                                                        cleanupScheduler);
        subscriptionQos = new PeriodicSubscriptionQos(400, END_DATE_MS, 500, 1000);
        subscriptionQosWithoutExpiryDate = new PeriodicSubscriptionQos(100, SubscriptionQos.NO_EXPIRY_DATE, 500, 1000);
        RequestCallerFactory requestCallerFactory = new RequestCallerFactory();
        requestCaller = requestCallerFactory.create(provider, testProvider.class);
    }

    @After
    public void tearDown() {
        publicationManager.shutdown();
    }

    @Test
    public void addPublication() {
        addPublicationMockBehaviour(subscriptionQos);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);
        verifyPublicationIsAdded(true);
    }

    @Test
    public void addPublicationWithoutExpiryDate() {
        int n = 3;
        addPublicationMockBehaviour(subscriptionQosWithoutExpiryDate);

        final int[] count = new int[]{ 0 };
        Answer<Void> answer = new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                count[0]++;
                return null;
            }
        };
        try {
            Mockito.doAnswer(answer)
                   .when(messageSender)
                   .sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                eq(PROXY_PARTICIPANT_ID),
                                                any(SubscriptionPublication.class),
                                                any(MessagingQos.class));
            publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                      PROVIDER_PARTICIPANT_ID,
                                                      subscriptionRequest,
                                                      requestCaller);
            verifyPublicationIsAdded(false);
            Assert.assertEquals(1, count[0]);

            boolean correctUpdate = false;
            // the publication is done completely asynchronously in an own thread. Thus, we cannot give any timing
            // guarantees
            for (int i = 0; i < 5; i++) {
                count[0] = 0;
                Thread.sleep(n * subscriptionQosWithoutExpiryDate.getPeriod());
                if (n >= count[0]) {
                    correctUpdate = true;
                    break;
                }
            }
            Assert.assertTrue(n + " instead of " + count[0] + " publications are expected within time interval of "
                    + (n * subscriptionQosWithoutExpiryDate.getPeriod()) + "ms", correctUpdate);
        } catch (Exception e) {
            Assert.fail(e.getMessage());
        }
    }

    private void verifyPublicationIsAdded(boolean expectCleanupTask) {
        verify(subscriptionId2SubscriptionRequest).putIfAbsent(eq(SUBSCRIPTION_ID), eq(publicationInformation));
        verify(publicationStates).putIfAbsent(eq(SUBSCRIPTION_ID), any(PubSubState.class));
        verify(publicationTimers).putIfAbsent(eq(SUBSCRIPTION_ID), any(PublicationTimer.class));
        if (expectCleanupTask) {
            verify(cleanupScheduler).schedule(any(Runnable.class),
                                              AdditionalMatchers.leq(DURATION_MS),
                                              eq(TimeUnit.MILLISECONDS));
            verify(subscriptionEndFutures).putIfAbsent(eq(SUBSCRIPTION_ID), any(ScheduledFuture.class));
        } else {
            verify(subscriptionEndFutures, never()).putIfAbsent(Mockito.anyString(), any(ScheduledFuture.class));
            verify(cleanupScheduler, never()).schedule(any(Runnable.class), Mockito.anyLong(), any(TimeUnit.class));
        }
    }

    private void addPublicationMockBehaviour(SubscriptionQos subscriptionQos) {
        when(subscriptionRequest.getSubscriptionId()).thenReturn(SUBSCRIPTION_ID);
        when(subscriptionRequest.getQos()).thenReturn(subscriptionQos);
        when(subscriptionRequest.getSubscribedToName()).thenReturn("testAttribute");
    }

    @Test
    public void stopPublication() {

        MockBehaviourOnStop(SUBSCRIPTION_ID);

        publicationManager.stopPublication(SUBSCRIPTION_ID);
        verifyPublicationIsDeleted(SUBSCRIPTION_ID);

    }

    private void verifyPublicationIsDeleted(String subscriptionId) {
        verify(queuedSubscriptionRequests, Mockito.atLeast(1)).containsKey(PROVIDER_PARTICIPANT_ID);
        verify(queuedSubscriptionRequests, Mockito.atLeast(1)).removeAll(eq(PROVIDER_PARTICIPANT_ID));
        verify(subscriptionId2SubscriptionRequest).remove(eq(subscriptionId));
        verify(publicationStates).remove(eq(subscriptionId));
        verify(publicationTimers).remove(eq(subscriptionId));

        verify(subscriptionEndFutures).remove(eq(subscriptionId));
    }

    private void MockBehaviourOnStop(String subscriptionId) {
        Mockito.when(publicationTimers.containsKey(subscriptionId)).thenReturn(true);
        Mockito.when(publicationTimers.get(subscriptionId)).thenReturn(publicationTimer);
        Mockito.when(subscriptionEndFutures.remove(subscriptionId)).thenReturn(scheduledFuture);
        Mockito.when(subscriptionId2SubscriptionRequest.containsKey(subscriptionId)).thenReturn(true);
        Mockito.when(subscriptionId2SubscriptionRequest.get(subscriptionId)).thenReturn(publicationInformation);
        Mockito.when(queuedSubscriptionRequests.containsKey(PROVIDER_PARTICIPANT_ID)).thenReturn(true);
    }

    @Test
    public void removeAllSubscriptions() {
        String providerId = "providerId";
        Collection<PublicationInformation> subscriptionRequests = new ArrayList<PublicationInformation>();

        subscriptionRequests.add(subscriptionRequest1);
        subscriptionRequests.add(subscriptionRequest2);
        Mockito.when(subscriptionId2SubscriptionRequest.values()).thenReturn(subscriptionRequests);
        Mockito.when(subscriptionRequest1.getSubscriptionId()).thenReturn("subscriptionId1");
        Mockito.when(subscriptionRequest1.getProviderParticipantId()).thenReturn(providerId);
        Mockito.when(subscriptionRequest2.getSubscriptionId()).thenReturn("subscriptionId2");
        Mockito.when(subscriptionRequest2.getProviderParticipantId()).thenReturn(providerId);
        MockBehaviourOnStop("subscriptionId1");
        MockBehaviourOnStop("subscriptionId2");
        publicationManager.stopPublicationByProviderId(providerId);

        verify(subscriptionRequest1).getSubscriptionId();
        verify(subscriptionRequest2).getSubscriptionId();

        verifyPublicationIsDeleted("subscriptionId1");
        verifyPublicationIsDeleted("subscriptionId2");

    }

    @Test
    public void restorePublication() {
        String providerId = "providerId";
        Collection<PublicationInformation> subscriptionRequests = new ArrayList<PublicationInformation>();
        subscriptionRequests.add(publicationInformation);

        Mockito.when(queuedSubscriptionRequests.get(providerId)).thenReturn(subscriptionRequests);
        Mockito.when(subscriptionRequest.getQos())
               .thenReturn(new PeriodicSubscriptionQos(1000, System.currentTimeMillis() + 30000, 1500, 1000));
        addPublicationMockBehaviour(subscriptionQos);

        publicationManager.restoreQueuedSubscription(providerId, requestCaller);

        verify(queuedSubscriptionRequests).remove(providerId, publicationInformation);
        verifyPublicationIsAdded(true);
    }

    @Test
    public void broadcastPublicationIsSent() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                            JsonGenerationException, JsonMappingException, IOException {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter, messageSender, cleanupScheduler);

        long minInterval_ms = 0;
        long ttl = 1000;
        SubscriptionQos qos = new OnChangeSubscriptionQos(minInterval_ms, SubscriptionQos.NO_EXPIRY_DATE, ttl);
        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   qos);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        Object[] eventValues = { "value1", "value2" };

        publicationManager.eventOccurred(subscriptionRequest.getSubscriptionId(),
                                         new ArrayList<BroadcastFilter>(),
                                         eventValues);

        ArgumentCaptor<SubscriptionPublication> publicationCaptured = ArgumentCaptor.forClass(SubscriptionPublication.class);
        ArgumentCaptor<MessagingQos> qosCaptured = ArgumentCaptor.forClass(MessagingQos.class);

        verify(messageSender).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                          eq(PROXY_PARTICIPANT_ID),
                                                          publicationCaptured.capture(),
                                                          qosCaptured.capture());

        Object[] response = (Object[]) publicationCaptured.getValue().getResponse();
        assertEquals(eventValues[0], response[0]);
        assertEquals(eventValues[1], response[1]);
        assertEquals(ttl, qosCaptured.getValue().getRoundTripTtl_ms());

    }
}
