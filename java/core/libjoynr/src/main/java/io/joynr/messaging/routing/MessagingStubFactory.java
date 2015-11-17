package io.joynr.messaging.routing;

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

import com.google.inject.Inject;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingStub;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

import java.util.Map;

public class MessagingStubFactory {

    private Map<Class<? extends Address>, AbstractMessagingStubFactory> messagingStubFactories;

    @Inject
    public MessagingStubFactory(Map<Class<? extends Address>, AbstractMessagingStubFactory> messagingStubFactories) {
        this.messagingStubFactories = messagingStubFactories;
    }

    public IMessaging create(Address address) {
        IMessaging messagingStub;

        if (address instanceof ChannelAddress) {
            messagingStub = messagingStubFactories.get(ChannelAddress.class).create(address);
        } else if (address instanceof InProcessAddress) {
            messagingStub = new InProcessMessagingStub(((InProcessAddress) address).getSkeleton());
        } else if (address instanceof WebSocketAddress) {
            messagingStub = messagingStubFactories.get(WebSocketAddress.class).create(address);
        } else if (address instanceof WebSocketClientAddress) {
            messagingStub = messagingStubFactories.get(WebSocketClientAddress.class).create(address);
        } else {
            throw new JoynrMessageNotSentException("Failed to send Request: Address type not supported");
        }
        return messagingStub;
    }

    public void shutdown() {
        for (AbstractMessagingStubFactory messagingStubFactory : messagingStubFactories.values()) {
            messagingStubFactory.shutdown();
        }
    }
}
