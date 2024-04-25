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
package io.joynr.messaging.websocket.server;

import javax.inject.Named;

import com.google.inject.Inject;
import com.google.inject.Singleton;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.websocket.JoynrWebSocketEndpoint;
import io.joynr.messaging.websocket.WebSocketEndpointFactory;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.WebSocketAddress;

@Singleton
public class WebSocketJettyServerFactory implements WebSocketEndpointFactory {

    private int maxMessageSize;
    private long websocketIdleTimeout;
    private ObjectMapper objectMapper;
    private WebSocketJettyServer jettyServer;

    @Inject
    public WebSocketJettyServerFactory(@Named(ConfigurableMessagingSettings.PROPERTY_MAX_MESSAGE_SIZE) int maxMessageSize,
                                       @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_IDLE_TIMEOUT) long websocketIdleTimeout,
                                       ObjectMapper objectMapper) {
        this.maxMessageSize = maxMessageSize;
        this.websocketIdleTimeout = websocketIdleTimeout;
        this.objectMapper = new ObjectMapper(objectMapper);
    }

    @Override
    public synchronized JoynrWebSocketEndpoint create(WebSocketAddress serverAddress) {
        if (jettyServer == null) {
            jettyServer = new WebSocketJettyServer((WebSocketAddress) serverAddress,
                                                   objectMapper,
                                                   maxMessageSize,
                                                   websocketIdleTimeout);
        }
        return jettyServer;
    }
}
