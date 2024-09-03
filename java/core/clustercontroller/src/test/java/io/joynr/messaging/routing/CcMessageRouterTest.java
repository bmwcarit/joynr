/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.atMost;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.accesscontrol.HasConsumerPermissionCallback;
import io.joynr.common.ExpiryDate;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingSkeleton;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Message.MessageType;
import joynr.MulticastPublication;
import joynr.MutableMessage;
import joynr.Reply;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.UdsClientAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class CcMessageRouterTest extends AbstractCcMessageRouterTest {

    @Test
    public void cleanupJobRemovesPurgesExpiredRoutingEntries() throws IllegalArgumentException {
        createDefaultMessageRouter();
        // Capture Runnable when cleanup job is invoked
        ArgumentCaptor<Runnable> runnableCaptor = ArgumentCaptor.forClass(Runnable.class);
        verify(scheduler, times(1)).scheduleWithFixedDelay(runnableCaptor.capture(),
                                                           eq(routingTableCleanupIntervalMs),
                                                           eq(routingTableCleanupIntervalMs),
                                                           eq(TimeUnit.MILLISECONDS));
        runnableCaptor.getValue().run();
        verify(routingTable).purge();
    }

    private void testScheduleMessageOk(Consumer<ImmutableMessage> route) throws Exception {
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1000000).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        route.accept(immutableMessage);

        ArgumentCaptor<DelayableImmutableMessage> passedDelaybleMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue).put(passedDelaybleMessage.capture());
        assertEquals(immutableMessage, passedDelaybleMessage.getAllValues().get(0).getMessage());
        final long delayMs = passedDelaybleMessage.getAllValues().get(0).getDelay(TimeUnit.MILLISECONDS);
        assertTrue("Delay was: " + delayMs, delayMs <= 0);

        verify(messagingStubMock, timeout(2000)).transmit(eq(immutableMessage),
                                                          any(SuccessAction.class),
                                                          any(FailureAction.class));
        verify(mqttMessagingStubFactoryMock).create(mqttAddress);
    }

    @Test
    public void testScheduleExpiredMessage() throws Exception {
        createDefaultMessageRouter();
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        Thread.sleep(5);
        ccMessageRouter.routeOut(immutableMessage);

        ArgumentCaptor<DelayableImmutableMessage> passedDelaybleMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue).put(passedDelaybleMessage.capture());
        assertEquals(immutableMessage, passedDelaybleMessage.getAllValues().get(0).getMessage());
        final long delayMs = passedDelaybleMessage.getAllValues().get(0).getDelay(TimeUnit.MILLISECONDS);
        assertTrue("Delay was: " + delayMs, delayMs <= 0);
    }

    // In theory, every test could be split like that. This is not done, since it
    // would just clutter the class without any benefits.
    @Test
    public void routeIn_scheduleMessageOk() throws Exception {
        createDefaultMessageRouter();
        testScheduleMessageOk(m -> ccMessageRouter.routeIn(m));
    }

    @Test
    public void routeOut_scheduleMessageOk() throws Exception {
        createDefaultMessageRouter();
        testScheduleMessageOk(m -> ccMessageRouter.routeOut(m));
    }

    private void prepareMulticastForMultipleAddresses(final Address receiverAddress1, final Address receiverAddress2) {
        final String multicastId = "multicastId";
        final String receiverParticipantId1 = "receiverParticipantId1";
        final String receiverParticipantId2 = "receiverParticipantId2";

        multicastReceiverRegistry.registerMulticastReceiver(multicastId, receiverParticipantId1);
        multicastReceiverRegistry.registerMulticastReceiver(multicastId, receiverParticipantId2);

        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        routingTable.put(receiverParticipantId1, receiverAddress1, isGloballyVisible, expiryDateMs);
        routingTable.put(receiverParticipantId2, receiverAddress2, isGloballyVisible, expiryDateMs);

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000).getValue());
        joynrMessage.setTtlAbsolute(true);
        joynrMessage.setType(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST);
        joynrMessage.setRecipient(multicastId);
    }

    @Test
    public void testNoMessageDuplicationForMulticastReceiversWithSameAddress() throws Exception {
        createDefaultMessageRouter();
        MqttAddress receiverAddress1 = new MqttAddress("what", "ever1");
        prepareMulticastForMultipleAddresses(receiverAddress1, receiverAddress1);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        ccMessageRouter.routeOut(immutableMessage);

        Thread.sleep(1000);

        verify(messagingStubMock, times(1)).transmit(eq(immutableMessage),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        verify(mqttMessagingStubFactoryMock, times(1)).create(receiverAddress1);
    }

    @Test
    public void testNoMessageDuplicationForMulticastForMultipleAddressesWithErrorFromStubForAllAddresses() throws Exception {
        createDefaultMessageRouter();
        MqttAddress receiverAddress1 = new MqttAddress("what", "ever1");
        MqttAddress receiverAddress2 = new MqttAddress("what", "ever2");
        prepareMulticastForMultipleAddresses(receiverAddress1, receiverAddress2);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        doAnswer(new Answer<Void>() {
            private int callCount = 0;

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                FailureAction failureAction = invocation.getArgument(2);
                if (callCount < 2) {
                    callCount++;
                    failureAction.execute(new JoynrDelayMessageException(10, "first retry"));
                } else {
                    failureAction.execute(new JoynrMessageNotSentException("do not retry twice"));
                }
                return null;
            }
        }).when(messagingStubMock).transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);

        Thread.sleep(1000);

        verify(messagingStubMock, times(4)).transmit(eq(immutableMessage),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        verify(mqttMessagingStubFactoryMock, times(2)).create(receiverAddress1);
        verify(mqttMessagingStubFactoryMock, times(2)).create(receiverAddress2);
    }

    @Test
    public void testMulticastIsOnlyResentForFailingAddress() throws Exception {
        createDefaultMessageRouter();
        MqttAddress receiverAddress1 = new MqttAddress("http://testUrl", "channelId1");
        MqttAddress receiverAddress2 = new MqttAddress("http://testUrl", "channelId2");
        prepareMulticastForMultipleAddresses(receiverAddress1, receiverAddress2);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        IMessagingStub successfulMessagingStubMock = Mockito.mock(IMessagingStub.class);
        IMessagingStub failingMessagingStubMock = Mockito.mock(IMessagingStub.class);
        when(mqttMessagingStubFactoryMock.create(receiverAddress1)).thenReturn(successfulMessagingStubMock);
        when(mqttMessagingStubFactoryMock.create(receiverAddress2)).thenReturn(failingMessagingStubMock);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                SuccessAction successAction = invocation.getArgument(1);
                successAction.execute();
                return null;
            }
        }).when(successfulMessagingStubMock)
          .transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));

        doAnswer(new Answer<Void>() {
            private int callCount = 0;

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                FailureAction failureAction = invocation.getArgument(2);
                if (callCount < 2) {
                    callCount++;
                    failureAction.execute(new JoynrDelayMessageException(10, "first retry"));
                } else {
                    failureAction.execute(new JoynrMessageNotSentException("do not retry thrice"));
                }
                return null;
            }
        }).when(failingMessagingStubMock)
          .transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);

        Thread.sleep(1000);

        verify(successfulMessagingStubMock, times(1)).transmit(eq(immutableMessage),
                                                               any(SuccessAction.class),
                                                               any(FailureAction.class));
        verify(failingMessagingStubMock, times(3)).transmit(eq(immutableMessage),
                                                            any(SuccessAction.class),
                                                            any(FailureAction.class));
        verify(addressManager, times(1)).getParticipantIdMap(immutableMessage);
        verify(addressManager, atLeast(4)).getAddressForDelayableImmutableMessage(any());
        verify(mqttMessagingStubFactoryMock, times(1)).create(receiverAddress1);
        verify(mqttMessagingStubFactoryMock, times(3)).create(receiverAddress2);
    }

    private ImmutableMessage retryRoutingWith1msDelay(int ttlMs) throws Exception {
        doThrow(new JoynrDelayMessageException(1, "test")).when(messagingStubMock).transmit(any(ImmutableMessage.class),
                                                                                            any(SuccessAction.class),
                                                                                            any(FailureAction.class));
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(ttlMs).getValue());
        joynrMessage.setTtlAbsolute(true);

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        ccMessageRouter.routeOut(immutableMessage);
        Thread.sleep(100);

        return immutableMessage;
    }

    private void createCcMessageRouterWithMaxRetryCount(final long routingMaxRetryCount) {
        Module testMaxRetryCountModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT))
                                .toInstance(routingMaxRetryCount);
            }
        });
        injector = Guice.createInjector(testMaxRetryCountModule);
        ccMessageRouter = (CcMessageRouter) injector.getInstance(MessageRouter.class);
    }

    @Test
    public void testRetryWithMaxRetryCount() throws Exception {
        final long routingMaxRetryCount = 3;
        createCcMessageRouterWithMaxRetryCount(routingMaxRetryCount);

        ImmutableMessage immutableMessage = retryRoutingWith1msDelay(100000000);

        verify(messagingStubMock, times((int) routingMaxRetryCount + 1)).transmit(eq(immutableMessage),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));
    }

    @Test
    public void testRetryWithoutMaxRetryCount() throws Exception {
        createDefaultMessageRouter();
        ImmutableMessage immutableMessage = retryRoutingWith1msDelay(10000);

        verify(messagingStubMock, atLeast(20)).transmit(eq(immutableMessage),
                                                        any(SuccessAction.class),
                                                        any(FailureAction.class));
    }

    @Test
    public void testDelayWithExponentialBackoffLimit() throws Exception {
        final long routingDuration = 10000;
        final long sendMsgRetryIntervalMs = 500;
        final long maxDelayMs = 700;
        final long toleranceMs = 200;

        Module testMaxRetryCountModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS))
                                .toInstance(sendMsgRetryIntervalMs);
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS))
                                .toInstance(maxDelayMs);

            }
        });
        injector = Guice.createInjector(testMaxRetryCountModule);
        ccMessageRouter = (CcMessageRouter) injector.getInstance(MessageRouter.class);

        doAnswer(new Answer<Object>() {
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

                ((FailureAction) invocation.getArgument(2)).execute(new Exception());

                return null;
            }
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        ccMessageRouter.routeOut(immutableMessage);
        Thread.sleep(routingDuration);

        // test that the mock is called multiple times which means that
        // the assert inside is multiple times correct
        verify(messagingStubMock, atLeast(10)).transmit(eq(immutableMessage),
                                                        any(SuccessAction.class),
                                                        any(FailureAction.class));
    }

    @Test
    public void testDelayWithoutExponentialBackoffLimit() throws Exception {
        // test idea is that on average more than sendMsgRetryIntervalMs ms are needed.
        // -> at least one run exists that takes longer than sendMsgRetryIntervalMs
        // -> exponential backoff for the retry interval is active
        final long routingDuration = 10000;
        final long sendMsgRetryIntervalMs = 200;
        final long expectedAverageIntervalMs = 500;

        final long maxruns = routingDuration / expectedAverageIntervalMs;

        Module testMaxRetryCountModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS))
                                .toInstance(sendMsgRetryIntervalMs);
            }
        });
        injector = Guice.createInjector(testMaxRetryCountModule);
        ccMessageRouter = (CcMessageRouter) injector.getInstance(MessageRouter.class);

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                ((FailureAction) invocation.getArgument(2)).execute(new Exception());
                return null;
            }
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        ccMessageRouter.routeOut(immutableMessage);
        Thread.sleep(routingDuration);

        // make sure that the stub is called at least few times
        // but not too often which means that the average retry interval
        // is much higher then initially set in sendMsgRetryIntervalMs
        verify(messagingStubMock, atLeast(5)).transmit(eq(immutableMessage),
                                                       any(SuccessAction.class),
                                                       any(FailureAction.class));
        verify(messagingStubMock, atMost((int) maxruns)).transmit(eq(immutableMessage),
                                                                  any(SuccessAction.class),
                                                                  any(FailureAction.class));
    }

    @Test
    public void testMessageProcessedListenerCalledOnSuccessForRequest() throws Exception {
        createDefaultMessageRouter();
        final Semaphore semaphore = new Semaphore(0);

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                ((SuccessAction) invocation.getArgument(1)).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
        verify(routingTable, never()).remove(anyString());
    }

    @Test
    public void testMessageProcessedListenerCalledOnSuccessForReply_inProcessRecipient() throws Exception {
        createDefaultMessageRouter();
        final Semaphore semaphore = new Semaphore(0);

        String toInProcess = "toInProcess";
        final Reply reply = new Reply("requestReplyId");
        joynrMessage = messageFactory.createReply(fromParticipantId, toInProcess, reply, new MessagingQos());
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        when(inProcessMessagingStubFactoryMock.create(any(InProcessAddress.class))).thenReturn(messagingStubMock);
        InProcessAddress inProcessAddress = new InProcessAddress(mock(InProcessMessagingSkeleton.class));
        routingTable.put(toInProcess, inProcessAddress, true, Long.MAX_VALUE);

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                ((SuccessAction) invocation.getArgument(1)).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeIn(immutableMessage);

        semaphore.tryAcquire(100000, TimeUnit.MILLISECONDS);
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
        verify(routingTable, never()).remove(eq(joynrMessage.getRecipient()));
    }

    @Test
    public void testMessageProcessedListenerCalledOnSuccessForReply_mqttRecipient() throws Exception {
        createDefaultMessageRouter();
        final Semaphore semaphore = new Semaphore(0);

        final Reply reply = new Reply("requestReplyId");
        joynrMessage = messageFactory.createReply(fromParticipantId, toParticipantId, reply, new MessagingQos());
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                ((SuccessAction) invocation.getArgument(1)).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
        verify(routingTable, times(1)).remove(eq(joynrMessage.getRecipient()));
    }

    @Test
    public void testReferenceCountDecreasedOnSubscriptionReplyOfInProcessProvider() throws Exception {
        createDefaultMessageRouter();
        final Semaphore semaphore = new Semaphore(0);

        InProcessAddress providerAddress = new InProcessAddress();
        routingTable.put(fromParticipantId, providerAddress, true, Long.MAX_VALUE, true);

        SubscriptionReply subscriptionReply = new SubscriptionReply("subscriptionId");

        joynrMessage = messageFactory.createSubscriptionReply(fromParticipantId,
                                                              toParticipantId,
                                                              subscriptionReply,
                                                              new MessagingQos());

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                verify(routingTable, times(0)).remove(toParticipantId);
                ((SuccessAction) invocation.getArgument(1)).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
        verify(routingTable).remove(toParticipantId);
    }

    private void testReferenceCountNotDecreasedOnSubscriptionReplyOfNotInProcessProvider(Address address) throws Exception {
        createDefaultMessageRouter();
        final Semaphore semaphore = new Semaphore(0);

        routingTable.put(fromParticipantId, address, true, Long.MAX_VALUE, true);

        SubscriptionReply subscriptionReply = new SubscriptionReply("subscriptionId");

        joynrMessage = messageFactory.createSubscriptionReply(fromParticipantId,
                                                              toParticipantId,
                                                              subscriptionReply,
                                                              new MessagingQos());

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                verify(routingTable, times(0)).remove(toParticipantId);
                ((SuccessAction) invocation.getArgument(1)).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeIn(immutableMessage);

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
        verify(routingTable, never()).remove(toParticipantId);
    }

    @Test
    public void testReferenceCountDecreasedOnSubscriptionReplyOfBinderProvider() throws Exception {
        testReferenceCountNotDecreasedOnSubscriptionReplyOfNotInProcessProvider(new BinderAddress());
    }

    @Test
    public void testReferenceCountDecreasedOnSubscriptionReplyOfMqttProvider() throws Exception {
        testReferenceCountNotDecreasedOnSubscriptionReplyOfNotInProcessProvider(new MqttAddress());
    }

    @Test
    public void testReferenceCountDecreasedOnSubscriptionReplyOfUdsClientProvider() throws Exception {
        testReferenceCountNotDecreasedOnSubscriptionReplyOfNotInProcessProvider(new UdsClientAddress());
    }

    @Test
    public void testReferenceCountDecreasedOnSubscriptionReplyOfWebsocketClientProvider() throws Exception {
        testReferenceCountNotDecreasedOnSubscriptionReplyOfNotInProcessProvider(new WebSocketClientAddress());
    }

    private void testMessageProcessedListenerCalledOnError(int messageProcessedInvocations) throws Exception {
        CountDownLatch countDownLatch = new CountDownLatch(messageProcessedInvocations);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        MqttAddress proxyAddress = new MqttAddress();
        boolean putResult = routingTable.put(fromParticipantId, proxyAddress, true, Long.MAX_VALUE, true);
        assertTrue(putResult);

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                countDownLatch.countDown();
                return null;
            }
        }).when(mockMessageProcessedListener).messageProcessed(anyString());
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        ccMessageRouter.routeIn(immutableMessage);

        assertTrue(countDownLatch.await(1000, TimeUnit.MILLISECONDS));
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));

        MessageType type = immutableMessage.getType();
        if (CcMessageRouter.MESSAGE_TYPE_REQUESTS.contains(type)) {
            verify(routingTable, times(1)).remove(eq(immutableMessage.getSender()));
        } else if (CcMessageRouter.MESSAGE_TYPE_REPLIES.contains(type)) {
            verify(routingTable, times(1)).remove(eq(immutableMessage.getRecipient()));
        }
    }

    @Test
    public void testMessageProcessedListenerCalledForExpiredRequestMessage() throws Exception {
        createDefaultMessageRouter();
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(0).getValue());
        joynrMessage.setTtlAbsolute(true);

        testMessageProcessedListenerCalledOnError(1);
    }

    @Test
    public void testMessageProcessedListenerCalledForExpiredReplyMessage() throws Exception {
        createDefaultMessageRouter();
        final Reply reply = new Reply("requestReplyId", new JoynrRuntimeException("TestException"));
        joynrMessage = messageFactory.createReply(fromParticipantId, toParticipantId, reply, new MessagingQos());
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(0).getValue());
        joynrMessage.setTtlAbsolute(true);

        testMessageProcessedListenerCalledOnError(1);
    }

    @Test
    public void testMessageProcessedListenerCalledForMessageWithRelativeTtl() throws Exception {
        createDefaultMessageRouter();
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(false);

        testMessageProcessedListenerCalledOnError(1);
    }

    @Test
    public void testMessageProcessedListenerCalledForAbortedMessage() throws Exception {
        createDefaultMessageRouter();
        doThrow(new JoynrMessageNotSentException("test")).when(messagingStubMock).transmit(any(ImmutableMessage.class),
                                                                                           any(SuccessAction.class),
                                                                                           any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);

        // If an aborted message is not expired, a reply will be sent to the sender.
        // The decrease of the reference count happens after that
        testMessageProcessedListenerCalledOnError(2);
    }

    @Test
    public void testMessageProcessedListenerCalledAfterMaxRetry() throws Exception {
        final long routingMaxRetryCount = 0;
        createCcMessageRouterWithMaxRetryCount(routingMaxRetryCount);

        doThrow(new JoynrDelayMessageException(1, "test")).when(messagingStubMock).transmit(any(ImmutableMessage.class),
                                                                                            any(SuccessAction.class),
                                                                                            any(FailureAction.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        joynrMessage.setTtlAbsolute(true);

        testMessageProcessedListenerCalledOnError(1);
    }

    @Test
    public void testMessageProcessedListenerOnlyCalledOnceForMulticast() throws Exception {
        createDefaultMessageRouter();
        final Semaphore semaphore = new Semaphore(-1);

        MqttAddress receiverAddress1 = new MqttAddress("what", "ever1");
        MqttAddress receiverAddress2 = new MqttAddress("what", "ever2");
        prepareMulticastForMultipleAddresses(receiverAddress1, receiverAddress2);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                verify(mockMessageProcessedListener, times(0)).messageProcessed(eq(immutableMessage.getId()));
                ((SuccessAction) invocation.getArgument(1)).execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock).transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeIn(immutableMessage);

        semaphore.tryAcquire(1000, TimeUnit.MILLISECONDS);
        verify(messagingStubMock, times(2)).transmit(eq(immutableMessage),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
    }

    @Test
    public void testWebSocketClientParticipantIdsRetrievedOnlyOnce() throws Exception {
        createDefaultMessageRouter();
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000).getValue());
        joynrMessage.setTtlAbsolute(true);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final HashMap<Address, Set<String>> recipientSet = new HashMap<>();
        recipientSet.put(new Address(), Set.of(joynrMessage.getImmutableMessage().getRecipient()));
        doReturn(recipientSet).when(addressManager).getParticipantIdMap(immutableMessage);

        doThrow(new JoynrDelayMessageException(100, "test")).when(messagingStubMock)
                                                            .transmit(any(ImmutableMessage.class),
                                                                      any(SuccessAction.class),
                                                                      any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);
        Thread.sleep(500);

        verify(addressManager, times(1)).getParticipantIdMap(immutableMessage);
        verify(addressManager, atLeast(2)).getAddressForDelayableImmutableMessage(any());
        final ArgumentCaptor<DelayableImmutableMessage> passedDelayableMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue, atLeast(2)).put(passedDelayableMessage.capture());
        verify(messagingStubMock, atLeast(2)).transmit(eq(immutableMessage),
                                                       any(SuccessAction.class),
                                                       any(FailureAction.class));
        assertTrue(passedDelayableMessage.getAllValues().size() >= 2);
        Set<ImmutableMessage> passedImmutableMessages = passedDelayableMessage.getAllValues()
                                                                              .stream()
                                                                              .map(DelayableImmutableMessage::getMessage)
                                                                              .collect(Collectors.toSet());
        assertTrue(passedImmutableMessages.size() == 1);
        assertTrue(passedImmutableMessages.contains(immutableMessage));
    }

    @Test
    public void testRepeatedAddressResolutionForMulticast() throws Exception {
        createDefaultMessageRouter();
        final String multicastId = "multicast/id/test";
        joynrMessage = messageFactory.createMulticast(fromParticipantId,
                                                      new MulticastPublication(new ArrayList<>(), multicastId),
                                                      new MessagingQos());
        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(1000).getValue());
        joynrMessage.setTtlAbsolute(true);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final HashMap<Address, Set<String>> participantIdSet = new HashMap<>();
        participantIdSet.put(new Address(), Set.of(AddressManager.multicastAddressCalculatorParticipantId));
        doReturn(participantIdSet).when(addressManager).getParticipantIdMap(immutableMessage);
        doReturn(Optional.of(mqttAddress)).when(addressManager).getAddressForDelayableImmutableMessage(any());

        doThrow(new JoynrDelayMessageException(200, "test42")).when(messagingStubMock)
                                                              .transmit(any(ImmutableMessage.class),
                                                                        any(SuccessAction.class),
                                                                        any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);
        Thread.sleep(550);

        verify(addressManager, times(1)).getParticipantIdMap(immutableMessage);
        verify(addressManager, atLeast(2)).getAddressForDelayableImmutableMessage(any());
        final ArgumentCaptor<DelayableImmutableMessage> passedDelayableMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue, atLeast(2)).put(passedDelayableMessage.capture());
        assertTrue(passedDelayableMessage.getAllValues().size() >= 2);
        Set<ImmutableMessage> passedImmutableMessages = passedDelayableMessage.getAllValues()
                                                                              .stream()
                                                                              .map(DelayableImmutableMessage::getMessage)
                                                                              .collect(Collectors.toSet());
        assertTrue(passedImmutableMessages.size() == 1);
        assertTrue(passedImmutableMessages.contains(immutableMessage));
    }

    private void testOnlyOneParticipantIdResolution(final Address address) throws Exception {
        reset(messageQueue);
        final int ttlMs = 550;

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(ttlMs).getValue());
        joynrMessage.setTtlAbsolute(true);
        final ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        final HashMap<Address, Set<String>> participantIdSet = new HashMap<>();
        participantIdSet.put(new Address(), Set.of(joynrMessage.getImmutableMessage().getRecipient()));
        doReturn(participantIdSet).when(addressManager).getParticipantIdMap(immutableMessage);
        doReturn(Optional.of(address)).when(addressManager).getAddressForDelayableImmutableMessage(any());

        doThrow(new JoynrDelayMessageException(200, "test")).when(messagingStubMock)
                                                            .transmit(any(ImmutableMessage.class),
                                                                      any(SuccessAction.class),
                                                                      any(FailureAction.class));

        ccMessageRouter.routeIn(immutableMessage);
        Thread.sleep(ttlMs);

        verify(addressManager, times(1)).getParticipantIdMap(immutableMessage);
        verify(addressManager, atLeast(2)).getAddressForDelayableImmutableMessage(any());
        final ArgumentCaptor<DelayableImmutableMessage> passedDelayableMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue, atLeast(2)).put(passedDelayableMessage.capture());
        assertTrue("Size was " + passedDelayableMessage.getAllValues().size(),
                   passedDelayableMessage.getAllValues().size() >= 2);
        Set<ImmutableMessage> passedImmutableMessages = passedDelayableMessage.getAllValues()
                                                                              .stream()
                                                                              .map(DelayableImmutableMessage::getMessage)
                                                                              .collect(Collectors.toSet());
        assertTrue("Size was " + passedImmutableMessages.size(), passedImmutableMessages.size() == 1);
        assertTrue(passedImmutableMessages.contains(immutableMessage));
    }

    @Test
    public void testOnlyOneParticipantIdResolution() throws Exception {
        createDefaultMessageRouter();
        testOnlyOneParticipantIdResolution(mqttAddress);

        when(mqttMessagingStubFactoryMock.create(any(MqttAddress.class))).thenReturn(messagingStubMock);
        testOnlyOneParticipantIdResolution(new MqttAddress("brokerUri", "topic"));

        when(webSocketMessagingStubFactoryMock.create(any(WebSocketAddress.class))).thenReturn(messagingStubMock);
        testOnlyOneParticipantIdResolution(new WebSocketAddress(WebSocketProtocol.WS, "host", 42, "path"));

        when(websocketClientMessagingStubFactoryMock.create(any(WebSocketClientAddress.class))).thenReturn(messagingStubMock);
        testOnlyOneParticipantIdResolution(new WebSocketClientAddress());

        when(inProcessMessagingStubFactoryMock.create(any(InProcessAddress.class))).thenReturn(messagingStubMock);
        testOnlyOneParticipantIdResolution(new InProcessAddress(mock(InProcessMessagingSkeleton.class)));
    }

    private void testNotRoutableMessageIsDropped(final MutableMessage mutableMessage) throws Exception {
        final ImmutableMessage immutableMessage = mutableMessage.getImmutableMessage();

        final HashMap<Address, Set<String>> recipientSet = new HashMap<>();
        recipientSet.put(new Address(), Set.of(mutableMessage.getImmutableMessage().getRecipient()));
        doReturn(recipientSet).when(addressManager).getParticipantIdMap(immutableMessage);
        doReturn(Optional.empty()).when(addressManager).getAddressForDelayableImmutableMessage(any());

        CountDownLatch countDownLatch = new CountDownLatch(1);
        MessageProcessedListener mockMsgProcessedListener = new MessageProcessedListener() {
            @Override
            public void messageProcessed(String messageId) {
                if (messageId.equals(immutableMessage.getId())) {
                    countDownLatch.countDown();
                }
            }
        };
        ccMessageRouter.registerMessageProcessedListener(mockMsgProcessedListener);

        ccMessageRouter.routeOut(immutableMessage);
        assertTrue(countDownLatch.await(3000, TimeUnit.MILLISECONDS));
        verify(messageQueue, times(1)).put(any(DelayableImmutableMessage.class));
        verify(addressManager, times(1)).getParticipantIdMap(immutableMessage);
        verify(addressManager, times(1)).getAddressForDelayableImmutableMessage(any(DelayableImmutableMessage.class));
        verifyNoMoreInteractions(messagingStubMock);
    }

    @Test
    public void testNotRoutableReplyDropped() throws Exception {
        createDefaultMessageRouter();
        final String unknownParticipantId = "unknown_participant_id";
        final String requestReplyId = "some_request_reply_id";

        final Reply reply = new Reply(requestReplyId, new JoynrRuntimeException("TestException"));

        final MutableMessage mutableMessage = messageFactory.createReply(fromParticipantId,
                                                                         unknownParticipantId,
                                                                         reply,
                                                                         new MessagingQos());

        testNotRoutableMessageIsDropped(mutableMessage);
    }

    @Test
    public void testNotRoutableMulticastDropped() throws Exception {
        createDefaultMessageRouter();
        final MulticastPublication multicastPublication = new MulticastPublication(new JoynrRuntimeException("Test Exception"),
                                                                                   "multicastId");
        final MutableMessage mutableMessage = messageFactory.createMulticast(fromParticipantId,
                                                                             multicastPublication,
                                                                             new MessagingQos());

        testNotRoutableMessageIsDropped(mutableMessage);
    }

    @Test
    public void testMulticastMessageIsDroppedIfNoRecipientIsFound() throws Exception {
        createDefaultMessageRouter();
        final MulticastPublication multicastPublication = new MulticastPublication(new JoynrRuntimeException("Test Exception"),
                                                                                   "multicastId");
        final MutableMessage mutableMessage = messageFactory.createMulticast(fromParticipantId,
                                                                             multicastPublication,
                                                                             new MessagingQos());

        final ImmutableMessage immutableMessage = mutableMessage.getImmutableMessage();

        CountDownLatch countDownLatch = new CountDownLatch(1);
        MessageProcessedListener mockMsgProcessedListener = new MessageProcessedListener() {
            @Override
            public void messageProcessed(String messageId) {
                if (messageId.equals(immutableMessage.getId())) {
                    countDownLatch.countDown();
                }
            }
        };
        ccMessageRouter.registerMessageProcessedListener(mockMsgProcessedListener);

        ccMessageRouter.routeIn(immutableMessage);
        assertTrue(countDownLatch.await(3000, TimeUnit.MILLISECONDS));
        verify(messageQueue, times(0)).put(any(DelayableImmutableMessage.class));

        verify(addressManager).getParticipantIdMap(immutableMessage);
        verifyNoMoreInteractions(messagingStubMock);
    }

    @Test
    public void testPublicationMessageIsDroppedIfNoAddressIsFound() throws Exception {
        createDefaultMessageRouter();
        final String unknownParticipantId = "unknown_participant_id";
        final SubscriptionPublication subscriptionPublication = new SubscriptionPublication(new JoynrRuntimeException("Test Exception"),
                                                                                            "subscriptionId");
        final MutableMessage mutableMessage = messageFactory.createPublication(fromParticipantId,
                                                                               unknownParticipantId,
                                                                               subscriptionPublication,
                                                                               new MessagingQos());

        testNotRoutableMessageIsDropped(mutableMessage);
    }

    @Test
    public void testSubscriptionReplyMessageIsDroppedIfNoAddressIsFound() throws Exception {
        createDefaultMessageRouter();
        final String unknownParticipantId = "unknown_participant_id";
        final SubscriptionReply subscriptionReply = new SubscriptionReply("subscriptionId");
        final MutableMessage mutableMessage = messageFactory.createSubscriptionReply(fromParticipantId,
                                                                                     unknownParticipantId,
                                                                                     subscriptionReply,
                                                                                     new MessagingQos());

        testNotRoutableMessageIsDropped(mutableMessage);
    }

    @Test
    public void testShutdown() throws InterruptedException {
        createDefaultMessageRouter();
        verify(shutdownNotifier).registerForShutdown(ccMessageRouter);
    }

    @Test(timeout = 3000)
    public void testFailedTransmitDoesNotLeadToThreadStarvation() throws Exception {
        createDefaultMessageRouter();
        final int MESSAGE_LOAD = 10;

        final String recipient = "to";
        final String requestReplyId1 = "requestReplyId";
        Map<String, String> customHeader1 = new HashMap<>();
        customHeader1.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId1);

        ImmutableMessage failingMessage = mock(ImmutableMessage.class);
        when(failingMessage.isTtlAbsolute()).thenReturn(true);
        when(failingMessage.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000L).getValue());
        when(failingMessage.getRecipient()).thenReturn(recipient);
        when(failingMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY);
        when(failingMessage.getId()).thenReturn(requestReplyId1);
        when(failingMessage.getCustomHeaders()).thenReturn(customHeader1);

        final HashMap<Address, Set<String>> participantIdSet = new HashMap<>();
        participantIdSet.put(new Address(), Set.of(recipient));

        doReturn(participantIdSet).when(addressManager).getParticipantIdMap(failingMessage);
        doReturn(Optional.of(mqttAddress)).when(addressManager).getAddressForDelayableImmutableMessage(any());

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
            ccMessageRouter.routeOut(failingMessage);
        }

        Thread.sleep(2000);
        verify(messagingStubMock, atLeast(MESSAGE_LOAD * 3)).transmit(eq(failingMessage),
                                                                      any(SuccessAction.class),
                                                                      any(FailureAction.class));

        final String requestReplyId2 = "requestReplyId";
        Map<String, String> customHeader2 = new HashMap<>();
        customHeader1.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId2);

        ImmutableMessage anotherMessage = mock(ImmutableMessage.class);
        when(anotherMessage.isTtlAbsolute()).thenReturn(true);
        when(anotherMessage.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000L).getValue());
        when(anotherMessage.getRecipient()).thenReturn("to");
        when(anotherMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY);
        when(anotherMessage.getId()).thenReturn(requestReplyId2);
        when(anotherMessage.getCustomHeaders()).thenReturn(customHeader2);
        doReturn(participantIdSet).when(addressManager).getParticipantIdMap(anotherMessage);

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

        ccMessageRouter.routeOut(anotherMessage);
        assertTrue(semaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void messageWorkerUsesSameFailureActionForStubAndThrownException() throws Exception {
        createDefaultMessageRouter();
        // route multicast message to two recipients
        // call failureAction in first stub call
        // throw exception in second stub call
        // make sure that the message is rescheduled only once
        // (multiple executions of the same failure action reschedule only in the first call, further calls are just logged)
        MqttAddress receiverAddress1 = new MqttAddress("what", "ever1");
        MqttAddress receiverAddress2 = new MqttAddress("what", "ever2");
        prepareMulticastForMultipleAddresses(receiverAddress1, receiverAddress2);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        IMessagingStub messagingStubMock1 = mock(IMessagingStub.class);
        IMessagingStub messagingStubMock2 = mock(IMessagingStub.class);
        reset(mqttMessagingStubFactoryMock);
        doReturn(messagingStubMock1).when(mqttMessagingStubFactoryMock).create(receiverAddress1);
        doReturn(messagingStubMock2).when(mqttMessagingStubFactoryMock).create(receiverAddress2);

        Answer<Void> stubAnswer = new Answer<Void>() {
            private volatile int callCount = 0;

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                callCount++;
                FailureAction failureAction = invocation.getArgument(2);
                switch (callCount) {
                case 1:
                    failureAction.execute(new JoynrDelayMessageException(32, "first stub call, failureAction"));
                    break;
                case 2:
                    throw new JoynrDelayMessageException(32, "first stub call, thrown exception");
                case 3:
                    // second stub call of stub 1
                    break;
                case 4:
                    // second stub call of stub 2
                    break;
                default:
                    fail("expected no more calls");
                }
                return null;
            }
        };
        doAnswer(stubAnswer).when(messagingStubMock1)
                            .transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));
        doAnswer(stubAnswer).when(messagingStubMock2)
                            .transmit(eq(immutableMessage), any(SuccessAction.class), any(FailureAction.class));

        ccMessageRouter.routeOut(immutableMessage);

        Thread.sleep(1000);

        verify(messagingStubMock1, times(2)).transmit(eq(immutableMessage),
                                                      any(SuccessAction.class),
                                                      any(FailureAction.class));
        verify(messagingStubMock2, times(2)).transmit(eq(immutableMessage),
                                                      any(SuccessAction.class),
                                                      any(FailureAction.class));
        verify(mqttMessagingStubFactoryMock, times(2)).create(receiverAddress1);
        verify(mqttMessagingStubFactoryMock, times(2)).create(receiverAddress2);
    }

    private void createCcMessageRouterWithEnabledAccessControl() {
        // Reconfigure testModule to enable access control
        // return messageRouter with an enabled access control
        Module testTryCatchModule = Modules.override(testModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bindConstant().annotatedWith(Names.named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE))
                              .to(true);
            }
        });
        injector = Guice.createInjector(testTryCatchModule);
        ccMessageRouter = (CcMessageRouter) injector.getInstance(MessageRouter.class);
    }

    @Test
    public void testExpiredMessageScheduledWhenHasConsumerPermissionTrue() throws Exception {
        // pre-conditions: access control is enabled and permission is granted
        // Expected behaviour: The message is enqueued in the MessageQueue
        createCcMessageRouterWithEnabledAccessControl();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                HasConsumerPermissionCallback callback = invocation.getArgument(1);
                callback.hasConsumerPermission(true);
                return null;
            }
        }).when(accessControllerMock).hasConsumerPermission(any(ImmutableMessage.class),
                                                            any(HasConsumerPermissionCallback.class));

        joynrMessage.setTtlMs(ExpiryDate.fromRelativeTtl(0).getValue());
        joynrMessage.setTtlAbsolute(true);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        ccMessageRouter.routeIn(immutableMessage);

        verify(accessControllerMock, times(1)).hasConsumerPermission(eq(immutableMessage),
                                                                     any(HasConsumerPermissionCallback.class));
        ArgumentCaptor<DelayableImmutableMessage> passedDelaybleMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue).put(passedDelaybleMessage.capture());
        assertEquals(immutableMessage, passedDelaybleMessage.getAllValues().get(0).getMessage());
    }

    @Test
    public void testMessageScheduledWhenHasConsumerPermissionTrueWithRelativeTtl() throws Exception {
        // pre-conditions: access control is enabled and permission is granted
        // Expected behaviour: The message is enqueued in the MessageQueue
        createCcMessageRouterWithEnabledAccessControl();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                HasConsumerPermissionCallback callback = invocation.getArgument(1);
                callback.hasConsumerPermission(true);
                return null;
            }
        }).when(accessControllerMock).hasConsumerPermission(any(ImmutableMessage.class),
                                                            any(HasConsumerPermissionCallback.class));

        joynrMessage.setTtlAbsolute(false);
        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        ccMessageRouter.routeOut(immutableMessage);

        verify(accessControllerMock, times(1)).hasConsumerPermission(eq(immutableMessage),
                                                                     any(HasConsumerPermissionCallback.class));
        ArgumentCaptor<DelayableImmutableMessage> passedDelaybleMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue).put(passedDelaybleMessage.capture());
        assertEquals(immutableMessage, passedDelaybleMessage.getAllValues().get(0).getMessage());
    }

    @Test
    public void testMessageProcessedCalledWhenHasConsumerPermissionFalse() throws Exception {
        // Test whether try_catch inside "hasConsumerPermission" callback in CcMessageRouter.route works as expected
        // pre-conditions: access control is enabled and permission is denied
        // Expected behaviour: The method messageProcessed is called only once when hasPermission = false
        // and the message will be dropped.
        createCcMessageRouterWithEnabledAccessControl();

        final MessageProcessedListener mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                HasConsumerPermissionCallback callback = invocation.getArgument(1);
                callback.hasConsumerPermission(false);
                return null;
            }
        }).when(accessControllerMock).hasConsumerPermission(any(ImmutableMessage.class),
                                                            any(HasConsumerPermissionCallback.class));

        ImmutableMessage immutableMessage = joynrMessage.getImmutableMessage();

        ccMessageRouter.routeOut(immutableMessage);

        verify(accessControllerMock, times(1)).hasConsumerPermission(eq(immutableMessage),
                                                                     any(HasConsumerPermissionCallback.class));
        verify(mockMessageProcessedListener, times(1)).messageProcessed(eq(immutableMessage.getId()));
        verify(routingTable).remove(eq(immutableMessage.getSender()));
    }

    @Test(expected = JoynrRuntimeException.class)
    public void addNextHopThrows() {
        createDefaultMessageRouter();
        final boolean isGloballyVisible = true;
        doReturn(false).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong(), anyBoolean());
        ccMessageRouter.addNextHop(fromParticipantId, mqttAddress, isGloballyVisible);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void removeMulticastReceiver_throwsWhenAddressIsUnknown() {
        createDefaultMessageRouter();
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.containsKey(providerParticipantId)).thenReturn(false);
        ccMessageRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void addMulticastReceiver_throwsWhenAddressIsUnknown() {
        createDefaultMessageRouter();
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.containsKey(providerParticipantId)).thenReturn(false);

        ccMessageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
    }

    @Test
    public void removeMulticastReceiver() {
        createDefaultMessageRouter();
        WebSocketClientAddress mockWebSocketAddress = mock(WebSocketClientAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);
        when(routingTable.containsKey(providerParticipantId)).thenReturn(true);
        ccMessageRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messagingSkeletonFactoryMock, times(1)).getSkeleton(mockWebSocketAddress);
        verify(multicastReceiverRegistry, times(1)).unregisterMulticastReceiver(multicastId, subscriberParticipantId);
    }

    @Test
    public void addMulticastReceiver() {
        createDefaultMessageRouter();
        WebSocketClientAddress mockWebSocketAddress = mock(WebSocketClientAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);
        when(routingTable.containsKey(providerParticipantId)).thenReturn(true);

        ccMessageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messagingSkeletonFactoryMock, times(1)).getSkeleton(mockWebSocketAddress);
        verify(multicastReceiverRegistry, times(1)).registerMulticastReceiver(multicastId, subscriberParticipantId);
    }

    @Test
    public void testShutdownCancelWorkerFuturesWhenTimeoutOccurs() throws NoSuchFieldException, IllegalAccessException {
        createDefaultMessageRouter();

        replaceWorker(new TestMessageWorker(2000L));

        ccMessageRouter.shutdown();

        final List<Future<?>> futures = getWorkerFutures();
        assertNotNull(futures);
        assertEquals(1, futures.size());
        futures.forEach(future -> assertTrue(future.isCancelled()));
    }

    @Test
    public void testShutdownDoesNotCancelWorkerFuturesWhenNoTimeoutOccurs() throws NoSuchFieldException,
                                                                            IllegalAccessException {
        createDefaultMessageRouter();

        replaceWorker(new TestMessageWorker(100L));

        ccMessageRouter.shutdown();

        final List<Future<?>> futures = getWorkerFutures();
        assertNotNull(futures);
        assertEquals(1, futures.size());
        futures.forEach(future -> assertFalse(future.isCancelled()));
    }

    private void replaceWorker(final CcMessageWorkable workable) throws NoSuchFieldException, IllegalAccessException {
        final List<Runnable> workers = getWorkers();
        final List<Future<?>> futures = getWorkerFutures();
        final ScheduledExecutorService executor = getExecutor();

        futures.forEach(future -> future.cancel(true));
        workers.clear();
        futures.clear();
        workers.add(workable);
        futures.add(executor.schedule(workable, 0, TimeUnit.MILLISECONDS));
    }

    private ScheduledExecutorService getExecutor() throws NoSuchFieldException, IllegalAccessException {
        final Field field = CcMessageRouter.class.getDeclaredField("scheduler");
        field.setAccessible(true);
        return (ScheduledExecutorService) field.get(ccMessageRouter);
    }

    private List<Runnable> getWorkers() throws NoSuchFieldException, IllegalAccessException {
        final Field field = CcMessageRouter.class.getDeclaredField("messageWorkers");
        field.setAccessible(true);
        return (List<Runnable>) field.get(ccMessageRouter);
    }

    private List<Future<?>> getWorkerFutures() throws NoSuchFieldException, IllegalAccessException {
        final Field field = CcMessageRouter.class.getDeclaredField("messageWorkerFutures");
        field.setAccessible(true);
        return (List<Future<?>>) field.get(ccMessageRouter);
    }

    class TestMessageWorker implements CcMessageWorkable {
        private Logger logger = LoggerFactory.getLogger(TestMessageWorker.class);
        private CountDownLatch countDownLatch;
        private volatile boolean stopped;
        private long sleepAfterStop;
        private ScheduledExecutorService executor;

        public TestMessageWorker(final long sleepAfterStop) {
            this.stopped = false;
            this.sleepAfterStop = sleepAfterStop;
            this.executor = Executors.newSingleThreadScheduledExecutor();
        }

        @Override
        public void stopWorker(final CountDownLatch countDownLatch) {
            this.countDownLatch = countDownLatch;
            this.stopped = true;
        }

        @Override
        public void run() {
            try {
                while (!stopped) {
                    sleep(100L);
                }
                sleep(sleepAfterStop);
            } catch (final InterruptedException | ExecutionException e) {
                logger.error("Unexpected exception occurred: " + e.getMessage());
            } finally {
                countDownLatch.countDown();
            }
        }

        private void sleep(final long millis) throws InterruptedException, ExecutionException {
            executor.schedule(() -> millis, millis, TimeUnit.MILLISECONDS).get();
        }
    }
}
