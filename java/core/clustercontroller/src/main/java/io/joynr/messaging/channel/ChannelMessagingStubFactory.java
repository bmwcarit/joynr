package io.joynr.messaging.channel;

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
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.MessageHandler;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.ChannelAddress;

import java.io.IOException;

/**
 *  Message stub factory for joynr channel addresses
 */
public class ChannelMessagingStubFactory extends AbstractMessagingStubFactory<ChannelAddress> {
    private MessageHandler messageSender;

    @Inject
    public ChannelMessagingStubFactory(MessageHandler messageSender) {
        this.messageSender = messageSender;
    }

    @Override
    protected IMessaging createInternal(final ChannelAddress address) {
        IMessaging messagingStub = new IMessaging() {
            @Override
            public void transmit(JoynrMessage message) throws IOException {
                messageSender.sendMessage(address, message);
            }
        };
        return messagingStub;
    }

    @Override
    public void shutdown() {
        messageSender.shutdown();
    }
}
