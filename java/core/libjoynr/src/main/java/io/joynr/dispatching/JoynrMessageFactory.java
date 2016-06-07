package io.joynr.dispatching;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.common.ExpiryDate;

import java.io.IOException;
import java.util.Collections;
import java.util.Map;

import joynr.JoynrMessage;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Maps;
import com.google.inject.Inject;

public class JoynrMessageFactory {

    private ObjectMapper objectMapper;

    private static final Logger logger = LoggerFactory.getLogger(JoynrMessageFactory.class);

    @Inject
    public JoynrMessageFactory(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    private JoynrMessage createMessage(String joynrMessageType,
                                       String fromParticipantId,
                                       String toParticipantId,
                                       Object payload,
                                       ExpiryDate ttlExpirationDate,
                                       Map<String, String> customHeaders) {
        JoynrMessage message = new JoynrMessage();
        message.setType(joynrMessageType);
        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(ttlExpirationDate.getValue()));
        message.setHeader(header);
        message.setPayload(serializePayload(payload));
        message.setCustomHeaders(customHeaders);
        return message;
    }

    private JoynrMessage createMessage(String messageType,
                                       String fromParticipantId,
                                       String toParticipantId,
                                       Object payload,
                                       ExpiryDate expiryDate) {
        return createMessage(messageType,
                             fromParticipantId,
                             toParticipantId,
                             payload,
                             expiryDate,
                             Collections.<String, String> emptyMap());
    }

    public JoynrMessage createOneWayRequest(final String fromParticipantId,
                                            final String toParticipantId,
                                            OneWayRequest request,
                                            ExpiryDate ttlExpirationDate,
                                            final Map<String, String> customHeaders) {
        return createMessage(JoynrMessage.MESSAGE_TYPE_ONE_WAY,
                             fromParticipantId,
                             toParticipantId,
                             request,
                             ttlExpirationDate,
                             customHeaders);
    }

    public JoynrMessage createRequest(final String fromParticipantId,
                                      final String toParticipantId,
                                      Request request,
                                      ExpiryDate expiryDate,
                                      final Map<String, String> customHeaders) {
        return createMessage(JoynrMessage.MESSAGE_TYPE_REQUEST,
                             fromParticipantId,
                             toParticipantId,
                             request,
                             expiryDate,
                             customHeaders);
    }

    public JoynrMessage createReply(final String fromParticipantId,
                                    final String toParticipantId,
                                    Reply reply,
                                    ExpiryDate expiryDate,
                                    final Map<String, String> customHeaders) {
        return createMessage(JoynrMessage.MESSAGE_TYPE_REPLY,
                             fromParticipantId,
                             toParticipantId,
                             reply,
                             expiryDate,
                             customHeaders);
    }

    public JoynrMessage createSubscriptionRequest(String fromParticipantId,
                                                  String toParticipantId,
                                                  SubscriptionRequest subscriptionRequest,
                                                  ExpiryDate expiryDate,
                                                  boolean broadcast) {
        String messageType;
        if (broadcast) {
            messageType = JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        } else {
            messageType = JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        }
        return createMessage(messageType, fromParticipantId, toParticipantId, subscriptionRequest, expiryDate);
    }

    public JoynrMessage createPublication(String fromParticipantId,
                                          String toParticipantId,
                                          SubscriptionPublication publication,
                                          ExpiryDate expiryDate) {
        return createMessage(JoynrMessage.MESSAGE_TYPE_PUBLICATION,
                             fromParticipantId,
                             toParticipantId,
                             publication,
                             expiryDate);
    }

    public JoynrMessage createSubscriptionStop(String fromParticipantId,
                                               String toParticipantId,
                                               SubscriptionStop subscriptionStop,
                                               ExpiryDate expiryDate) {
        return createMessage(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP,
                             fromParticipantId,
                             toParticipantId,
                             subscriptionStop,
                             expiryDate);
    }

    private Map<String, String> createHeader(final String fromParticipantId, final String toParticipantId) {
        Map<String, String> header = Maps.newHashMap();
        header.put(JoynrMessage.HEADER_NAME_CREATOR_USER_ID, "todo");
        header.put(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID, fromParticipantId);
        header.put(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_CONTENT_TYPE, JoynrMessage.CONTENT_TYPE_APPLICATION_JSON);

        return header;
    }

    private String serializePayload(Object payload) {
        // when using javax.annotatoins.NonNull annotation on capablities parameter it will
        // cause a NoSuchMethodError
        assert (payload != null);

        String serializedPayload;
        try {
            if (payload.getClass() == String.class) {
                serializedPayload = (String) payload;
            } else {
                serializedPayload = objectMapper.writeValueAsString(payload);
                logger.trace("serializePayload as: {}", serializedPayload);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return serializedPayload;
    }

}
