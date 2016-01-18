package io.joynr.messaging;

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

import javax.annotation.Nullable;

import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

/**
 * Storage class for all data (channelId, serializedMessage, ttl, time stamp of last sending attempt), needed to send
 * out a message. Besides the serialized version of the original message, the remaining part of the ttl is stored and
 * updated by calling updateTtl(). Each time an attempt to send the message is made the current time has to be stored by
 * calling updateLastTryTimeMs().
 */

public class MessageContainer {
    @SuppressWarnings("unused")
    private static final Logger logger = LoggerFactory.getLogger(MessageContainer.class);

    private final Address address;
    private final String serializedMessage;
    private long expiryDate;

    private final String messageId;

    private JoynrMessage message;

    private int retries = 0;

    public MessageContainer(final Address address,
                            final JoynrMessage message,
                            final long expiryDate,
                            ObjectMapper objectMapper) throws JsonProcessingException {
        this.address = address;
        this.message = message;
        this.serializedMessage = objectMapper.writeValueAsString(message); // jsonConverter.toJson(message);
        this.expiryDate = expiryDate;
        this.messageId = message.getHeaderValue(JoynrMessage.HEADER_NAME_MESSAGE_ID);
    }

    public boolean isExpired() {
        return System.currentTimeMillis() > expiryDate;
    }

    public long getExpiryDate() {
        return expiryDate;
    }

    public Address getAddress() {
        return address;
    }

    public String getSerializedMessage() {
        return serializedMessage;

    }

    @Nullable
    public String getMessageId() {
        return messageId;
    }

    @Override
    public String toString() {
        return message.toString();
    }

    public int getRetries() {
        return retries;
    }

    public void incrementRetries() {
        retries++;
    }
}
