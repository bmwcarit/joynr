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
import org.eclipse.jetty.websocket.api.Session;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public abstract class WebSocketMessagingStub implements IMessaging {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketMessagingStub.class);

    private ObjectMapper objectMapper;

    protected Future<Session> sessionFuture = null;

    public WebSocketMessagingStub(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    protected abstract void initConnection();

    @Override
    public void transmit(JoynrMessage message) throws IOException {
        logger.debug("WebSocketMessagingStub.transmit with message " + message);
        long timeout = message.getExpiryDate() - System.currentTimeMillis();
        sendString(objectMapper.writeValueAsString(message), timeout);
    }

    protected synchronized void sendString(String string, long timeout) throws IOException {
        Session session = null;
        if (sessionFuture == null) {
            initConnection();
        }
        try {
            session = sessionFuture.get(timeout, TimeUnit.MILLISECONDS);
            session.getRemote().sendString(string);
        } catch (InterruptedException | ExecutionException | TimeoutException e) {
            throw new JoynrMessageNotSentException("Websocket transmit error: " + e);
        }
    }

    public void shutdown() throws ExecutionException, InterruptedException {
        Session session = sessionFuture.get();
        session.close();
    }

}
