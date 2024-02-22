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

import java.util.HashMap;
import java.util.Map;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.websocket.JoynrWebSocketEndpoint;
import io.joynr.messaging.websocket.WebSocketEndpointFactory;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

@Singleton
public class WebSocketJettyClientFactory implements WebSocketEndpointFactory {

    private WebSocketClientAddress ownAddress;
    private int maxMessageSize;
    private long reconnectDelay;
    private long websocketIdleTimeout;
    private Map<WebSocketAddress, JoynrWebSocketEndpoint> jettyClientsMap = new HashMap<WebSocketAddress, JoynrWebSocketEndpoint>();
    private ObjectMapper objectMapper;

    @Inject
    public WebSocketJettyClientFactory(@Named(WebsocketModule.WEBSOCKET_CLIENT_ADDRESS) WebSocketClientAddress ownAddress,
                                       @Named(ConfigurableMessagingSettings.PROPERTY_MAX_MESSAGE_SIZE) int maxMessageSize,
                                       @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_RECONNECT_DELAY) long reconnectDelay,
                                       @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_IDLE_TIMEOUT) long websocketIdleTimeout,
                                       ObjectMapper objectMapper) {
        this.ownAddress = (ownAddress != null) ? new WebSocketClientAddress(ownAddress) : null;
        this.maxMessageSize = maxMessageSize;
        this.reconnectDelay = reconnectDelay;
        this.websocketIdleTimeout = websocketIdleTimeout;
        this.objectMapper = objectMapper;
    }

    @Override
    public synchronized JoynrWebSocketEndpoint create(WebSocketAddress serverAddress) {
        if (!jettyClientsMap.containsKey(serverAddress)) {
            JoynrWebSocketEndpoint jettyClient = new WebSocketJettyClient(serverAddress,
                                                                          ownAddress,
                                                                          maxMessageSize,
                                                                          reconnectDelay,
                                                                          websocketIdleTimeout,
                                                                          objectMapper);
            jettyClientsMap.put(serverAddress, jettyClient);
        }
        return jettyClientsMap.get(serverAddress);
    }
}
