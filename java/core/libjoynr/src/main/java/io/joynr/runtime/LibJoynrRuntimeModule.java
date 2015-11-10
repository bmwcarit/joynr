package io.joynr.runtime;

import java.util.Map;

import javax.inject.Named;

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

import com.google.common.collect.Maps;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Names;

import io.joynr.accesscontrol.AccessController;
import io.joynr.accesscontrol.AccessControllerDummy;
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.websocket.LibWebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import io.joynr.messaging.websocket.WebsocketModule;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

/**
 *  Use this module if you want to start a lib joynr instance which connects to a cluster controller by websockets
 */
public class LibJoynrRuntimeModule extends DefaultRuntimeModule {

    @Override
    protected void configure() {
        super.configure();
        bind(AccessController.class).to(AccessControllerDummy.class).in(Singleton.class);
        bind(JoynrRuntime.class).to(LibJoynrRuntime.class).in(Singleton.class);
        bind(WebSocketMessagingSkeleton.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_SKELETON))
                                              .to(LibWebSocketMessagingSkeleton.class)
                                              .in(Singleton.class);
        bind(WebSocketMessagingStubFactory.class).in(Singleton.class);
    }

    @Provides
    @Singleton
    Map<Class<? extends Address>, AbstractMessagingStubFactory> provideMessagingStubFactories(WebSocketMessagingStubFactory webSocketMessagingStubFactory) {
        Map<Class<? extends Address>, AbstractMessagingStubFactory> factories = Maps.newHashMap();
        factories.put(WebSocketAddress.class, webSocketMessagingStubFactory);
        return factories;
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_ADDRESS)
    Address getDiscoveryProviderAddress(@Named(ConfigurableMessagingSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address cc_address) {
        return cc_address;
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_CC_ROUTING_PROVIDER_ADDRESS)
    Address getRoutingProviderAddress(@Named(ConfigurableMessagingSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address cc_address) {
        return cc_address;
    }

    @Provides
    @Named(ConfigurableMessagingSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    public Address provideCCMessagingAddress(@Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST) String host,
                                             @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL) String protocol,
                                             @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT) int port,
                                             @Named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH) String path) {
        return new WebSocketAddress(WebSocketProtocol.valueOf(protocol.toUpperCase()), host, port, path);
    }
}
