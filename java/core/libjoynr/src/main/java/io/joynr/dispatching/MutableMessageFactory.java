/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.dispatching;

import static joynr.Message.CUSTOM_HEADER_REQUEST_REPLY_ID;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.common.ExpiryDate;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.util.ObjectMapper;
import joynr.BroadcastSubscriptionRequest;
import joynr.Message;
import joynr.MulticastPublication;
import joynr.MutableMessage;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;

public class MutableMessageFactory {
    private final Set<JoynrMessageProcessor> messageProcessors;
    private ObjectMapper objectMapper;
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS)
    private long ttlUpliftMs = 0;

    private static final Logger logger = LoggerFactory.getLogger(MutableMessageFactory.class);

    @Inject
    public MutableMessageFactory(ObjectMapper objectMapper, Set<JoynrMessageProcessor> messageProcessors) {
        this.objectMapper = new ObjectMapper(objectMapper);
        this.messageProcessors = new HashSet<>(messageProcessors);
    }

    private MutableMessage createMessage(final Message.MessageType joynrMessageType,
                                         final String fromParticipantId,
                                         final String toParticipantId,
                                         final Object payload,
                                         final MessagingQos messagingQos,
                                         final ExpiryDate expiryDate) {
        return createMessage(joynrMessageType,
                             fromParticipantId,
                             toParticipantId,
                             payload,
                             messagingQos,
                             true,
                             expiryDate);
    }

    private MutableMessage createMessage(final Message.MessageType joynrMessageType,
                                         final String fromParticipantId,
                                         final String toParticipantId,
                                         final Object payload,
                                         final MessagingQos messagingQos,
                                         final boolean upliftTtl,
                                         final ExpiryDate expiryDate) {
        final ExpiryDate adjustedExpiryDate = adjustExpiryDate(expiryDate, upliftTtl);

        MutableMessage message = new MutableMessage();
        message.setType(joynrMessageType);
        if (messagingQos.getEffort() != null && !MessagingQosEffort.NORMAL.equals(messagingQos.getEffort())) {
            message.setEffort(String.valueOf(messagingQos.getEffort()));
        }
        message.setSender(fromParticipantId);
        message.setRecipient(toParticipantId);
        message.setTtlAbsolute(true);
        message.setTtlMs(adjustedExpiryDate.getValue());
        message.setPayload(serializePayload(payload));
        message.setCustomHeaders(messagingQos.getCustomMessageHeaders());
        message.setCompressed(messagingQos.getCompress());
        for (JoynrMessageProcessor processor : messageProcessors) {
            message = processor.processOutgoing(message);
        }

        logger.trace("Message {} has expiry date: {}", message.getId(), adjustedExpiryDate);

        return message;
    }

    private ExpiryDate adjustExpiryDate(final ExpiryDate expiryDate, final boolean upliftTtl) {
        if (!upliftTtl) {
            return expiryDate;
        } else if (expiryDate.getRelativeTtl() > (Long.MAX_VALUE - ttlUpliftMs)) {
            return ExpiryDate.fromRelativeTtl(Long.MAX_VALUE);
        } else {
            return ExpiryDate.fromRelativeTtl(expiryDate.getRelativeTtl() + ttlUpliftMs);
        }
    }

    public MutableMessage createOneWayRequest(final String fromParticipantId,
                                              final String toParticipantId,
                                              final OneWayRequest request,
                                              final MessagingQos messagingQos) {
        final ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        return createOneWayRequest(fromParticipantId, toParticipantId, request, messagingQos, expiryDate);
    }

    public MutableMessage createOneWayRequest(final String fromParticipantId,
                                              final String toParticipantId,
                                              final OneWayRequest request,
                                              final MessagingQos messagingQos,
                                              final ExpiryDate expiryDate) {
        return createMessage(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY,
                             fromParticipantId,
                             toParticipantId,
                             request,
                             messagingQos,
                             expiryDate);
    }

    public MutableMessage createRequest(final String fromParticipantId,
                                        final String toParticipantId,
                                        final Request request,
                                        final MessagingQos messagingQos) {
        final ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        return createRequest(fromParticipantId, toParticipantId, request, messagingQos, expiryDate);
    }

    public MutableMessage createRequest(final String fromParticipantId,
                                        final String toParticipantId,
                                        final Request request,
                                        final MessagingQos messagingQos,
                                        final ExpiryDate expiryDate) {
        MutableMessage msg = createMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                           fromParticipantId,
                                           toParticipantId,
                                           request,
                                           messagingQos,
                                           expiryDate);
        addRequestReplyIdCustomHeader(msg, request.getRequestReplyId());
        return msg;
    }

    private MutableMessage addRequestReplyIdCustomHeader(MutableMessage msg, String requestReplyId) {
        Map<String, String> customHeaders = new HashMap<>();
        customHeaders.put(CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId);
        msg.setCustomHeaders(customHeaders);
        return msg;
    }

    public MutableMessage createReply(final String fromParticipantId,
                                      final String toParticipantId,
                                      final Reply reply,
                                      final MessagingQos messagingQos) {
        final ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());
        return createReply(fromParticipantId, toParticipantId, reply, messagingQos, expiryDate);
    }

    public MutableMessage createReply(final String fromParticipantId,
                                      final String toParticipantId,
                                      final Reply reply,
                                      final MessagingQos messagingQos,
                                      final ExpiryDate expiryDate) {
        MutableMessage msg = createMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY,
                                           fromParticipantId,
                                           toParticipantId,
                                           reply,
                                           messagingQos,
                                           false,
                                           expiryDate);
        addRequestReplyIdCustomHeader(msg, reply.getRequestReplyId());
        return msg;
    }

    public MutableMessage createSubscriptionReply(final String fromParticipantId,
                                                  final String toParticipantId,
                                                  SubscriptionReply subscriptionReply,
                                                  MessagingQos messagingQos) {
        MutableMessage msg = createMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY,
                                           fromParticipantId,
                                           toParticipantId,
                                           subscriptionReply,
                                           messagingQos,
                                           ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms()));
        addRequestReplyIdCustomHeader(msg, subscriptionReply.getSubscriptionId());
        return msg;
    }

    public MutableMessage createSubscriptionRequest(String fromParticipantId,
                                                    String toParticipantId,
                                                    SubscriptionRequest subscriptionRequest,
                                                    MessagingQos messagingQos) {
        Message.MessageType messageType;
        if (subscriptionRequest instanceof BroadcastSubscriptionRequest) {
            messageType = Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        } else {
            messageType = Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        }
        MutableMessage msg = createMessage(messageType,
                                           fromParticipantId,
                                           toParticipantId,
                                           subscriptionRequest,
                                           messagingQos,
                                           ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms()));
        addRequestReplyIdCustomHeader(msg, subscriptionRequest.getSubscriptionId());
        return msg;
    }

    public MutableMessage createPublication(String fromParticipantId,
                                            String toParticipantId,
                                            SubscriptionPublication publication,
                                            MessagingQos messagingQos) {
        MutableMessage msg = createMessage(Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION,
                                           fromParticipantId,
                                           toParticipantId,
                                           publication,
                                           messagingQos,
                                           ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms()));
        addRequestReplyIdCustomHeader(msg, publication.getSubscriptionId());
        return msg;
    }

    public MutableMessage createSubscriptionStop(String fromParticipantId,
                                                 String toParticipantId,
                                                 SubscriptionStop subscriptionStop,
                                                 MessagingQos messagingQos) {
        MutableMessage msg = createMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP,
                                           fromParticipantId,
                                           toParticipantId,
                                           subscriptionStop,
                                           messagingQos,
                                           ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms()));
        addRequestReplyIdCustomHeader(msg, subscriptionStop.getSubscriptionId());
        return msg;
    }

    public MutableMessage createMulticast(String fromParticipantId,
                                          MulticastPublication multicastPublication,
                                          MessagingQos messagingQos) {
        return createMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST,
                             fromParticipantId,
                             multicastPublication.getMulticastId(),
                             multicastPublication,
                             messagingQos,
                             ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms()));
    }

    private byte[] serializePayload(Object payload) {
        // when using javax.annotatoins.NonNull annotation on capabilities parameter it will
        // cause a NoSuchMethodError
        assert (payload != null);

        String serializedPayload;
        try {
            if (payload.getClass() == String.class) {
                serializedPayload = (String) payload;
            } else {
                serializedPayload = objectMapper.writeValueAsString(payload);
                logger.trace("SerializePayload as: {}", serializedPayload);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return serializedPayload.getBytes(StandardCharsets.UTF_8);
    }
}
