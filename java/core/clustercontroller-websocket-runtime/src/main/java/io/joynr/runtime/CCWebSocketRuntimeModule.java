/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.websocket.WebSocketClientMessagingStubFactory;
import io.joynr.messaging.websocket.WebsocketMessagingSkeletonFactory;
import io.joynr.messaging.websocket.server.WebSocketJettyServerModule;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketClientAddress;

/**
 *
 */
public class CCWebSocketRuntimeModule extends ClusterControllerRuntimeModule {
    @Override
    protected void configure() {
        super.configure();
        install(new WebSocketJettyServerModule());
        bind(JoynrRuntime.class).to(ClusterControllerRuntime.class);
        bind(ClusterControllerRuntime.class).in(Singleton.class);

        messagingSkeletonFactory.addBinding(WebSocketClientAddress.class).to(WebsocketMessagingSkeletonFactory.class);
        messagingStubFactory.addBinding(WebSocketClientAddress.class).to(WebSocketClientMessagingStubFactory.class);

    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    Address provideCCMessagingAddress() {
        return new InProcessAddress();
    }

}
