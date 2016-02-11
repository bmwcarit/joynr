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

import joynr.system.RoutingTypes.WebSocketAddress;
import org.eclipse.jetty.websocket.client.WebSocketClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.URI;
import java.util.concurrent.ExecutionException;

/**
 * Messaging stub used on libjoynr side. It gets a WebSocketAddress and creates a new connection to it when transmit
 * or sendString is called for the first time
 */
public class LibWebSocketMessagingStub extends WebSocketMessagingStub {

    private static final Logger logger = LoggerFactory.getLogger(LibWebSocketMessagingStub.class);
    private WebSocketAddress address;
    private WebSocketMessagingSkeleton libWebSocketMessagingSkeleton;
    private WebSocketClient client;
    private int maxMessageSize;

    public LibWebSocketMessagingStub(WebSocketAddress address,
                                     ObjectMapper objectMapper,
                                     WebSocketMessagingSkeleton libWebSocketMessagingSkeleton,
                                     int maxMessageSize) {
        super(objectMapper);
        this.address = address;
        this.libWebSocketMessagingSkeleton = libWebSocketMessagingSkeleton;
        this.maxMessageSize = maxMessageSize;
    }

    @Override
    protected void initConnection() {
        logger.debug("Starting WebSocketMessagingStub with address " + address);

        URI uri = URI.create(address.getProtocol() + "://" + address.getHost() + ":" + address.getPort() + ""
                + address.getPath());

        if (client != null) {
            shutdownWebSocketClient();
        }
        client = new WebSocketClient();
        client.getPolicy().setMaxTextMessageSize(maxMessageSize);
        try {
            client.start();
            // Attempt Connect
            sessionFuture = client.connect(libWebSocketMessagingSkeleton, uri);
        } catch (Throwable t) {
            logger.error("Failed to create websocket connection: ", t);
        }
    }

    private void shutdownWebSocketClient() {
        try {
            if (client != null) {
                client.stop();
            }
        } catch (Exception e) {
            logger.error("Failed to stop websocket client: ", e);
        } finally {
            if (client != null) {
                client.destroy();
            }
        }

    }

    @Override
    public void sendString(String string, long timeout) throws IOException {
        super.sendString(string, timeout);
    }

    @Override
    public void shutdown() throws ExecutionException, InterruptedException {
        shutdownWebSocketClient();
        super.shutdown();
    }
}
