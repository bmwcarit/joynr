package io.joynr.messaging.routing;

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

import static org.junit.Assert.fail;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashSet;
import java.util.UUID;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Sets;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.Provides;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;
import com.google.inject.name.Named;
import com.google.inject.util.Modules;

import io.joynr.accesscontrol.AccessController;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.JoynrMessageFactory;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.channel.ChannelMessagingSkeleton;
import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.util.MulticastWildcardRegexFactory;
import io.joynr.messaging.routing.CcMessageRouter;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class CcMessageRouterTest {

    private String channelId = "MessageSchedulerTest_" + UUID.randomUUID().toString();
    private final ChannelAddress channelAddress = new ChannelAddress("http://testUrl", channelId);

    private RoutingTable routingTable = spy(new RoutingTableImpl());
    private AddressManager addressManager = spy(new AddressManager(routingTable,
                                                                   new AddressManager.PrimaryGlobalTransportHolder(null),
                                                                   Sets.<MulticastAddressCalculator> newHashSet(),
                                                                   new InMemoryMulticastReceiverRegistry(new MulticastWildcardRegexFactory())));

    @Mock
    private ChannelMessagingStubFactory messagingStubFactoryMock;
    @Mock
    private IMessaging messagingStubMock;
    @Mock
    private ChannelMessagingSkeleton messagingSkeletonMock;

    private MessageRouter messageRouter;
    private MutableMessage joynrMessage;
    protected String toParticipantId = "toParticipantId";
    protected String fromParticipantId = "fromParticipantId";

    @Before
    public void setUp() throws Exception {
        when(messagingStubFactoryMock.create(any(ChannelAddress.class))).thenReturn(messagingStubMock);

        AbstractModule mockModule = new AbstractModule() {

            private Long msgRetryIntervalMs = 10L;
            private int maximumParallelSends = 1;

            @Override
            protected void configure() {
                bind(MessageRouter.class).to(CcMessageRouter.class);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(AddressManager.class).toInstance(addressManager);
                bind(MulticastReceiverRegistry.class).to(InMemoryMulticastReceiverRegistry.class).asEagerSingleton();
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS))
                                .toInstance(msgRetryIntervalMs);
                bindConstant().annotatedWith(Names.named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE))
                              .to(false);

                bind(AccessController.class).toInstance(Mockito.mock(AccessController.class));

                MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>> messagingStubFactory;
                messagingStubFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
                }, new TypeLiteral<AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>>() {
                }, Names.named(MessagingStubFactory.MIDDLEWARE_MESSAGING_STUB_FACTORIES));
                messagingStubFactory.addBinding(ChannelAddress.class).toInstance(messagingStubFactoryMock);

                MapBinder<Class<? extends Address>, IMessagingSkeleton> messagingSkeletonFactory;
                messagingSkeletonFactory = MapBinder.newMapBinder(binder(),
                                                                  new TypeLiteral<Class<? extends Address>>() {
                                                                  },
                                                                  new TypeLiteral<IMessagingSkeleton>() {
                                                                  },
                                                                  Names.named(MessagingSkeletonFactory.MIDDLEWARE_MESSAGING_SKELETONS));
                messagingSkeletonFactory.addBinding(ChannelAddress.class).toInstance(messagingSkeletonMock);

                Multibinder.newSetBinder(binder(), new TypeLiteral<MulticastAddressCalculator>() {
                });

                ObjectMapper objectMapper = new ObjectMapper();
                JoynrMessageFactory messageFactory = new JoynrMessageFactory(objectMapper,
                                                                             new HashSet<JoynrMessageProcessor>());

                final boolean isGloballyVisible = true; // toParticipantId is globally visible
                routingTable.put(toParticipantId, channelAddress, isGloballyVisible);

                Request request = new Request("noMethod", new Object[]{}, new String[]{}, "requestReplyId");

                joynrMessage = messageFactory.createRequest(fromParticipantId,
                                                            toParticipantId,
                                                            request,
                                                            new MessagingQos());
                joynrMessage.setLocalMessage(true);
            }

            @Provides
            @Named(MessageRouter.SCHEDULEDTHREADPOOL)
            ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor() {
                ThreadFactory schedulerNamedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.MessageScheduler-scheduler-%d")
                                                                                      .build();
                ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(maximumParallelSends,
                                                                                        schedulerNamedThreadFactory);
                scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
                scheduler.allowCoreThreadTimeOut(true);
                return scheduler;
            }
        };

        Module testModule = Modules.override(mockModule).with(new TestGlobalAddressModule());

        Injector injector = Guice.createInjector(testModule);
        messageRouter = injector.getInstance(MessageRouter.class);
    }

    @Test
    public void testScheduleMessageOk() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        messageRouter.route(immutableMessage);
        Thread.sleep(1000);
        verify(messagingStubFactoryMock).create(eq(channelAddress));
        verify(messagingStubMock).transmit(eq(immutableMessage), any(FailureAction.class));
    }

    @Test
    public void testScheduleExpiredMessageFails() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        Thread.sleep(5);
        try {
            messageRouter.route(immutableMessage);
        } catch (JoynrMessageNotSentException e) {
            verify(messagingStubFactoryMock, never()).create(any(ChannelAddress.class));
            return;
        }
        fail("scheduling an expired message should throw");
    }

    @Test
    public void testRetryForNoParticipantFound() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1000).getValue());
        joynrMessage.setTtlAbsolute(true);
        joynrMessage.setRecipient("I don't exist");

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        messageRouter.route(immutableMessage);
        Thread.sleep(100);
        verify(routingTable, atLeast(2)).containsKey("I don't exist");
        verify(addressManager, atLeast(2)).getAddresses(immutableMessage);
    }

    @Test
    public void testNoRetryForMulticastWithoutAddress() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1000).getValue());
        joynrMessage.setType(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        joynrMessage.setRecipient("multicastId");

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        messageRouter.route(immutableMessage);
        Thread.sleep(100);
        verify(addressManager).getAddresses(immutableMessage);
    }

}
