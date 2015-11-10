package io.joynr.proxy;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ScheduledExecutorService;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import com.google.common.collect.Maps;

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.rpc.JoynrSyncInterface;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.dispatching.JoynrMessageFactory;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.RequestReplyManagerImpl;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessageRouterImpl;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.routing.RoutingTableImpl;
import joynr.JoynrMessage;
import joynr.Reply;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

@RunWith(MockitoJUnitRunner.class)
public class ProxyArbitrationTest {

    private static final String CORRECT_CHANNELID = "correctEndpointAddress";
    @Mock
    MessageSender messageSender;
    @Mock
    private ReplyCallerDirectory replyCallerDirectory;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private JoynrMessageFactory joynrMessageFactory;
    @Mock
    private Reply jsonReply;

    @Mock
    private InProcessAddress libJoynrMessagingAddress;

    ProxyInvocationHandlerImpl proxyHandler;
    private RoutingTable routingTable;
    private String participantId;
    private Address correctEndpointAddress;
    private MessageRouter messageRouter;
    private Map<Class<? extends Address>, AbstractMessagingStubFactory> websocketMessagingStubFactories = new HashMap<Class<? extends Address>, AbstractMessagingStubFactory>();

    public static interface TestSyncInterface extends JoynrSyncInterface {
        public String demoMethod3();

        public void demoMethod1(@JoynrRpcParam("a") Integer a, @JoynrRpcParam("b") String b);

        public void demoMethod2();
    }

    @Before
    public void setUp() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        MessagingQos messagingQos = new MessagingQos();

        routingTable = new RoutingTableImpl("channelurldirectory_participantid",
                                            new ChannelAddress("discoverydirectory_channelid"),
                                            "capabilitiesdirectory_participantid",
                                            new ChannelAddress("discoverydirectory_channelid"),
                                            "domainaccesscontroller_participantid",
                                            new ChannelAddress("domainaccesscontroller_channelid"),
                                            "discovery_participantid",
                                            new ChannelAddress("discovery_channelid"));
        websocketMessagingStubFactories = Maps.newHashMap();
        websocketMessagingStubFactories.put(ChannelAddress.class, new ChannelMessagingStubFactory(messageSender));
        MessagingStubFactory messagingStubFactory = new MessagingStubFactory(messageSender);
        messageRouter = new MessageRouterImpl(routingTable, messagingStubFactory);

        participantId = "testParticipant";
        correctEndpointAddress = new ChannelAddress(CORRECT_CHANNELID);

        routingTable.put(participantId, correctEndpointAddress);

        RequestReplyManager requestReplyManager = new RequestReplyManagerImpl(joynrMessageFactory,
                                                                              replyCallerDirectory,
                                                                              Mockito.mock(RequestCallerDirectory.class),
                                                                              messageRouter,
                                                                              Mockito.mock(RequestInterpreter.class),
                                                                              Mockito.mock(ScheduledExecutorService.class));
        JoynrMessagingConnectorFactory joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplyManager,
                                                                                                           replyCallerDirectory,
                                                                                                           subscriptionManager);
        ConnectorFactory connectorFactory = new ConnectorFactory(joynrMessagingConnectorFactory,
                                                                 messageRouter,
                                                                 libJoynrMessagingAddress);
        proxyHandler = new ProxyInvocationHandlerImpl("domain",
                                                      "interfaceName",
                                                      participantId,
                                                      discoveryQos,
                                                      messagingQos,
                                                      connectorFactory);

        proxyHandler.createConnector(new ArbitrationResult(participantId));

        Request request = Mockito.<Request> any();
        Mockito.when(joynrMessageFactory.createRequest(Mockito.anyString(),
                                                       Mockito.anyString(),
                                                       request,
                                                       Mockito.any(ExpiryDate.class)))
               .thenAnswer(new Answer<JoynrMessage>() {

                   @Override
                   public JoynrMessage answer(InvocationOnMock invocation) throws Throwable {
                       JoynrMessage result = new JoynrMessage();
                       result.setFrom((String) invocation.getArguments()[0]);
                       result.setTo((String) invocation.getArguments()[1]);
                       return result;
                   }

               });

        Mockito.doAnswer(new Answer<Object>() {

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                final ReplyCaller replyCaller = (ReplyCaller) args[1];

                new Thread(new Runnable() {

                    @Override
                    public void run() {
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        replyCaller.messageCallBack(jsonReply);
                    }
                }).start();
                return null;
            }

        }).when(replyCallerDirectory).addReplyCaller(Mockito.anyString(),
                                                     Mockito.<ReplyCaller> any(),
                                                     Mockito.any(ExpiryDate.class));
    }

    @Test(timeout = 5000)
    public void proxyUsesCorrectEndpointToSendRequest() throws IllegalArgumentException, SecurityException,
                                                       InterruptedException, NoSuchMethodException, Throwable {
        proxyHandler.invoke(TestSyncInterface.class.getDeclaredMethod("demoMethod2", new Class<?>[]{}), new Object[]{});
        Mockito.verify(messageSender).sendMessage(Mockito.eq(CORRECT_CHANNELID), Mockito.<JoynrMessage> any());
    }

}
