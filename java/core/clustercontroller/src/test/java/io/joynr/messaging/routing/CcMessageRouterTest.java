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
package io.joynr.messaging.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.lessThan;

import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.Semaphore;
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
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.channel.ChannelMessagingSkeleton;
import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.util.MulticastWildcardRegexFactory;
import io.joynr.messaging.routing.CcMessageRouter;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.statusmetrics.MessageWorkerStatus;
import io.joynr.statusmetrics.StatusReceiver;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MulticastPublication;
import joynr.MutableMessage;
import joynr.Reply;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

@RunWith(MockitoJUnitRunner.class)
public class CcMessageRouterTest {

    private String channelId = "MessageSchedulerTest_" + UUID.randomUUID().toString();
    private final ChannelAddress channelAddress = new ChannelAddress("http://testUrl", channelId);
    private final int maximumParallelSends = 1;

    private RoutingTable routingTable = spy(new RoutingTableImpl());
    InMemoryMulticastReceiverRegistry multicastReceiverRegistry = new InMemoryMulticastReceiverRegistry(new MulticastWildcardRegexFactory());
    private AddressManager addressManager = spy(new AddressManager(routingTable,
                                                                   new AddressManager.PrimaryGlobalTransportHolder(null),
                                                                   Sets.<MulticastAddressCalculator> newHashSet(),
                                                                   multicastReceiverRegistry));

    @Mock
    private ChannelMessagingStubFactory middlewareMessagingStubFactoryMock;
    @Mock
    private IMessagingStub messagingStubMock;
    @Mock
    private ChannelMessagingSkeleton messagingSkeletonMock;
    @Mock
    private StatusReceiver statusReceiver;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    private MessageRouter messageRouter;
    private MutableMessage joynrMessage;
    protected String toParticipantId = "toParticipantId";
    protected String fromParticipantId = "fromParticipantId";

    private Module testModule;
    private MutableMessageFactory messageFactory;

    @Before
    public void setUp() throws Exception {
        when(middlewareMessagingStubFactoryMock.create(any(ChannelAddress.class))).thenReturn(messagingStubMock);

        AbstractModule mockModule = new AbstractModule() {

            private Long msgRetryIntervalMs = 10L;
            // message runnables + cleanup thread
            private int numberOfThreads = maximumParallelSends + 1;
            private long routingTableGracePeriodMs = 30000;
            private long routingTableCleanupIntervalMs = 60000;

            @Override
            protected void configure() {
                bind(MessageRouter.class).to(CcMessageRouter.class);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(AddressManager.class).toInstance(addressManager);
                bind(MulticastReceiverRegistry.class).toInstance(multicastReceiverRegistry);
                bind(ShutdownNotifier.class).toInstance(shutdownNotifier);
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

                bind(AccessController.class).toInstance(Mockito.mock(AccessController.class));
                bind(StatusReceiver.class).toInstance(statusReceiver);

                MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>> messagingStubFactory;
                messagingStubFactory = MapBinder.newMapBinder(binder(),
                                                              new TypeLiteral<Class<? extends Address>>() {
                                                              },
                                                              new TypeLiteral<AbstractMiddlewareMessagingStubFactory<? extends IMessagingStub, ? extends Address>>() {
                                                              },
                                                              Names.named(MessagingStubFactory.MIDDLEWARE_MESSAGING_STUB_FACTORIES));
                messagingStubFactory.addBinding(ChannelAddress.class).toInstance(middlewareMessagingStubFactoryMock);

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
            }

            @Provides
            @Named(MessageRouter.SCHEDULEDTHREADPOOL)
            ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor() {
                ThreadFactory schedulerNamedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.MessageScheduler-scheduler-%d")
                                                                                      .build();
                ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(numberOfThreads,
                                                                                        schedulerNamedThreadFactory);
                scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
                scheduler.allowCoreThreadTimeOut(true);
                return scheduler;
            }
        };

        testModule = Modules.override(mockModule).with(new TestGlobalAddressModule());

        Injector injector = Guice.createInjector(testModule);
        messageRouter = injector.getInstance(MessageRouter.class);

        ObjectMapper objectMapper = new ObjectMapper();
        messageFactory = new MutableMessageFactory(objectMapper, new HashSet<JoynrMessageProcessor>());

        final boolean isGloballyVisible = true; // toParticipantId is globally visible
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = true;
        final boolean allowUpdate = false;
        routingTable.put(toParticipantId, channelAddress, isGloballyVisible, expiryDateMs, isSticky, allowUpdate);

        Request request = new Request("noMethod", new Object[]{}, new String[]{}, "requestReplyId");

        joynrMessage = messageFactory.createRequest(fromParticipantId, toParticipantId, request, new MessagingQos());
        joynrMessage.setLocalMessage(true);
    }

