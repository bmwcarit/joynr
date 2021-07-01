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
package io.joynr.messaging;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyBoolean;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Optional;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.common.ExpiryDate;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.persistence.MessagePersister;
import io.joynr.messaging.routing.AddressManager;
import io.joynr.messaging.routing.DelayableImmutableMessage;
import io.joynr.messaging.routing.LibJoynrMessageRouter;
import io.joynr.messaging.routing.MessageQueue;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.MulticastReceiverRegistry;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.JoynrThreadFactory;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.UdsClientAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

@RunWith(MockitoJUnitRunner.class)
public class LibJoynrMessageRouterTest {

    @Mock
    IMessagingStub messagingStub;
    @Mock
    private RoutingTable routingTable;
    @Mock
    private RoutingProxy messageRouterParent;
    @Mock
    private RoutingProxy messageRouterParentUdsAddress;
    @Mock
    private MqttAddress parentAddress;
    @Mock
    private MqttAddress nextHopAddress;
    @Mock
    private WebSocketClientAddress incomingAddress;
    @Mock
    private UdsClientAddress incomingUdsClientAddress;
    @Mock
    private MessagingStubFactory messagingStubFactory;
    @Mock
    private MessagingSkeletonFactory messagingSkeletonFactory;
    @Mock
    private AddressManager addressManager;
    @Mock
    private MulticastReceiverRegistry multicastReceiverRegistry;
    @Mock
    private ImmutableMessage message;
    @Mock
    private ShutdownNotifier shutdownNotifier;
    @Mock
    private MessagePersister messagePersisterMock;
    @Mock
    RoutingTable routingTableMock;

    private MessageQueue messageQueue;
    private LibJoynrMessageRouter messageRouter;
    private LibJoynrMessageRouter messageRouterForUdsAddresses;
    private String unknownParticipantId = "unknownParticipantId";
    private String unknownSenderParticipantId = "unknownSenderParticipantId";
    private Long sendMsgRetryIntervalMs = 10L;
    private int maxParallelSends = 10;
    private long routingTableCleanupIntervalMs = 60000L;

    private String globalAddress = "global-address";

    @Before
    public void setUp() {
        when(message.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000000).getValue());
        when(message.isTtlAbsolute()).thenReturn(true);
        when(message.getRecipient()).thenReturn(unknownParticipantId);
        when(message.getSender()).thenReturn(unknownSenderParticipantId);
        when(message.isLocalMessage()).thenReturn(false);
        when(message.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);

        when(routingTable.containsKey(unknownParticipantId)).thenReturn(false);
        when(messageRouterParent.resolveNextHop(unknownParticipantId)).thenReturn(true);
        when(messageRouterParent.getReplyToAddress()).thenReturn(globalAddress);
        when(messagingStubFactory.create(any(Address.class))).thenReturn(messagingStub);
        when(parentAddress.getTopic()).thenReturn("LibJoynrMessageRouterTestChannel");
        when(messagingSkeletonFactory.getSkeleton(any(Address.class))).thenReturn(Optional.empty());

        messageQueue = new MessageQueue(new DelayQueue<DelayableImmutableMessage>(),
                                        new MessageQueue.MaxTimeoutHolder(),
                                        createUuidString(),
                                        messagePersisterMock,
                                        routingTableMock);

