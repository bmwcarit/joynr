package io.joynr.messaging.channel;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static joynr.JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.JoynrMessageSerializer;
import io.joynr.messaging.http.HttpMessageSender;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.ChannelAddress;

public class ChannelMessagingStub implements IMessaging {

    private ChannelAddress address;
    private JoynrMessageSerializer messageSerializer;
    private HttpMessageSender httpMessageSender;
    private ChannelAddress replyToAddress;

    public ChannelMessagingStub(ChannelAddress address,
                                ChannelAddress replyToAddress,
                                JoynrMessageSerializer messageSerializer,
                                HttpMessageSender httpMessageSender) {
        this.address = address;
        this.replyToAddress = replyToAddress;
        this.messageSerializer = messageSerializer;
        this.httpMessageSender = httpMessageSender;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        setReplyTo(message);
        String serializedMessage = messageSerializer.serialize(message);
        transmit(serializedMessage, failureAction);
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        httpMessageSender.sendMessage(address, serializedMessage, failureAction);
    }

    private void setReplyTo(JoynrMessage message) {
        String type = message.getType();
        if (type != null
                && message.getReplyTo() == null
                && (type.equals(MESSAGE_TYPE_REQUEST) || type.equals(MESSAGE_TYPE_SUBSCRIPTION_REQUEST) || type.equals(MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST))) {
            message.setReplyTo(replyToAddress.getChannelId());
        }
    }
}
