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
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

import java.io.IOException;

public class MessagingStubFactory {

    private MessageSender messageSender;

    @Inject
    public MessagingStubFactory(MessageSender messageSender) {
        this.messageSender = messageSender;
    }

    public IMessaging create(Address address) {
        IMessaging messagingStub;

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
        } else {
            throw new JoynrMessageNotSentException("Failed to send Request: Address type not supported");
        }
        return messagingStub;
    }

    public void shutdown() {
        messageSender.shutdown();
    }
}
