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

import com.google.common.collect.Maps;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Names;
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.websocket.CCWebSocketMessagingSkeleton;
import io.joynr.messaging.websocket.WebSocketClientMessagingStubFactory;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import joynr.system.RoutingTypes.WebSocketProtocol;

import javax.inject.Named;
import java.util.Map;

/**
 *
 */
public class CCRuntimeModule extends DefaultRuntimeModule {
    @Override
    protected void configure() {
        super.configure();
        bind(CCJoynrRuntime.class).in(Singleton.class);
        bind(JoynrRuntime.class).to(CCJoynrRuntime.class);
        bind(WebSocketMessagingSkeleton.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_CLUSTERCONTROLER_MESSAGING_SKELETON))
                                              .to(CCWebSocketMessagingSkeleton.class)
                                              .in(Singleton.class);
        bind(WebSocketClientMessagingStubFactory.class).in(Singleton.class);
    }

    @Provides
    @Singleton
    Map<Class<? extends Address>, AbstractMessagingStubFactory> provideMessagingStubFactories(WebSocketClientMessagingStubFactory webSocketClientMessagingStubFactory,
                                                                                              ChannelMessagingStubFactory channelMessagingStubFactory) {
        Map<Class<? extends Address>, AbstractMessagingStubFactory> factories = Maps.newHashMap();
        factories.put(WebSocketClientAddress.class, webSocketClientMessagingStubFactory);
        factories.put(ChannelAddress.class, channelMessagingStubFactory);
        return factories;
    }

    @Provides
    @Named(ConfigurableMessagingSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    public WebSocketAddress provideCCMessagingAddress(@Named(MessagingPropertyKeys.CC_MESSAGING_HOST) String host,
                                                      @Named(MessagingPropertyKeys.CC_MESSAGING_PROTOCOL) String protocol,
                                                      @Named(MessagingPropertyKeys.CC_MESSAGING_PORT) int port,
                                                      @Named(MessagingPropertyKeys.CC_MESSAGING_PATH) String path) {
        return new WebSocketAddress(WebSocketProtocol.valueOf(protocol.toUpperCase()), host, port, path);
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_ADDRESS)
    Address getDiscoveryProviderAddress() {
        return new InProcessAddress();
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_CC_ROUTING_PROVIDER_ADDRESS)
    Address getRoutingProviderAddress() {
        return new InProcessAddress();
    }
}
