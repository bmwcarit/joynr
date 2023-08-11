/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.messaging.routing;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.Singleton;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.OptionalBinder;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;
import io.joynr.accesscontrol.AccessController;
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.util.MulticastWildcardRegexFactory;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.JoynrThreadFactory;
import io.joynr.util.ObjectMapper;
import joynr.MutableMessage;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.test.JoynrTestLoggingRule;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashSet;
import java.util.Optional;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.spy;

public abstract class AbstractCcMessageRouterTest {

    private static final Logger logger = LoggerFactory.getLogger(AbstractCcMessageRouterTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private final String mqttTopic = "MessageSchedulerTest_" + createUuidString();
    protected final MqttAddress mqttAddress = new MqttAddress("mqtt://testUrl:42", mqttTopic);
    private final int maximumParallelSends = 1;
    protected final long routingTableGracePeriodMs = 30000;
    protected final long routingTableCleanupIntervalMs = 60000;

    @Mock
    protected RoutingTableAddressValidator addressValidatorMock;
    protected RoutingTable routingTable;
    protected InMemoryMulticastReceiverRegistry multicastReceiverRegistry = spy(new InMemoryMulticastReceiverRegistry(new MulticastWildcardRegexFactory()));
    protected AddressManager addressManager;

    @Mock
    protected IMessagingStub messagingStubMock;
    @Mock
    protected AbstractMiddlewareMessagingStubFactory<IMessagingStub, MqttAddress> mqttMessagingStubFactoryMock;
    @Mock
    protected AbstractMiddlewareMessagingStubFactory<IMessagingStub, WebSocketClientAddress> websocketClientMessagingStubFactoryMock;
    @Mock
    protected AbstractMiddlewareMessagingStubFactory<IMessagingStub, WebSocketAddress> webSocketMessagingStubFactoryMock;
    @Mock
    protected AbstractMiddlewareMessagingStubFactory<IMessagingStub, InProcessAddress> inProcessMessagingStubFactoryMock;
    @Mock
    protected ShutdownNotifier shutdownNotifier;
    @Mock
    protected MessagingSkeletonFactory messagingSkeletonFactoryMock;
    @Mock
    protected AccessController accessControllerMock;
    protected MessageQueue messageQueue;
    protected ScheduledExecutorService scheduler;
    protected CcMessageRouter ccMessageRouter;
    protected MutableMessage joynrMessage;
    protected String toParticipantId = "toParticipantId";
    protected String fromParticipantId = "fromParticipantId";
    protected Module testModule;
    protected Injector injector;
    protected MutableMessageFactory messageFactory;

    @Before
    public void setUp() throws Exception {
        // message runnables + cleanup thread
        final int numberOfThreads = maximumParallelSends + 1;
        scheduler = Mockito.spy(provideMessageSchedulerThreadPoolExecutor(numberOfThreads));
        doReturn(true).when(addressValidatorMock).isValidForRoutingTable(any(Address.class));
        final String[] gbidsArray = { "joynrtestgbid1", "joynrtestgbid2" };
        routingTable = spy(new RoutingTableImpl(42, gbidsArray, addressValidatorMock));
        messageQueue = spy(new MessageQueue(new DelayQueue<>()));
        addressManager = spy(new AddressManager(routingTable, Optional.empty(), multicastReceiverRegistry));

        lenient().when(mqttMessagingStubFactoryMock.create(any(MqttAddress.class))).thenReturn(messagingStubMock);
        lenient().when(messagingSkeletonFactoryMock.getSkeleton(any(Address.class))).thenReturn(Optional.empty());

        final AbstractModule mockModule = new AbstractModule() {
            @Override
            protected void configure() {
                requestStaticInjection(RoutingTypesUtil.class, MessageRouterUtil.class);
                bind(CcMessageRouter.class).in(Singleton.class);
                bind(MessageRouter.class).to(CcMessageRouter.class);
                bind(MulticastReceiverRegistrar.class).to(CcMessageRouter.class);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(AddressManager.class).toInstance(addressManager);
                bind(MulticastReceiverRegistry.class).toInstance(multicastReceiverRegistry);
                bind(ShutdownNotifier.class).toInstance(shutdownNotifier);
                Long msgRetryIntervalMs = 10L;
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS))
                                .toInstance(msgRetryIntervalMs);
                bind(Integer.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS))
                                   .toInstance(maximumParallelSends);
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_GRACE_PERIOD_MS))
                                .toInstance(routingTableGracePeriodMs);
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS))
                                .toInstance(routingTableCleanupIntervalMs);

                bindConstant().annotatedWith(Names.named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE))
                              .to(false);

                bind(AccessController.class).toInstance(accessControllerMock);

                MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory;
                messagingStubFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<>() {
                }, new TypeLiteral<>() {
                }, Names.named(MessagingStubFactory.MIDDLEWARE_MESSAGING_STUB_FACTORIES));
                messagingStubFactory.addBinding(WebSocketClientAddress.class)
                                    .toInstance(websocketClientMessagingStubFactoryMock);
                messagingStubFactory.addBinding(WebSocketAddress.class).toInstance(webSocketMessagingStubFactoryMock);
                messagingStubFactory.addBinding(MqttAddress.class).toInstance(mqttMessagingStubFactoryMock);
                messagingStubFactory.addBinding(InProcessAddress.class).toInstance(inProcessMessagingStubFactoryMock);

                bind(MessagingSkeletonFactory.class).toInstance(messagingSkeletonFactoryMock);

                OptionalBinder.newOptionalBinder(binder(), MulticastAddressCalculator.class);

                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                    .toInstance(scheduler);
                bind(MessageQueue.class).toInstance(messageQueue);
            }
        };

        testModule = Modules.override(new JsonMessageSerializerModule()).with(mockModule,
                                                                              new TestGlobalAddressModule());

        final ObjectMapper objectMapper = new ObjectMapper();
        messageFactory = new MutableMessageFactory(objectMapper, new HashSet<>());

        final boolean isGloballyVisible = true; // toParticipantId is globally visible
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = true;
        routingTable.put(toParticipantId, mqttAddress, isGloballyVisible, expiryDateMs, isSticky);

        final Request request = new Request("noMethod", new Object[]{}, new String[]{}, "requestReplyId");

        joynrMessage = messageFactory.createRequest(fromParticipantId, toParticipantId, request, new MessagingQos());
        joynrMessage.setLocalMessage(true);
    }

    protected void createDefaultMessageRouter() {
        injector = Guice.createInjector(testModule);
        ccMessageRouter = (CcMessageRouter) injector.getInstance(MessageRouter.class);
    }

    @After
    public void tearDown() {
        ccMessageRouter.shutdown();
        scheduler.shutdown();
    }

    private ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor(final int numberOfThreads) {
        final ThreadFactory schedulerNamedThreadFactory = new JoynrThreadFactory("joynr.MessageScheduler-scheduler");
        final ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(numberOfThreads,
                                                                                      schedulerNamedThreadFactory);
        scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
        scheduler.allowCoreThreadTimeOut(true);
        return scheduler;
    }
}
