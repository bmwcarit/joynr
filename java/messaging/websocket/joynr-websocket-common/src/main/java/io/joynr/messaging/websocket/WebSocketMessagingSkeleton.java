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
package io.joynr.messaging.websocket;

import java.util.Set;

import org.eclipse.jetty.websocket.api.WebSocketAdapter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.routing.MessageRouter;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.WebSocketAddress;

public class WebSocketMessagingSkeleton extends WebSocketAdapter implements IWebSocketMessagingSkeleton {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketMessagingSkeleton.class);

    public final static String WEBSOCKET_IS_MAIN_TRANSPORT = "io.joynr.websocket.is.main.transport";

    private MessageRouter messageRouter;
    private JoynrWebSocketEndpoint webSocketEndpoint;
    private WebSocketEndpointFactory webSocketEndpointFactory;
    private WebSocketAddress serverAddress;
    private boolean mainTransport;
    private Set<JoynrMessageProcessor> messageProcessors;

    public static class MainTransportFlagBearer {
        @Inject(optional = true)
        @Named(WEBSOCKET_IS_MAIN_TRANSPORT)
        private Boolean mainTransport;

        public MainTransportFlagBearer() {
        }

        protected MainTransportFlagBearer(Boolean mainTransport) {
            this.mainTransport = mainTransport;
        }

        public boolean isMainTransport() {
            return Boolean.TRUE.equals(mainTransport);
        }
    }

    @Inject
    public WebSocketMessagingSkeleton(@Named(WebsocketModule.WEBSOCKET_SERVER_ADDRESS) WebSocketAddress serverAddress,
                                      WebSocketEndpointFactory webSocketEndpointFactory,
                                      MessageRouter messageRouter,
                                      MainTransportFlagBearer mainTransportFlagBearer,
                                      Set<JoynrMessageProcessor> messageProcessors) {
        this.serverAddress = serverAddress;
        this.webSocketEndpointFactory = webSocketEndpointFactory;
        this.messageRouter = messageRouter;
        this.mainTransport = mainTransportFlagBearer.isMainTransport();
        this.messageProcessors = messageProcessors;
    }

    @Override
    public void init() {
        logger.debug("Initializing Websocket skeleton ...");
        webSocketEndpoint = webSocketEndpointFactory.create(serverAddress);
        webSocketEndpoint.setMessageListener(this);
        webSocketEndpoint.start();
    }

    @Override
    public void transmit(byte[] serializedMessage, FailureAction failureAction) {
        try {
            ImmutableMessage message = new ImmutableMessage(serializedMessage);

            logger.debug("<<< INCOMING <<< {}", message);

            if (messageProcessors != null) {
                for (JoynrMessageProcessor processor : messageProcessors) {
                    message = processor.processIncoming(message);
                }
            }

            if (Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType()) && this.isMainTransport()) {
                message.setReceivedFromGlobal(true);
            }
            messageRouter.route(message);
        } catch (Exception error) {
            failureAction.execute(error);
        }
    }

    @Override
    public void shutdown() {
        if (webSocketEndpoint != null) {
            webSocketEndpoint.shutdown();
        }
    }

    private boolean isMainTransport() {
        return mainTransport;
    }
}
