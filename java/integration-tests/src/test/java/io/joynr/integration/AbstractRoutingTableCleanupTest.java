/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import static com.google.inject.util.Modules.override;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import javax.inject.Inject;

import org.junit.After;
import org.junit.Before;
import org.mockito.Mock;
import org.mockito.Spy;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Named;
import com.google.inject.name.Names;

import io.joynr.arbitration.ArbitrationStrategyFunction;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessagingStub;
import io.joynr.messaging.mqtt.MqttMessagingStubFactory;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.routing.RoutingEntry;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.websocket.WebSocketClientMessagingStubFactory;
import io.joynr.messaging.websocket.WebSocketMessagingStub;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.ProxyBuilderFactoryImpl;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import joynr.system.DiscoveryAsync;
import joynr.tests.DefaulttestProvider;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

/**
 * Integration tests for RoutingTable reference count handling.
 *
 */
public class AbstractRoutingTableCleanupTest {

    private final String TESTGBID1 = "testgbid1";
    private final String TESTGBID2 = "testgbid2";
    protected final String[] gbids = new String[]{ TESTGBID1, TESTGBID2 };

    protected final String TESTCUSTOMDOMAIN1 = "testCustomDomain1";
    protected final String TESTCUSTOMDOMAIN2 = "testCustomDomain2";
    protected final String TESTCUSTOMDOMAIN3 = "testCustomDomain3";

    protected final String FIXEDPARTICIPANTID1 = "fixedParticipantId1";
    protected final String FIXEDPARTICIPANTID2 = "fixedParticipantId2";
    protected final String FIXEDPARTICIPANTID3 = "fixedParticipantId3";

    private final long ROUTINGTABLE_CLEANUP_INTERVAL_MS = 100l;

    private Properties properties;
    private Injector injector;
    // JoynrRuntime to simulate user operations
    protected JoynrRuntime joynrRuntime;

    // RoutingTable for verification, use the apply() method to count entries and get info about stored addresses
    protected RoutingTable routingTable;

    protected DiscoveryQos discoveryQosGlobal;
    protected DiscoveryQos discoveryQosLocal;
    protected ProviderQos providerQosGlobal;
    protected ProviderQos providerQosLocal;
    protected MessagingQos defaultMessagingQos;

    private ConcurrentMap<String, RoutingEntry> routingTableHashMap;

    @Mock
    private MqttMessagingStubFactory mqttMessagingStubFactoryMock;
    @Mock
    private WebSocketClientMessagingStubFactory websocketClientMessagingStubFactoryMock;
    @Mock
    private WebSocketMessagingStubFactory webSocketMessagingStubFactoryMock;
    @Mock
    protected MqttMessagingStub mqttMessagingStubMock;
    @Mock
    private WebSocketMessagingStub webSocketClientMessagingStubMock;
    @Mock
    private WebSocketMessagingStub webSocketMessagingStubMock;
    @Mock
    private HivemqMqttClientFactory hiveMqMqttClientFactory;
    @Mock
    private JoynrMqttClient joynrMqttClient1;
    @Mock
    private JoynrMqttClient joynrMqttClient2;

    @Spy
    ShutdownNotifier shutdownNotifier = new ShutdownNotifier();

    protected static String sProxyParticipantId;

    protected static class TestProxyBuilderFactory extends ProxyBuilderFactoryImpl {
        private final Set<String> internalProxyParticipantIds;

