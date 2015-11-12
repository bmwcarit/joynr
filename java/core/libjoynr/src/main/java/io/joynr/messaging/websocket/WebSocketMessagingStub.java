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
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.IMessaging;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.WebSocketAddress;
import org.eclipse.jetty.util.FuturePromise;
import org.eclipse.jetty.websocket.api.Session;
import org.eclipse.jetty.websocket.client.WebSocketClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.URI;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

public class WebSocketMessagingStub implements IMessaging {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketMessagingStub.class);
    private WebSocketAddress address;
    private ObjectMapper objectMapper;
    private WebSocketMessagingSkeleton libWebSocketMessagingSkeleton;
    private Future<Session> sessionFuture = null;

    /*
     *	Constructor used to create a messaging stub on LibJoynr side
     */
    public WebSocketMessagingStub(WebSocketAddress address,
                                  ObjectMapper objectMapper,
                                  WebSocketMessagingSkeleton libWebSocketMessagingSkeleton) {
        this.address = address;
        this.objectMapper = objectMapper;

        this.libWebSocketMessagingSkeleton = libWebSocketMessagingSkeleton;
    }

    private void initConnection() {
        logger.debug("Starting WebSocketMessagingStub with address " + address);

        URI uri = URI.create(address.getProtocol() + "://" + address.getHost() + ":" + address.getPort() + ""
                + address.getPath());

        WebSocketClient client = new WebSocketClient();
        try {
            client.start();
            // Attempt Connect
            sessionFuture = client.connect(libWebSocketMessagingSkeleton, uri);
        } catch (Throwable t) {
            logger.error("Failed to create websocket connection: ", t);
        }
    }

    /*
     *	Constructor used to create a messaging stub on ClusterController side
     */
    public WebSocketMessagingStub(Session session, ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
        sessionFuture = new FuturePromise<>(session);
	}

    @Override
    public void transmit(JoynrMessage message) throws IOException {
        logger.debug("WebSocketMessagingStub.transmit with message " + message);
        long timeout = message.getExpiryDate() - System.currentTimeMillis();
        sendString(objectMapper.writeValueAsString(message), timeout);
    }

    public void sendString(String string, long timeout) {
        Session session = null;
        if (sessionFuture == null) {
            initConnection();
        }
        try {
            session = sessionFuture.get(timeout, TimeUnit.MILLISECONDS);
            session.getRemote().sendString(string);
            return;
        } catch (Exception e) {
            throw new JoynrMessageNotSentException("Websocket transmit error: " + e);
        }

    }

    public void shutdown() throws ExecutionException, InterruptedException {
        Session session = sessionFuture.get();
        session.close();
    }

}
