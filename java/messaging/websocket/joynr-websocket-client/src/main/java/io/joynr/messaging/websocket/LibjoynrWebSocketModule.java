package io.joynr.messaging.websocket;

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

import java.util.UUID;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;

import io.joynr.runtime.SystemServicesSettings;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketClientAddress;

public class LibjoynrWebSocketModule extends AbstractModule {

    String messagingUUID = UUID.randomUUID().toString().replace("-", "");

    @Override
    protected void configure() {
        install(new WebsocketModule());
    }

    @Provides
    @Named(WebsocketModule.WEBSOCKET_CLIENT_ADDRESS)
    WebSocketClientAddress getWebSocketClientAddress() {
        return new WebSocketClientAddress("libjoynr.messaging.participantid_" + messagingUUID);
    }

    @Provides
    @Named(SystemServicesSettings.LIBJOYNR_MESSAGING_ADDRESS)
    Address getLibjoynrMessagingAddress() {
        return getWebSocketClientAddress();
    }

}
