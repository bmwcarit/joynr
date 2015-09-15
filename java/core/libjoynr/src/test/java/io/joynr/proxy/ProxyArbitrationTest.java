package io.joynr.proxy;

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

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.ArbitrationStatus;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.JoynrMessageFactory;
import io.joynr.dispatcher.MessagingEndpointDirectory;
import io.joynr.dispatcher.ReplyCaller;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.RequestReplySenderImpl;
import io.joynr.dispatcher.rpc.JoynrMessagingConnectorFactory;
import io.joynr.dispatcher.rpc.JoynrSyncInterface;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessagingQos;
import io.joynr.pubsub.subscription.SubscriptionManager;

import java.util.List;

import joynr.JoynrMessage;
import joynr.Reply;
import joynr.Request;
import joynr.system.routingtypes.Address;
import joynr.system.routingtypes.ChannelAddress;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.common.collect.Lists;

@RunWith(MockitoJUnitRunner.class)
public class ProxyArbitrationTest {

    private static final String CORRECT_CHANNELID = "correctEndpointAddress";
    @Mock
    MessageSender messageSender;
    @Mock
    private RequestReplyDispatcher dispatcher;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private JoynrMessageFactory joynrMessageFactory;
    @Mock
    private Reply jsonReply;

    ProxyInvocationHandlerImpl proxyHandler;

    private MessagingEndpointDirectory messagingEndpointDirectory;
    private String participantId;
    private Address correctEndpointAddress;
    private Address wrongEndpointAddress;

    public static interface TestSyncInterface extends JoynrSyncInterface {
        public String demoMethod3();

        public void demoMethod1(@JoynrRpcParam("a") Integer a, @JoynrRpcParam("b") String b);

        public void demoMethod2();
    }

    @Before
    public void setUp() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        MessagingQos messagingQos = new MessagingQos();

        messagingEndpointDirectory = new MessagingEndpointDirectory("channelurldirectory_participantid",
                                                                    "discoverydirectory_channelid",
                                                                    "capabilitiesdirectory_participantid",
                                                                    "discoverydirectory_channelid");
        participantId = "testParticipant";
        correctEndpointAddress = new ChannelAddress(CORRECT_CHANNELID);
        wrongEndpointAddress = new ChannelAddress("wrongEndpointAddress");

        messagingEndpointDirectory.put(participantId, wrongEndpointAddress);
        messagingEndpointDirectory.put(participantId, correctEndpointAddress);

        RequestReplySender requestReplySender = new RequestReplySenderImpl(joynrMessageFactory,
                                                                           messageSender,
                                                                           messagingEndpointDirectory);
        JoynrMessagingConnectorFactory joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplySender,
                                                                                                           dispatcher,
                                                                                                           subscriptionManager);
        ConnectorFactory connectorFactory = new ConnectorFactory(joynrMessagingConnectorFactory);
        proxyHandler = new ProxyInvocationHandlerImpl("domain",
                                                      "interfaceName",
                                                      participantId,
                                                      discoveryQos,
                                                      messagingQos,
                                                      connectorFactory);
        List<Address> endpoints = Lists.newArrayList(correctEndpointAddress);

        DiscoveryAgent discoveryAgent = new DiscoveryAgent();
        discoveryAgent.setProxyInvocationHandler(proxyHandler);
        discoveryAgent.setArbitrationResult(ArbitrationStatus.ArbitrationSuccesful,
                                            new ArbitrationResult(participantId, endpoints));

        Request request = Mockito.<Request> any();
        Mockito.when(joynrMessageFactory.createRequest(Mockito.anyString(),
                                                       Mockito.anyString(),
                                                       request,
                                                       Mockito.any(ExpiryDate.class),
                                                       Mockito.anyString())).thenReturn(new JoynrMessage());

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

        }).when(dispatcher).addReplyCaller(Mockito.anyString(), Mockito.<ReplyCaller> any(), Mockito.anyLong());
    }

    @Test
    public void proxyUsesCorrectEndpointToSendRequest() throws IllegalArgumentException, SecurityException,
                                                       InterruptedException, NoSuchMethodException, Throwable {
        proxyHandler.invoke(TestSyncInterface.class.getDeclaredMethod("demoMethod2", new Class<?>[]{}), new Object[]{});
        Mockito.verify(messageSender).sendMessage(Mockito.eq(CORRECT_CHANNELID), Mockito.<JoynrMessage> any());
    }

}