    @Test
    public void testScheduleMessageOk() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        messageRouter.route(immutableMessage);
        Thread.sleep(1000);
        verify(middlewareMessagingStubFactoryMock).create(eq(channelAddress));
        verify(messagingStubMock).transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));
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
            verify(middlewareMessagingStubFactoryMock, never()).create(any(ChannelAddress.class));
            return;
        }
        fail("scheduling an expired message should throw");
    }

    @Test
    public void testRetryForNoParticipantFound() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000).getValue());
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

    private void prepareMulticastForMultipleAddresses(final Address receiverAddress1, final Address receiverAddress2) {
        final String multicastId = "multicastId";
        final String receiverParticipantId1 = "receiverParticipantId1";
        final String receiverParticipantId2 = "receiverParticipantId2";

        multicastReceiverRegistry.registerMulticastReceiver(multicastId, receiverParticipantId1);
        multicastReceiverRegistry.registerMulticastReceiver(multicastId, receiverParticipantId2);

        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        final boolean allowUpdate = false;
        routingTable.put(receiverParticipantId1,
                         receiverAddress1,
                         isGloballyVisible,
                         expiryDateMs,
                         isSticky,
                         allowUpdate);
        routingTable.put(receiverParticipantId2,
                         receiverAddress2,
                         isGloballyVisible,
                         expiryDateMs,
                         isSticky,
                         allowUpdate);

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000).getValue());
        joynrMessage.setType(Message.VALUE_MESSAGE_TYPE_MULTICAST);
        joynrMessage.setRecipient(multicastId);
    }

    @Test
    public void testNoMessageDuplicationForMulticastForMultipleAddressesWithErrorFromStub() throws Exception {
        ChannelAddress receiverAddress1 = new ChannelAddress("http://testUrl", "channelId1");
        ChannelAddress receiverAddress2 = new ChannelAddress("http://testUrl", "channelId2");
        prepareMulticastForMultipleAddresses(receiverAddress1, receiverAddress2);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        doAnswer(new Answer<Void>() {
            private int callCount = 0;

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                FailureAction failureAction = invocation.getArgumentAt(2, FailureAction.class);
                if (callCount < 2) {
                    callCount++;
                    failureAction.execute(new JoynrDelayMessageException(10, "first retry"));
                } else {
                    failureAction.execute(new JoynrMessageNotSentException("do not retry twice"));
                }
                return null;
            }
        }).when(messagingStubMock).transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));

        messageRouter.route(immutableMessage);

        Thread.sleep(1000);

        verify(messagingStubMock, times(4)).transmit(eq(immutableMessage),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        verify(middlewareMessagingStubFactoryMock, times(2)).create(receiverAddress1);
        verify(middlewareMessagingStubFactoryMock, times(2)).create(receiverAddress2);
    }

    private ImmutableMessage retryRoutingWith1msDelay(MessageRouter messageRouter, int ttlMs) throws Exception {
        doThrow(new JoynrDelayMessageException(1, "test")).when(messagingStubMock)
                                                          .transmit(any(ImmutableMessage.class),
                                                                    any(SuccessAction.class),
                                                                    any(FailureAction.class));
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(ttlMs).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        messageRouter.route(immutableMessage);
        Thread.sleep(100);

        return immutableMessage;
    }

    private MessageRouter getMessageRouterWithMaxRetryCount(final long routingMaxRetryCount) {
        Module testMaxRetryCountModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT))
                                .toInstance(routingMaxRetryCount);
            }
        });
        Injector injector2 = Guice.createInjector(testMaxRetryCountModule);
        return injector2.getInstance(MessageRouter.class);
    }

    @Test
    public void testRetryWithMaxRetryCount() throws Exception {
        final long routingMaxRetryCount = 3;
        MessageRouter messageRouterWithMaxRetryCount = getMessageRouterWithMaxRetryCount(routingMaxRetryCount);

        ImmutableMessage immutableMessage = retryRoutingWith1msDelay(messageRouterWithMaxRetryCount, 100000000);

        verify(messagingStubMock, times((int) routingMaxRetryCount + 1)).transmit(eq(immutableMessage),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));
    }

    @Test
    public void testRetryWithoutMaxRetryCount() throws Exception {
        ImmutableMessage immutableMessage = retryRoutingWith1msDelay(messageRouter, 200);

        verify(messagingStubMock, Mockito.atLeast(10)).transmit(eq(immutableMessage),
                                                                any(SuccessAction.class),
                                                                any(FailureAction.class));
    }

    @Test
    public void testDelayWithExponentialBackoffLimit() throws Exception {
        final long routingDuration = 1000;
        final long sendMsgRetryIntervalMs = 50;
        final long maxDelayMs = 70;
        final long toleranceMs = 20;

        Module testMaxRetryCountModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS))
                                .toInstance(sendMsgRetryIntervalMs);
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS))
                                .toInstance(maxDelayMs);

            }
        });
        Injector injector3 = Guice.createInjector(testMaxRetryCountModule);
        MessageRouter messageRouterWithMaxExponentialBackoff = injector3.getInstance(MessageRouter.class);

        Mockito.doAnswer(new Answer<Object>() {
            private long previousInvocationTimeMs = -1;

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                if (previousInvocationTimeMs == -1) { // firstRun
                    previousInvocationTimeMs = System.currentTimeMillis();

                } else {
                    long now = System.currentTimeMillis();
                    assertThat(now - previousInvocationTimeMs, lessThan(maxDelayMs + toleranceMs));
                    previousInvocationTimeMs = now;
                }

                invocation.getArgumentAt(2, FailureAction.class).execute(new Exception());

                return null;
            }
        })
               .when(messagingStubMock)
               .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        messageRouterWithMaxExponentialBackoff.route(immutableMessage);
        Thread.sleep(routingDuration);

        // test that the mock is called multiple times which means that
        // the assert inside is multiple times correct
        verify(messagingStubMock, Mockito.atLeast(10)).transmit(eq(immutableMessage),
                                                                any(SuccessAction.class),
                                                                any(FailureAction.class));
    }

    @Test
    public void testDelayWithoutExponentialBackoffLimit() throws Exception {
        // test idea is that on average more than sendMsgRetryIntervalMs ms are needed.
        // -> at least one run exists that takes longer than sendMsgRetryIntervalMs
        // -> exponential backoff for the retry interval is active

        final long routingDuration = 1000;
        final long sendMsgRetryIntervalMs = 20;
        final long expectedAverageIntervalMs = 50;

        final long maxruns = routingDuration / expectedAverageIntervalMs;

        Module testMaxRetryCountModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS))
                                .toInstance(sendMsgRetryIntervalMs);
            }
        });
        Injector injector4 = Guice.createInjector(testMaxRetryCountModule);
        MessageRouter messageRouterWithHighRetryInterval = injector4.getInstance(MessageRouter.class);

        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                invocation.getArgumentAt(2, FailureAction.class).execute(new Exception());
                return null;
            }
        })
               .when(messagingStubMock)
               .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        messageRouterWithHighRetryInterval.route(immutableMessage);
        Thread.sleep(routingDuration);

        // make sure that the stub is called at least few times
        // but not too often which means that the average retry interval
        // is much higher then initially set in sendMsgRetryIntervalMs
        verify(messagingStubMock, Mockito.atLeast(5)).transmit(eq(immutableMessage),
                                                               any(SuccessAction.class),
                                                               any(FailureAction.class));
        verify(messagingStubMock, Mockito.atMost((int) maxruns)).transmit(eq(immutableMessage),
                                                                          any(SuccessAction.class),
                                                                          any(FailureAction.class));
    }

    @Test
    public void testMessageProcessedListenerCalledOnSuccess() throws Exception {
        final Semaphore semaphore = new Semaphore(0);

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        messageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                invocation.getArgumentAt(1, SuccessAction.class).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock).transmit(any(ImmutableMessage.class),
                                            any(SuccessAction.class),
                                            any(FailureAction.class));

        messageRouter.route(immutableMessage);

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
    }

    private void testMessageProcessedListenerCalled(MessageRouter messageRouter,
                                                    Class<? extends Exception> expectedException) throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                semaphore.release();
                return null;
            }
        }).when(mockMessageProcessedListener).messageProcessed(anyString());
        messageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        if (expectedException == null) {
            messageRouter.route(immutableMessage);
        } else {
            try {
                messageRouter.route(immutableMessage);
                fail("Expected exception of type " + expectedException);
            } catch (Exception e) {
                assertEquals(expectedException, e.getClass());
            }
        }

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
    }

    @Test
    public void testMessageProcessedListenerCalledForExpiredMessage() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(0).getValue());
        joynrMessage.setTtlAbsolute(true);

        testMessageProcessedListenerCalled(messageRouter, JoynrMessageNotSentException.class);
    }

    @Test
    public void testMessageProcessedListenerCalledForMessageWithRelativeTtl() throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(false);

        testMessageProcessedListenerCalled(messageRouter, JoynrRuntimeException.class);
    }

    @Test
    public void testMessageProcessedListenerCalledForAbortedMessage() throws Exception {
        doThrow(new JoynrMessageNotSentException("test")).when(messagingStubMock).transmit(any(ImmutableMessage.class),
                                                                                           any(SuccessAction.class),
                                                                                           any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);

        testMessageProcessedListenerCalled(messageRouter, null);
    }

    @Test
    public void testMessageProcessedListenerCalledAfterMaxRetry() throws Exception {
        final long routingMaxRetryCount = 0;
        MessageRouter messageRouterWithMaxRetryCount = getMessageRouterWithMaxRetryCount(routingMaxRetryCount);

        doThrow(new JoynrDelayMessageException(1, "test")).when(messagingStubMock)
                                                          .transmit(any(ImmutableMessage.class),
                                                                    any(SuccessAction.class),
                                                                    any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);

        testMessageProcessedListenerCalled(messageRouterWithMaxRetryCount, null);
    }

    @Test
    public void testMessageProcessedListenerOnlyCalledOnceForMulticast() throws Exception {
        final Semaphore semaphore = new Semaphore(-1);

        ChannelAddress receiverAddress1 = new ChannelAddress("http://testUrl", "channelId1");
        ChannelAddress receiverAddress2 = new ChannelAddress("http://testUrl", "channelId2");
        prepareMulticastForMultipleAddresses(receiverAddress1, receiverAddress2);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        messageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                invocation.getArgumentAt(1, SuccessAction.class).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock).transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));

        messageRouter.route(immutableMessage);

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(messagingStubMock, times(2)).transmit(eq(immutableMessage),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
    }

    @Test
    public void testNotRoutableReplyDropped() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        final String unknownParticipantId = "unknown_participant_id";
        final String requestReplyId = "some_request_reply_id";

        final Reply reply = new Reply(requestReplyId, new JoynrRuntimeException("TestException"));

        final MutableMessage mutableMessage = messageFactory.createReply(fromParticipantId,
                                                                         unknownParticipantId,
                                                                         reply,
                                                                         new MessagingQos());
        final ImmutableMessage immutableMessage = mutableMessage.getImmutableMessage();

        MessageProcessedListener mockMsgProcessedListener = Mockito.mock(MessageProcessedListener.class);
        messageRouter.registerMessageProcessedListener(mockMsgProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                semaphore.release();
                return null;
            }

        }).when(mockMsgProcessedListener).messageProcessed(anyString());

        messageRouter.route(immutableMessage);
        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);

        verify(mockMsgProcessedListener).messageProcessed(immutableMessage.getId());
        verifyNoMoreInteractions(messagingStubMock);
    }

    @Test
    public void testMulticastMessageIsDroppedIfNoAddressIsFound() throws Exception {
        final Semaphore semaphore = new Semaphore(0);

        final MulticastPublication multicastPublication = new MulticastPublication(new JoynrRuntimeException("Test Exception"),
                                                                                   "multicastId");
        final MutableMessage mutableMessage = messageFactory.createMulticast("fromParticipantId",
                                                                             multicastPublication,
                                                                             new MessagingQos());
        final ImmutableMessage immutableMessage = mutableMessage.getImmutableMessage();

        MessageProcessedListener mockMsgProcessedListener = Mockito.mock(MessageProcessedListener.class);
        messageRouter.registerMessageProcessedListener(mockMsgProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                semaphore.release();
                return null;
            }

        }).when(mockMsgProcessedListener).messageProcessed(anyString());

        messageRouter.route(immutableMessage);
        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);

        verify(mockMsgProcessedListener).messageProcessed(immutableMessage.getId());
        verifyNoMoreInteractions(messagingStubMock);
    }

    @Test
    public void testMessageWorkerStatusUpdatedWhenMessageWasQueued() throws Exception {
        ArgumentCaptor<MessageWorkerStatus> messageWorkerStatusCaptor = ArgumentCaptor.forClass(MessageWorkerStatus.class);

        messageRouter.route(joynrMessage.getImmutableMessage());
        Thread.sleep(250);

        verify(statusReceiver, atLeast(1)).updateMessageWorkerStatus(eq(0), messageWorkerStatusCaptor.capture());

        // Workaround: At the beginning, the abstract message router queues two initial updates ("waiting for message") with the
        // same timestamp. Remove this duplicate by inserting all values into a LinkedHashSet which preserves the order of insertion.
        LinkedHashSet<MessageWorkerStatus> uniqueStatusUpdates = new LinkedHashSet<MessageWorkerStatus>(messageWorkerStatusCaptor.getAllValues());
        MessageWorkerStatus[] statusUpdates = uniqueStatusUpdates.toArray(new MessageWorkerStatus[0]);

        assertEquals(3, statusUpdates.length);
        assertEquals(true, statusUpdates[0].isWaitingForMessage());
        assertEquals(false, statusUpdates[1].isWaitingForMessage());
        assertEquals(true, statusUpdates[2].isWaitingForMessage());
    }

    public void testScheduleMessage() throws InterruptedException {
        final DelayQueue<DelayableImmutableMessage> messageQueue = spy(new DelayQueue<DelayableImmutableMessage>());
        Module messageQueueSpyModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(new TypeLiteral<DelayQueue<DelayableImmutableMessage>>() {
                }).toInstance(messageQueue);
            }
        });
        messageRouter = Guice.createInjector(messageQueueSpyModule).getInstance(MessageRouter.class);

        Address address = new Address();
        ImmutableMessage message = Mockito.mock(ImmutableMessage.class);
        when(message.isTtlAbsolute()).thenReturn(true);
        when(message.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(60000L).getValue());
        when(message.getRecipient()).thenReturn("to");
        when(routingTable.get("to")).thenReturn(address);

        messageRouter.route(message);

        ArgumentCaptor<DelayableImmutableMessage> passedDelaybleMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue, atLeast(1)).put(passedDelaybleMessage.capture());
        assertEquals(message, passedDelaybleMessage.getAllValues().get(0).getMessage());
        assertTrue(passedDelaybleMessage.getAllValues().get(0).getDelay(TimeUnit.MILLISECONDS) <= 0);
    }

    @Test
    public void testShutdown() throws InterruptedException {
        verify(shutdownNotifier).registerForShutdown((CcMessageRouter) messageRouter);
    }

    @Test(timeout = 3000)
    public void testFailedTransmitDoesNotLeadToThreadStarvation() throws Exception {
        final int MESSAGE_LOAD = 10;

        ImmutableMessage failingMessage = mock(ImmutableMessage.class);
        when(failingMessage.isTtlAbsolute()).thenReturn(true);
        when(failingMessage.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000L).getValue());
        when(failingMessage.getRecipient()).thenReturn("to");

        when(routingTable.get("to")).thenReturn(channelAddress);
        Set<Address> addressSet = new HashSet<>();
        addressSet.add(channelAddress);
        Mockito.doReturn(addressSet).when(addressManager).getAddresses(failingMessage);

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                assertEquals(invocation.getArguments().length, 3);
                FailureAction failureAction = (FailureAction) invocation.getArguments()[2];
                failureAction.execute(new Exception("Some error"));
                return null;
            }
        }).when(messagingStubMock).transmit(eq(failingMessage), any(SuccessAction.class), any(FailureAction.class));

        for (int i = 0; i < MESSAGE_LOAD; i++) {
            messageRouter.route(failingMessage);
        }

        Thread.sleep(2000);
        verify(messagingStubMock, atLeast(MESSAGE_LOAD * 3)).transmit(eq(failingMessage),
                                                                      any(SuccessAction.class),
                                                                      any(FailureAction.class));

        ImmutableMessage anotherMessage = mock(ImmutableMessage.class);
        when(anotherMessage.isTtlAbsolute()).thenReturn(true);
        when(anotherMessage.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000L).getValue());
        when(anotherMessage.getRecipient()).thenReturn("to");
        Mockito.doReturn(addressSet).when(addressManager).getAddresses(anotherMessage);

        final Semaphore semaphore = new Semaphore(0);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                assertEquals(invocation.getArguments().length, 3);
                SuccessAction successAction = (SuccessAction) invocation.getArguments()[1];
                successAction.execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock).transmit(eq(anotherMessage), any(SuccessAction.class), any(FailureAction.class));

        messageRouter.route(anotherMessage);
        assertTrue(semaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
    }

}
