package io.joynr.integration.websocket;

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

import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.JoynrMessageFactory;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.websocket.CCWebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.LibWebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.LibWebSocketMessagingStub;
import io.joynr.messaging.websocket.WebSocketClientMessagingStubFactory;
import io.joynr.security.DummyPlatformSecurityManager;

import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.ExecutionException;

import joynr.JoynrMessage;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

import org.eclipse.jetty.websocket.api.Session;
import org.junit.After;
import org.junit.Assert;
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

import com.fasterxml.jackson.databind.ObjectMapper;

@RunWith(MockitoJUnitRunner.class)
public class WebsocketTest {
    private static Logger logger = LoggerFactory.getLogger(WebsocketTest.class);
    private LibWebSocketMessagingStub webSocketMessagingStub;
    private CCWebSocketMessagingSkeleton ccWebSocketMessagingSkeleton;
    private WebSocketAddress serverAddress = new WebSocketAddress(WebSocketProtocol.WS, "localhost", 8080, "/test");
    private WebSocketClientAddress clientAddress = new WebSocketClientAddress(UUID.randomUUID().toString().replace("-",
                                                                                                                   ""));

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
        ccWebSocketMessagingSkeleton.init();
    }

    @After
    public void stop() throws Exception {
        Thread.sleep(1000); // wait a short time to let the server finish
        logger.debug("Stopping server...");
        ccWebSocketMessagingSkeleton.shutdown();
        logger.debug("Server stopped");
    }

    @Test
    public void testSendMessage() {
        JoynrMessage msg = joynrMessageFactory.createOneWay("fromID",
                "toID",
                "Test Payload",
                ExpiryDate.fromRelativeTtl(100000));
        try {
            webSocketMessagingStub = new LibWebSocketMessagingStub(serverAddress,
                    new ObjectMapper(),
                    libWebSocketMessagingSkeleton);
            webSocketMessagingStub.transmit(msg);
            Mockito.verify(messageRouterMock, Mockito.timeout(1000)).route(msg);
        } catch (IOException e) {
            logger.error("Error: ", e);
            Assert.fail(e.getMessage());
        } finally {
            try {
                webSocketMessagingStub.shutdown();
            } catch (InterruptedException | ExecutionException e) {
                logger.error("Error: ", e);
            }
        }
    }

    @Test
    public void testStubCreatedOnInit() throws IOException {
        try {
            webSocketMessagingStub = new LibWebSocketMessagingStub(serverAddress,
                    new ObjectMapper(),
                    libWebSocketMessagingSkeleton);

            ObjectMapper objectMapper = new ObjectMapper();
            String serializedAddress = objectMapper.writeValueAsString(clientAddress);

            webSocketMessagingStub.sendString(serializedAddress, 30000);
            // check if the server created a new
            Mockito.verify(webSocketMessagingStubFactory, Mockito.timeout(1000).times(1)).addSession(Mockito.eq(clientAddress), Mockito.any(Session.class));

        } finally {
            try {
                webSocketMessagingStub.shutdown();
            } catch (ExecutionException | InterruptedException e) {
                logger.error("Error: ", e);
            }
        }

    }
}
