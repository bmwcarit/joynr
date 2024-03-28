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

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

public class WebsocketModule extends AbstractModule {

    public static final String PROPERTY_WEBSOCKET_MESSAGING_SKELETON = "joynr.websockets.messaging_skeleton";

    public static final String PROPERTY_WEBSOCKET_MESSAGING_URL_ADDRESS = "joynr.websockets.messagingurl";

    public static final String PROPERTY_WEBSOCKET_MESSAGING_HOST = "joynr.messaging.cc.host";
    public static final String PROPERTY_WEBSOCKET_MESSAGING_PORT = "joynr.messaging.cc.port";
    public static final String PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL = "joynr.messaging.cc.protocol";
    public static final String PROPERTY_WEBSOCKET_MESSAGING_PATH = "joynr.messaging.cc.path";
    public static final String PROPERTY_WEBSOCKET_MESSAGING_RECONNECT_DELAY = "joynr.messaging.cc.reconnectdelay";
    public static final String PROPERTY_WEBSOCKET_MESSAGING_IDLE_TIMEOUT = "joynr.messaging.cc.idletimeout";

    public static final String WEBSOCKET_CLIENT_ADDRESS = "websocket_client_address";
    public static final String WEBSOCKET_SERVER_ADDRESS = "websocket_server_address";

    @Override
    protected void configure() {
    }

    @Provides
    @Named(WEBSOCKET_SERVER_ADDRESS)
    public WebSocketAddress provideWebsocketUrlWebSocketAddress(@Named(PROPERTY_WEBSOCKET_MESSAGING_HOST) String host,
                                                                @Named(PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL) String protocol,
                                                                @Named(PROPERTY_WEBSOCKET_MESSAGING_PORT) int port,
                                                                @Named(PROPERTY_WEBSOCKET_MESSAGING_PATH) String path) {
        return new WebSocketAddress(WebSocketProtocol.valueOf(protocol.toUpperCase()), host, port, path);
    }
}