        @Inject
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
                  minimumArbitrationRetryDelay);
            internalProxyParticipantIds = new HashSet<String>(Arrays.asList(discoveryProviderParticipantId,
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
    public void setUp() throws InterruptedException {
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createReceiver(TESTGBID1);
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createReceiver(TESTGBID2);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createSender(TESTGBID2);

        doReturn(mqttMessagingStubMock).when(mqttMessagingStubFactoryMock).create(any());
        doReturn(webSocketClientMessagingStubMock).when(websocketClientMessagingStubFactoryMock).create(any());
        doReturn(webSocketMessagingStubMock).when(webSocketMessagingStubFactoryMock).create(any());

        properties = createProperties(TESTGBID1 + ", " + TESTGBID2, "tcp://localhost:1883, tcp://otherhost:1883");

        discoveryQosGlobal = new DiscoveryQos();
        discoveryQosGlobal.setDiscoveryTimeoutMs(30000);
        discoveryQosGlobal.setRetryIntervalMs(discoveryQosGlobal.getDiscoveryTimeoutMs() + 1); // no retry
        discoveryQosGlobal.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        discoveryQosLocal = new DiscoveryQos();
        discoveryQosLocal.setDiscoveryTimeoutMs(30000);
        discoveryQosLocal.setRetryIntervalMs(discoveryQosLocal.getDiscoveryTimeoutMs() + 1); // no retry
        discoveryQosLocal.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        providerQosGlobal = new ProviderQos();
        providerQosGlobal.setScope(ProviderScope.GLOBAL);

        providerQosLocal = new ProviderQos();
        providerQosLocal.setScope(ProviderScope.LOCAL);

        defaultMessagingQos = new MessagingQos();

        joynrRuntime = createJoynrRuntime();

        // Get routingTable.hashMap
        routingTableHashMap = getFieldValue(routingTable, "hashMap");
        assertFalse(routingTableHashMap.isEmpty());

        sProxyParticipantId = "";
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

    private Properties createProperties(String gbids, String brokerUris) {
        Properties properties = new Properties();
        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, brokerUris);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "60,30");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,30");
        properties.put(ConfigurableMessagingSettings.PROPERTY_GBIDS, gbids);
        // Put properties for fixed participantIds
        String interfaceName = ProviderAnnotations.getInterfaceName(DefaulttestProvider.class);
        int majorVersion = ProviderAnnotations.getMajorVersion(DefaulttestProvider.class);
        properties.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(TESTCUSTOMDOMAIN1, interfaceName, majorVersion),
                       FIXEDPARTICIPANTID1);
        properties.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(TESTCUSTOMDOMAIN2, interfaceName, majorVersion),
                       FIXEDPARTICIPANTID2);
        properties.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(TESTCUSTOMDOMAIN3, interfaceName, majorVersion),
                       FIXEDPARTICIPANTID3);
        return properties;
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

    private Field getPrivateField(Class<?> privateClass, String fieldName) {
        Field result = null;
        try {
            result = privateClass.getDeclaredField(fieldName);
        } catch (Exception e) {
            fail(e.getMessage());
        }
        return result;
    }

    @SuppressWarnings("unchecked")
    private <T> T getFieldValue(Object object, String fieldName) {
        Field objectField = getPrivateField(object.getClass(), fieldName);
        assertNotNull(objectField);
        objectField.setAccessible(true);
        T result = null;
        try {
            result = (T) objectField.get(object);
        } catch (Exception e) {
            fail(e.getMessage());
        }
        return result;
    }

    protected void registerProvider(JoynrProvider provider, String domain, ProviderQos providerQos) {
        try {
            // wait for successful registration
            joynrRuntime.getProviderRegistrar(domain, provider)
                        .withProviderQos(providerQos)
                        .awaitGlobalRegistration()
                        .register()
                        .get();
        } catch (Exception e) {
            fail("Provider registration failed: " + e.toString());
        }
    }

    protected void checkRefCnt(String participantId, long expectedRefCnt) {
        assertTrue(routingTable.containsKey(participantId));
        RoutingEntry routingEntry = routingTableHashMap.get(participantId);
        long actualRefCnt = routingEntry.getRefCount();
        assertEquals(expectedRefCnt, actualRefCnt);
    }

    protected ArbitrationStrategyFunction customArbitrationStrategyFor(Set<String> participantIds) {
        return new ArbitrationStrategyFunction() {
            @Override
            public Set<DiscoveryEntryWithMetaInfo> select(Map<String, String> parameters,
                                                          Collection<DiscoveryEntryWithMetaInfo> capabilities) {
                Set<DiscoveryEntryWithMetaInfo> result = new HashSet<>();
                for (String selectedDomain : participantIds) {
                    for (DiscoveryEntryWithMetaInfo entry : capabilities) {
                        if (entry.getParticipantId().equals(selectedDomain)) {
                            result.add(entry);
                        }
                    }
                }
                return result;
            }
        };
    }

    protected void waitForGarbageCollection(String proxyParticipantId) throws InterruptedException {
        Semaphore gcSemaphore = new Semaphore(0);
        doAnswer((invocation) -> {
            invocation.callRealMethod();
            gcSemaphore.release();
            return null;
        }).when(shutdownNotifier).unregister(any(ShutdownListener.class));
        // wait until unregister of shutdownNotifier
        for (int i = 0; i < 120; i++) { // try for 1 minute
            System.gc();
            if (gcSemaphore.tryAcquire(500, TimeUnit.MILLISECONDS)) {
                System.out.println("Garbage collector has been called to remove proxy at " + i + " iteration!");
                break;
            }
        }
        assertFalse(routingTable.containsKey(proxyParticipantId));
    }

    protected <T> T createProxy(ProxyBuilder<T> proxyBuilder, MessagingQos messagingQos, DiscoveryQos discoveryQos) {
        T proxy = null;
        Future<T> proxyFuture = new Future<>();
        proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<T>() {

            @Override
            public void onProxyCreationFinished(T result) {
                proxyFuture.resolve(result);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                proxyFuture.onFailure(error);
            }
        });
        try {
            proxy = proxyFuture.get();
        } catch (Exception e) {
            fail("Proxy creation failed: " + e.toString());
        }
        return proxy;
    }

    protected JoynrRuntime createJoynrRuntime() throws InterruptedException {
        AbstractModule testBindingsModule = new AbstractModule() {
            @Override
            protected void configure() {
                // Any mqtt clients shall be mocked
                bind(MqttClientFactory.class).toInstance(hiveMqMqttClientFactory);
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY)).toInstance(gbids);
                // We want to mock all MessagingStubs except the InProcess one
                bind(WebSocketClientMessagingStubFactory.class).toInstance(websocketClientMessagingStubFactoryMock);
                bind(WebSocketMessagingStubFactory.class).toInstance(webSocketMessagingStubFactoryMock);
                bind(MqttMessagingStubFactory.class).toInstance(mqttMessagingStubFactoryMock);
                bind(ShutdownNotifier.class).toInstance(shutdownNotifier);
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS))
                                .toInstance(ROUTINGTABLE_CLEANUP_INTERVAL_MS);
                bind(ProxyBuilderFactory.class).to(TestProxyBuilderFactory.class);
            }
        };

        injector = Guice.createInjector(override(new CCInProcessRuntimeModule(),
                                                 new HivemqMqttClientModule()).with(new JoynrPropertiesModule(properties),
                                                                                    testBindingsModule));
        CountDownLatch cdl = waitForRemoveStale(mqttMessagingStubMock);

        JoynrRuntime joynrRuntime = injector.getInstance(JoynrRuntime.class);
        routingTable = injector.getInstance(RoutingTable.class);

        assertTrue(cdl.await(10, TimeUnit.SECONDS));
        reset(mqttMessagingStubMock);
        return joynrRuntime;
    }

}