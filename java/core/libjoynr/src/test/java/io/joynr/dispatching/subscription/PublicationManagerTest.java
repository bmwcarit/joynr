/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import static org.hamcrest.Matchers.contains;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.dispatching.DispatcherImpl;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerFactory;
import io.joynr.dispatching.subscription.PublicationManagerImpl.PublicationInformation;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.Deferred;
import io.joynr.provider.Promise;
import io.joynr.provider.ProviderContainer;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.publication.BroadcastFilter;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.MultiMap;
import joynr.BroadcastFilterParameters;
import joynr.BroadcastSubscriptionRequest;
import joynr.MulticastPublication;
import joynr.MulticastSubscriptionQos;
import joynr.MulticastSubscriptionRequest;
import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.tests.testBroadcastInterface;
import joynr.tests.testLocationUpdateSelectiveBroadcastFilter;
import joynr.tests.testLocationUpdateWithSpeedSelectiveBroadcastFilter;
import joynr.tests.testProvider;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;

@RunWith(MockitoJUnitRunner.class)
public class PublicationManagerTest {

    private static final String PROVIDER_PARTICIPANT_ID = "providerParticipantId";
    private static final String PROXY_PARTICIPANT_ID = "proxyParticipantId";

    private static final String SUBSCRIPTION_ID = "PublicationTest_id";
    private static final boolean SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED = true;
    private static final boolean SUBSCRIPTIONSREQUEST_PERSISTENCY_DISABLED = false;

    ScheduledExecutorService cleanupScheduler;
    PublicationManagerImpl publicationManager;

    @Mock
    AttributePollInterpreter attributePollInterpreter;
    @Mock
    private ProviderDirectory providerDirectory;
    @Mock
    private RoutingTable routingTable;
    @Mock
    private DispatcherImpl dispatcher;
    @Mock
    private testProvider provider;

    @Mock
    private AbstractSubscriptionPublisher subscriptionPublisher;

    private RequestCaller requestCaller;

    @Mock
    private ProviderContainer providerContainer;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    String valueToPublish = "valuePublished";

    @Before
    public void setUp() {
        Deferred<String> valueToPublishDeferred = new Deferred<String>();
        valueToPublishDeferred.resolve(valueToPublish);
        Promise<Deferred<String>> valueToPublishPromise = new Promise<Deferred<String>>(valueToPublishDeferred);

        cleanupScheduler = new ScheduledThreadPoolExecutor(1);
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        providerDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        Mockito.mock(SubscriptionRequestStorage.class),
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        requestCaller = new RequestCallerFactory().create(provider);
        when(providerContainer.getProviderProxy()).thenReturn(requestCaller.getProxy());
        when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(null);

        doReturn(Optional.of(valueToPublishPromise)).when(attributePollInterpreter)
                                                    .execute(any(ProviderContainer.class), any(Method.class));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 4000)
    public void doNotDelayBroadcastPublicationBurstsForOnChangeSubscriptionsWithoutMinInterval() throws Exception {
        int subscriptionLength = 500;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setValidityMs(subscriptionLength);
        qos.setPublicationTtlMs(400);

        String subscriptionId = "subscriptionId";
        String proxyId = "proxyId";
        String providerId = "providerId";
        String broadcastName = "location";

        ProviderDirectory requestCallerDirectory = mock(ProviderDirectory.class);
        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(subscriptionId,
                                                                                   broadcastName,
                                                                                   new BroadcastFilterParameters(),
                                                                                   qos);
        PublicationManager publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                                           dispatcher,
                                                                           requestCallerDirectory,
                                                                           routingTable,
                                                                           cleanupScheduler,
                                                                           Mockito.mock(SubscriptionRequestStorage.class),
                                                                           shutdownNotifier,
                                                                           SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        when(requestCallerDirectory.get(eq(providerId))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest);
        List<BroadcastFilter> noFilters = new ArrayList<>();

        int nrBroadcasts = 100;
        for (int i = 0; i < nrBroadcasts; i++) {
            publicationManager.broadcastOccurred(subscriptionId, noFilters, 2 * i + 1);
        }

        Thread.sleep(subscriptionLength);

        verify(dispatcher, times(nrBroadcasts)).sendSubscriptionPublication(eq(providerId),
                                                                            (Set<String>) argThat(contains(proxyId)),
                                                                            any(SubscriptionPublication.class),
                                                                            any(MessagingQos.class));

        Thread.sleep(subscriptionLength);

        verify(dispatcher).sendSubscriptionReply(eq(providerId),
                                                 eq(proxyId),
                                                 any(SubscriptionReply.class),
                                                 any(MessagingQos.class));
        //(eq(subscriptionRequest), eq(proxyId), eq(providerId));
        verifyNoMoreInteractions(dispatcher);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 4000)
    public void delayBroadcastPublicationBurstsForOnChangeSubscriptions() throws Exception {
        int subscriptionLength = 500;
        int minIntervalMs = 100;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(minIntervalMs);
        qos.setValidityMs(subscriptionLength);
        qos.setPublicationTtlMs(400);

        String subscriptionId = "subscriptionId";
        String proxyId = "proxyId";
        String providerId = "providerId";
        String broadcastName = "location";

        ProviderDirectory providerDirectory = mock(ProviderDirectory.class);
        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(subscriptionId,
                                                                                   broadcastName,
                                                                                   new BroadcastFilterParameters(),
                                                                                   qos);
        PublicationManager publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                                           dispatcher,
                                                                           providerDirectory,
                                                                           routingTable,
                                                                           cleanupScheduler,
                                                                           Mockito.mock(SubscriptionRequestStorage.class),
                                                                           shutdownNotifier,
                                                                           SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);
        verifyNoMoreInteractions(routingTable);

