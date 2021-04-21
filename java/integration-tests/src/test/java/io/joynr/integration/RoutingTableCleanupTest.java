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
import static org.mockito.Mockito.spy;

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
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.runners.MockitoJUnitRunner;

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
import io.joynr.messaging.mqtt.MqttMessagingSkeletonProvider;
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
import io.joynr.proxy.GuidedProxyBuilder;
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
import joynr.tests.testProxy;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@RunWith(MockitoJUnitRunner.class)
public class RoutingTableCleanupTest {

    private final String TESTGBID1 = "testgbid1";
    private final String TESTGBID2 = "testgbid2";
    private final String[] gbids = new String[]{ TESTGBID1, TESTGBID2 };

    private final String TESTCUSTOMDOMAIN1 = "testCustomDomain1";
    private final String TESTCUSTOMDOMAIN2 = "testCustomDomain2";
    private final String TESTCUSTOMDOMAIN3 = "testCustomDomain3";

    private final String FIXEDPARTICIPANTID1 = "fixedParticipantId1";
    private final String FIXEDPARTICIPANTID2 = "fixedParticipantId2";
    private final String FIXEDPARTICIPANTID3 = "fixedParticipantId3";

    private final long ROUTINGTABLE_CLEANUP_INTERVAL_MS = 100l;

    private Properties properties;
    private Injector injector;
    // JoynrRuntime to simulate user operations
    private JoynrRuntime joynrRuntime;
    // MessagingSkeletonFactory to simulate incoming messages through messaging skeletons
    private MqttMessagingSkeletonProvider mqttMessagingSkeletonProvider;

    // RoutingTable for verification, use the apply() method to count entries and get info about stored addresses
    private RoutingTable routingTable;

    private DiscoveryQos discoveryQosGlobal;
    private DiscoveryQos discoveryQosLocal;
    private ProviderQos providerQosGlobal;
    private ProviderQos providerQosLocal;
    private MessagingQos defaultMessagingQos;

    private ConcurrentMap<String, RoutingEntry> routingTableHashMap;

    @Mock
    private MqttMessagingStubFactory mqttMessagingStubFactoryMock;
    @Mock
    private WebSocketClientMessagingStubFactory websocketClientMessagingStubFactoryMock;
    @Mock
    private WebSocketMessagingStubFactory webSocketMessagingStubFactoryMock;

    @Mock
    private MqttMessagingStub mqttMessagingStubMock;
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

    private static String sProxyParticipantId;

    private static class TestProxyBuilderFactory extends ProxyBuilderFactoryImpl {
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
                RoutingTableCleanupTest.sProxyParticipantId = proxyParticipantId;
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

