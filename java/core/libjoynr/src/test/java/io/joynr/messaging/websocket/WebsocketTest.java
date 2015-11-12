package io.joynr.messaging.websocket;

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

import com.fasterxml.jackson.databind.ObjectMapper;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.JoynrMessageFactory;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.security.DummyPlatformSecurityManager;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;

@RunWith(MockitoJUnitRunner.class)
public class WebsocketTest {
    private static Logger logger = LoggerFactory.getLogger(WebsocketTest.class);
    private WebSocketMessagingStub webSocketMessagingStub;
    private CCWebSocketMessagingSkeleton ccWebSocketMessagingSkeleton;
    private WebSocketAddress serverAddress = new WebSocketAddress(WebSocketProtocol.WS, "localhost", 9080, "/test");

    private JoynrMessageFactory joynrMessageFactory;
    @Mock
    private WebSocketClientMessagingStubFactory webSocketMessagingStubFactory;
    @Mock
    private LibWebSocketMessagingSkeleton libWebSocketMessagingSkeleton;

    @Mock
    MessageRouter messageRouterMock;

    @Before
    public void init() throws IOException {
        logger.debug("INIT WebsocketTest");
        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocationOnMock) throws Throwable {
                logger.debug("message arrived: " + invocationOnMock.getArguments().toString());
                return null;
            }
        }).when(messageRouterMock).route(Mockito.any(JoynrMessage.class));
        ccWebSocketMessagingSkeleton = new CCWebSocketMessagingSkeleton(serverAddress,
                                                                        new ObjectMapper(),
                                                                        messageRouterMock,
                                                                        webSocketMessagingStubFactory);
        joynrMessageFactory = new JoynrMessageFactory(new ObjectMapper(), new DummyPlatformSecurityManager());
    }

    @Test
    public void test1() {
        logger.debug("TEST");
        ccWebSocketMessagingSkeleton.initializeConnection();
        webSocketMessagingStub = new WebSocketMessagingStub(serverAddress,
                                                            new ObjectMapper(),
                                                            libWebSocketMessagingSkeleton);
        try {
            JoynrMessage msg = joynrMessageFactory.createOneWay("fromID",
                                                                "toID",
                                                                "Test Payload",
                                                                ExpiryDate.fromRelativeTtl(100000));
            webSocketMessagingStub.transmit(msg);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