        when(providerDirectory.get(eq(providerId))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest);
        List<BroadcastFilter> noFilters = new ArrayList<>();

        publicationManager.broadcastOccurred(subscriptionId, noFilters, 0);
        int nrIterations = 10;

        for (int i = 1; i <= nrIterations; i++) {
            publicationManager.broadcastOccurred(subscriptionId, noFilters, i);
        }

        Thread.sleep(minIntervalMs);

        publicationManager.broadcastOccurred(subscriptionId, noFilters, nrIterations + 1);
        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(providerId),
                                                                 (Set<String>) argThat(contains(proxyId)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));
        verify(dispatcher).sendSubscriptionReply(anyString(),
                                                 anyString(),
                                                 any(SubscriptionReply.class),
                                                 any(MessagingQos.class));

        Thread.sleep(subscriptionLength);
        verifyNoMoreInteractions(dispatcher);

        reset(dispatcher);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 4000)
    public void delayAttributePublicationBurstsForOnChangeSubscriptions() throws Exception {
        int subscriptionLength = 500;
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(400);
        qos.setValidityMs(subscriptionLength);
        qos.setPublicationTtlMs(400);

        String subscriptionId = "subscriptionId";
        String proxyId = "proxyId";
        String providerId = "providerId";
        String attributeName = "location";

        ProviderDirectory providerDirectory = mock(ProviderDirectory.class);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, qos);
        PublicationManager publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                                           dispatcher,
                                                                           providerDirectory,
                                                                           routingTable,
                                                                           cleanupScheduler,
                                                                           Mockito.mock(SubscriptionRequestStorage.class),
                                                                           shutdownNotifier,
                                                                           SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);
        verifyNoMoreInteractions(routingTable);

        when(providerDirectory.get(eq(providerId))).thenReturn(providerContainer);

        final Semaphore onReceiveSemaphore = new Semaphore(0);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                onReceiveSemaphore.release();
                return null;
            }
        }).when(dispatcher).sendSubscriptionPublication(eq(providerId),
                                                        (Set<String>) argThat(contains(proxyId)),
                                                        any(SubscriptionPublication.class),
                                                        any(MessagingQos.class));

        publicationManager.addSubscriptionRequest(proxyId, providerId, subscriptionRequest);

        /* a burst of attribute changes only leads to one publication send out */
        for (int i = 0; i < 3; i++) {
            publicationManager.attributeValueChanged(subscriptionId, i);
        }

        assertTrue(onReceiveSemaphore.tryAcquire(2, subscriptionLength + 1000, TimeUnit.MILLISECONDS));

        assertFalse(onReceiveSemaphore.tryAcquire(1, Math.max(subscriptionLength, 200), TimeUnit.MILLISECONDS));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void addPublicationWithExpiryDate() throws Exception {
        long publicationActiveForMs = 300;

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0);
        qos.setValidityMs(publicationActiveForMs);
        qos.setPublicationTtlMs(1000);

        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);

        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        // sending initial value plus the attributeValueChanged
        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        Thread.sleep(publicationActiveForMs);
        reset(dispatcher);

        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);

        verify(dispatcher,
               timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  any(MessagingQos.class));
    }

    @Test(timeout = 3000)
    public void addAndStopMulticastSubscriptionNonQueued() throws Exception {
        long publicationActiveForMs = 300;
        final String multicastId = "multicastId";
        final String multicastName = "multicastName";

        SubscriptionQos qos = new MulticastSubscriptionQos().setValidityMs(publicationActiveForMs);
        SubscriptionRequest subscriptionRequest = new MulticastSubscriptionRequest(multicastId,
                                                                                   SUBSCRIPTION_ID,
                                                                                   multicastName,
                                                                                   qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        verify(routingTable, times(0)).incrementReferenceCount(PROXY_PARTICIPANT_ID);

        verify(dispatcher, times(1)).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                           eq(PROXY_PARTICIPANT_ID),
                                                           any(SubscriptionReply.class),
                                                           any(MessagingQos.class));

        publicationManager.stopPublication(SUBSCRIPTION_ID);
        verify(routingTable, times(0)).remove(PROXY_PARTICIPANT_ID);
        verifyNoMoreInteractions(dispatcher, routingTable);
    }

    @Test(timeout = 3000)
    public void addMulticastSubscriptionQueuedWithExpiryDate() throws Exception {
        long publicationActiveForMs = 200;
        final String multicastId = "multicastId";
        final String multicastName = "multicastName";

        SubscriptionQos qos = new MulticastSubscriptionQos().setValidityMs(publicationActiveForMs);
        SubscriptionRequest subscriptionRequest = new MulticastSubscriptionRequest(multicastId,
                                                                                   SUBSCRIPTION_ID,
                                                                                   multicastName,
                                                                                   qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(null);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        checkRequestsQueue(subscriptionRequest);

        verify(routingTable, times(0)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(dispatcher, times(0)).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                           eq(PROXY_PARTICIPANT_ID),
                                                           any(SubscriptionReply.class),
                                                           any(MessagingQos.class));

        Thread.sleep(publicationActiveForMs * 2);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(routingTable, times(0)).remove(PROXY_PARTICIPANT_ID);
        verifyNoMoreInteractions(dispatcher, routingTable);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void addPublicationWithoutExpiryDate() throws Exception {
        int period = 200;
        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos();
        qos.setPeriodMs(period).setValidityMs(SubscriptionQos.IGNORE_VALUE);
        qos.setAlertAfterIntervalMs(500).setPublicationTtlMs(1000);

        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);

        verify(dispatcher,
               timeout(period * 5).times(6)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                         (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                         any(SubscriptionPublication.class),
                                                                         any(MessagingQos.class));

        reset(dispatcher);
        verify(routingTable, times(0)).remove(any());
        publicationManager.stopPublication(SUBSCRIPTION_ID);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);

        verify(dispatcher,
               timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void startAndStopPeriodicPublication() throws Exception {
        int period = 200;
        int testLengthMax = 3000;
        PeriodicSubscriptionQos qos = new PeriodicSubscriptionQos();
        qos.setPeriodMs(200).setValidityMs(testLengthMax).setPublicationTtlMs(testLengthMax);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(0)).remove(any());
        verify(dispatcher,
               timeout(period * 5).times(6)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                         (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                         any(SubscriptionPublication.class),
                                                                         any(MessagingQos.class));

        reset(routingTable, dispatcher);
        publicationManager.stopPublication(SUBSCRIPTION_ID);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
        verify(dispatcher,
               timeout(testLengthMax).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                            (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                            any(SubscriptionPublication.class),
                                                                            any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void stopAllPublicationsFromProvider() throws Exception {
        String subscriptionId1 = "subscriptionid1";
        String subscriptionId2 = "subscriptionid2";
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos();
        qos.setMinIntervalMs(0).setValidityMs(3000).setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qos);
        SubscriptionRequest subscriptionRequest2 = new SubscriptionRequest(subscriptionId2, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest2);
        verify(routingTable, times(0)).remove(any()); //The reference count is bound do the subscription ID. 
        verify(routingTable, times(2)).incrementReferenceCount(PROXY_PARTICIPANT_ID);

        publicationManager.attributeValueChanged(subscriptionId1, valueToPublish);
        publicationManager.attributeValueChanged(subscriptionId2, valueToPublish);

        // sending initial values for 2 subscriptions, plus the 2 attributeValueChanged
        verify(dispatcher, times(4)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        reset(dispatcher);
        publicationManager.entryRemoved(PROVIDER_PARTICIPANT_ID);
        verify(routingTable, times(2)).remove(PROXY_PARTICIPANT_ID);
        verifyNoMoreInteractions(routingTable);

        publicationManager.attributeValueChanged(subscriptionId1, valueToPublish);
        publicationManager.attributeValueChanged(subscriptionId2, valueToPublish);

        verify(dispatcher,
               timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                  (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                  any(SubscriptionPublication.class),
                                                                  any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void restoreQueuedPublications() throws Exception {
        int period = 200;
        String subscriptionId1 = "subscriptionid1";
        String subscriptionId2 = "subscriptionid2";
        long validityMs = 3000;
        SubscriptionQos qosNoExpiry = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setAlertAfterIntervalMs(500)
                                                                   .setPublicationTtlMs(1000);
        SubscriptionQos qosExpires = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                  .setValidityMs(validityMs)
                                                                  .setAlertAfterIntervalMs(500)
                                                                  .setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosNoExpiry);
        SubscriptionRequest subscriptionRequest2 = new SubscriptionRequest(subscriptionId2, "location", qosExpires);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest2);
        verify(routingTable, times(2)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verifyNoMoreInteractions(routingTable); //validityMs no checked by this test case

        Thread.sleep(period);
        verify(dispatcher, times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);

        verify(dispatcher,
               timeout(period * 5).times(12)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                          (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                          any(SubscriptionPublication.class),
                                                                          any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void restoreQueuedPublicationsForUnknownAttribute() throws Exception {
        int period = 200;
        String subscriptionId1 = "subscriptionid1";
        String subscriptionId2 = "subscriptionid2";
        long validityMs = 3000;
        SubscriptionQos qosNoExpiry = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setAlertAfterIntervalMs(500)
                                                                   .setPublicationTtlMs(1000);
        SubscriptionQos qosExpires = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                  .setValidityMs(validityMs)
                                                                  .setAlertAfterIntervalMs(500)
                                                                  .setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1,
                                                                           "unknownAttribute",
                                                                           qosNoExpiry);
        SubscriptionRequest subscriptionRequest2 = new SubscriptionRequest(subscriptionId2, "location", qosExpires);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest2);
        verify(routingTable, times(2)).incrementReferenceCount(PROXY_PARTICIPANT_ID);

        Thread.sleep(period);
        verifyNoMoreInteractions(routingTable);
        verify(dispatcher, times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);

        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                 eq(PROXY_PARTICIPANT_ID),
                                                 argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                     @Override
                                                     public boolean matches(Object argument) {
                                                         SubscriptionReply reply = (SubscriptionReply) argument;
                                                         String subscriptionId = reply.getSubscriptionId();
                                                         return null != reply.getError()
                                                                 && subscriptionId1.equals(subscriptionId);
                                                     }
                                                 }),
                                                 any(MessagingQos.class));
        verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                 eq(PROXY_PARTICIPANT_ID),
                                                 argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                     @Override
                                                     public boolean matches(Object argument) {
                                                         SubscriptionReply reply = (SubscriptionReply) argument;
                                                         String subscriptionId = reply.getSubscriptionId();
                                                         return null == reply.getError()
                                                                 && subscriptionId2.equals(subscriptionId);
                                                     }
                                                 }),
                                                 any(MessagingQos.class));
        verify(dispatcher,
               timeout(period * 5).times(6)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                         (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                         any(SubscriptionPublication.class),
                                                                         any(MessagingQos.class));
        verifyNoMoreInteractions(routingTable);
    }

    @SuppressWarnings("unchecked")
    private MultiMap<String, PublicationInformation> getQueuedSubscriptionRequests() throws ReflectiveOperationException {
        Field queuedSubscriptionRequestsField = PublicationManagerImpl.class.getDeclaredField("queuedSubscriptionRequests");
        queuedSubscriptionRequestsField.setAccessible(true);

        MultiMap<String, PublicationInformation> queuedSubscriptionRequests = (MultiMap<String, PublicationInformation>) queuedSubscriptionRequestsField.get(publicationManager);
        return queuedSubscriptionRequests;
    }

    private void checkRequestsQueue(SubscriptionRequest subscriptionRequest1) throws ReflectiveOperationException {
        PublicationInformation pubInfo = new PublicationInformation(PROVIDER_PARTICIPANT_ID,
                                                                    PROXY_PARTICIPANT_ID,
                                                                    subscriptionRequest1);
        Set<PublicationInformation> pubInfoSet = new HashSet<>();
        pubInfoSet.add(pubInfo);
        assertEquals(1, getQueuedSubscriptionRequests().size());
        assertEquals(pubInfoSet, getQueuedSubscriptionRequests().get(PROVIDER_PARTICIPANT_ID));
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void stopPublication_removesQueuedSubscriptionsProperly() throws Exception {
        int period = 200;
        String subscriptionId1 = "subscriptionid_removeQueuedSubscriptionsProperly";
        SubscriptionQos qosNoExpiry = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setAlertAfterIntervalMs(500)
                                                                   .setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosNoExpiry);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);

        checkRequestsQueue(subscriptionRequest1);

        publicationManager.stopPublication(subscriptionId1);

        // queued subscriptions are removed
        assertEquals(0, getQueuedSubscriptionRequests().size());

        // at this point no pending request(s) in the subscription queue exist
        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);

        Thread.sleep(period);
        verify(dispatcher, times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
    }

    @Test(timeout = 3000)
    public void removePendingQueuedSubscriptionsAfterProviderRegistered() throws Exception {
        String subscriptionId1 = "subscriptionid_subscriptionsDoesNotExpire";
        SubscriptionQos qosNoExpiry = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setAlertAfterIntervalMs(500)
                                                                   .setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosNoExpiry);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);

        checkRequestsQueue(subscriptionRequest1);

        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);

        assertEquals(0, getQueuedSubscriptionRequests().size());

        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(0)).remove(any());
    }

    @Test(timeout = 3000)
    public void removePendingQueuedSubscriptionsAfterProviderRegistered_continuesOnException() throws Exception {
        String subscriptionId1 = "subscriptionid_subscriptionsDoesNotExpire";
        SubscriptionQos qosNoExpiry = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setAlertAfterIntervalMs(500)
                                                                   .setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosNoExpiry);
        String subscriptionId2 = "subscriptionId2";
        SubscriptionRequest subscriptionRequest2 = new SubscriptionRequest(subscriptionId2, "location", qosNoExpiry);
        String proxyParticipantId2 = "proxy2";

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);
        publicationManager.addSubscriptionRequest(proxyParticipantId2, PROVIDER_PARTICIPANT_ID, subscriptionRequest2);

        doThrow(new JoynrMessageExpiredException("message expired")).when(dispatcher)
                                                                    .sendSubscriptionReply(any(), any(), any(), any());
        verify(routingTable, times(0)).remove(any());

        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);

        assertEquals(0, getQueuedSubscriptionRequests().size());

        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(1)).incrementReferenceCount(proxyParticipantId2);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(1)).remove(proxyParticipantId2);
        InOrder inOrder1 = inOrder(dispatcher);
        InOrder inOrder2 = inOrder(dispatcher);
        inOrder1.verify(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                eq(Collections.singleton(PROXY_PARTICIPANT_ID)),
                                                                any(SubscriptionPublication.class),
                                                                any(MessagingQos.class));
        inOrder1.verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                          eq(PROXY_PARTICIPANT_ID),
                                                          argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                              @Override
                                                              public boolean matches(Object argument) {
                                                                  SubscriptionReply reply = (SubscriptionReply) argument;
                                                                  return null == reply.getError();
                                                              }
                                                          }),
                                                          any(MessagingQos.class));
        inOrder1.verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                          eq(PROXY_PARTICIPANT_ID),
                                                          argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                              @Override
                                                              public boolean matches(Object argument) {
                                                                  SubscriptionReply reply = (SubscriptionReply) argument;
                                                                  return null != reply.getError();
                                                              }
                                                          }),
                                                          any(MessagingQos.class));
        inOrder2.verify(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                eq(Collections.singleton(proxyParticipantId2)),
                                                                any(SubscriptionPublication.class),
                                                                any(MessagingQos.class));
        inOrder2.verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                          eq(proxyParticipantId2),
                                                          argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                              @Override
                                                              public boolean matches(Object argument) {
                                                                  SubscriptionReply reply = (SubscriptionReply) argument;
                                                                  return null == reply.getError();
                                                              }
                                                          }),
                                                          any(MessagingQos.class));
        inOrder2.verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                          eq(proxyParticipantId2),
                                                          argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                              @Override
                                                              public boolean matches(Object argument) {
                                                                  SubscriptionReply reply = (SubscriptionReply) argument;
                                                                  return null != reply.getError();
                                                              }
                                                          }),
                                                          any(MessagingQos.class));
        verifyNoMoreInteractions(routingTable, dispatcher);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 4000)
    public void removeExpiredQueuedSubscriptionsAfterProviderRegistered() throws Exception {
        int period = 500;
        String subscriptionId1 = "subscriptionid_subscriptionsDoesNotExpire";
        long validityMs = 150; // the number of milliseconds until the subscription will expire
        SubscriptionQos qosExpires = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                  .setValidityMs(validityMs)
                                                                  .setAlertAfterIntervalMs(500)
                                                                  .setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosExpires);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);

        checkRequestsQueue(subscriptionRequest1);

        Semaphore semaphore = new Semaphore(0);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                semaphore.release();
                return null;
            }
        }).when(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                        (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                        any(SubscriptionPublication.class),
                                                        any(MessagingQos.class));

        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);
        assertEquals(0, getQueuedSubscriptionRequests().size());

        // wait for 2 publications
        assertTrue(semaphore.tryAcquire(2, 1, TimeUnit.SECONDS));

        // when the sleep time elapsed, the subscription would be already expired and removed from the queue.
        // sendSubscriptionPublication wont' be called
        // Based on the sleep period = 500 ms and the reschedule of publication setPeriodMs = 100 ms, sendSubscriptionPublication
        // supposed to be called 5 times. But since the subscription expires after 150 ms, the sendSubscriptionPublication will
        // be rescheduled only 2 times, then the subscription expires and it will be removed from the queue.
        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(0)).remove(any());

        reset(routingTable, dispatcher);
        Thread.sleep(period);
        verify(dispatcher, times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(routingTable, times(0)).incrementReferenceCount(any());
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
    }

    @Test(timeout = 3000)
    public void checkCleanupIfNoProviderIsRegisteredSubReqExpired() throws Exception {
        int period = 500;
        String subscriptionId1 = "subscriptionid_subscriptionsDoesNotExpire";
        long validityMs = 150; // the number of milliseconds until the subscription will expire
        SubscriptionQos qosExpires = new PeriodicSubscriptionQos().setPeriodMs(100)
                                                                  .setValidityMs(validityMs)
                                                                  .setAlertAfterIntervalMs(500)
                                                                  .setPublicationTtlMs(1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosExpires);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);

        checkRequestsQueue(subscriptionRequest1);

        Thread.sleep(period);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void broadcastPublicationIsSent() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        providerDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        Mockito.mock(SubscriptionRequestStorage.class),
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        long minInterval_ms = 0;
        long ttl = 1000;
        testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters();
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(minInterval_ms)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setPublicationTtlMs(ttl);
        ;

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);
        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);

        GpsLocation location = new GpsLocation(1.0, 2.0, 3.0, GpsFixEnum.MODE2D, 4.0, 5.0, 6.0, 7.0, 9l, 10l, 11);
        double speed = 100;

        publicationManager.broadcastOccurred(subscriptionRequest.getSubscriptionId(),
                                             new ArrayList<BroadcastFilter>(),
                                             location,
                                             speed);

        ArgumentCaptor<SubscriptionPublication> publicationCaptured = ArgumentCaptor.forClass(SubscriptionPublication.class);
        ArgumentCaptor<MessagingQos> qosCaptured = ArgumentCaptor.forClass(MessagingQos.class);

        verify(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                       (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                       publicationCaptured.capture(),
                                                       qosCaptured.capture());

        List<?> response = (List<?>) publicationCaptured.getValue().getResponse();
        assertEquals(location, response.get(0));
        assertEquals(speed, response.get(1));
        assertEquals(ttl, qosCaptured.getValue().getRoundTripTtl_ms());

    }

    @SuppressWarnings("unchecked")
    @Test
    public void broadcastPublicationCallsAllFiltersWithFilterParametersAndValues() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        providerDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        Mockito.mock(SubscriptionRequestStorage.class),
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        long minInterval_ms = 0;
        long ttl = 1000;

        testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters();
        filterParameters.setCountry("Germany");
        filterParameters.setStartTime("4:00");

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(minInterval_ms)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setPublicationTtlMs(ttl);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);

        GpsLocation eventValue = new GpsLocation();

        ArrayList<BroadcastFilter> filters = new ArrayList<BroadcastFilter>();
        testLocationUpdateSelectiveBroadcastFilter filter1 = mock(testLocationUpdateSelectiveBroadcastFilter.class);
        when(filter1.filter(any(GpsLocation.class),
                            any(testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters.class))).thenReturn(true);
        filters.add(filter1);

        testLocationUpdateSelectiveBroadcastFilter filter2 = mock(testLocationUpdateSelectiveBroadcastFilter.class);
        when(filter2.filter(any(GpsLocation.class),
                            any(testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters.class))).thenReturn(true);
        filters.add(filter2);

        publicationManager.broadcastOccurred(subscriptionRequest.getSubscriptionId(), filters, eventValue);

        ArgumentCaptor<SubscriptionPublication> publicationCaptured = ArgumentCaptor.forClass(SubscriptionPublication.class);
        ArgumentCaptor<MessagingQos> qosCaptured = ArgumentCaptor.forClass(MessagingQos.class);

        verify(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                       (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                       publicationCaptured.capture(),
                                                       qosCaptured.capture());

        verify(filter1).filter(eventValue, filterParameters);
        verify(filter2).filter(eventValue, filterParameters);

    }

    @SuppressWarnings("unchecked")
    @Test
    public void broadcastPublicationIsSentWhenFiltersPass() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        providerDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        Mockito.mock(SubscriptionRequestStorage.class),
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        long minInterval_ms = 0;
        long ttl = 1000;
        testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters();
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(minInterval_ms)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setPublicationTtlMs(ttl);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);
        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);

        GpsLocation location = new GpsLocation(1.0, 2.0, 3.0, GpsFixEnum.MODE2D, 4.0, 5.0, 6.0, 7.0, 9l, 10l, 11);
        float speed = 100;

        ArrayList<BroadcastFilter> filters = new ArrayList<BroadcastFilter>();
        testLocationUpdateWithSpeedSelectiveBroadcastFilter filterTrue = mock(testLocationUpdateWithSpeedSelectiveBroadcastFilter.class);
        when(filterTrue.filter(any(GpsLocation.class),
                               any(Float.class),
                               any(testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters.class))).thenReturn(true);
        filters.add(filterTrue);

        publicationManager.broadcastOccurred(subscriptionRequest.getSubscriptionId(), filters, location, speed);

        ArgumentCaptor<SubscriptionPublication> publicationCaptured = ArgumentCaptor.forClass(SubscriptionPublication.class);
        ArgumentCaptor<MessagingQos> qosCaptured = ArgumentCaptor.forClass(MessagingQos.class);

        verify(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                       (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                       publicationCaptured.capture(),
                                                       qosCaptured.capture());

        List<?> response = (List<?>) publicationCaptured.getValue().getResponse();
        assertEquals(location, response.get(0));
        assertEquals(speed, response.get(1));

    }

    @SuppressWarnings("unchecked")
    @Test
    public void broadcastPublicationNotSentWhenFiltersFail() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        providerDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        Mockito.mock(SubscriptionRequestStorage.class),
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        long minInterval_ms = 0;
        long ttl = 1000;
        testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters();
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(minInterval_ms)
                                                                   .setExpiryDateMs(SubscriptionQos.NO_EXPIRY_DATE)
                                                                   .setPublicationTtlMs(ttl);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);

        GpsLocation eventValue = new GpsLocation();

        ArrayList<BroadcastFilter> filters = new ArrayList<BroadcastFilter>();
        testLocationUpdateSelectiveBroadcastFilter filterTrue = mock(testLocationUpdateSelectiveBroadcastFilter.class);
        when(filterTrue.filter(any(GpsLocation.class),
                               any(testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters.class))).thenReturn(true);
        filters.add(filterTrue);

        testLocationUpdateSelectiveBroadcastFilter filterFalse = mock(testLocationUpdateSelectiveBroadcastFilter.class);
        when(filterFalse.filter(any(GpsLocation.class),
                                any(testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters.class))).thenReturn(false);
        filters.add(filterFalse);

        publicationManager.broadcastOccurred(subscriptionRequest.getSubscriptionId(), filters, eventValue);

        verify(dispatcher, never()).sendSubscriptionPublication(any(String.class),
                                                                any(Set.class),
                                                                any(SubscriptionPublication.class),
                                                                any(MessagingQos.class));

    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000)
    public void modifySubscriptionTypeForExistingSubscription() throws Exception {
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        providerDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        Mockito.mock(SubscriptionRequestStorage.class),
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);
        int period = 200;
        int testLengthMax = 3000;
        long validityMs = testLengthMax;
        long publicationTtl = testLengthMax;
        SubscriptionQos qos = new PeriodicSubscriptionQos().setPeriodMs(period)
                                                           .setValidityMs(validityMs)
                                                           .setPublicationTtlMs(publicationTtl);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        //Routing entry added
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(0)).remove(any());
        reset(routingTable);

        verify(dispatcher,
               timeout(period * 5).times(6)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                         (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                         any(SubscriptionPublication.class),
                                                                         any(MessagingQos.class));

        qos = new OnChangeSubscriptionQos().setMinIntervalMs(0)
                                           .setValidityMs(validityMs)
                                           .setPublicationTtlMs(publicationTtl);
        subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        when(providerDirectory.get(eq(PROVIDER_PARTICIPANT_ID))).thenReturn(providerContainer);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        InOrder incrementBeforeDecrement = Mockito.inOrder(routingTable);
        incrementBeforeDecrement.verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        incrementBeforeDecrement.verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);

        reset(dispatcher);
        verifyNoMoreInteractions(routingTable);
        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        verify(dispatcher,
               timeout(testLengthMax).times(1)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                            (Set<String>) argThat(contains(PROXY_PARTICIPANT_ID)),
                                                                            any(SubscriptionPublication.class),
                                                                            any(MessagingQos.class));
    }

    @Test
    public void testMulticastOccurred() {
        String providerParticipantId = "providerParticipantId";
        String multicastName = "multicastName";
        String[] partitions = new String[]{ "first", "second" };

        publicationManager.multicastOccurred(providerParticipantId, multicastName, partitions, "one", 1);

        verify(dispatcher).sendMulticast(eq(providerParticipantId),
                                         Mockito.<MulticastPublication> any(),
                                         Mockito.<MessagingQos> any());
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 3000000)
    public void persistedSubscriptionRequestsAreQueued() throws Exception {
        String persistenceFileName = "target/" + PublicationManagerTest.class.getCanonicalName()
                + ".test_persistenceSubscriptionRequests";

        String providerParticipantId = "providerParticipantId";
        String proxyParticipantId = "proxyParticipantId";
        int period = 200;
        int times = 5;
        int validityMs = (period * times) + period;
        long publicationTtl = validityMs;
        SubscriptionQos qos = new PeriodicSubscriptionQos().setPeriodMs(period)
                                                           .setValidityMs(validityMs)
                                                           .setPublicationTtlMs(publicationTtl);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        new File(persistenceFileName).delete();

        // pre-fill the persistence file
        FileSubscriptionRequestStorage fileSubscriptionRequestStorage = new FileSubscriptionRequestStorage(persistenceFileName);
        assertEquals(0, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());

        // no providers are currently registered
        ProviderDirectory myProviderDirectory = new ProviderDirectory();
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        myProviderDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        fileSubscriptionRequestStorage,
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        publicationManager.addSubscriptionRequest(proxyParticipantId, providerParticipantId, subscriptionRequest);
        assertEquals(1, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);

        publicationManager.shutdown();

        // open the persistence file that should now contain one element
        fileSubscriptionRequestStorage = new FileSubscriptionRequestStorage(persistenceFileName);
        assertEquals(1, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        myProviderDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        fileSubscriptionRequestStorage,
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        // when the provider is registered, persisted subscription requests should be activated
        myProviderDirectory.add(providerParticipantId, providerContainer);

        verify(dispatcher,
               timeout(validityMs).atLeast(times)).sendSubscriptionPublication(any(String.class),
                                                                               any(Set.class),
                                                                               any(SubscriptionPublication.class),
                                                                               any(MessagingQos.class));

        Thread.sleep(validityMs + 1000);
        publicationManager.shutdown();

        // Start again with the same file, that should now be empty as all persisted subscriptions were already queued
        reset(dispatcher);
        fileSubscriptionRequestStorage = new FileSubscriptionRequestStorage(persistenceFileName);
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        myProviderDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        fileSubscriptionRequestStorage,
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);
        verifyNoMoreInteractions(dispatcher);
        fileSubscriptionRequestStorage = new FileSubscriptionRequestStorage(persistenceFileName);
        assertEquals(0, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());
    }

    @Test(timeout = 5000)
    public void multicastSubscriptionRequestsAreNeverPersisted() throws Exception {
        final String persistenceFileName = "target/" + PublicationManagerTest.class.getCanonicalName()
                + ".test_persistenceSubscriptionRequests";

        final String providerParticipantId = "providerParticipantId";
        final String proxyParticipantId = "proxyParticipantId";
        final String multicastId = "multicastId";
        final String multicastName = "multicastName";
        final int validityMs = 5000;
        SubscriptionQos qos = new MulticastSubscriptionQos().setValidityMs(validityMs);
        SubscriptionRequest subscriptionRequest = new MulticastSubscriptionRequest(multicastId,
                                                                                   SUBSCRIPTION_ID,
                                                                                   multicastName,
                                                                                   qos);

        new File(persistenceFileName).delete();

        // pre-fill the persistence file
        FileSubscriptionRequestStorage fileSubscriptionRequestStorage = new FileSubscriptionRequestStorage(persistenceFileName);
        assertEquals(0, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());

        // run test with generally enabled persistency
        // no providers are currently registered
        ProviderDirectory myProviderDirectory = new ProviderDirectory();
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        myProviderDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        fileSubscriptionRequestStorage,
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_ENABLED);

        publicationManager.addSubscriptionRequest(proxyParticipantId, providerParticipantId, subscriptionRequest);
        assertEquals(0, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());

        publicationManager.shutdown();

        // run test with generally disabled persistency
        // no providers are currently registered
        myProviderDirectory = new ProviderDirectory();
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        myProviderDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        fileSubscriptionRequestStorage,
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_DISABLED);

        publicationManager.addSubscriptionRequest(proxyParticipantId, providerParticipantId, subscriptionRequest);
        assertEquals(0, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());

        publicationManager.shutdown();
    }

    @Test(timeout = 5000)
    public void subscriptionRequestsAreNotPersistedIfPersistencyIsDisabled() throws Exception {
        String persistenceFileName = "target/" + PublicationManagerTest.class.getCanonicalName()
                + ".test_persistenceSubscriptionRequests";

        String providerParticipantId = "providerParticipantId";
        String proxyParticipantId = "proxyParticipantId";
        int period = 200;
        int times = 5;
        int validityMs = (period * times) + period;
        long publicationTtl = validityMs;
        SubscriptionQos qos = new PeriodicSubscriptionQos().setPeriodMs(period)
                                                           .setValidityMs(validityMs)
                                                           .setPublicationTtlMs(publicationTtl);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        new File(persistenceFileName).delete();

        // pre-fill the persistence file
        FileSubscriptionRequestStorage fileSubscriptionRequestStorage = new FileSubscriptionRequestStorage(persistenceFileName);
        assertEquals(0, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());

        // no providers are currently registered
        ProviderDirectory myProviderDirectory = new ProviderDirectory();
        publicationManager = new PublicationManagerImpl(attributePollInterpreter,
                                                        dispatcher,
                                                        myProviderDirectory,
                                                        routingTable,
                                                        cleanupScheduler,
                                                        fileSubscriptionRequestStorage,
                                                        shutdownNotifier,
                                                        SUBSCRIPTIONSREQUEST_PERSISTENCY_DISABLED);

        publicationManager.addSubscriptionRequest(proxyParticipantId, providerParticipantId, subscriptionRequest);
        assertEquals(0, fileSubscriptionRequestStorage.getSavedSubscriptionRequests().size());

        publicationManager.shutdown();
    }

    @Test
    public void subscriptionRequestWithoutRoutingTableEntry() throws Exception {
        doThrow(JoynrIllegalStateException.class).when(routingTable).incrementReferenceCount(anyString());
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID,
                                                                          "location",
                                                                          new OnChangeSubscriptionQos());
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                 eq(PROXY_PARTICIPANT_ID),
                                                 argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                     @Override
                                                     public boolean matches(Object argument) {
                                                         SubscriptionReply reply = (SubscriptionReply) argument;
                                                         return null != reply.getError();
                                                     }
                                                 }),
                                                 any(MessagingQos.class));
    }

    @Test
    public void multicastSubscriptionRequestWithoutRoutingTableEntry() throws Exception {
        doThrow(JoynrIllegalStateException.class).when(routingTable).incrementReferenceCount(anyString());
        MulticastSubscriptionRequest subscriptionRequest = new MulticastSubscriptionRequest("multicastId",
                                                                                            SUBSCRIPTION_ID,
                                                                                            "multicastName",
                                                                                            new OnChangeSubscriptionQos());
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        assertEquals(1, getQueuedSubscriptionRequests().size());
        verify(dispatcher, never()).sendSubscriptionReply(any(), any(), any(), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).remove(any());
    }

    @Test(expected = SubscriptionException.class)
    public void addSubscriptionRequestExpired() throws Exception {
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID,
                                                                          "location",
                                                                          new OnChangeSubscriptionQos().setValidityMs(0));
        Thread.sleep(5);
        when(providerDirectory.get(PROVIDER_PARTICIPANT_ID)).thenReturn(providerContainer);
        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
    }

    @Test
    public void addSubscriptionRequestExpiredAtReply() throws Exception {
        doThrow(JoynrMessageExpiredException.class).doNothing()
                                                   .when(dispatcher)
                                                   .sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                                          eq(PROXY_PARTICIPANT_ID),
                                                                          any(),
                                                                          any());
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID,
                                                                          "location",
                                                                          new OnChangeSubscriptionQos());
        when(providerDirectory.get(PROVIDER_PARTICIPANT_ID)).thenReturn(providerContainer);
        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                 eq(PROXY_PARTICIPANT_ID),
                                                 argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                     @Override
                                                     public boolean matches(Object argument) {
                                                         SubscriptionReply reply = (SubscriptionReply) argument;
                                                         return null != reply.getError();
                                                     }
                                                 }),
                                                 any(MessagingQos.class));
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
    }

    @Test
    public void addSubscriptionForUnknownAttribute() throws Exception {
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID,
                                                                          "unknown",
                                                                          new OnChangeSubscriptionQos());
        when(providerDirectory.get(PROVIDER_PARTICIPANT_ID)).thenReturn(providerContainer);
        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                 eq(PROXY_PARTICIPANT_ID),
                                                 argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                     @Override
                                                     public boolean matches(Object argument) {
                                                         SubscriptionReply reply = (SubscriptionReply) argument;
                                                         return null != reply.getError();
                                                     }
                                                 }),
                                                 any(MessagingQos.class));
        verify(routingTable, times(1)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(1)).remove(PROXY_PARTICIPANT_ID);
        verifyNoMoreInteractions(dispatcher, routingTable);
    }

    @Test(expected = SubscriptionException.class)
    public void addMulticastSubscriptionRequestExpired() throws Exception {
        final String multicastId = "multicastId";
        final String multicastName = "multicastName";
        SubscriptionRequest subscriptionRequest = new MulticastSubscriptionRequest(multicastId,
                                                                                   SUBSCRIPTION_ID,
                                                                                   multicastName,
                                                                                   new OnChangeSubscriptionQos().setValidityMs(0));
        Thread.sleep(5);
        when(providerDirectory.get(PROVIDER_PARTICIPANT_ID)).thenReturn(providerContainer);
        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(routingTable, times(0)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(0)).remove(PROXY_PARTICIPANT_ID);
    }

    @Test
    public void addMulticastSubscriptionRequestExpiredAtReply() throws Exception {
        doThrow(JoynrMessageExpiredException.class).doNothing()
                                                   .when(dispatcher)
                                                   .sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                                          eq(PROXY_PARTICIPANT_ID),
                                                                          any(),
                                                                          any());
        final String multicastId = "multicastId";
        final String multicastName = "multicastName";
        SubscriptionRequest subscriptionRequest = new MulticastSubscriptionRequest(multicastId,
                                                                                   SUBSCRIPTION_ID,
                                                                                   multicastName,
                                                                                   new OnChangeSubscriptionQos());
        when(providerDirectory.get(PROVIDER_PARTICIPANT_ID)).thenReturn(providerContainer);
        publicationManager.entryAdded(PROVIDER_PARTICIPANT_ID, providerContainer);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest);
        assertEquals(0, getQueuedSubscriptionRequests().size());
        verify(dispatcher).sendSubscriptionReply(eq(PROVIDER_PARTICIPANT_ID),
                                                 eq(PROXY_PARTICIPANT_ID),
                                                 argThat(new ArgumentMatcher<SubscriptionReply>() {
                                                     @Override
                                                     public boolean matches(Object argument) {
                                                         SubscriptionReply reply = (SubscriptionReply) argument;
                                                         return null != reply.getError();
                                                     }
                                                 }),
                                                 any(MessagingQos.class));
        verify(routingTable, times(0)).incrementReferenceCount(PROXY_PARTICIPANT_ID);
        verify(routingTable, times(0)).remove(PROXY_PARTICIPANT_ID);
    }

}
