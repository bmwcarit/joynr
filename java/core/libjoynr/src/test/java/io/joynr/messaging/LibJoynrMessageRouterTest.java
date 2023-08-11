/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2023 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.AddressManager;
import io.joynr.messaging.routing.DelayableImmutableMessage;
import io.joynr.messaging.routing.LibJoynrMessageRouter;
import io.joynr.messaging.routing.MessageQueue;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.tracking.MessageTrackerForGracefulShutdown;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.JoynrThreadFactory;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.Request;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.UdsClientAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

@RunWith(MockitoJUnitRunner.class)
public class LibJoynrMessageRouterTest {
    // TODO test queue for removeNextHop

    @Mock
    IMessagingStub messagingStub;
    @Mock
    private RoutingProxy messageRouterParent;
    @Mock
    private RoutingProxy deferredMessageRouterParent;
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
    private ShutdownNotifier shutdownNotifier;
    @Mock
    RoutingTable routingTableMock;
    @Mock
    private Dispatcher dispatcherMock;
    private MessageQueue messageQueue;
    @Mock
    private MessageTrackerForGracefulShutdown messageTrackerMock;
    private LibJoynrMessageRouter messageRouter;
    private LibJoynrMessageRouter messageRouterForUdsAddresses;
    private String unknownParticipantId = "unknownParticipantId";
    private int maxParallelSends = 2;

    private MutableMessageFactory messageFactory;
    private MutableMessage joynrMessage;

    private String globalAddress = "global-address";
    private String toParticipantId = "toParticipantId";
    private String fromParticipantId = "fromParticipantId";

    @Before
    public void setUp() {
        messageQueue = spy(new MessageQueue(new DelayQueue<DelayableImmutableMessage>()));
        lenient().when(messageRouterParent.getReplyToAddress()).thenReturn(globalAddress);
        when(messagingStubFactory.create(any(Address.class))).thenReturn(messagingStub);
        lenient().when(parentAddress.getTopic()).thenReturn("LibJoynrMessageRouterTestChannel");
        lenient().when(messagingSkeletonFactory.getSkeleton(any(Address.class))).thenReturn(Optional.empty());

        messageRouter = new LibJoynrMessageRouter(incomingAddress,
                                                  provideMessageSchedulerThreadPoolExecutor(),
                                                  maxParallelSends,
                                                  messagingStubFactory,
                                                  messageQueue,
                                                  shutdownNotifier,
                                                  dispatcherMock,
                                                  messageTrackerMock);
        messageRouterForUdsAddresses = new LibJoynrMessageRouter(incomingUdsClientAddress,
                                                                 provideMessageSchedulerThreadPoolExecutor(),
                                                                 maxParallelSends,
                                                                 messagingStubFactory,
                                                                 messageQueue,
                                                                 shutdownNotifier,
                                                                 dispatcherMock,
                                                                 messageTrackerMock);
        messageRouter.setParentRouter(messageRouterParent, parentAddress, "parentParticipantId", "proxyParticipantId");
        ObjectMapper objectMapper = new ObjectMapper();
        messageFactory = new MutableMessageFactory(objectMapper, new HashSet<JoynrMessageProcessor>());
        Request request = new Request("noMethod", new Object[]{}, new String[]{}, "requestReplyId");
        joynrMessage = messageFactory.createRequest(fromParticipantId, toParticipantId, request, new MessagingQos());
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
    public void testAlwaysAtLeastTwoMessageWorkersAvailable() throws NoSuchFieldException, SecurityException,
                                                              IllegalArgumentException, IllegalAccessException {
        int localMaxParallelSends = 1;
        LibJoynrMessageRouter localMessageRouter = new LibJoynrMessageRouter(incomingAddress,
                                                                             provideMessageSchedulerThreadPoolExecutor(),
                                                                             localMaxParallelSends,
                                                                             messagingStubFactory,
                                                                             messageQueue,
                                                                             shutdownNotifier,
                                                                             dispatcherMock,
                                                                             messageTrackerMock);
        Field messageWorkerField = LibJoynrMessageRouter.class.getDeclaredField("messageWorkers");
        messageWorkerField.setAccessible(true);
        assertTrue(((List) messageWorkerField.get(localMessageRouter)).size() >= 2);
    }

    @Test
    public void passesNextHopToParent() {
        final boolean isGloballyVisible = true;
        messageRouter.addNextHop(unknownParticipantId, nextHopAddress, isGloballyVisible);
        verify(messageRouterParent).addNextHop(eq(unknownParticipantId), eq(incomingAddress), eq(isGloballyVisible));
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
    public void testAddMulticastReceiver() {
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messageRouterParent).addMulticastReceiver(eq(multicastId),
                                                         eq(subscriberParticipantId),
                                                         eq(providerParticipantId));
    }

    @Test
    public void testRemoveMulticastReceiver() {
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";

        messageRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messageRouterParent).removeMulticastReceiver(eq(multicastId),
                                                            eq(subscriberParticipantId),
                                                            eq(providerParticipantId));
    }

