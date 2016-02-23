package io.joynr.messaging;

import io.joynr.common.ExpiryDate;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.messaging.routing.ChildMessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.Callback;
import joynr.JoynrMessage;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.common.util.concurrent.ThreadFactoryBuilder;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

@RunWith(MockitoJUnitRunner.class)
public class ChildMessageRouterTest {

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

    private JoynrMessage message;
    private ChildMessageRouter messageRouter;
    private String unknownParticipantId = "unknownParticipantId";
    private Long sendMsgRetryIntervalMs = 10L;

    @Before
    public void setUp() {
        message = new JoynrMessage();
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(10000));
        message.setTo(unknownParticipantId);

        messageRouter = new ChildMessageRouter(routingTable,
                                               incomingAddress,
                                               provideMessageSchedulerThreadPoolExecutor(),
                                               sendMsgRetryIntervalMs,
                                               messagingStubFactory);
        messageRouter.setParentRouter(messageRouterParent, parentAddress, "parentParticipantId", "proxyParticipantId");

        Mockito.when(routingTable.containsKey(unknownParticipantId)).thenReturn(false);
        Mockito.when(messageRouterParent.resolveNextHop(unknownParticipantId)).thenReturn(true);
        Mockito.when(parentAddress.getChannelId()).thenReturn("MessageRouterImplTestChannel");

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
        Mockito.verify(routingTable).put(Mockito.eq(unknownParticipantId), Mockito.eq(parentAddress));
    }

    @Test
    @SuppressWarnings("unchecked")
    public void passesNextHopToParent() {
        messageRouter.addNextHop(unknownParticipantId, nextHopAddress);
        Mockito.verify(messageRouterParent).addNextHop(Mockito.any(Callback.class),
                                                       Mockito.eq(unknownParticipantId),
                                                       Mockito.eq(incomingAddress));
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
