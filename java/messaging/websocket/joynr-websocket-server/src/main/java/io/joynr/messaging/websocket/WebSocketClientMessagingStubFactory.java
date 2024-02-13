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

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

@Singleton
public class WebSocketClientMessagingStubFactory
        extends AbstractMiddlewareMessagingStubFactory<WebSocketMessagingStub, WebSocketClientAddress> {

    private WebSocketEndpointFactory webSocketEndpointFactory;
    private WebSocketAddress serverAddress;

    @Inject
    public WebSocketClientMessagingStubFactory(@Named(WebsocketModule.WEBSOCKET_SERVER_ADDRESS) WebSocketAddress serverAddress,
                                               WebSocketEndpointFactory webSocketEndpointFactory) {
        this.serverAddress = (serverAddress != null) ? new WebSocketAddress(serverAddress) : null;
        this.webSocketEndpointFactory = webSocketEndpointFactory;
    }

    @Override
    protected WebSocketMessagingStub createInternal(WebSocketClientAddress clientAddress) {
        JoynrWebSocketEndpoint webSocketServer = webSocketEndpointFactory.create(serverAddress);
        return new WebSocketMessagingStub(clientAddress, webSocketServer);
    }
}
