/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.integration;

import static io.joynr.util.JoynrUtil.createUuidString;
import static com.google.inject.util.Modules.override;
import static java.lang.Thread.sleep;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assert.assertNotNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.times;

import joynr.system.DiscoveryAsync;
import org.mockito.Spy;
import org.mockito.Mock;

import java.io.IOException;
import java.util.Set;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Named;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.integration.util.TestSetup;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.inprocess.InProcessMessagingStubFactory;
import io.joynr.messaging.mqtt.MqttMessagingStubFactory;
import io.joynr.messaging.mqtt.MqttMessagingStub;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebSocketClientMessagingStubFactory;
import io.joynr.messaging.websocket.WebSocketMessagingStub;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.provider.JoynrProvider;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;

import io.joynr.proxy.ProxyBuilderFactoryImpl;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.DiscoveryResult;

import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import joynr.MutableMessage;
import joynr.tests.DefaulttestProvider;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.testProxy;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import jakarta.inject.Inject;
import java.util.Properties;
import java.util.HashSet;
import java.util.List;
import java.util.Arrays;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/*
 * This testClass registers one consumer and one provider both on the same runtime.
 * It can be used to test local communication.
 */
@RunWith(MockitoJUnitRunner.class)
public class LocalCommunicationTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCommunicationTest.class);

    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    protected final String TEST_CUSTOM_DOMAIN_1 = "testCustomDomain1";
    protected final String FIXED_PARTICIPANT_ID_1 = "provider-1";
    private static final boolean USE_SEPARATE_REPLY_RECEIVER = true;

    private final int lengthInMS = 2000;

    private Properties properties;
    protected Injector injector;
    // JoynrRuntime to simulate user operations
    protected JoynrRuntime joynrRuntime;

    protected DiscoveryQos discoveryQosGlobal;
    protected DiscoveryQos discoveryQosLocal;
    protected ProviderQos providerQosGlobal;
    protected ProviderQos providerQosLocal;
    protected MessagingQos defaultMessagingQos;

    @Mock
    private MqttMessagingStubFactory mqttMessagingStubFactoryMock;
    @Mock
    private WebSocketClientMessagingStubFactory websocketClientMessagingStubFactoryMock;
    @Mock
    private WebSocketMessagingStubFactory webSocketMessagingStubFactoryMock;
    @Spy
    protected InProcessMessagingStubFactory inProcessMessagingStubFactorySpy;

    @Mock
    protected MqttMessagingStub mqttMessagingStubMock;
    @Mock
    protected WebSocketMessagingStub webSocketClientMessagingStubMock;
    @Mock
    private WebSocketMessagingStub webSocketMessagingStubMock;

    @Mock
    protected JoynrMessageProcessor messageProcessorMock;
    @Mock
    private HivemqMqttClientFactory hiveMqMqttClientFactory;

    @Mock
    private JoynrMqttClient joynrMqttClient1;
    @Mock
    private JoynrMqttClient joynrMqttClient2;

    @Spy
    ShutdownNotifier shutdownNotifier = new ShutdownNotifier();

    // set messaging period
    private final int times = 5;
    private final int initialValue = 42;
    private final int period = lengthInMS / times;

    // Setup Joynr runtime is taken from AbstractRoutingTableCleanupTest
    protected static class TestProxyBuilderFactory extends ProxyBuilderFactoryImpl {
        private final Set<String> internalProxyParticipantIds;

        @Inject
        // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
        public TestProxyBuilderFactory(DiscoveryAsync localDiscoveryAggregator,
                                       ProxyInvocationHandlerFactory proxyInvocationHandlerFactory,
                                       ShutdownNotifier shutdownNotifier,
                                       StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory,
                                       @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_TTL_MS) long maxMessagingTtl,
                                       @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS) long defaultDiscoveryTimeoutMs,
                                       @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS) long defaultDiscoveryRetryIntervalMs,
                                       @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS) long minimumArbitrationRetryDelay,
                                       @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                       @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String routingProviderParticipantId,
                                       @Named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY) GlobalDiscoveryEntry capabilitiesDirectoryEntry) {
            super(localDiscoveryAggregator,
                  proxyInvocationHandlerFactory,
                  shutdownNotifier,
                  statelessAsyncCallbackDirectory,
                  maxMessagingTtl,
                  defaultDiscoveryTimeoutMs,
                  defaultDiscoveryRetryIntervalMs,
                  minimumArbitrationRetryDelay,
                  USE_SEPARATE_REPLY_RECEIVER);
            internalProxyParticipantIds = new HashSet<>(Arrays.asList(discoveryProviderParticipantId,
                                                                      routingProviderParticipantId,
                                                                      capabilitiesDirectoryEntry.getParticipantId()));
        }

        @Override
        public <T> ProxyBuilder<T> get(Set<String> domains, Class<T> interfaceClass) {
            ProxyBuilder<T> proxyBuilder = super.get(domains, interfaceClass);
            String proxyParticipantId = proxyBuilder.getParticipantId();
            if (!internalProxyParticipantIds.contains(proxyParticipantId)) {
                AbstractRoutingTableCleanupTest.sProxyParticipantId = proxyParticipantId;
            }
            System.out.println("##### TestProxyBuilderFactory.get() called!");
            return proxyBuilder;
        }
    }

    @Before
    public void setUp() throws IOException, InterruptedException {
        String TESTGBID1 = "testgbid1";
        String TESTGBID2 = "testgbid2";
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createReceiver(TESTGBID1);
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createReceiver(TESTGBID2);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createSender(TESTGBID2);

        lenient().doReturn(mqttMessagingStubMock).when(mqttMessagingStubFactoryMock).create(any());
        lenient().doReturn(webSocketClientMessagingStubMock)
                 .when(websocketClientMessagingStubFactoryMock)
                 .create(any());
        lenient().doReturn(webSocketMessagingStubMock).when(webSocketMessagingStubFactoryMock).create(any());

        doAnswer(invocation -> invocation.getArguments()[0]).when(messageProcessorMock)
                                                            .processOutgoing(any(MutableMessage.class));

        discoveryQosGlobal = new DiscoveryQos();
        discoveryQosGlobal.setDiscoveryTimeoutMs(30000);
        discoveryQosGlobal.setRetryIntervalMs(discoveryQosGlobal.getDiscoveryTimeoutMs() + 1); // no retry
        discoveryQosGlobal.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQosGlobal.setCacheMaxAgeMs(10000);

        discoveryQosLocal = new DiscoveryQos();
        discoveryQosLocal.setDiscoveryTimeoutMs(30000);
        discoveryQosLocal.setRetryIntervalMs(discoveryQosLocal.getDiscoveryTimeoutMs() + 1); // no retry
        discoveryQosLocal.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        providerQosGlobal = new ProviderQos();
        providerQosGlobal.setScope(ProviderScope.GLOBAL);

        providerQosLocal = new ProviderQos();
        providerQosLocal.setScope(ProviderScope.LOCAL);

        defaultMessagingQos = new MessagingQos();

        properties = createProperties(TESTGBID1 + ", " + TESTGBID2);
        joynrRuntime = createJoynrRuntime();
    }

    private Properties createProperties(String gbIds) throws IOException {
        Properties properties = new Properties();
        String channelId = createUuidString() + "-end2endA";
        properties.setProperty(MessagingPropertyKeys.CHANNELID, channelId);

        properties.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        properties.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + TestSetup.findFreePort());

        properties.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS, gbIds);
        properties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:1883, tcp://otherhost:1883");
        properties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "60,30");
        properties.setProperty(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,30");
        long ROUTING_MAX_RETRY_COUNT = 2;
        properties.setProperty(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT,
                               String.valueOf(ROUTING_MAX_RETRY_COUNT));
        // Put properties for fixed participantIds
        String interfaceName = ProviderAnnotations.getInterfaceName(DefaulttestProvider.class);
        int majorVersion = ProviderAnnotations.getMajorVersion(DefaulttestProvider.class);
        properties.setProperty(ParticipantIdKeyUtil.getProviderParticipantIdKey(TEST_CUSTOM_DOMAIN_1,
                                                                                interfaceName,
                                                                                majorVersion),
                               FIXED_PARTICIPANT_ID_1);

        long ROUTING_TABLE_CLEANUP_INTERVAL_MS = 100L;
        properties.setProperty(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS,
                               String.valueOf(ROUTING_TABLE_CLEANUP_INTERVAL_MS));
        properties.setProperty(SystemServicesSettings.PROPERTY_CC_REMOVE_STALE_DELAY_MS, String.valueOf(500));
        return properties;
    }

    protected JoynrRuntime createJoynrRuntime() throws InterruptedException {
        AbstractModule testBindingsModule = new AbstractModule() {
            @Override
            protected void configure() {
                // Any mqtt clients shall be mocked
                bind(MqttClientFactory.class).toInstance(hiveMqMqttClientFactory);
                // We want to mock all MessagingStubs except the InProcess one
                bind(WebSocketClientMessagingStubFactory.class).toInstance(websocketClientMessagingStubFactoryMock);
                bind(WebSocketMessagingStubFactory.class).toInstance(webSocketMessagingStubFactoryMock);
                bind(MqttMessagingStubFactory.class).toInstance(mqttMessagingStubFactoryMock);
                bind(InProcessMessagingStubFactory.class).toInstance(inProcessMessagingStubFactorySpy);

                bind(ShutdownNotifier.class).toInstance(shutdownNotifier);
                bind(ProxyBuilderFactory.class).to(LocalCommunicationTest.TestProxyBuilderFactory.class);

                Multibinder<JoynrMessageProcessor> processors = Multibinder.newSetBinder(binder(),
                                                                                         new TypeLiteral<JoynrMessageProcessor>() {
                                                                                         });
                processors.addBinding().toInstance(messageProcessorMock);
            }
        };

        injector = Guice.createInjector(override(new CCWebSocketRuntimeModule(),
                                                 new HivemqMqttClientModule()).with(new JoynrPropertiesModule(properties),
                                                                                    testBindingsModule));
        CountDownLatch cdl = waitForRemoveStale(mqttMessagingStubMock);

        JoynrRuntime joynrRuntime = injector.getInstance(JoynrRuntime.class);

        assertTrue(cdl.await(10, TimeUnit.SECONDS));
        reset(mqttMessagingStubMock);
        return joynrRuntime;
    }

    private CountDownLatch waitForRemoveStale(MqttMessagingStub mqttMessagingStubMock) throws InterruptedException {
        // removeStale is called once at startup because ClusterControllerRuntime calls removeStale
        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer((invocation) -> {
            cdl.countDown();
            return null;
        }).when(mqttMessagingStubMock).transmit(any(), any(), any());
        return cdl;
    }

    @After
    public void tearDown() {
        shutdownRuntime();
    }

    private void shutdownRuntime() {
        if (joynrRuntime != null) {
            joynrRuntime.shutdown(true);
        }
    }

    private MyTestProvider setupProvider() {
        // Setup provider
        logger.info("Creating provider...");
        MyTestProvider testProvider = new MyTestProvider();
        logger.info("Registering provider...");
        registerProvider(testProvider, TEST_CUSTOM_DOMAIN_1, providerQosLocal);
        return testProvider;
    }

    protected void registerProvider(JoynrProvider provider, String domain, ProviderQos providerQos) {
        try {
            // wait for successful registration
            joynrRuntime.getProviderRegistrar(domain, provider)
                        .withProviderQos(providerQos)
                        .awaitGlobalRegistration()
                        .register()
                        .get(10000);
        } catch (Exception e) {
            fail("Provider registration failed: " + e);
        }
    }

    private AttributeSubscriptionListener<Integer> createListener() {
        // Set listener behavior
        logger.info("Setting listener behavior...");
        AttributeSubscriptionListener<Integer> testListener = spy(new AttributeSubscriptionListener<>() {
            @Override
            public void onSubscribed(String subscriptionId) {
                logger.info("Listener subscribed with ID: {}", subscriptionId);
            }

            @Override
            public void onReceive(Integer value) {
                logger.info("Listener receiving value {}", value);
            }

            @Override
            public void onError(JoynrRuntimeException error) {
                logger.info("Listener ran into exception");
                throw new JoynrRuntimeException("onError exception was triggered for testListener");
            }
        });
        logger.info("Set listener behavior.");
        return testListener;
    }

    // Class MyTesProvider adds the ATTRIBUTEWITHCAPITALLETTERSChanged to the generated class DefaulttestProvider
    private class MyTestProvider extends DefaulttestProvider {
        @Override
        public void ATTRIBUTEWITHCAPITALLETTERSChanged(Integer attributeWithCapitalLetters) {
            this.ATTRIBUTEWITHCAPITALLETTERS = attributeWithCapitalLetters;
        }
    }

    private void sendValuesFromProvider(MyTestProvider testProvider) {
        new Timer().scheduleAtFixedRate(new TimerTask() {
            Integer value = initialValue;

            @Override
            public void run() {
                if (value < initialValue + times) {
                    testProvider.ATTRIBUTEWITHCAPITALLETTERSChanged(value);
                    value++;
                }
            }
        }, 0, period);
    }

    private testProxy buildTestProxy() {
        Set<String> domains = new HashSet<>(List.of(TEST_CUSTOM_DOMAIN_1));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        logger.info("Creating proxy...");
        DiscoveryResult discoveryResult = guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal)
                                                            .setMessagingQos(defaultMessagingQos)
                                                            .discover();
        DiscoveryEntry lastSeenEntry = discoveryResult.getLastSeen();
        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class, lastSeenEntry.getParticipantId());
        logger.info("Created proxy.");
        return proxy;
    }

    /* This is an integration test that subscribes consumer to a producer,
     both on a single Joynr runtime, for a period of 2000 ms,
     sends off 5 Integer values and checks if all subscribed attributes (values) arrive
     Type of subscription is periodic
    */
    @Test
    public void registerPeriodicSubscriptionAndReceiveUpdates() throws InterruptedException {

        MyTestProvider testProvider = setupProvider();
        testProxy proxy = buildTestProxy();
        assertNotNull(proxy);

        AttributeSubscriptionListener<Integer> testListener = createListener();

        PeriodicSubscriptionQos listenerSubscriptionQos = new PeriodicSubscriptionQos().setPeriodMs(period)
                                                                                       .setValidityMs(lengthInMS)
                                                                                       .setAlertAfterIntervalMs(lengthInMS);

        // Register listener
        proxy.subscribeToATTRIBUTEWITHCAPITALLETTERS(testListener, listenerSubscriptionQos);
        logger.info("Subscribed listener.");

        sendValuesFromProvider(testProvider);

        sleep(lengthInMS);

        // Perform verification
        logger.info("Starting Verification");
        verify(testListener, never()).onError(null);
        // verify publications shipped correct data
        for (int i = 42; i < 42 + times; i++) {
            verify(testListener, times(1)).onReceive(i);

        }
        logger.info("Clean up lingering invocations on mockito, since called on a real object");
        clearInvocations(testListener);
        verifyNoMoreInteractions(testListener);
        logger.info("Ending Verification");
    }

    /* This is an integration test that subscribes consumer to a producer,
     both on a single Joynr runtime, for a period of 2000 ms,
     sends off 5 Integer values and checks if all subscribed attributes (values) arrive
     Type of subscription is onChange
    */
    @Ignore
    @Test
    public void registerSubscriptionOnChangeAndReceiveUpdates() throws InterruptedException {
        MyTestProvider testProvider = setupProvider();
        testProxy proxy = buildTestProxy();
        assertNotNull(proxy);

        AttributeSubscriptionListener<Integer> testListener = createListener();

        OnChangeSubscriptionQos listenerSubscriptionQos = new OnChangeSubscriptionQos().setMinIntervalMs(0)
                                                                                       .setValidityMs(lengthInMS);

        // Register listener
        proxy.subscribeToATTRIBUTEWITHCAPITALLETTERS(testListener, listenerSubscriptionQos);
        logger.info("Subscribed listener.");

        sendValuesFromProvider(testProvider);

        sleep(lengthInMS);

        // Perform verification
        logger.info("Starting Verification");

        sleep(lengthInMS + 100);
        verify(testListener, never()).onError(null);
        for (int i = 42; i < 42 + times; i++) {
            verify(testListener, times(1)).onReceive(i);
        }
        logger.info("Clean up lingering invocations on mockito, since called on a real object");
        clearInvocations(testListener);
        verifyNoMoreInteractions(testListener);
        logger.info("Ending Verification");
    }
}