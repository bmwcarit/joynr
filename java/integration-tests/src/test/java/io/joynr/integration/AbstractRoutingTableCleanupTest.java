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
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.IOException;
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
import org.junit.Rule;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Named;
import com.google.inject.name.Names;
import com.hivemq.client.internal.checkpoint.Confirmable;
import com.hivemq.client.internal.mqtt.message.publish.MqttPublish;
import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;

import io.joynr.arbitration.ArbitrationStrategyFunction;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.integration.util.TestSetup;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.inprocess.InProcessMessagingStubFactory;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonFactory;
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
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.ProxyBuilderFactoryImpl;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.BroadcastFilterParameters;
import joynr.BroadcastSubscriptionRequest;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

/**
 * Integration tests for RoutingTable reference count handling.
 */
public class AbstractRoutingTableCleanupTest {

    private static final Logger logger = LoggerFactory.getLogger(AbstractRoutingTableCleanupTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private final String TESTGBID1 = "testgbid1";
    private final String TESTGBID2 = "testgbid2";
    protected final String[] gbids = new String[]{ TESTGBID1, TESTGBID2 };
    protected final MqttAddress replyToAddress = new MqttAddress(gbids[1], "");

    protected final String TESTCUSTOMDOMAIN1 = "testCustomDomain1";
    protected final String TESTCUSTOMDOMAIN2 = "testCustomDomain2";
    protected final String TESTCUSTOMDOMAIN3 = "testCustomDomain3";

    protected final String FIXEDPARTICIPANTID1 = "provider-1";
    protected final String FIXEDPARTICIPANTID2 = "provider-2";
    protected final String FIXEDPARTICIPANTID3 = "provider-3";

    private final long ROUTINGTABLE_CLEANUP_INTERVAL_MS = 100l;
    private final long ROUTING_MAX_RETRY_COUNT = 2;

    private Properties properties;
    protected Injector injector;
    // JoynrRuntime to simulate user operations
    protected JoynrRuntime joynrRuntime;
    // MessagingSkeletonFactory to simulate incoming messages through messaging skeletons
    protected MqttMessagingSkeletonFactory mqttSkeletonFactory;
    protected MessagingSkeletonFactory messagingSkeletonFactory;
    protected MutableMessageFactory messageFactory;

    // RoutingTable for verification, use the apply() method to count entries and get info about stored addresses
    protected RoutingTable routingTable;

    protected DiscoveryQos discoveryQosGlobal;
    protected DiscoveryQos discoveryQosLocal;
    protected ProviderQos providerQosGlobal;
    protected ProviderQos providerQosLocal;
    protected MessagingQos defaultMessagingQos;

    protected ConcurrentMap<String, RoutingEntry> routingTableHashMap;

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

    protected static String sProxyParticipantId;

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
    public void setUp() throws InterruptedException, IOException {
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createReceiver(TESTGBID1);
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createReceiver(TESTGBID2);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createSender(TESTGBID2);

        lenient().doReturn(mqttMessagingStubMock).when(mqttMessagingStubFactoryMock).create(any());
        lenient().doReturn(webSocketClientMessagingStubMock)
                 .when(websocketClientMessagingStubFactoryMock)
                 .create(any());
        lenient().doReturn(webSocketMessagingStubMock).when(webSocketMessagingStubFactoryMock).create(any());

        doAnswer(invocation -> {
            return (ImmutableMessage) invocation.getArguments()[0];
        }).when(messageProcessorMock).processIncoming(any(ImmutableMessage.class));
        doAnswer(invocation -> {
            return (MutableMessage) invocation.getArguments()[0];
        }).when(messageProcessorMock).processOutgoing(any(MutableMessage.class));

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

        properties = createProperties(TESTGBID1 + ", " + TESTGBID2, "tcp://localhost:1883, tcp://otherhost:1883");
        joynrRuntime = createJoynrRuntime();

        routingTable = injector.getInstance(RoutingTable.class);
        MqttMessagingSkeletonProvider mqttMessagingSkeletonProvider = injector.getInstance(MqttMessagingSkeletonProvider.class);
        mqttSkeletonFactory = (MqttMessagingSkeletonFactory) mqttMessagingSkeletonProvider.get();
        messagingSkeletonFactory = injector.getInstance(MessagingSkeletonFactory.class);
        messageFactory = injector.getInstance(MutableMessageFactory.class);

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

    private Properties createProperties(String gbids, String brokerUris) throws IOException {
        Properties properties = new Properties();
        properties.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        properties.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + TestSetup.findFreePort());

        properties.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS, gbids);
        properties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, brokerUris);
        properties.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "60,30");
        properties.setProperty(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,30");
        properties.setProperty(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT,
                               String.valueOf(ROUTING_MAX_RETRY_COUNT));
        // Put properties for fixed participantIds
        String interfaceName = ProviderAnnotations.getInterfaceName(DefaulttestProvider.class);
        int majorVersion = ProviderAnnotations.getMajorVersion(DefaulttestProvider.class);
        properties.setProperty(ParticipantIdKeyUtil.getProviderParticipantIdKey(TESTCUSTOMDOMAIN1,
                                                                                interfaceName,
                                                                                majorVersion),
                               FIXEDPARTICIPANTID1);
        properties.setProperty(ParticipantIdKeyUtil.getProviderParticipantIdKey(TESTCUSTOMDOMAIN2,
                                                                                interfaceName,
                                                                                majorVersion),
                               FIXEDPARTICIPANTID2);
        properties.setProperty(ParticipantIdKeyUtil.getProviderParticipantIdKey(TESTCUSTOMDOMAIN3,
                                                                                interfaceName,
                                                                                majorVersion),
                               FIXEDPARTICIPANTID3);

        properties.setProperty(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS,
                               String.valueOf(ROUTINGTABLE_CLEANUP_INTERVAL_MS));
        properties.setProperty(SystemServicesSettings.PROPERTY_CC_REMOVE_STALE_DELAY_MS, String.valueOf(0));
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

    protected Answer<Void> createVoidCountDownAnswer(CountDownLatch countDownLatch) {
        return invocation -> {
            countDownLatch.countDown();
            return null;
        };
    }

    protected String getGcdParticipantId() {
        GlobalDiscoveryEntry gcdDiscoveryEntry = injector.getInstance(Key.get(GlobalDiscoveryEntry.class,
                                                                              Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY)));
        return gcdDiscoveryEntry.getParticipantId();
    }

    protected MutableMessage createVoidReply(ImmutableMessage requestMessage) {
        String requestReplyId = requestMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
        return messageFactory.createReply(requestMessage.getRecipient(),
                                          requestMessage.getSender(),
                                          new Reply(requestReplyId),
                                          new MessagingQos());
    }

    protected MutableMessage createReplyWithException(ImmutableMessage requestMessage, JoynrException error) {
        String requestReplyId = requestMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
        return messageFactory.createReply(requestMessage.getRecipient(),
                                          requestMessage.getSender(),
                                          new Reply(requestReplyId, error),
                                          new MessagingQos());
    }

    protected MutableMessage createRequestMsg(final String from, final String to) {
        Request request = new Request("voidOperation", new Object[0], new Class[0]);
        MutableMessage requestMsg = messageFactory.createRequest(from, to, request, defaultMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(replyToAddress);
        requestMsg.setReplyTo(replyTo);
        return requestMsg;
    }

    protected MutableMessage createSrqMsg(final String from, final String to, String subscriptionId, long validityMs) {
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(0).setValidityMs(validityMs);
        BroadcastSubscriptionRequest request = new BroadcastSubscriptionRequest(subscriptionId,
                                                                                "intBroadcast",
                                                                                new BroadcastFilterParameters(),
                                                                                qos);
        MutableMessage requestMsg = messageFactory.createSubscriptionRequest(from, to, request, defaultMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(replyToAddress);
        requestMsg.setReplyTo(replyTo);
        return requestMsg;
    }

    protected void fakeIncomingMqttMessage(String targetGbid, MutableMessage msg) throws EncodingException,
                                                                                  UnsuppportedVersionException {
        IMqttMessagingSkeleton skeleton = (IMqttMessagingSkeleton) mqttSkeletonFactory.getSkeleton(new MqttAddress(targetGbid,
                                                                                                                   ""));
        Confirmable confirmableMock = mock(Confirmable.class);
        when(confirmableMock.confirm()).thenReturn(true, false);
        Mqtt5Publish publish = Mqtt5Publish.builder()
                                           .topic("testTopic")
                                           .payload(msg.getImmutableMessage().getSerializedMessage())
                                           .build();
        MqttPublish publish1 = (MqttPublish) publish;
        skeleton.transmit(publish1.withConfirmable(confirmableMock),
                          msg.getImmutableMessage().getPrefixedCustomHeaders(),
                          new FailureAction() {
                              @Override
                              public void execute(Throwable error) {
                                  fail("fake incoming MQTT message failed in skeleton.transmit: " + error);
                              }
                          });
    }

    protected void checkRefCnt(String participantId, long expectedRefCnt) {
        if (expectedRefCnt == 0) {
            assertFalse(routingTable.containsKey(participantId));
            return;
        }
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

    protected void waitFor(CountDownLatch cdl, long timeout) {
        try {
            assertTrue(cdl.await(timeout, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("wait failed: " + e);
        }
    }

    protected void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (Exception e) {
            fail("Sleep failed: " + e);
        }
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
                // We want to mock all MessagingStubs except the InProcess one
                bind(WebSocketClientMessagingStubFactory.class).toInstance(websocketClientMessagingStubFactoryMock);
                bind(WebSocketMessagingStubFactory.class).toInstance(webSocketMessagingStubFactoryMock);
                bind(MqttMessagingStubFactory.class).toInstance(mqttMessagingStubFactoryMock);
                bind(InProcessMessagingStubFactory.class).toInstance(inProcessMessagingStubFactorySpy);

                bind(ShutdownNotifier.class).toInstance(shutdownNotifier);
                bind(ProxyBuilderFactory.class).to(TestProxyBuilderFactory.class);

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

    protected void registerProvider(JoynrProvider provider, String domain, ProviderQos providerQos) {
        try {
            // wait for successful registration
            joynrRuntime.getProviderRegistrar(domain, provider)
                        .withProviderQos(providerQos)
                        .awaitGlobalRegistration()
                        .register()
                        .get(10000);
        } catch (Exception e) {
            fail("Provider registration failed: " + e.toString());
        }
    }

    protected void registerGlobal(JoynrProvider provider, String domain, ProviderQos providerQos) {
        reset(mqttMessagingStubMock);
        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(mqttMessagingStubMock)
                                                                  .transmit(messageCaptor.capture(),
                                                                            any(SuccessAction.class),
                                                                            any(FailureAction.class));

        Future<Void> future = joynrRuntime.getProviderRegistrar(domain, provider)
                                          .withProviderQos(providerQos)
                                          .withGbids(gbids)
                                          .awaitGlobalRegistration()
                                          .register();

        try {
            assertTrue(publishCountDownLatch.await(1500, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        verify(mqttMessagingStubMock).transmit(any(ImmutableMessage.class),
                                               any(SuccessAction.class),
                                               any(FailureAction.class));
        ImmutableMessage capturedMessage = messageCaptor.getValue();
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, capturedMessage.getType());
        assertEquals(getGcdParticipantId(), capturedMessage.getRecipient());

        reset(mqttMessagingStubMock);

        try {
            MutableMessage replyMessage = createVoidReply(capturedMessage);
            fakeIncomingMqttMessage(gbids[0], replyMessage);
            // wait for successful registration
            future.get(10000);
        } catch (Exception e) {
            fail("Provider registration failed: " + e.toString());
        }
    }

    protected ArgumentCaptor<ImmutableMessage> prepareGlobalRemove(CountDownLatch removeCdl) {
        ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        doAnswer(createVoidCountDownAnswer(removeCdl)).when(mqttMessagingStubMock).transmit(messageCaptor.capture(),
                                                                                            any(SuccessAction.class),
                                                                                            any(FailureAction.class));
        return messageCaptor;
    }

    protected void waitForGlobalRemove(CountDownLatch removeCdl, ArgumentCaptor<ImmutableMessage> msgCaptor) {

        try {
            assertTrue(removeCdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        verify(mqttMessagingStubMock).transmit(any(ImmutableMessage.class),
                                               any(SuccessAction.class),
                                               any(FailureAction.class));
        ImmutableMessage capturedMessage = msgCaptor.getValue();
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, capturedMessage.getType());
        assertEquals(getGcdParticipantId(), capturedMessage.getRecipient());

        try {
            MutableMessage replyMessage = createVoidReply(capturedMessage);
            fakeIncomingMqttMessage(gbids[0], replyMessage);
        } catch (Exception e) {
            fail("Provider unregistration failed: " + e.toString());
        }

        reset(mqttMessagingStubMock);
    }

    protected void unregisterGlobal(String domain, Object provider) {
        reset(mqttMessagingStubMock);
        CountDownLatch removeCdl = new CountDownLatch(1);
        ArgumentCaptor<ImmutableMessage> msgCaptor = prepareGlobalRemove(removeCdl);
        joynrRuntime.unregisterProvider(domain, provider);
        waitForGlobalRemove(removeCdl, msgCaptor);
        // Wait for a while until global remove has finished (reply processed at LCD)
        try {
            Thread.sleep(200);
        } catch (Exception e) {
            fail("Sleeping failed: " + e.toString());
        }
    }

}
