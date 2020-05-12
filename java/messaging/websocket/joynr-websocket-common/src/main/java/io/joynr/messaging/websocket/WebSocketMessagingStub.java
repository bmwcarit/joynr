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

import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;

public class WebSocketMessagingStub implements IMessagingStub {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketMessagingStub.class);

    private JoynrWebSocketEndpoint webSocketEndpoint;

    private Address toAddress;

    public WebSocketMessagingStub(Address toAddress, JoynrWebSocketEndpoint webSocketEndpoint) {
        this.toAddress = toAddress;
        this.webSocketEndpoint = webSocketEndpoint;
    }

    @Override
    public void transmit(ImmutableMessage message, SuccessAction successAction, FailureAction failureAction) {
        if (logger.isTraceEnabled()) {
            logger.trace(">>> OUTGOING >>> {}", message);
        } else {
            logger.debug(">>> OUTGOING >>> {}", message.getTrackingInfo());
        }

        if (!message.isTtlAbsolute()) {
            throw new JoynrRuntimeException("Relative TTL not supported");
        }

        long timeout = message.getTtlMs() - System.currentTimeMillis();
        byte[] serializedMessage = message.getSerializedMessage();

        webSocketEndpoint.writeBytes(toAddress,
                                     serializedMessage,
                                     timeout,
                                     TimeUnit.MILLISECONDS,
                                     successAction,
                                     failureAction);
    }
}
