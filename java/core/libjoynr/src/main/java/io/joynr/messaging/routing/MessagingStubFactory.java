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
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingStub;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketAddress;

import java.io.IOException;

public class MessagingStubFactory {

    private final WebSocketMessagingStubFactory webSocketMessagingStubFactory;
    private MessageSender messageSender;

    @Inject
    public MessagingStubFactory(MessageSender messageSender) {
        this.messageSender = messageSender;
        //TODO create a injected map of factories
        webSocketMessagingStubFactory = new WebSocketMessagingStubFactory();
    }

    public IMessaging create(Address address) {
        IMessaging messagingStub;

        //TODO move creation to {Middleware}MessagingStubFactory.java and inject a list of Factories
        if (address instanceof ChannelAddress) {
            final String destinationChannelId = ((ChannelAddress) address).getChannelId();
            messagingStub = new IMessaging() {
                @Override
                public void transmit(JoynrMessage message) throws IOException {
                    messageSender.sendMessage(destinationChannelId, message);
                }
            };
        } else if (address instanceof InProcessAddress) {
            messagingStub = new InProcessMessagingStub(((InProcessAddress) address).getSkeleton());
        } else if (address instanceof WebSocketAddress) {
            messagingStub = webSocketMessagingStubFactory.create((WebSocketAddress) address);
        } else {
            throw new JoynrMessageNotSentException("Failed to send Request: Address type not supported");
        }
        return messagingStub;
    }

    public void shutdown() {
        messageSender.shutdown();
    }
}
