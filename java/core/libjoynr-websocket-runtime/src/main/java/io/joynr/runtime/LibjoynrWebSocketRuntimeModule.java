package io.joynr.runtime;

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

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.routing.ChildMessageRouter;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.websocket.LibWebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import io.joynr.messaging.websocket.WebsocketModule;

import java.util.Map;
import java.util.UUID;

import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

import com.google.common.collect.Maps;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import com.google.inject.name.Names;

/**
 *  Use this module if you want to start a lib joynr instance which connects to a cluster controller by websockets
 */
public class LibjoynrWebSocketRuntimeModule extends AbstractRuntimeModule {

    @Override
    protected void configure() {
        super.configure();
        install(new WebsocketModule());
        bind(JoynrRuntime.class).to(LibjoynrWebSocketRuntime.class).in(Singleton.class);
        bind(WebSocketMessagingSkeleton.class).annotatedWith(Names.named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_SKELETON))
                                              .to(LibWebSocketMessagingSkeleton.class)
                                              .in(Singleton.class);
        bind(WebSocketMessagingStubFactory.class).in(Singleton.class);
        bind(ChildMessageRouter.class).in(Singleton.class);
        bind(MessageRouter.class).to(ChildMessageRouter.class);
    }

    @Provides
    @Singleton
    Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> provideMessagingStubFactories(WebSocketMessagingStubFactory webSocketMessagingStubFactory) {
        Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> factories = Maps.newHashMap();
        factories.put(WebSocketAddress.class, webSocketMessagingStubFactory);
        return factories;
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    public Address provideCCMessagingAddress(@Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST) String host,
                                             @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL) String protocol,
                                             @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT) int port,
                                             @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH) String path) {
        return new WebSocketAddress(WebSocketProtocol.valueOf(protocol.toUpperCase()), host, port, path);
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS)
    WebSocketClientAddress getLibjoynrMessagingAddress() {
        String messagingUUID = UUID.randomUUID().toString().replace("-", "");
        return new WebSocketClientAddress("libjoynr.messaging.participantid_" + messagingUUID);
    }
}
