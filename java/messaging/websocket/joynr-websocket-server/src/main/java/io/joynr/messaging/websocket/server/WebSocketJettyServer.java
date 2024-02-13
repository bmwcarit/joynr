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
package io.joynr.messaging.websocket.server;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.TimeUnit;

import io.joynr.messaging.websocket.MessageHelper;
import org.eclipse.jetty.http.HttpVersion;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.SslConnectionFactory;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.websocket.api.Session;
import org.eclipse.jetty.websocket.api.WebSocketAdapter;
import org.eclipse.jetty.websocket.api.WriteCallback;
import org.eclipse.jetty.websocket.api.exceptions.WebSocketException;
import org.eclipse.jetty.websocket.server.JettyServerUpgradeRequest;
import org.eclipse.jetty.websocket.server.JettyServerUpgradeResponse;
import org.eclipse.jetty.websocket.server.JettyWebSocketCreator;
import org.eclipse.jetty.websocket.server.config.JettyWebSocketServletContainerInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.websocket.IWebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.JoynrWebSocketEndpoint;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

public class WebSocketJettyServer implements JoynrWebSocketEndpoint, WebSocketMessageArrivedListener {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketJettyServer.class);

    private Server server;
    private WebSocketAddress address;
    private int maxMessageSize;
    private long websocketIdleTimeout;
    private Map<String, Session> sessionMap = new HashMap<>();
    private List<CCWebSocketMessagingSkeletonSocket> openSockets = new ArrayList<>();

    public ObjectMapper objectMapper;

    private IWebSocketMessagingSkeleton messageListener;

    private boolean shutdown = false;

    public WebSocketJettyServer(WebSocketAddress address,
                                ObjectMapper objectMapper,
                                int maxMessageSize,
                                long websocketIdleTimeout) {
        this.address = (address != null) ? new WebSocketAddress(address) : null;
        this.objectMapper = objectMapper;
        this.maxMessageSize = maxMessageSize;
        this.websocketIdleTimeout = websocketIdleTimeout;
    }

    @Override
    public void messageArrived(byte[] message) {
        messageListener.transmit(message, new FailureAction() {
            @Override
            public void execute(Throwable error) {
                logger.error("Unable to process message: {}", error.getMessage());
            }
        });
    }

    @Override
    public void start() {
        if (server != null && server.isRunning()) {
            return;
        }
        server = new Server();
        ServerConnector connector;

        if (address.getProtocol().equals(WebSocketProtocol.WSS)) {
            SslConnectionFactory sslConnectionFactory = new SslConnectionFactory(HttpVersion.HTTP_1_1.asString());
            connector = new ServerConnector(server, sslConnectionFactory);
        } else {
            connector = new ServerConnector(server);
        }
        connector.setPort(address.getPort());
        server.addConnector(connector);

        ServletContextHandler context = new ServletContextHandler(ServletContextHandler.SESSIONS);
        context.setContextPath("/");
        server.setHandler(context);

        JettyWebSocketServletContainerInitializer.configure(context, (servletContext, wsContainer) -> {
            wsContainer.setMaxTextMessageSize(maxMessageSize);
            wsContainer.setMaxBinaryMessageSize(maxMessageSize);
            wsContainer.setIdleTimeout(Duration.ofMillis(websocketIdleTimeout));

            // Add CCWebSocketMessagingSkeletonSocket
            wsContainer.addMapping(address.getPath(), new JettyWebSocketCreator() {
                @Override
                public Object createWebSocket(JettyServerUpgradeRequest req, JettyServerUpgradeResponse resp) {
                    logger.trace("Creating CCWebSocketMessagingSkeletonSocket ...");
                    CCWebSocketMessagingSkeletonSocket socket = new CCWebSocketMessagingSkeletonSocket(WebSocketJettyServer.this);
                    openSockets.add(socket);
                    logger.trace("CCWebSocketMessagingSkeletonSocket created");
                    return socket;
                }
            });
        });

        try {
            logger.info("Starting WebSocket server ...");
            server.start();
            logger.info("WebSocket server started");
        } catch (Exception t) {
            logger.error("Error while starting websocket server: ", t);
        }
    }

    @Override
    public void setMessageListener(IWebSocketMessagingSkeleton messageListener) {
        this.messageListener = messageListener;
    }

    @Override
    public void shutdown() {
        shutdown = true;
        for (Entry<String, Session> session : sessionMap.entrySet()) {
            try {
                session.getValue().disconnect();
            } catch (Exception e) {
                logger.error("Error closing session: {}", session.getKey(), e);
            }
        }
        try {
            server.stop();
        } catch (Exception e) {
            logger.error("Error stopping WebSocket server: ", e);
        }
    }

    @Override
    public synchronized void writeBytes(Address toAddress,
                                        byte[] message,
                                        long timeout,
                                        TimeUnit unit,
                                        final SuccessAction successAction,
                                        final FailureAction failureAction) {
        if (!(toAddress instanceof WebSocketClientAddress)) {
            throw new JoynrIllegalStateException("Web Socket Server can only send to WebSocketClientAddresses");
        }

        WebSocketClientAddress toClientAddress = (WebSocketClientAddress) toAddress;
        Session session = sessionMap.get(toClientAddress.getId());
        if (session == null) {
            //TODO We need a delay with invalidation of the stub
            throw new JoynrDelayMessageException("no active session for WebSocketClientAddress: "
                    + toClientAddress.getId());
        }
        try {
            session.getRemote().sendBytes(ByteBuffer.wrap(message), new WriteCallback() {
                @Override
                public void writeSuccess() {
                    successAction.execute();
                }

                @Override
                public void writeFailed(Throwable error) {
                    if (shutdown) {
                        return;
                    }
                    failureAction.execute(error);
                }
            });
        } catch (WebSocketException e) {
            // Jetty throws WebSocketException when expecting [OPEN or CONNECTED] but found a different state
            // The client must reconnect, but the message can be queued in the mean time.
            sessionMap.remove(toClientAddress.getId());
            //TODO We need a delay with invalidation of the stub
            throw new JoynrDelayMessageException(e.getMessage(), e);
        }
    }

    @Override
    public void reconnect() {
        // Server is not responsible for reconnect. In case of connection loss, the server expects a reconnect from the client.
    }

    /**
     * Inner class which handles incoming requests and represents a websocket session. Delegates to the singleton skeleton
     */
    private class CCWebSocketMessagingSkeletonSocket extends WebSocketAdapter {

        private WebSocketMessageArrivedListener messageArrivedListener;

        public CCWebSocketMessagingSkeletonSocket(WebSocketMessageArrivedListener messageArrivedListener) {
            this.messageArrivedListener = messageArrivedListener;
        }

        @Override
        public void onWebSocketText(String message) {
            logger.error("Received text Message: {}", message);
        }

        @Override
        public void onWebSocketBinary(byte[] payload, int offset, int len) {
            String serializedMessage = new String(payload, offset, len, CHARSET);
            logger.trace("Received message: {}", serializedMessage);
            if (isInitializationMessage(serializedMessage)) {
                try {
                    WebSocketClientAddress webSocketClientAddress = objectMapper.readValue(serializedMessage,
                                                                                           WebSocketClientAddress.class);
                    logger.debug("Registering WebSocketClientAddress: {}", webSocketClientAddress);
                    sessionMap.put(webSocketClientAddress.getId(), getSession());
                } catch (IOException e) {
                    logger.error("Error parsing WebSocketClientAddress: ", e);
                }
            } else {
                messageArrivedListener.messageArrived(MessageHelper.extractMessage(payload, offset, len));
            }
        }

        @Override
        public void onWebSocketClose(int statusCode, String reason) {
            super.onWebSocketClose(statusCode, reason);
            openSockets.remove(CCWebSocketMessagingSkeletonSocket.this);
            // TODO remove address from routing table???
        }
    }

    /*
     * Checks if the message contains a WebSockeClientAddress of a libjoynr instance.
     * @return true if the json object's _typeName field contains joynr.system.WebSocketClientAddress
     */
    private boolean isInitializationMessage(String json) {
        return json.startsWith("{\"_typeName\":\"joynr.system.RoutingTypes.WebSocketClientAddress\"");
    }
}
