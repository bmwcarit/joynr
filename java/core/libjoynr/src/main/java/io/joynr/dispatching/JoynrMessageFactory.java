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
import java.util.Map;

import joynr.JoynrMessage;
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

    public JoynrMessage createOneWay(final String fromParticipantId,
                                     final String toParticipantId,
                                     Object payload,
                                     ExpiryDate ttlExpirationDate) {
        JoynrMessage message = new JoynrMessage();

        message.setType(JoynrMessage.MESSAGE_TYPE_ONE_WAY);
        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(ttlExpirationDate.getValue()));
        message.setHeader(header);
        message.setPayload(serializePayload(payload));
        return message;

    }

    public JoynrMessage createRequest(final String fromParticipantId,
                                      final String toParticipantId,
                                      Request request,
                                      ExpiryDate expiryDate) {
        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);

        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(expiryDate.getValue()));

        message.setHeader(header);
        message.setPayload(serializePayload(request));

        return message;
    }

    public JoynrMessage createReply(final String fromParticipantId,
                                    final String toParticipantId,
                                    Reply payload,
                                    ExpiryDate expiryDate) {
        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_REPLY);
        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(expiryDate.getValue()));
        message.setHeader(header);
        message.setPayload(serializePayload(payload));
        return message;

    }

    public JoynrMessage createSubscriptionRequest(String fromParticipantId,
                                                  String toParticipantId,
                                                  SubscriptionRequest subscriptionRequest,
                                                  ExpiryDate expiryDate,
                                                  boolean broadcast) {
        JoynrMessage message = new JoynrMessage();

        if (broadcast) {
            message.setType(JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
        } else {
            message.setType(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST);
        }

        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(expiryDate.getValue()));
        message.setHeader(header);
        message.setPayload(serializePayload(subscriptionRequest));
        return message;
    }

    public JoynrMessage createPublication(String fromParticipantId,
                                          String toParticipantId,
                                          SubscriptionPublication publication,
                                          ExpiryDate expiryDate) {
        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_PUBLICATION);
        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(expiryDate.getValue()));
        message.setHeader(header);
        message.setPayload(serializePayload(publication));
        return message;
    }

    public JoynrMessage createSubscriptionStop(String fromParticipantId,
                                               String toParticipantId,
                                               SubscriptionStop subscriptionStop,
                                               ExpiryDate expiryDate) {
        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP);
        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(expiryDate.getValue()));
        message.setHeader(header);
        message.setPayload(serializePayload(subscriptionStop));
        return message;
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
