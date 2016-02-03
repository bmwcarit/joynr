package io.joynr.messaging.channel;

import static joynr.JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_REQUEST;
import static joynr.JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST;

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

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrSerializationException;
import io.joynr.messaging.JoynrMessageSerializer;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.serialize.AbstractMiddlewareMessageSerializerFactory;
import io.joynr.messaging.serialize.JsonSerializer;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.ChannelAddress;

public class ChannelMessageSerializerFactory extends AbstractMiddlewareMessageSerializerFactory<ChannelAddress> {

    private JsonSerializer jsonSerializer;
    private String replyToChannelId;

    @Inject
    public ChannelMessageSerializerFactory(@Named(MessagingPropertyKeys.CHANNELID) String replyToChannelId,
                                           JsonSerializer jsonSerializer) {
        this.replyToChannelId = replyToChannelId;
        this.jsonSerializer = jsonSerializer;
    }

    @Override
    protected JoynrMessageSerializer createInternal(ChannelAddress address) {
        return new JoynrMessageSerializer() {

            @Override
            public String serialize(JoynrMessage message) throws JoynrSerializationException {
                String type = message.getType();
                if (type != null
                        && (type.equals(MESSAGE_TYPE_REQUEST) || type.equals(MESSAGE_TYPE_SUBSCRIPTION_REQUEST) || type.equals(MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST))) {
                    message.setReplyTo(replyToChannelId);
                }

                return jsonSerializer.serialize(message);
            }

            @Override
            public JoynrMessage deserialize(String serializedMessage) throws JoynrSerializationException {
                return jsonSerializer.deserialize(serializedMessage);
            }
        };
    }

}