        messageRouter = new LibJoynrMessageRouter(routingTable,
                                                  incomingAddress,
                                                  provideMessageSchedulerThreadPoolExecutor(),
                                                  sendMsgRetryIntervalMs,
                                                  maxParallelSends,
                                                  routingTableCleanupIntervalMs,
                                                  messagingStubFactory,
                                                  messagingSkeletonFactory,
                                                  addressManager,
                                                  multicastReceiverRegistry,
                                                  messageQueue,
                                                  shutdownNotifier);
        messageRouterForUdsAddresses = new LibJoynrMessageRouter(routingTable,
                                                                 incomingUdsClientAddress,
                                                                 provideMessageSchedulerThreadPoolExecutor(),
                                                                 sendMsgRetryIntervalMs,
                                                                 maxParallelSends,
                                                                 routingTableCleanupIntervalMs,
                                                                 messagingStubFactory,
                                                                 messagingSkeletonFactory,
                                                                 addressManager,
                                                                 multicastReceiverRegistry,
                                                                 messageQueue,
                                                                 shutdownNotifier);
        messageRouter.setParentRouter(messageRouterParent, parentAddress, "parentParticipantId", "proxyParticipantId");
    }

    @Test
    public void itQueriesParentForNextHop() throws Exception {
        messageRouter.route(message);
        Thread.sleep(100);
        verify(messageRouterParent).resolveNextHop(eq(unknownParticipantId));
    }

    @Test
    public void preventRoutingBackToCC() throws Exception {
        when(message.isReceivedFromGlobal()).thenReturn(true);
        message.setReceivedFromGlobal(true);
        messageRouter.route(message);
        Thread.sleep(100);
        verify(messageRouterParent, never()).resolveNextHop(any());
    }

    @Test
    public void addsNextHopAfterQueryingParent() throws Exception {
        messageRouter.route(message);
        Thread.sleep(100);
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        verify(routingTable).put(eq(unknownParticipantId), eq(parentAddress), eq(isGloballyVisible), eq(expiryDateMs));
    }

    @Test
    public void passesNextHopToParent() {
        final boolean isGloballyVisible = true;
        messageRouter.addNextHop(unknownParticipantId, nextHopAddress, isGloballyVisible);
        verify(messageRouterParent).addNextHop(eq(unknownParticipantId), eq(incomingAddress), eq(isGloballyVisible));
    }

    @Test(expected = ProviderRuntimeException.class)
    public void setParentRouter_UdsClientAddress_throws() {
        // throws because UdsClientAddress is not supported in Java
        messageRouterForUdsAddresses.setParentRouter(messageRouterParentUdsAddress,
                                                     parentAddress,
                                                     "anotherParentParticipantId",
                                                     "anotherProxyParticipantId");
    }

    @Test
    public void passesAddNextHopToParent_UdsClientAddress() {
        // parent router is not called because UdsClientAddress is not supported
        final boolean isGloballyVisible = true;
        messageRouterForUdsAddresses.addNextHop(unknownParticipantId, nextHopAddress, isGloballyVisible);
        verify(messageRouterParentUdsAddress, times(0)).addNextHop(anyString(),
                                                                   any(UdsClientAddress.class),
                                                                   anyBoolean());
    }

    ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor() {
        ThreadFactory schedulerNamedThreadFactory = new JoynrThreadFactory("joynr.MessageScheduler-scheduler");
        ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(2, schedulerNamedThreadFactory);
        scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
        scheduler.allowCoreThreadTimeOut(true);
        return scheduler;
    }

    @Test
    public void addMulticastReceiverForInProcessProvider() {
        InProcessAddress mockInProcessAddress = mock(InProcessAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockInProcessAddress);
        when(routingTable.containsKey(providerParticipantId)).thenReturn(true);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        // we don't expect this to be called
        verify(messageRouterParent, times(0)).addMulticastReceiver(any(String.class),
                                                                   any(String.class),
                                                                   any(String.class));
    }

    @Test
    public void addMulticastReceiverForNotInProcessProvider() {
        WebSocketAddress mockWebSocketAddress = mock(WebSocketAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);
        when(routingTable.containsKey(providerParticipantId)).thenReturn(true);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messageRouterParent).addMulticastReceiver(eq(multicastId),
                                                         eq(subscriberParticipantId),
                                                         eq(providerParticipantId));
    }

    @Test
    public void removeMulticastReceiverForNotInProcessProvider() {
        WebSocketAddress mockWebSocketAddress = mock(WebSocketAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);
        when(routingTable.containsKey(providerParticipantId)).thenReturn(true);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        messageRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messageRouterParent).removeMulticastReceiver(eq(multicastId),
                                                            eq(subscriberParticipantId),
                                                            eq(providerParticipantId));
    }

    @Test
    public void setToKnownAddsCcAddressToRoutingTable() {
        final String participantId = "setToKnownParticipantId";
        messageRouter.setToKnown(participantId);
        verify(routingTable).put(participantId, parentAddress, false, Long.MAX_VALUE);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void addMulticastReceiver_throwsWhenAddressIsUnknown() {
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.containsKey(providerParticipantId)).thenReturn(false);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messagingSkeletonFactory, times(0)).getSkeleton(any());
    }

    @Test
    public void addMulticastReceiver_callsMessagingSkeletonFactoryWhenAddressIsKnown() {
        WebSocketAddress mockWebSocketAddress = mock(WebSocketAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);
        when(routingTable.containsKey(providerParticipantId)).thenReturn(true);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messagingSkeletonFactory, times(1)).getSkeleton(mockWebSocketAddress);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void removeMulticastRecevier_throwsWhenAddressIsUnknown() {
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.containsKey(providerParticipantId)).thenReturn(false);

        messageRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(multicastReceiverRegistry, times(1)).unregisterMulticastReceiver(multicastId, subscriberParticipantId);
        verify(messagingSkeletonFactory, times(0)).getSkeleton(any());
    }

    @Test
    public void removeMulticastReceveiver_callsMessagingSkeletonFactoryWhenAddressIsKnown() {
        WebSocketAddress mockWebSocketAddress = mock(WebSocketAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);
        when(routingTable.containsKey(providerParticipantId)).thenReturn(true);

        messageRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(multicastReceiverRegistry, times(1)).unregisterMulticastReceiver(multicastId, subscriberParticipantId);
        verify(messagingSkeletonFactory, times(1)).getSkeleton(any());
    }
}