    @Test
    public void routeInEnqueuesNonExpiredMessage() throws Exception {
        ImmutableMessage immutableMessage;
        immutableMessage = joynrMessage.getImmutableMessage();
        // incoming messages must have been received from cluster controller and have receivedFromGlobal set to true
        immutableMessage.setReceivedFromGlobal(true);
        messageRouter.routeIn(immutableMessage);
        ArgumentCaptor<DelayableImmutableMessage> messageCaptor = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue).put(messageCaptor.capture());
        ImmutableMessage capturedMessage = messageCaptor.getValue().getMessage();
        assertEquals(immutableMessage.getSender(), capturedMessage.getSender());
        assertEquals(immutableMessage.getRecipient(), capturedMessage.getRecipient());
        assertEquals(immutableMessage.getReplyTo(), capturedMessage.getReplyTo());
        verify(dispatcherMock, timeout(1000)).messageArrived(immutableMessage);
        verify(messagingStub, never()).transmit(any(), any(), any());
    }

    @Test
    public void routeOutEnqueuesNonExpiredMessage() throws Exception {
        ImmutableMessage immutableMessage;
        immutableMessage = joynrMessage.getImmutableMessage();
        // outgoing messages were not been received from cluster controller and thus have receivedFromGlobal set to false
        immutableMessage.setReceivedFromGlobal(false);
        messageRouter.routeOut(immutableMessage);
        ArgumentCaptor<DelayableImmutableMessage> messageCaptor = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue).put(messageCaptor.capture());
        ImmutableMessage capturedMessage = messageCaptor.getValue().getMessage();
        assertEquals(immutableMessage.getSender(), capturedMessage.getSender());
        assertEquals(immutableMessage.getRecipient(), capturedMessage.getRecipient());
        assertEquals(immutableMessage.getReplyTo(), capturedMessage.getReplyTo());
        verify(messagingStubFactory, timeout(1000)).create(parentAddress);
        verify(messagingStub, timeout(1000)).transmit(eq(immutableMessage), any(), any());
        verify(dispatcherMock, never()).messageArrived(any());
    }

    @Test
    public void queuedParentHopCallsHappenAfterParentRouterSet() {
        LibJoynrMessageRouter deferredMessageRouter = new LibJoynrMessageRouter(incomingAddress,
                                                                                provideMessageSchedulerThreadPoolExecutor(),
                                                                                maxParallelSends,
                                                                                messagingStubFactory,
                                                                                messageQueue,
                                                                                shutdownNotifier,
                                                                                dispatcherMock,
                                                                                messageTrackerMock);
        String routingProxyParticipantId = "proxyParticipantId";
        String[] participantIdsAdd = new String[]{ "participant0", "participant1", "participant2", "participant3" };
        String[] participantIdsRemove = new String[]{ "particpantIdRemoveOnly", "participant1", "participant3",
                "participant2" };
        deferredMessageRouter.addNextHop(participantIdsAdd[0], new WebSocketAddress(), true);
        deferredMessageRouter.addNextHop(participantIdsAdd[1], new InProcessAddress(), false);
        deferredMessageRouter.removeNextHop(participantIdsRemove[0]);
        deferredMessageRouter.addNextHop(participantIdsAdd[2], new BinderAddress(), true);
        deferredMessageRouter.removeNextHop(participantIdsRemove[1]);
        deferredMessageRouter.addNextHop(participantIdsAdd[3], new WebSocketClientAddress(), true);
        deferredMessageRouter.removeNextHop(participantIdsRemove[2]);
        deferredMessageRouter.removeNextHop(participantIdsRemove[3]);
        verifyNoInteractions(deferredMessageRouterParent);
        deferredMessageRouter.setParentRouter(deferredMessageRouterParent,
                                              parentAddress,
                                              "parentParticipantId",
                                              routingProxyParticipantId);
        InOrder inOrder = inOrder(deferredMessageRouterParent);
        inOrder.verify(deferredMessageRouterParent)
               .addNextHop(eq(routingProxyParticipantId), eq(incomingAddress), eq(false));
        inOrder.verify(deferredMessageRouterParent).addNextHop(eq(participantIdsAdd[0]), eq(incomingAddress), eq(true));
        inOrder.verify(deferredMessageRouterParent)
               .addNextHop(eq(participantIdsAdd[1]), eq(incomingAddress), eq(false));
        inOrder.verify(deferredMessageRouterParent).removeNextHop(eq(participantIdsRemove[0]));
        inOrder.verify(deferredMessageRouterParent).addNextHop(eq(participantIdsAdd[2]), eq(incomingAddress), eq(true));
        inOrder.verify(deferredMessageRouterParent).removeNextHop(eq(participantIdsRemove[1]));
        inOrder.verify(deferredMessageRouterParent).addNextHop(eq(participantIdsAdd[3]), eq(incomingAddress), eq(true));
        inOrder.verify(deferredMessageRouterParent).removeNextHop(eq(participantIdsRemove[2]));
        inOrder.verify(deferredMessageRouterParent).removeNextHop(eq(participantIdsRemove[3]));
        verifyNoMoreInteractions(deferredMessageRouterParent);
    }

    @Test
    public void queuedMulticastReceiversRegisteredAfterParentRouterSet() {
        String routingProxyParticipantId = "proxyParticipantId";
        LibJoynrMessageRouter deferredMessageRouter = new LibJoynrMessageRouter(incomingAddress,
                                                                                provideMessageSchedulerThreadPoolExecutor(),
                                                                                maxParallelSends,
                                                                                messagingStubFactory,
                                                                                messageQueue,
                                                                                shutdownNotifier,
                                                                                dispatcherMock,
                                                                                messageTrackerMock);
        String[] multicastIds = new String[]{ "multicastId1", "multicastId2", "multicastId3" };
        String[] subscriberParticipantIds = new String[]{ "subscriberParticipantId1", "subscriberParticipantId2",
                "subscriberParticipantId3" };
        String[] providerParticipantIds = new String[]{ "providerParticipantId1", "providerParticipantId2",
                "providerParticipantId3" };
        deferredMessageRouter.addMulticastReceiver("multicastId0",
                                                   "subscriberParticipantId0",
                                                   "providerParticipantId0");
        deferredMessageRouter.addMulticastReceiver(multicastIds[0],
                                                   subscriberParticipantIds[0],
                                                   providerParticipantIds[0]);
        deferredMessageRouter.addMulticastReceiver(multicastIds[1],
                                                   subscriberParticipantIds[1],
                                                   providerParticipantIds[1]);
        deferredMessageRouter.removeMulticastReceiver("multicastId0",
                                                      "subscriberParticipantId0",
                                                      "providerParticipantId0");
        deferredMessageRouter.addMulticastReceiver(multicastIds[2],
                                                   subscriberParticipantIds[2],
                                                   providerParticipantIds[2]);
        verifyNoInteractions(deferredMessageRouterParent);
        deferredMessageRouter.setParentRouter(deferredMessageRouterParent,
                                              parentAddress,
                                              "parentParticipantId",
                                              routingProxyParticipantId);
        verify(deferredMessageRouterParent).addNextHop(eq(routingProxyParticipantId), eq(incomingAddress), eq(false));
        verify(deferredMessageRouterParent).addMulticastReceiver(eq(multicastIds[0]),
                                                                 eq(subscriberParticipantIds[0]),
                                                                 eq(providerParticipantIds[0]));
        verify(deferredMessageRouterParent).addMulticastReceiver(eq(multicastIds[1]),
                                                                 eq(subscriberParticipantIds[1]),
                                                                 eq(providerParticipantIds[1]));
        verify(deferredMessageRouterParent).addMulticastReceiver(eq(multicastIds[2]),
                                                                 eq(subscriberParticipantIds[2]),
                                                                 eq(providerParticipantIds[2]));
        verifyNoMoreInteractions(deferredMessageRouterParent);
        deferredMessageRouter.removeMulticastReceiver(multicastIds[2],
                                                      subscriberParticipantIds[2],
                                                      providerParticipantIds[2]);
        verify(deferredMessageRouterParent).removeMulticastReceiver(eq(multicastIds[2]),
                                                                    eq(subscriberParticipantIds[2]),
                                                                    eq(providerParticipantIds[2]));
    }

    @Test
    public void testShutdown() throws InterruptedException {
        verify(shutdownNotifier).registerForShutdown(messageRouter);
    }

}