    private void registerProvider(JoynrProvider provider, String domain, ProviderQos providerQos) {
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

    private void checkRefCnt(String participantId, long expectedRefCnt) {
        assertTrue(routingTable.containsKey(participantId));
        RoutingEntry routingEntry = routingTableHashMap.get(participantId);
        long actualRefCnt = routingEntry.getRefCount();
        assertEquals(expectedRefCnt, actualRefCnt);
    }

    private ArbitrationStrategyFunction customArbitrationStrategyFor(Set<String> participantIds) {
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

    private void waitForGarbageCollection(String proxyParticipantId) throws InterruptedException {
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

    private <T> T createProxy(ProxyBuilder<T> proxyBuilder, MessagingQos messagingQos, DiscoveryQos discoveryQos) {
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
        mqttMessagingSkeletonProvider = injector.getInstance(MqttMessagingSkeletonProvider.class);

        assertTrue(cdl.await(10, TimeUnit.SECONDS));
        reset(mqttMessagingStubMock);
        return joynrRuntime;
    }

    @Test
    public void createAndDestroyProxy_routingEntryAddedAndRemoved() throws InterruptedException {
        // register provider
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        Thread.sleep(1l);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosLocal);
        assertNotNull(proxy);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        // set proxy and proxyBuilder to null
        proxy = null;
        proxyBuilder = null;

        waitForGarbageCollection(proxyParticipantId);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void createAndDestroyMultiProxy_routingEntryAddedAndRemoved() throws InterruptedException {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // Define custom arbitration strategy function to get multiple providers
        ArbitrationStrategyFunction arbitrationStrategyFunction = customArbitrationStrategyFor(new HashSet<>(Arrays.asList(FIXEDPARTICIPANTID1,
                                                                                                                           FIXEDPARTICIPANTID2)));

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQos);
        assertNotNull(proxy);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // set proxy and proxyBuilder to null
        proxy = null;
        proxyBuilder = null;

        waitForGarbageCollection(proxyParticipantId);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void registerAndUnregisterProviders_routingEntriesAddedAdnRemoved() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);

        // Check reference count values of providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);

        // Wait for a while until providers are unregistered
        try {
            Thread.sleep(200);
        } catch (Exception e) {
            fail("Sleeping failed: " + e.toString());
        }

        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
    }

    @Test
    public void registerProviders_createProxy_refCountForSelectedProviderIncremented() throws InterruptedException {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        Thread.sleep(1l);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // create proxy
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosLocal);
        assertNotNull(proxy);

        // Check refCount values of routing entries for fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void registerProviders_createMultiProxy_refCountForSelectedProviderIncremented() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // Define custom arbitration strategy function to get multiple providers
        ArbitrationStrategyFunction arbitrationStrategyFunction = customArbitrationStrategyFor(new HashSet<>(Arrays.asList(FIXEDPARTICIPANTID1,
                                                                                                                           FIXEDPARTICIPANTID2)));

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // create proxy
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQos);
        assertNotNull(proxy);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void callProxyOperation_refCountOfProviderAndProxyIsTheSame() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTCUSTOMDOMAIN1, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosLocal);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);
        // Check refCount value of routing entries for fixed provider
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // Perform any proxy operation
        proxy.addNumbers(10, 20, 30);

        // Check if the refCount values of proxy and provider are the same
        checkRefCnt(proxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
    }

    @Test
    public void callMultiProxyOperation_refCountOfProviderAndProxyIsTheSame() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = spy(new DefaulttestProvider());

        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // Define custom arbitration strategy function to get multiple providers
        ArbitrationStrategyFunction arbitrationStrategyFunction = customArbitrationStrategyFor(new HashSet<>(Arrays.asList(FIXEDPARTICIPANTID1,
                                                                                                                           FIXEDPARTICIPANTID2)));

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQos);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);
        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((invocation) -> {
            checkRefCnt(proxyParticipantId, 1l);
            checkRefCnt(FIXEDPARTICIPANTID1, 2l);
            checkRefCnt(FIXEDPARTICIPANTID2, 2l);
            checkRefCnt(FIXEDPARTICIPANTID3, 1l);
            Void result = (Void) invocation.callRealMethod();
            cdl.countDown();
            return result;
        }).when(testProvider).methodFireAndForgetWithoutParams();

        // Perform any proxy operation
        proxy.methodFireAndForgetWithoutParams();
        try {
            assertTrue(cdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_createAndDestroyProxy_routingEntryAddedAndRemoved() throws InterruptedException {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<String>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();

        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class, FIXEDPARTICIPANTID1);
        assertNotNull(proxy);

        // Check whether routing entry for built proxy has been created
        assertFalse(sProxyParticipantId.isEmpty());
        checkRefCnt(sProxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // set proxy and proxyBuilder to null
        proxy = null;
        guidedProxyBuilder = null;

        waitForGarbageCollection(sProxyParticipantId);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_registerProviders_createProxy_refCountForSelectedProviderIncremented() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();

        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class, FIXEDPARTICIPANTID1);
        assertNotNull(proxy);

        // Check refCount values of routing entries for fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_callProxyOperation_refCountOfProviderAndProxyIsTheSame() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();
        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class, FIXEDPARTICIPANTID1);

        // Check whether routing entry for built proxy has been created
        assertFalse(sProxyParticipantId.isEmpty());
        checkRefCnt(sProxyParticipantId, 1l);

        // Get refCount values of routing entries for fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // Perform any proxy operation
        proxy.addNumbers(10, 20, 30);

        // Check if the refCount values of proxy and provider are the same
        checkRefCnt(sProxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_discoverAndBuildNone_routingEntryOfProxyNotCreated() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        // Get refCount values of routing entries for fixed provider
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // perform discovery
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();

        // Check whether routing entry for proxy has not been created
        assertTrue(sProxyParticipantId.isEmpty());

        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 2l);

        // build none
        guidedProxyBuilder.buildNone();

        // Check whether number of routing entries has not been changed
        assertTrue(sProxyParticipantId.isEmpty());
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN3, testProvider);
    }
}
