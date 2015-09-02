package io.joynr.dispatching.subscription;

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
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCaller;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.Deferred;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.Promise;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.publication.BroadcastFilter;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import joynr.BroadcastSubscriptionRequest;
import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.tests.testBroadcastInterface;
import joynr.tests.testLocationUpdateSelectiveBroadcastFilter;
import joynr.tests.testLocationUpdateWithSpeedSelectiveBroadcastFilter;
import joynr.tests.testProvider;
import joynr.types.localisation.GpsFixEnum;
import joynr.types.localisation.GpsLocation;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class PublicationManagerTest {

    private static final String PROVIDER_PARTICIPANT_ID = "providerParticipantId";
    private static final String PROXY_PARTICIPANT_ID = "proxyParticipantId";

    private static final String SUBSCRIPTION_ID = "PublicationTest_id";

    ScheduledExecutorService cleanupScheduler;
    PublicationManager publicationManager;

    @Captor
    ArgumentCaptor<String> sentProviderParticipantId;
    @Captor
    ArgumentCaptor<String> sentProxyParticipantId;
    @Captor
    ArgumentCaptor<SubscriptionPublication> sentPublication;
    @Captor
    ArgumentCaptor<MessagingQos> sentMessagingQos;

    @Mock
    AttributePollInterpreter attributePollInterpreter;

    private RequestCaller requestCaller;

    @Mock
    private Dispatcher dispatcher;

    @Mock
    private JoynrProvider provider;

    String valueToPublish = "valuePublished";

    @Before
    public void setUp() {
        Deferred<String> valueToPublishDeferred = new Deferred<String>();
        valueToPublishDeferred.resolve(valueToPublish);
        Promise<Deferred<String>> valueToPublishPromise = new Promise<Deferred<String>>(valueToPublishDeferred);
        doReturn(testProvider.class).when(provider).getProvidedInterface();

        cleanupScheduler = new ScheduledThreadPoolExecutor(1);
        publicationManager = new PublicationManagerImpl(attributePollInterpreter, dispatcher, cleanupScheduler);

        RequestCallerFactory requestCallerFactory = new RequestCallerFactory();
        requestCaller = requestCallerFactory.create(provider);

        doReturn(valueToPublishPromise).when(attributePollInterpreter).execute(any(RequestCaller.class),
                                                                               any(Method.class));
    }

    @Test(timeout = 3000)
    public void addPublicationWithExpiryDate() throws Exception {
        long pubicationActiveForMs = 300;
        long expiryDate = System.currentTimeMillis() + pubicationActiveForMs;
        long publicationTtl = 1000;
        SubscriptionQos qos = new OnChangeSubscriptionQos(0, expiryDate, publicationTtl);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        // sending initial value plus the attributeValueChanged
        verify(dispatcher, times(2)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 eq(PROXY_PARTICIPANT_ID),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        Thread.sleep(pubicationActiveForMs);
        reset(dispatcher);

        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        verify(dispatcher, timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                              eq(PROXY_PARTICIPANT_ID),
                                                                              any(SubscriptionPublication.class),
                                                                              any(MessagingQos.class));
    }

    @Test(timeout = 3000)
    public void addPublicationWithoutExpiryDate() throws Exception {
        int period = 200;
        SubscriptionQos qos = new PeriodicSubscriptionQos(100, SubscriptionQos.NO_EXPIRY_DATE, 500, 1000);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        verify(dispatcher, timeout(period * 5).times(6)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                                     eq(PROXY_PARTICIPANT_ID),
                                                                                     any(SubscriptionPublication.class),
                                                                                     any(MessagingQos.class));

        reset(dispatcher);
        publicationManager.stopPublication(SUBSCRIPTION_ID);

        verify(dispatcher, timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                              eq(PROXY_PARTICIPANT_ID),
                                                                              any(SubscriptionPublication.class),
                                                                              any(MessagingQos.class));
    }

    @Test(timeout = 3000)
    public void startAndStopPeriodicPublication() throws Exception {
        int period = 200;
        int testLengthMax = 3000;
        long expiryDate = System.currentTimeMillis() + testLengthMax;
        long publicationTtl = testLengthMax;
        SubscriptionQos qos = new PeriodicSubscriptionQos(period, expiryDate, publicationTtl);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        verify(dispatcher, timeout(period * 5).times(6)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                                     eq(PROXY_PARTICIPANT_ID),
                                                                                     any(SubscriptionPublication.class),
                                                                                     any(MessagingQos.class));

        reset(dispatcher);
        publicationManager.stopPublication(SUBSCRIPTION_ID);

        verify(dispatcher, timeout(testLengthMax).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                                        eq(PROXY_PARTICIPANT_ID),
                                                                                        any(SubscriptionPublication.class),
                                                                                        any(MessagingQos.class));
    }

    @Test(timeout = 3000)
    public void stopAllPublicationsFromProvider() throws Exception {
        long expiryDate = System.currentTimeMillis() + 3000;
        long publicationTtl = 1000;
        String subscriptionId1 = "subscriptionid1";
        String subscriptionId2 = "subscriptionid2";
        SubscriptionQos qos = new OnChangeSubscriptionQos(0, expiryDate, publicationTtl);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qos);
        SubscriptionRequest subscriptionRequest2 = new SubscriptionRequest(subscriptionId2, "location", qos);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest1,
                                                  requestCaller);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest2,
                                                  requestCaller);

        publicationManager.attributeValueChanged(subscriptionId1, valueToPublish);
        publicationManager.attributeValueChanged(subscriptionId2, valueToPublish);

        // sending initial values for 2 subscriptions, plus the 2 attributeValueChanged
        verify(dispatcher, times(4)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 eq(PROXY_PARTICIPANT_ID),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        reset(dispatcher);
        publicationManager.stopPublicationByProviderId(PROVIDER_PARTICIPANT_ID);

        publicationManager.attributeValueChanged(subscriptionId1, valueToPublish);
        publicationManager.attributeValueChanged(subscriptionId2, valueToPublish);

        verify(dispatcher, timeout(300).times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                              eq(PROXY_PARTICIPANT_ID),
                                                                              any(SubscriptionPublication.class),
                                                                              any(MessagingQos.class));
    }

    @Test(timeout = 3000)
    public void restorePublications() throws Exception {
        int period = 200;
        String subscriptionId1 = "subscriptionid1";
        String subscriptionId2 = "subscriptionid2";
        long expiryDate = System.currentTimeMillis() + 3000;
        SubscriptionQos qosNoExpiry = new PeriodicSubscriptionQos(100, SubscriptionQos.NO_EXPIRY_DATE, 500, 1000);
        SubscriptionQos qosExpires = new PeriodicSubscriptionQos(100, expiryDate, 500, 1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosNoExpiry);
        SubscriptionRequest subscriptionRequest2 = new SubscriptionRequest(subscriptionId2, "location", qosExpires);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest2);

        Thread.sleep(period);
        verify(dispatcher, times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 eq(PROXY_PARTICIPANT_ID),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));

        publicationManager.restoreQueuedSubscription(PROVIDER_PARTICIPANT_ID, requestCaller);

        verify(dispatcher, timeout(period * 5).times(12)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                                      eq(PROXY_PARTICIPANT_ID),
                                                                                      any(SubscriptionPublication.class),
                                                                                      any(MessagingQos.class));
    }

    @Test(timeout = 3000)
    public void removeQueuedSubscriptionsProperly() throws Exception {
        int period = 200;
        String subscriptionId1 = "subscriptionid_removeQueuedSubscriptionsProperly";
        SubscriptionQos qosNoExpiry = new PeriodicSubscriptionQos(100, SubscriptionQos.NO_EXPIRY_DATE, 500, 1000);
        SubscriptionRequest subscriptionRequest1 = new SubscriptionRequest(subscriptionId1, "location", qosNoExpiry);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID, PROVIDER_PARTICIPANT_ID, subscriptionRequest1);

        publicationManager.stopPublication(subscriptionId1);

        publicationManager.restoreQueuedSubscription(PROVIDER_PARTICIPANT_ID, requestCaller);
        Thread.sleep(period);
        verify(dispatcher, times(0)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                 eq(PROXY_PARTICIPANT_ID),
                                                                 any(SubscriptionPublication.class),
                                                                 any(MessagingQos.class));
    }

    @Test
    public void broadcastPublicationIsSent() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter, dispatcher, cleanupScheduler);

        long minInterval_ms = 0;
        long ttl = 1000;
        testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters();
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos(minInterval_ms, SubscriptionQos.NO_EXPIRY_DATE, ttl);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        GpsLocation location = new GpsLocation(1.0, 2.0, 3.0, GpsFixEnum.MODE2D, 4.0, 5.0, 6.0, 7.0, 9l, 10l, 11);
        double speed = 100;

        publicationManager.broadcastOccurred(subscriptionRequest.getSubscriptionId(),
                                             new ArrayList<BroadcastFilter>(),
                                             location,
                                             speed);

        ArgumentCaptor<SubscriptionPublication> publicationCaptured = ArgumentCaptor.forClass(SubscriptionPublication.class);
        ArgumentCaptor<MessagingQos> qosCaptured = ArgumentCaptor.forClass(MessagingQos.class);

        verify(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                       eq(PROXY_PARTICIPANT_ID),
                                                       publicationCaptured.capture(),
                                                       qosCaptured.capture());

        List<?> response = (List<?>) publicationCaptured.getValue().getResponse();
        assertEquals(location, response.get(0));
        assertEquals(speed, response.get(1));
        assertEquals(ttl, qosCaptured.getValue().getRoundTripTtl_ms());

    }

    @Test
    public void broadcastPublicationCallsAllFiltersWithFilterParametersAndValues() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter, dispatcher, cleanupScheduler);

        long minInterval_ms = 0;
        long ttl = 1000;

        testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters();
        filterParameters.setCountry("Germany");
        filterParameters.setStartTime("4:00");

        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos(minInterval_ms, SubscriptionQos.NO_EXPIRY_DATE, ttl);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

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
                                                       eq(PROXY_PARTICIPANT_ID),
                                                       publicationCaptured.capture(),
                                                       qosCaptured.capture());

        verify(filter1).filter(eventValue, filterParameters);
        verify(filter2).filter(eventValue, filterParameters);

    }

    @Test
    public void broadcastPublicationIsSentWhenFiltersPass() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter, dispatcher, cleanupScheduler);

        long minInterval_ms = 0;
        long ttl = 1000;
        testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters();
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos(minInterval_ms, SubscriptionQos.NO_EXPIRY_DATE, ttl);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        GpsLocation location = new GpsLocation(1.0, 2.0, 3.0, GpsFixEnum.MODE2D, 4.0, 5.0, 6.0, 7.0, 9l, 10l, 11);
        double speed = 100;

        ArrayList<BroadcastFilter> filters = new ArrayList<BroadcastFilter>();
        testLocationUpdateWithSpeedSelectiveBroadcastFilter filterTrue = mock(testLocationUpdateWithSpeedSelectiveBroadcastFilter.class);
        when(filterTrue.filter(any(GpsLocation.class),
                               any(Double.class),
                               any(testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters.class))).thenReturn(true);
        filters.add(filterTrue);

        publicationManager.broadcastOccurred(subscriptionRequest.getSubscriptionId(), filters, location, speed);

        ArgumentCaptor<SubscriptionPublication> publicationCaptured = ArgumentCaptor.forClass(SubscriptionPublication.class);
        ArgumentCaptor<MessagingQos> qosCaptured = ArgumentCaptor.forClass(MessagingQos.class);

        verify(dispatcher).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                       eq(PROXY_PARTICIPANT_ID),
                                                       publicationCaptured.capture(),
                                                       qosCaptured.capture());

        List<?> response = (List<?>) publicationCaptured.getValue().getResponse();
        assertEquals(location, response.get(0));
        assertEquals(speed, response.get(1));

    }

    @Test
    public void broadcastPublicationNotSentWhenFiltersFail() throws Exception {

        publicationManager = new PublicationManagerImpl(attributePollInterpreter, dispatcher, cleanupScheduler);

        long minInterval_ms = 0;
        long ttl = 1000;
        testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters();
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos(minInterval_ms, SubscriptionQos.NO_EXPIRY_DATE, ttl);

        SubscriptionRequest subscriptionRequest = new BroadcastSubscriptionRequest(SUBSCRIPTION_ID,
                                                                                   "subscribedToName",
                                                                                   filterParameters,
                                                                                   qos);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

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
                                                                any(String.class),
                                                                any(SubscriptionPublication.class),
                                                                any(MessagingQos.class));

    }

    @Test(timeout = 3000)
    public void modifySubscriptionTypeForExistingSubscription() throws Exception {
        publicationManager = new PublicationManagerImpl(attributePollInterpreter, dispatcher, cleanupScheduler);
        int period = 200;
        int testLengthMax = 3000;
        long expiryDate = System.currentTimeMillis() + testLengthMax;
        long publicationTtl = testLengthMax;
        SubscriptionQos qos = new PeriodicSubscriptionQos(period, expiryDate, publicationTtl);
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);

        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        verify(dispatcher, timeout(period * 5).times(6)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                                     eq(PROXY_PARTICIPANT_ID),
                                                                                     any(SubscriptionPublication.class),
                                                                                     any(MessagingQos.class));

        qos = new OnChangeSubscriptionQos(0, expiryDate, publicationTtl);
        subscriptionRequest = new SubscriptionRequest(SUBSCRIPTION_ID, "location", qos);
        publicationManager.addSubscriptionRequest(PROXY_PARTICIPANT_ID,
                                                  PROVIDER_PARTICIPANT_ID,
                                                  subscriptionRequest,
                                                  requestCaller);

        reset(dispatcher);
        publicationManager.attributeValueChanged(SUBSCRIPTION_ID, valueToPublish);

        verify(dispatcher, timeout(testLengthMax).times(1)).sendSubscriptionPublication(eq(PROVIDER_PARTICIPANT_ID),
                                                                                        eq(PROXY_PARTICIPANT_ID),
                                                                                        any(SubscriptionPublication.class),
                                                                                        any(MessagingQos.class));
    }
}
