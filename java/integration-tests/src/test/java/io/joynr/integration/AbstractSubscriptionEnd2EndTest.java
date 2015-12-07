package io.joynr.integration;

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

import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.atMost;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.dispatching.subscription.SubscriptionTestsProviderImpl;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.PropertyLoader;

import java.util.Properties;
import java.util.UUID;

import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.tests.testProxy;
import joynr.tests.testTypes.TestEnum;
import joynr.types.Localisation.GpsLocation;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.mockito.MockitoAnnotations;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;

public abstract class AbstractSubscriptionEnd2EndTest extends JoynrEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(AbstractSubscriptionEnd2EndTest.class);

    private static final long expected_latency_ms = 50;

    // The timeout applies to all test environments
    private static final int CONST_DEFAULT_TEST_TIMEOUT = 8000;

    @Rule
    public TestName name = new TestName();

    private SubscriptionTestsProviderImpl provider;
    private String domain;
    private testProxy proxy;

    private int period_ms = 200;

    private JoynrRuntime providerRuntime;
    private JoynrRuntime consumerRuntime;

    // Overridden by test environment implementations
    protected abstract JoynrRuntime getRuntime(Properties joynrConfig, Module... modules);

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        String methodName = name.getMethodName();
        logger.info("Starting {} ...", methodName);
        domain = "ProviderDomain-SubscriptionEnd2End-" + methodName + "-" + System.currentTimeMillis();
        provisionPermissiveAccessControlEntry(domain, SubscriptionTestsProviderImpl.INTERFACE_NAME);

        setupProviderRuntime(methodName);
        setupConsumerRuntime(methodName);
    }

    @After
    public void tearDown() throws InterruptedException {
        providerRuntime.shutdown(true);
        consumerRuntime.shutdown(true);
    }

    private void setupProviderRuntime(String methodName) throws InterruptedException, ApplicationException {
        Properties factoryPropertiesProvider;

        String channelIdProvider = "JavaTest-" + UUID.randomUUID().getLeastSignificantBits()
                + "-Provider-SubscriptionEnd2EndTest-" + methodName;

        factoryPropertiesProvider = PropertyLoader.loadProperties("testMessaging.properties");
        factoryPropertiesProvider.put(MessagingPropertyKeys.CHANNELID, channelIdProvider);
        factoryPropertiesProvider.put(MessagingPropertyKeys.RECEIVERID, UUID.randomUUID().toString());
        factoryPropertiesProvider.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, domain);
        providerRuntime = getRuntime(factoryPropertiesProvider, new StaticDomainAccessControlProvisioningModule());

        provider = new SubscriptionTestsProviderImpl();
        providerRuntime.registerProvider(domain, provider).get(CONST_DEFAULT_TEST_TIMEOUT);
    }

    private void setupConsumerRuntime(String methodName) throws DiscoveryException, JoynrIllegalStateException,
                                                        InterruptedException {
        String channelIdConsumer = "JavaTest-" + UUID.randomUUID().getLeastSignificantBits()
                + "-Consumer-SubscriptionEnd2EndTest-" + methodName;

        Properties factoryPropertiesB = PropertyLoader.loadProperties("testMessaging.properties");
        factoryPropertiesB.put(MessagingPropertyKeys.CHANNELID, channelIdConsumer);
        factoryPropertiesB.put(MessagingPropertyKeys.RECEIVERID, UUID.randomUUID().toString());
        factoryPropertiesB.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "ClientDomain-" + methodName + "-"
                + UUID.randomUUID().toString());

        consumerRuntime = getRuntime(factoryPropertiesB);

        MessagingQos messagingQos = new MessagingQos(5000);
        DiscoveryQos discoveryQos = new DiscoveryQos(5000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        // Wait until all the registrations and lookups are finished, to make
        // sure the timings are correct once the tests start by sending a sync
        // request to the test-proxy
        proxy.getFirstPrime();
        logger.trace("Sync call to proxy finished");

    }

    @Test
    @Ignore
    @SuppressWarnings("unchecked")
    public void registerSubscriptionAndReceiveUpdates() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);

        int subscriptionDuration = (period_ms * 4);
        long alertInterval_ms = 500;
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms, expiryDate_ms, alertInterval_ms, 0);
        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        provider.waitForAttributeSubscription("testAttribute");
        Thread.sleep(subscriptionDuration);
        verify(integerListener, times(0)).onError(null);
        // TODO verify publications shipped correct data
        verify(integerListener, times(1)).onReceive(eq(42));
        verify(integerListener, times(1)).onReceive(eq(43));
        verify(integerListener, times(1)).onReceive(eq(44));
        verify(integerListener, times(1)).onReceive(eq(45));

        proxy.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerSubscriptionForComplexDatatype() throws InterruptedException {
        AttributeSubscriptionListener<GpsLocation> gpsListener = mock(AttributeSubscriptionListener.class);
        int subscriptionDuration = (period_ms * 4);
        long alertInterval_ms = 500;
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms, expiryDate_ms, alertInterval_ms, 0);

        String subscriptionId = proxy.subscribeToComplexTestAttribute(gpsListener, subscriptionQos);
        Thread.sleep(subscriptionDuration);
        // 100 2100 4100 6100
        verify(gpsListener, times(0)).onError(null);
        verify(gpsListener, atLeast(4)).onReceive(eq(provider.getComplexTestAttributeSync()));

        proxy.unsubscribeFromComplexTestAttribute(subscriptionId);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscribeToEnumAttribute() throws InterruptedException {
        AttributeSubscriptionListener<TestEnum> testEnumListener = mock(AttributeSubscriptionListener.class);
        TestEnum expectedTestEnum = TestEnum.TWO;
        provider.setEnumAttribute(expectedTestEnum);

        int subscriptionDuration = (period_ms * 4);
        long alertInterval_ms = 500;
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms, expiryDate_ms, alertInterval_ms, 0);

        String subscriptionId = proxy.subscribeToEnumAttribute(testEnumListener, subscriptionQos);
        Thread.sleep(subscriptionDuration);
        // 100 2100 4100 6100
        verify(testEnumListener, times(0)).onError(null);
        verify(testEnumListener, atLeast(4)).onReceive(eq(expectedTestEnum));

        proxy.unsubscribeFromEnumAttribute(subscriptionId);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerSubscriptionForListAndReceiveUpdates() throws InterruptedException {
        AttributeSubscriptionListener<Integer[]> integersListener = mock(AttributeSubscriptionListener.class);
        provider.setTestAttribute(42);

        int subscriptionDuration = (period_ms * 3);
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms, expiryDate_ms, 0, 0);

        String subscriptionId = proxy.subscribeToListOfInts(integersListener, subscriptionQos);
        Thread.sleep(subscriptionDuration);
        verify(integersListener, times(0)).onError(null);

        verify(integersListener, times(1)).onReceive(eq(new Integer[]{ 42 }));
        verify(integersListener, times(1)).onReceive(eq(new Integer[]{ 42, 43 }));
        verify(integersListener, times(1)).onReceive(eq(new Integer[]{ 42, 43, 44 }));
        verifyNoMoreInteractions(integersListener);

        proxy.unsubscribeFromListOfInts(subscriptionId);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerAndStopSubscription() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);
        int subscriptionDuration = (period_ms * 2);
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms, expiryDate_ms, 0, 0);

        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(subscriptionDuration);
        verify(integerListener, times(0)).onError(null);
        verify(integerListener, atLeast(2)).onReceive(anyInt());

        reset(integerListener);
        Thread.sleep(subscriptionDuration);
        verifyNoMoreInteractions(integerListener);
        proxy.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }

    @SuppressWarnings("unchecked")
    @Test
    @Ignore
    public void testOnChangeWithKeepAliveSubscriptionSendsOnChange() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);

        // NOTE: 50 is the minimum minInterval supported
        long minInterval_ms = 50;
        long subscriptionDuration = 1000;
        // publications don't live longer than subscription
        long publicationTtl_ms = subscriptionDuration;
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;

        // do not want to see the interval
        long maxInterval_ms = subscriptionDuration + 1;
        SubscriptionQos subscriptionQosMixed = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                        maxInterval_ms,
                                                                                        expiryDate_ms,
                                                                                        publicationTtl_ms);

        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQosMixed);
        provider.waitForAttributeSubscription("testAttribute");
        verify(integerListener, times(0)).onError(null);

        // when subscribing, we automatically get 1 publication. Expect the starting-publication
        verify(integerListener, times(1)).onReceive(anyInt());

        // Wait minimum time between onChanged
        Thread.sleep(minInterval_ms);
        provider.setTestAttribute(5);
        Thread.sleep(expected_latency_ms);
        // expect the onChangeSubscription to have arrived
        verify(integerListener, times(2)).onReceive(anyInt());

        Thread.sleep(subscriptionDuration);
        // expect no more publications to arrive
        verifyNoMoreInteractions(integerListener);

        proxy.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }

    @SuppressWarnings("unchecked")
    @Ignore
    @Test
    public void testOnChangeWithKeepAliveSubscriptionSendsKeepAlive() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);

        // NOTE: 50 is the minimum minInterval supported
        long minInterval_ms = 50;
        // get an interval update after 200 ms
        long maxInterval_ms = 200;
        int numberExpectedKeepAlives = 3;
        // the subscription duration is a little longer so that it does not expire exactly as the last keep alive is to be sent
        long subscriptionDuration = maxInterval_ms * numberExpectedKeepAlives + (maxInterval_ms / 2);
        long publicationTtl_ms = maxInterval_ms; // publications don't live
        // longer than next interval
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;

        SubscriptionQos subscriptionQosMixed = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                        maxInterval_ms,
                                                                                        expiryDate_ms,
                                                                                        0,
                                                                                        publicationTtl_ms);

        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQosMixed);
        provider.waitForAttributeSubscription("testAttribute");
        verify(integerListener, times(0)).onError(null);

        // when subscribing, we automatically get 1 publication. Expect the
        // starting-publication
        verify(integerListener, times(1)).onReceive(anyInt());

        for (int i = 1; i <= numberExpectedKeepAlives; i++) {

            Thread.sleep(maxInterval_ms + expected_latency_ms);
            // expect the next keep alive notification to have now arrived (plus the original one at subscription start)
            verify(integerListener, times(i + 1)).onReceive(anyInt());
        }

        proxy.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testOnChangeWithKeepAliveSubscription() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);

        long minInterval_ms = 50; // NOTE: 50 is the minimum minInterval
        // supported
        long maxInterval_ms = 500; // get an interval update after 200 ms
        long subscriptionDuration = maxInterval_ms * 3;
        long alertInterval_ms = maxInterval_ms + 100;
        long publicationTtl_ms = maxInterval_ms; // publications don't live
        // longer than next interval
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;

        SubscriptionQos subscriptionQosMixed = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                        maxInterval_ms,
                                                                                        expiryDate_ms,
                                                                                        alertInterval_ms,
                                                                                        publicationTtl_ms);

        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQosMixed);
        provider.waitForAttributeSubscription("testAttribute");
        verify(integerListener, times(0)).onError(null);

        // when subscribing, we automatically get 1 publication. Expect the
        // starting-publication
        verify(integerListener, times(1)).onReceive(anyInt());

        // Wait minimum time between onChanged
        Thread.sleep(minInterval_ms);
        provider.setTestAttribute(5);
        Thread.sleep(expected_latency_ms);
        // expect the onChangeSubscription to have arrived
        verify(integerListener, times(2)).onReceive(anyInt());

        Thread.sleep(maxInterval_ms + 50);
        // expect a keep alive notification to have now arrived
        verify(integerListener, atLeast(3)).onReceive(anyInt());

        proxy.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testOnChangeSubscription() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);

        long minInterval_ms = 0;
        long publicationTtl_ms = 1000;
        long expiryDate_ms = System.currentTimeMillis() + 1000;

        SubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms, expiryDate_ms, publicationTtl_ms);

        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        provider.waitForAttributeSubscription("testAttribute");

        verify(integerListener, times(0)).onError(null);
        // when subscribing, we automatically get 1 publication. This might not
        // be the case in java?
        verify(integerListener, times(1)).onReceive(anyInt());

        provider.setTestAttribute(5);
        Thread.sleep(expected_latency_ms);

        verify(integerListener, times(2)).onReceive(anyInt());

        proxy.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscribeToAttributeWithProviderRuntimeException() throws InterruptedException {
        AttributeSubscriptionListener<Integer> providerRuntimeExceptionListener = mock(AttributeSubscriptionListener.class);
        ProviderRuntimeException expectedException = new ProviderRuntimeException(SubscriptionTestsProviderImpl.MESSAGE_PROVIDERRUNTIMEEXCEPTION);

        int periods = 4;
        int subscriptionDuration = (period_ms * periods);
        long alertInterval_ms = 500;
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms, expiryDate_ms, alertInterval_ms, 0);

        String subscriptionId = proxy.subscribeToAttributeWithProviderRuntimeException(providerRuntimeExceptionListener,
                                                                                       subscriptionQos);
        Thread.sleep(subscriptionDuration);
        // 100 2100 4100 6100
        verify(providerRuntimeExceptionListener, atLeast(periods)).onError(expectedException);
        verify(providerRuntimeExceptionListener, atMost(periods + 1)).onError(expectedException);
        verify(providerRuntimeExceptionListener, times(0)).onReceive(null);

        proxy.unsubscribeFromEnumAttribute(subscriptionId);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscribeToAttributeWithThrownException() throws InterruptedException {
        AttributeSubscriptionListener<Integer> providerRuntimeExceptionListener = mock(AttributeSubscriptionListener.class);
        ProviderRuntimeException expectedException = new ProviderRuntimeException(new IllegalArgumentException(SubscriptionTestsProviderImpl.MESSAGE_THROWN_PROVIDERRUNTIMEEXCEPTION).toString());

        int periods = 4;
        int subscriptionDuration = (period_ms * periods);
        long alertInterval_ms = 500;
        long expiryDate_ms = System.currentTimeMillis() + subscriptionDuration;
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms, expiryDate_ms, alertInterval_ms, 0);

        String subscriptionId = proxy.subscribeToAttributeWithThrownException(providerRuntimeExceptionListener,
                                                                              subscriptionQos);
        Thread.sleep(subscriptionDuration);
        // 100 2100 4100 6100
        verify(providerRuntimeExceptionListener, atLeast(periods)).onError(expectedException);
        verify(providerRuntimeExceptionListener, atMost(periods + 1)).onError(expectedException);
        verify(providerRuntimeExceptionListener, times(0)).onReceive(null);

        proxy.unsubscribeFromEnumAttribute(subscriptionId);
    }

    @SuppressWarnings("unchecked")
    @Ignore
    @Test
    public void testExpiredOnChangeSubscription() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);

        // Only get onChange messages
        long minInterval_ms = 0;
        long duration = 500;
        // Expire quickly
        long expiryDate_ms = System.currentTimeMillis() + duration;
        // Have a large TTL on subscription messages
        long publicationTtl_ms = 10000;

        SubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms, expiryDate_ms, publicationTtl_ms);

        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        provider.waitForAttributeSubscription("testAttribute");

        // There should have only been one call - the automatic publication when a subscription is made
        verify(integerListener, times(1)).onReceive(anyInt());

        Thread.sleep(duration + expected_latency_ms);
        // We should now have an expired onChange subscription
        provider.setTestAttribute(5);
        Thread.sleep(100);
        verifyNoMoreInteractions(integerListener);

        proxy.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "NP_NULL_ON_SOME_PATH_EXCEPTION", justification = "NPE in test would fail test")
    @SuppressWarnings("unchecked")
    @Ignore
    @Test
    public void testSubscribeToNonExistentDomain() throws InterruptedException {
        AttributeSubscriptionListener<Integer> integerListener = mock(AttributeSubscriptionListener.class);
        testProxy proxyToNonexistentDomain = null;
        try {
            ProxyBuilder<testProxy> proxyBuilder;
            String nonExistentDomain = UUID.randomUUID().toString() + "-domaindoesnotexist-end2end";
            MessagingQos messagingQos = new MessagingQos(20000);
            DiscoveryQos discoveryQos = new DiscoveryQos(50000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
            proxyBuilder = consumerRuntime.getProxyBuilder(nonExistentDomain, testProxy.class);
            proxyToNonexistentDomain = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        } catch (DiscoveryException e) {
            e.printStackTrace();
        } catch (JoynrIllegalStateException e) {
            e.printStackTrace();
        }

        // This should not cause an exception
        SubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(period_ms,
                                                                      System.currentTimeMillis() + 30000,
                                                                      0,
                                                                      0);

        String subscriptionId = proxyToNonexistentDomain.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(4000);
        proxyToNonexistentDomain.unsubscribeFromTestAttribute(subscriptionId);
        provider.waitForAttributeUnsubscription("testAttribute");
    }
}
