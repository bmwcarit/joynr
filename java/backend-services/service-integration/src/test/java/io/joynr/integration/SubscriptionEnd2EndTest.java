package io.joynr.integration;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.pubsub.PubSubTestProviderImpl;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.SubscriptionListener;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.PropertyLoader;

import java.io.IOException;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Properties;
import java.util.UUID;

import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.tests.TestProxy;
import joynr.types.GpsLocation;

import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(SubscriptionEnd2EndTest.class);

    @Rule
    public TestName name = new TestName();

    private PubSubTestProviderImpl provider;
    private String domain;
    private TestProxy proxy;

    @Mock
    private SubscriptionListener<GpsLocation> gpsListener;
    @Mock
    private SubscriptionListener<Integer> integerListener;

    @Mock
    private SubscriptionListener<List<Integer>> integerListListener;

    private SubscriptionQos subscriptionQos;
    private DummyJoynrApplication dummyApplicationA;
    private DummyJoynrApplication dummyApplicationB;

    private String methodName;

    private static Server server;

    @BeforeClass
    public static void startServer() throws Exception {
        server = ServersUtil.startServers();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        server.stop();
    }

    @Before
    public void setUp() throws JoynrArbitrationException, InterruptedException, IOException {

        methodName = name.getMethodName();
        logger.info("setup beginning...");

        domain = "TestDomain" + System.currentTimeMillis();

        setupApplicationA();
        setupApplicationB();

        // Thread.sleep(4000);

        ProxyBuilder<TestProxy> proxyBuilder;

        MessagingQos messagingQos = new MessagingQos(40000);
        DiscoveryQos discoveryQos = new DiscoveryQos(50000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        proxyBuilder = dummyApplicationB.getRuntime().getProxyBuilder(domain, joynr.tests.TestProxy.class);
        proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        // Wait until all the registrations and lookups are finished, to make
        // sure the timings are correct once the
        // tests start.

        // Also try to send a sync request to the test-proxy, to make sure
        // everything works ok:
        logger.trace("Waited for all Setups to finish, now make a sync call to the proxy");
        proxy.getFirstPrime();
        logger.trace("Sync call to proxy finished");

        // TODO first publication will arrive too late if timeouts are reduced,
        // investigate why the first publication
        // needs about 1000 msec to be sent. TM: Most likely reason is, that
        // registration and arbitration takes some
        // time to finish.
        long period_ms = 2000;
        long endDate_ms = System.currentTimeMillis() + 8000;
        long alertInterval_ms = 5000;
        long publicationTtl_ms = 2500;
        Date endDate = new Date(endDate_ms);
        logger.trace("Creating SubscriptionQos with Enddate: " + endDate.toString());
        subscriptionQos = new PeriodicSubscriptionQos(period_ms, endDate_ms, alertInterval_ms, publicationTtl_ms);
        Thread.sleep(100);
    }

    // Setup dummyApplicationA and register a provider
    private void setupApplicationA() {
        Properties factoryPropertiesA;

        String channelIdA = "JavaTest-" + methodName + UUID.randomUUID().getLeastSignificantBits()
                + "-A-SubscriptionEnd2EndTest";

        factoryPropertiesA = PropertyLoader.loadProperties("testMessaging.properties");
        factoryPropertiesA.put(MessagingPropertyKeys.CHANNELID, channelIdA);
        factoryPropertiesA.put(MessagingPropertyKeys.RECEIVERID, UUID.randomUUID().toString());
        factoryPropertiesA.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "localdomain-"
                + UUID.randomUUID().toString());
        dummyApplicationA = (DummyJoynrApplication) new JoynrInjectorFactory(factoryPropertiesA).createApplication(DummyJoynrApplication.class);

        provider = new PubSubTestProviderImpl();
        dummyApplicationA.getRuntime().registerCapability(domain,
                                                          provider,
                                                          joynr.tests.TestSync.class,
                                                          "SubscriptionEnd2End");
    }

    // Setup dummyApplicationB
    private void setupApplicationB() {
        String channelIdB = "JavaTest-" + methodName + UUID.randomUUID().getLeastSignificantBits()
                + "-B-SubscriptionEnd2EndTest";

        Properties factoryPropertiesB = PropertyLoader.loadProperties("testMessaging.properties");
        factoryPropertiesB.put(MessagingPropertyKeys.CHANNELID, channelIdB);
        factoryPropertiesB.put(MessagingPropertyKeys.RECEIVERID, UUID.randomUUID().toString());
        factoryPropertiesB.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "localdomain-"
                + UUID.randomUUID().toString());
        dummyApplicationB = (DummyJoynrApplication) new JoynrInjectorFactory(factoryPropertiesB).createApplication(DummyJoynrApplication.class);
    }

    @After
    public void tearDown() throws InterruptedException {
        // Get the messageReceiver from each injector and delete the channels
        dummyApplicationA.shutdown();
        dummyApplicationA = null;
        Thread.sleep(200);
        dummyApplicationB.shutdown();
        dummyApplicationB = null;
    }

    @Test
    public void registerSubscriptionAndReceiveUpdates() throws InterruptedException {

        logger.trace("Starting registerSubscriptionAndReceiveUpdates");
        proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(12000);
        verify(integerListener, times(0)).publicationMissed();
        // TODO verify publications shipped correct data
        verify(integerListener, times(1)).receive(eq(42));
        verify(integerListener, times(1)).receive(eq(43));
        verify(integerListener, times(1)).receive(eq(44));
        verify(integerListener, times(1)).receive(eq(45));
        verifyNoMoreInteractions(integerListener);
    }

    @Test
    public void registerSubscriptionForComplexDatatype() throws InterruptedException {
        // TODO this test fails sometimes: timing issue
        proxy.subscribeToComplexTestAttribute(gpsListener, subscriptionQos);
        // 100 2100 4100 6100
        Thread.sleep(12000);
        verify(gpsListener, times(0)).publicationMissed();
        verify(gpsListener, atLeast(4)).receive(eq(provider.getComplexTestAttribute()));
    }

    @Test
    public void registerSubscriptionForListAndReceiveUpdates() throws InterruptedException {

        logger.trace("Starting registerSubscriptionAndReceiveUpdates");
        proxy.subscribeToListOfInts(integerListListener, subscriptionQos);
        Thread.sleep(12000);
        verify(integerListener, times(0)).publicationMissed();
        // TODO verify publications shipped correct data
        verify(integerListListener, times(1)).receive(eq(Arrays.asList(42)));
        verify(integerListListener, times(1)).receive(eq(Arrays.asList(42, 43)));
        verify(integerListListener, times(1)).receive(eq(Arrays.asList(42, 43, 44)));
        verifyNoMoreInteractions(integerListener);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerAndStopSubscription() throws InterruptedException {
        String subscriptionId = proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(4000);
        verify(integerListener, times(0)).publicationMissed();
        verify(integerListener, atLeast(1)).receive(anyInt());

        proxy.unsubscribeFromTestAttribute(subscriptionId);
        reset(integerListener);
        Thread.sleep(5000);
        verifyNoMoreInteractions(integerListener);

    }

    @Test
    public void testMixedSubscriptionSendsOnSet() throws InterruptedException {

        subscriptionQos = getMixedSubscriptionQos();

        proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(4000);
        verify(integerListener, times(0)).publicationMissed();
        // when subscribing, we automatically get 1 publication.
        // verify(integerListener, times(1)).receive(anyInt());

        provider.setTestAttribute(5);
        Thread.sleep(4000);

        verify(integerListener, times(2)).receive(anyInt());

    }

    @Test
    public void testMixedSubscriptionSendsOnSetAndOnInterval() throws InterruptedException {

        subscriptionQos = getMixedSubscriptionQos();

        proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(2000);
        verify(integerListener, times(0)).publicationMissed();
        // when subscribing, we automatically get 1 publication.
        // verify(integerListener, times(1)).receive(anyInt());

        // expect the starting-publication
        verify(integerListener, times(1)).receive(anyInt());
        provider.setTestAttribute(5);
        Thread.sleep(2000);
        // expect the onChangeSubscription
        verify(integerListener, times(2)).receive(anyInt());
        Thread.sleep(4000);
        // expect the Regular Subscription
        verify(integerListener, times(3)).receive(anyInt());
    }

    @Test
    public void testPureOnchangedSubscription() throws InterruptedException {

        logger.trace("TestPureOnChangedSubscription");
        long minInterval_ms = 500;
        long endDate_ms = System.currentTimeMillis() + 100000;
        long publicationTtl_ms = 15500;

        subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms, endDate_ms, publicationTtl_ms);

        proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(4000);
        verify(integerListener, times(0)).publicationMissed();
        // when subscribing, we automatically get 1 publication. This might not
        // be the case in java?
        verify(integerListener, times(1)).receive(anyInt());
        provider.setTestAttribute(5);
        Thread.sleep(1000);

        verify(integerListener, atLeast(2)).receive(anyInt());

    }

    @Test
    public void testExpiredSubscriptionOnChange() throws InterruptedException {

        logger.trace("testExpiredSubscriptionOnChange");
        // Only get onChange messages
        long minInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 4000; // Expire
        // quickly
        long publicationTtl_ms = 10000; // Have a large TTL on subscription
        // messages

        subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms, expiryDate_ms, publicationTtl_ms);

        proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(5000);
        // We should now have an expired onChange subscription
        provider.setTestAttribute(5);
        Thread.sleep(1000);

        // There should have only been one call - the automatic publication when
        // a subscription is made
        verify(integerListener, times(1)).receive(anyInt());

    }

    @Test
    public void testSubscribeToNonExistentDomain() throws InterruptedException {
        try {
            ProxyBuilder<TestProxy> proxyBuilder;
            String nonExistentDomain = UUID.randomUUID().toString() + "-end2end";
            MessagingQos messagingQos = new MessagingQos(20000);
            DiscoveryQos discoveryQos = new DiscoveryQos(50000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
            proxyBuilder = dummyApplicationB.getRuntime().getProxyBuilder(nonExistentDomain,
                                                                          joynr.tests.TestProxy.class);
            proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
        } catch (JoynrIllegalStateException e) {
            e.printStackTrace();
        }

        // This should not cause an exception
        proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        Thread.sleep(4000);
    }

    // Test is currently failing because subscription persistence not
    // implemented.
    @Test
    @Ignore
    public void testSubscriptionToRestartedProvider() throws InterruptedException {
        logger.trace("TestSubscriptionToRestartedProvider");
        long minInterval_ms = 500;
        long expiryDate_ms = System.currentTimeMillis() + 100000;
        long publicationTtl_ms = 30000;

        subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms, expiryDate_ms, publicationTtl_ms);

        // Subscribe to an attribute on-change
        proxy.subscribeToTestAttribute(integerListener, subscriptionQos);
        verify(integerListener, times(0)).publicationMissed();
        verify(integerListener, times(1)).receive(anyInt());
        provider.setTestAttribute(5);
        verify(integerListener, times(2)).receive(anyInt());

        // Shutdown the provider
        dummyApplicationA.getRuntime().shutdown(true);

        // Start the provider again
        setupApplicationA();

        // Cause attribute onchange
        for (int i = 0; i < 2; i++) {
            provider.setTestAttribute(8);
            Thread.sleep(1000);
        }

        // See if the subscription got the on-change events
        verify(integerListener, times(4)).receive(anyInt());
    }

    private SubscriptionQos getMixedSubscriptionQos() {
        long minInterval_ms = 500;
        long maxInterval_ms = 5000;
        long endDate_ms = System.currentTimeMillis() + 100000;
        long alertInterval_ms = 10000;
        long publicationTtl_ms = 5500;

        return new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                        maxInterval_ms,
                                                        endDate_ms,
                                                        alertInterval_ms,
                                                        publicationTtl_ms);
    }

}
