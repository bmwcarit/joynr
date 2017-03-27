package io.joynr.messaging.websocket;

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

import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;

public class WebSocketMessagingStub implements IMessaging {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketMessagingStub.class);

    protected ObjectMapper objectMapper;
    private JoynrWebSocketEndpoint webSocketEndpoint;

    private Address toAddress;

    public WebSocketMessagingStub(Address toAddress, JoynrWebSocketEndpoint webSocketEndpoint, ObjectMapper objectMapper) {
        this.toAddress = toAddress;
        this.webSocketEndpoint = webSocketEndpoint;
        this.objectMapper = objectMapper;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        logger.debug(">>> OUTGOING >>> {}", message.toLogMessage());
        long timeout = message.getExpiryDate() - System.currentTimeMillis();
        String serializedMessage;
        try {
            serializedMessage = objectMapper.writeValueAsString(message);
            webSocketEndpoint.writeText(toAddress, serializedMessage, timeout, TimeUnit.MILLISECONDS, failureAction);
        } catch (JsonProcessingException error) {
            failureAction.execute(error);
        }
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        logger.debug(">>> OUTGOING >>> {}", serializedMessage);
        webSocketEndpoint.writeText(toAddress, serializedMessage, 30, TimeUnit.SECONDS, failureAction);
    }
}
