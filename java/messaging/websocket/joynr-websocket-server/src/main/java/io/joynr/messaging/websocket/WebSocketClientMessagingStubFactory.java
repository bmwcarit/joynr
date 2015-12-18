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
import com.google.inject.Inject;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.IMessaging;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import org.eclipse.jetty.websocket.api.Session;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/**
 * Factory for messaging stubs used on cluster controller side to create a connection to registered clients
 */
public class WebSocketClientMessagingStubFactory extends AbstractMessagingStubFactory<WebSocketClientAddress> {

    private Logger logger = LoggerFactory.getLogger(WebSocketClientMessagingStubFactory.class);
    private Map<String, Session> sessionMap = new HashMap<>();

    @Inject
    ObjectMapper objectMapper;

    @Override
    protected IMessaging createInternal(WebSocketClientAddress address) {
        if (sessionMap.containsKey(address.getId())) {
            return new CCWebSocketMessagingStub(sessionMap.get(address.getId()), objectMapper);
        } else {
            throw new JoynrIllegalStateException("No session available for WebSocketClientAddress: " + address);
        }
    }

    @Override
    public void shutdown() {
        for (Session session : sessionMap.values()) {
            try {
                session.disconnect();
            } catch (IOException e) {
                logger.error("Error: ", e);
            }
        }

    }


    public void addSession(WebSocketClientAddress webSocketClientAddress, Session session) {
        sessionMap.put(webSocketClientAddress.getId(), session);
    }
}
