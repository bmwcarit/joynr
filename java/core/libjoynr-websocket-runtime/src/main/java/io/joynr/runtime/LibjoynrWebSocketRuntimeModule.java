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
package io.joynr.runtime;

import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;

import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import com.google.inject.name.Names;

import io.joynr.messaging.GbidArrayFactory;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.routing.DummyRoutingTable;
import io.joynr.messaging.routing.LibJoynrMessageRouter;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.sender.LibJoynrMessageSender;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import io.joynr.messaging.websocket.WebSocketMulticastAddressCalculator;
import io.joynr.messaging.websocket.WebsocketMessagingSkeletonFactory;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.messaging.websocket.jetty.client.WebSocketJettyClientModule;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

/**
 *  Use this module if you want to start a lib joynr instance which connects to a cluster controller by websockets
 */
public class LibjoynrWebSocketRuntimeModule extends AbstractRuntimeModule {

    @Override
    protected void configure() {
        super.configure();
        install(new WebSocketJettyClientModule());
        bind(JoynrRuntime.class).to(LibjoynrRuntime.class).in(Singleton.class);
        bind(LibJoynrMessageRouter.class).in(Singleton.class);
        bind(MessageRouter.class).to(LibJoynrMessageRouter.class);
        bind(MulticastReceiverRegistrar.class).to(LibJoynrMessageRouter.class);
        bind(MessageSender.class).to(LibJoynrMessageSender.class);
        bind(RoutingTable.class).to(DummyRoutingTable.class).asEagerSingleton();
        bind(Boolean.class).annotatedWith(Names.named(WebSocketMessagingSkeleton.WEBSOCKET_IS_MAIN_TRANSPORT))
                           .toInstance(Boolean.TRUE);

        messagingSkeletonFactory.addBinding(WebSocketAddress.class).to(WebsocketMessagingSkeletonFactory.class);
        messagingStubFactory.addBinding(WebSocketAddress.class).to(WebSocketMessagingStubFactory.class);
        multicastAddressCalculators.addBinding().to(WebSocketMulticastAddressCalculator.class);
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
    @Named(GBID_ARRAY)
    public String[] provideGbidArray(GbidArrayFactory gbidArrayFactory) {
        return gbidArrayFactory.create();
    }
}
