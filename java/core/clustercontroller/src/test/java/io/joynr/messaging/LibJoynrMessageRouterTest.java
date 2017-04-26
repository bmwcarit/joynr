package io.joynr.messaging;

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

import static org.mockito.Mockito.when;
import static org.mockito.Mockito.any;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import io.joynr.messaging.routing.AddressManager;
import io.joynr.messaging.routing.MulticastReceiverRegistry;
import static org.junit.Assert.assertEquals;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.common.util.concurrent.ThreadFactoryBuilder;

import io.joynr.common.ExpiryDate;

import io.joynr.messaging.routing.LibJoynrMessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.RoutingTable;
import joynr.JoynrMessage;
import joynr.system.RoutingProxy;
import joynr.system.RoutingSync.ResolveNextHopReturned;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketAddress;

@RunWith(MockitoJUnitRunner.class)
public class LibJoynrMessageRouterTest {

    @Mock
    IMessaging messagingStub;
    @Mock
    private RoutingTable routingTable;
    @Mock
    private RoutingProxy messageRouterParent;
    @Mock
    private ChannelAddress parentAddress;
    @Mock
    private ChannelAddress nextHopAddress;
    @Mock
    private WebSocketAddress incomingAddress;
    @Mock
    private MessagingStubFactory messagingStubFactory;
    @Mock
    private MessagingSkeletonFactory messagingSkeletonFactory;
    @Mock
    private AddressManager addressManager;
    @Mock
    private MulticastReceiverRegistry multicastReceiverRegistry;

    private JoynrMessage message;
    private LibJoynrMessageRouter messageRouter;
    private String unknownParticipantId = "unknownParticipantId";
    private Long sendMsgRetryIntervalMs = 10L;
    private String globalAddress = "global-address";
    private final boolean isGloballyVisible = true;

    @Before
    public void setUp() {
        message = new JoynrMessage();
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(10000));
        message.setTo(unknownParticipantId);
        message.setLocalMessage(false);
        message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        boolean resolved = true;
        when(routingTable.containsKey(unknownParticipantId)).thenReturn(false);
        // Presumably the participant of next hop is globally visible.
        when(messageRouterParent.resolveNextHop(unknownParticipantId)).thenReturn(new ResolveNextHopReturned(resolved,
                                                                                                             isGloballyVisible));
        when(messageRouterParent.getReplyToAddress()).thenReturn(globalAddress);
        when(messagingStubFactory.create(Mockito.any(Address.class))).thenReturn(messagingStub);
        when(parentAddress.getChannelId()).thenReturn("LibJoynrMessageRouterTestChannel");

        messageRouter = new LibJoynrMessageRouter(routingTable,
                                                  incomingAddress,
                                                  provideMessageSchedulerThreadPoolExecutor(),
                                                  sendMsgRetryIntervalMs,
                                                  messagingStubFactory,
                                                  messagingSkeletonFactory,
                                                  addressManager,
                                                  multicastReceiverRegistry);
        messageRouter.setParentRouter(messageRouterParent, parentAddress, "parentParticipantId", "proxyParticipantId");
    }

    @Test
    public void itSetsReplyTo() throws Exception {
        // message that is a request and not directed to routing provider should get set replyTo
        messageRouter.route(message);
        Thread.sleep(100);
        ArgumentCaptor<JoynrMessage> messageCaptor = ArgumentCaptor.forClass(JoynrMessage.class);
        Mockito.verify(messagingStub).transmit(messageCaptor.capture(), any(FailureAction.class));
        assertEquals(globalAddress, messageCaptor.getValue().getReplyTo());
    }

    @Test
    public void itQueriesParentForNextHop() throws Exception {
        messageRouter.route(message);
        Thread.sleep(100);
        Mockito.verify(messageRouterParent).resolveNextHop(Mockito.eq(unknownParticipantId));
    }

    @Test
    public void addsNextHopAfterQueryingParent() throws Exception {
        messageRouter.route(message);
        Thread.sleep(100);
        Mockito.verify(routingTable).put(Mockito.eq(unknownParticipantId),
                                         Mockito.eq(parentAddress),
                                         Mockito.eq(isGloballyVisible));
    }

    @Test
    public void passesNextHopToParent() {
        final boolean isGloballyVisible = true;
        messageRouter.addNextHop(unknownParticipantId, nextHopAddress, isGloballyVisible);
        Mockito.verify(messageRouterParent).addNextHop(Mockito.eq(unknownParticipantId),
                                                       Mockito.eq(incomingAddress),
                                                       Mockito.eq(isGloballyVisible));
    }

    ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor() {
        ThreadFactory schedulerNamedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.MessageScheduler-scheduler-%d")
                                                                              .build();
        ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(2, schedulerNamedThreadFactory);
        scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
        scheduler.allowCoreThreadTimeOut(true);
        return scheduler;
    }
}
