package io.joynr.messaging.websocket;

import java.util.concurrent.ExecutionException;

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
import com.google.inject.name.Named;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import joynr.system.RoutingTypes.WebSocketAddress;

public class WebSocketMessagingStubFactory extends
        AbstractMiddlewareMessagingStubFactory<LibWebSocketMessagingStub, WebSocketAddress> {

    ObjectMapper objectMapper;
    WebSocketMessagingSkeleton webSocketMessagingSkeleton;
    int maxMessageSize;

    @Inject
    public WebSocketMessagingStubFactory(ObjectMapper objectMapper,
                                         @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_SKELETON) WebSocketMessagingSkeleton webSocketMessagingSkeleton,
                                         @Named(ConfigurableMessagingSettings.PROPERTY_MAX_MESSAGE_SIZE) int maxMessageSize) {
        this.objectMapper = objectMapper;
        this.webSocketMessagingSkeleton = webSocketMessagingSkeleton;
        this.maxMessageSize = maxMessageSize;
    }

    @Override
    protected LibWebSocketMessagingStub createInternal(WebSocketAddress address) {
        return new LibWebSocketMessagingStub(address, objectMapper, webSocketMessagingSkeleton, maxMessageSize);
    }

    @Override
    public void shutdown() {
        for (LibWebSocketMessagingStub stub : getAllMessagingStubs()) {
            try {
                stub.shutdown();
            } catch (ExecutionException | InterruptedException e) {
            }
        }
    }
}
