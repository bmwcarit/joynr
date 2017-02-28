package io.joynr.dispatching;

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

import java.io.IOException;
import java.util.Map;
import java.util.Set;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.common.ExpiryDate;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import joynr.BroadcastSubscriptionRequest;
import joynr.JoynrMessage;
import joynr.MulticastPublication;
import joynr.MulticastSubscriptionRequest;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class JoynrMessageFactory {

    private static final String REQUEST_REPLY_ID_CUSTOM_HEADER = "z4";
    private final Set<JoynrMessageProcessor> messageProcessors;
    private ObjectMapper objectMapper;
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS)
    private long ttlUpliftMs = 0;

    private static final Logger logger = LoggerFactory.getLogger(JoynrMessageFactory.class);

    @Inject
    public JoynrMessageFactory(ObjectMapper objectMapper, Set<JoynrMessageProcessor> messageProcessors) {
        this.objectMapper = objectMapper;
        this.messageProcessors = messageProcessors;
    }

    private JoynrMessage createMessage(String joynrMessageType,
                                       String fromParticipantId,
                                       String toParticipantId,
                                       Object payload,
                                       MessagingQos messagingQos) {
        return createMessage(joynrMessageType, fromParticipantId, toParticipantId, payload, messagingQos, true);
    }

    private JoynrMessage createMessage(String joynrMessageType,
                                       String fromParticipantId,
                                       String toParticipantId,
                                       Object payload,
                                       MessagingQos messagingQos,
                                       boolean upliftTtl) {
        ExpiryDate expiryDate;
        if (!upliftTtl) {
            expiryDate = DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms());
        } else if (messagingQos.getRoundTripTtl_ms() > (Long.MAX_VALUE - ttlUpliftMs)) {
            expiryDate = DispatcherUtils.convertTtlToExpirationDate(Long.MAX_VALUE);
        } else {
            expiryDate = DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms() + ttlUpliftMs);
        }
        JoynrMessage message = new JoynrMessage();
        message.setType(joynrMessageType);
        Map<String, String> header = createHeader(fromParticipantId, toParticipantId);
        header.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(expiryDate.getValue()));
        if (messagingQos.getEffort() != null && !MessagingQosEffort.NORMAL.equals(messagingQos.getEffort())) {
            header.put(JoynrMessage.HEADER_NAME_EFFORT, String.valueOf(messagingQos.getEffort()));
        }
        message.setHeader(header);
        message.setPayload(serializePayload(payload));
        message.setCustomHeaders(messagingQos.getCustomMessageHeaders());
        for (JoynrMessageProcessor processor : messageProcessors) {
            message = processor.process(message);
        }
        return message;
    }

    public JoynrMessage createOneWayRequest(final String fromParticipantId,
                                            final String toParticipantId,
                                            OneWayRequest request,
                                            MessagingQos messagingQos) {
        return createMessage(JoynrMessage.MESSAGE_TYPE_ONE_WAY,
                             fromParticipantId,
                             toParticipantId,
                             request,
                             messagingQos);
    }

    public JoynrMessage createRequest(final String fromParticipantId,
                                      final String toParticipantId,
                                      Request request,
                                      MessagingQos messagingQos) {
        JoynrMessage msg = createMessage(JoynrMessage.MESSAGE_TYPE_REQUEST,
                                         fromParticipantId,
                                         toParticipantId,
                                         request,
                                         messagingQos);
        addRequestReplyIdCustomHeader(msg, request.getRequestReplyId());
        return msg;
    }

    private JoynrMessage addRequestReplyIdCustomHeader(JoynrMessage msg, String requestReplyId) {
        Map<String, String> customHeaders = Maps.newHashMap();
        customHeaders.put(REQUEST_REPLY_ID_CUSTOM_HEADER, requestReplyId);
        msg.setCustomHeaders(customHeaders);
        return msg;
    }

    public JoynrMessage createReply(final String fromParticipantId,
                                    final String toParticipantId,
                                    Reply reply,
                                    MessagingQos messagingQos) {
        JoynrMessage msg = createMessage(JoynrMessage.MESSAGE_TYPE_REPLY,
                                         fromParticipantId,
                                         toParticipantId,
                                         reply,
                                         messagingQos,
                                         false);
        addRequestReplyIdCustomHeader(msg, reply.getRequestReplyId());
        return msg;
    }

    public JoynrMessage createSubscriptionReply(final String fromParticipantId,
                                                final String toParticipantId,
                                                SubscriptionReply subscriptionReply,
                                                MessagingQos messagingQos) {
        JoynrMessage msg = createMessage(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REPLY,
                                         fromParticipantId,
                                         toParticipantId,
                                         subscriptionReply,
                                         messagingQos);
        addRequestReplyIdCustomHeader(msg, subscriptionReply.getSubscriptionId());
        return msg;
    }

    public JoynrMessage createSubscriptionRequest(String fromParticipantId,
                                                  String toParticipantId,
                                                  SubscriptionRequest subscriptionRequest,
                                                  MessagingQos messagingQos) {
        String messageType;
        if (subscriptionRequest instanceof BroadcastSubscriptionRequest) {
            messageType = JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        } else if (subscriptionRequest instanceof MulticastSubscriptionRequest) {
            messageType = JoynrMessage.MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
        } else {
            messageType = JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        }
        JoynrMessage msg = createMessage(messageType,
                                         fromParticipantId,
                                         toParticipantId,
                                         subscriptionRequest,
                                         messagingQos);
        addRequestReplyIdCustomHeader(msg, subscriptionRequest.getSubscriptionId());
        return msg;
    }

    public JoynrMessage createPublication(String fromParticipantId,
                                          String toParticipantId,
                                          SubscriptionPublication publication,
                                          MessagingQos messagingQos) {
        JoynrMessage msg = createMessage(JoynrMessage.MESSAGE_TYPE_PUBLICATION,
                                         fromParticipantId,
                                         toParticipantId,
                                         publication,
                                         messagingQos);
        addRequestReplyIdCustomHeader(msg, publication.getSubscriptionId());
        return msg;
    }

    public JoynrMessage createSubscriptionStop(String fromParticipantId,
                                               String toParticipantId,
                                               SubscriptionStop subscriptionStop,
                                               MessagingQos messagingQos) {
        JoynrMessage msg = createMessage(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP,
                                         fromParticipantId,
                                         toParticipantId,
                                         subscriptionStop,
                                         messagingQos);
        addRequestReplyIdCustomHeader(msg, subscriptionStop.getSubscriptionId());
        return msg;
    }

    public JoynrMessage createMulticast(String fromParticipantId,
                                        MulticastPublication multicastPublication,
                                        MessagingQos messagingQos) {
        return createMessage(JoynrMessage.MESSAGE_TYPE_MULTICAST,
                             fromParticipantId,
                             multicastPublication.getMulticastId(),
                             multicastPublication,
                             messagingQos);
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
