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
package io.joynr.messaging.websocket.jetty.client;

import java.io.IOException;
import java.net.URI;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;

import org.eclipse.jetty.websocket.api.Session;
import org.eclipse.jetty.websocket.api.WebSocketAdapter;
import org.eclipse.jetty.websocket.api.WebSocketException;
import org.eclipse.jetty.websocket.api.WriteCallback;
import org.eclipse.jetty.websocket.client.WebSocketClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;

import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.websocket.IWebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.JoynrWebSocketEndpoint;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

public class WebSocketJettyClient extends WebSocketAdapter implements JoynrWebSocketEndpoint {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketJettyClient.class);

    private Timer reconnectTimer = new Timer();
    private AtomicBoolean reconnectTimerRunning = new AtomicBoolean(false);
    private long reconnectDelay;

    private WebSocketClient jettyClient;
    private int maxMessageSize;
    private long websocketIdleTimeout;
    Future<Session> sessionFuture;
    private WebSocketAddress serverAddress;
    private IWebSocketMessagingSkeleton messageListener;
    private ObjectMapper objectMapper;
    private WebSocketClientAddress ownAddress;

    private boolean shutdown = false;

    public WebSocketJettyClient(WebSocketAddress serverAddress,
                                WebSocketClientAddress ownAddress,
                                int maxMessageSize,
                                long reconnectDelay,
                                long websocketIdleTimeout,
                                ObjectMapper objectMapper) {
        this.serverAddress = serverAddress;
        this.ownAddress = ownAddress;
        this.maxMessageSize = maxMessageSize;
        this.reconnectDelay = reconnectDelay;
        this.websocketIdleTimeout = websocketIdleTimeout;
        this.objectMapper = objectMapper;
    }

    @Override
    public synchronized void start() {
        if (jettyClient == null) {
            jettyClient = new WebSocketClient();
            jettyClient.getPolicy().setMaxTextMessageSize(maxMessageSize);
            jettyClient.getPolicy().setMaxBinaryMessageSize(maxMessageSize);
            jettyClient.setMaxIdleTimeout(websocketIdleTimeout);
        }

        try {
            jettyClient.start();
            sessionFuture = jettyClient.connect(this, toUrl(serverAddress));
            sendInitializationMessage();
        } catch (JoynrShutdownException | JoynrIllegalStateException e) {
            logger.error("Unrecoverable error starting WebSocket client: {}", e);
            return;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } catch (Exception e) {
            // TODO which exceptions are recoverable? Only catch those ones
            // JoynrCommunicationExeption is thrown if the initialization message could not be sent
            logger.debug("Error starting WebSocket client. Will retry", e);
            if (shutdown) {
                return;
            }

            if (reconnectTimerRunning.compareAndSet(false, true)) {
                reconnectTimer.schedule(new TimerTask() {
                    @Override
                    public void run() {
                        reconnectTimerRunning.set(false);
                        start();
                    }
                }, reconnectDelay);
            }
        }
    }

    private void sendInitializationMessage() throws InterruptedException, JoynrCommunicationException {
        String serializedAddress;
        try {
            serializedAddress = objectMapper.writeValueAsString(ownAddress);
        } catch (JsonProcessingException e) {
            throw new JoynrIllegalStateException("unable to serialize WebSocket Client address: " + ownAddress, e);
        }

        try {
            sessionFuture.get(30, TimeUnit.SECONDS)
                         .getRemote()
                         .sendBytes(ByteBuffer.wrap(serializedAddress.getBytes(CHARSET)));
        } catch (IOException | ExecutionException | TimeoutException e) {
            throw new JoynrCommunicationException(e.getMessage(), e);
        }
    }

    private URI toUrl(WebSocketAddress address) {
        try {
            return URI.create(address.getProtocol() + "://" + address.getHost() + ":" + address.getPort() + ""
                    + address.getPath());
        } catch (IllegalArgumentException e) {
            throw new JoynrIllegalStateException("unable to parse WebSocket Server Address", e);
        }
    }

    @Override
    public void setMessageListener(IWebSocketMessagingSkeleton messaging) {
        this.messageListener = messaging;
    }

    @Override
    public synchronized void shutdown() {
        shutdown = true;
        closeSession();
        try {
            if (jettyClient != null) {
                jettyClient.stop();
            }
        } catch (Exception e) {
            logger.error("Error: ", e);
        }

    }

    private void closeSession() {
        try {
            if (sessionFuture != null) {
                Session session = sessionFuture.get();
                if (session != null) {
                    session.close();
                }
                sessionFuture = null;
            }
        } catch (InterruptedException | ExecutionException e) {
            logger.error("Error while closing websocket connection: ", e);
        } catch (Exception e) {
            logger.error("Error: ", e);
        }
    }

    @Override
    public synchronized void reconnect() {

        try {
            if (sessionFuture != null && sessionFuture.get().isOpen()) {
                return;
            }
        } catch (ExecutionException e) {
            // continue reconnecting if there was a problem
            logger.debug("Error getting session future", e);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        if (shutdown) {
            return;
        }
        closeSession();
        start();
    }

    @Override
    public void onWebSocketBinary(byte[] payload, int offset, int len) {
        if (logger.isTraceEnabled()) {
            logger.trace("Received message: {}", new String(payload, CHARSET));
        }
        messageListener.transmit(Arrays.copyOfRange(payload, offset, offset + len), new FailureAction() {

            @Override
            public void execute(Throwable error) {
                if (error instanceof JoynrMessageExpiredException) {
                    logger.warn("WebSocket message not processed: ", error);
                } else {
                    logger.error("WebSocket message not processed: ", error);
                }
            }
        });
    }

    @Override
    public synchronized void writeBytes(Address to,
                                        byte[] message,
                                        long timeout,
                                        TimeUnit unit,
                                        final SuccessAction successAction,
                                        final FailureAction failureAction) {
        if (messageListener == null) {
            throw new JoynrDelayMessageException(20, "WebSocket write failed: receiver has not been set yet");
        }

        if (sessionFuture == null) {
            try {
                reconnect();
            } catch (Exception e) {
                throw new JoynrDelayMessageException(10, "WebSocket reconnect failed. Will try later", e);
            }
        }

        try {
            Session session = sessionFuture.get(timeout, unit);
            session.getRemote().sendBytes(ByteBuffer.wrap(message), new WriteCallback() {

                @Override
                public void writeSuccess() {
                    successAction.execute();
                }

                @Override
                public void writeFailed(Throwable error) {
                    if (error instanceof WebSocketException) {
                        reconnect();
                        failureAction.execute(new JoynrDelayMessageException(reconnectDelay, error.getMessage()));
                    } else {
                        failureAction.execute(error);
                    }
                }
            });
        } catch (WebSocketException | ExecutionException e) {
            reconnect();
            throw new JoynrDelayMessageException(10, "WebSocket write timed out", e);
        } catch (TimeoutException e) {
            throw new JoynrDelayMessageException("WebSocket write timed out", e);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    @Override
    public void onWebSocketClose(int statusCode, String reason) {
        super.onWebSocketClose(statusCode, reason);
        /* maybe the socket has been disconnected by the server, so let's retry */
        if (!shutdown) {
            reconnect();
        }
    }
}
