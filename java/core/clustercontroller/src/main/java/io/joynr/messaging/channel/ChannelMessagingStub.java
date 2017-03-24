package io.joynr.messaging.channel;

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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.JoynrMessageSerializer;
import io.joynr.messaging.http.HttpMessageSender;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.ChannelAddress;

public class ChannelMessagingStub implements IMessaging {
    private static final Logger LOG = LoggerFactory.getLogger(ChannelMessagingStub.class);

    private ChannelAddress address;
    private JoynrMessageSerializer messageSerializer;
    private HttpMessageSender httpMessageSender;

    public ChannelMessagingStub(ChannelAddress address,
                                JoynrMessageSerializer messageSerializer,
                                HttpMessageSender httpMessageSender) {
        this.address = address;
        this.messageSerializer = messageSerializer;
        this.httpMessageSender = httpMessageSender;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        LOG.debug(">>> OUTGOING >>> {}", message.toLogMessage());
        String serializedMessage = messageSerializer.serialize(message);
        transmit(serializedMessage, failureAction);
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        LOG.debug(">>> OUTGOING >>> {}", serializedMessage);
        httpMessageSender.sendMessage(address, serializedMessage, failureAction);
    }
}
