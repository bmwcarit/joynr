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
package io.joynr.dispatching;

import java.io.IOException;
import java.io.Serializable;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.inject.Singleton;

import io.joynr.messaging.MulticastReceiverRegistrar;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.provider.ProviderCallback;
import io.joynr.proxy.StatelessAsyncIdCalculator;
import io.joynr.smrf.EncodingException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MulticastPublication;
import joynr.MulticastSubscriptionRequest;
import joynr.MutableMessage;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.types.DiscoveryEntryWithMetaInfo;

public class DispatcherImpl implements Dispatcher {

    private static final Logger logger = LoggerFactory.getLogger(DispatcherImpl.class);
    private final MutableMessageFactory messageFactory;
    private final StatelessAsyncIdCalculator statelessAsyncIdCalculator;
    private RequestReplyManager requestReplyManager;
    private SubscriptionManager subscriptionManager;
    private PublicationManager publicationManager;
    private final MulticastReceiverRegistrar multicastReceiverRegistrar;
    private final MessageSender messageSender;
    private ObjectMapper objectMapper;
    private boolean overrideCompress;

    @Inject
    @Singleton
    // CHECKSTYLE:OFF
    public DispatcherImpl(RequestReplyManager requestReplyManager,
                          SubscriptionManager subscriptionManager,
                          PublicationManager publicationManager,
                          MulticastReceiverRegistrar multicastReceiverRegistrar,
                          MessageSender messageSender,
                          MutableMessageFactory messageFactory,
                          ObjectMapper objectMapper,
                          @Named(MessagingPropertyKeys.PROPERTY_MESSAGING_COMPRESS_REPLIES) boolean overrideCompress,
                          StatelessAsyncIdCalculator statelessAsyncIdCalculator) {
        this.requestReplyManager = requestReplyManager;
        this.subscriptionManager = subscriptionManager;
        this.publicationManager = publicationManager;
        this.multicastReceiverRegistrar = multicastReceiverRegistrar;
        this.messageSender = messageSender;
        this.messageFactory = messageFactory;
        this.objectMapper = objectMapper;
        this.overrideCompress = overrideCompress;
        this.statelessAsyncIdCalculator = statelessAsyncIdCalculator;
    }

    // CHECKSTYLE:ON

    @Override
    public void sendSubscriptionRequest(String fromParticipantId,
                                        Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                        SubscriptionRequest subscriptionRequest,
                                        MessagingQos messagingQos) {
        for (DiscoveryEntryWithMetaInfo toDiscoveryEntry : toDiscoveryEntries) {
            MutableMessage message = messageFactory.createSubscriptionRequest(fromParticipantId,
                                                                              toDiscoveryEntry.getParticipantId(),
                                                                              subscriptionRequest,
                                                                              messagingQos);
            message.setLocalMessage(toDiscoveryEntry.getIsLocal());

            if (subscriptionRequest instanceof MulticastSubscriptionRequest) {
                String multicastId = ((MulticastSubscriptionRequest) subscriptionRequest).getMulticastId();
                logger.debug("REGISTER MULTICAST SUBSCRIPTION: subscriptionId: {}, multicastId {}, subscribedToName: {}, subscriptionQos.expiryDate: {}, proxy participantId: {}, provider participantId: {}, domain {}, interfaceName {}, {}",
                             subscriptionRequest.getSubscriptionId(),
                             multicastId,
                             subscriptionRequest.getSubscribedToName(),
                             (subscriptionRequest.getQos() == null) ? 0
                                     : subscriptionRequest.getQos().getExpiryDateMs(),
                             message.getId(),
                             fromParticipantId,
                             toDiscoveryEntry.getParticipantId(),
                             toDiscoveryEntry.getDomain(),
                             toDiscoveryEntry.getInterfaceName(),
                             toDiscoveryEntry.getProviderVersion());
                multicastReceiverRegistrar.addMulticastReceiver(multicastId,
                                                                fromParticipantId,
                                                                toDiscoveryEntry.getParticipantId());
                SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionRequest.getSubscriptionId());
                sendSubscriptionReply(toDiscoveryEntry.getParticipantId(),
                                      fromParticipantId,
                                      subscriptionReply,
                                      messagingQos);
            } else {
                logger.debug("REGISTER SUBSCRIPTION call proxy: subscriptionId: {}, subscribedToName: {}, subscriptionQos.expiryDate: {}, messageId: {}, proxy participantId: {}, provider participantId: {}, domain {}, interfaceName {}, {}",
                             subscriptionRequest.getSubscriptionId(),
                             subscriptionRequest.getSubscribedToName(),
                             (subscriptionRequest.getQos() == null) ? 0
                                     : subscriptionRequest.getQos().getExpiryDateMs(),
                             message.getId(),
                             fromParticipantId,
                             toDiscoveryEntry.getParticipantId(),
                             toDiscoveryEntry.getDomain(),
                             toDiscoveryEntry.getInterfaceName(),
                             toDiscoveryEntry.getProviderVersion());
                messageSender.sendMessage(message);
            }
        }
    }

    @Override
    public void sendSubscriptionStop(String fromParticipantId,
                                     Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                     SubscriptionStop subscriptionStop,
                                     MessagingQos messagingQos) {
        for (DiscoveryEntryWithMetaInfo toDiscoveryEntry : toDiscoveryEntries) {
            MutableMessage message = messageFactory.createSubscriptionStop(fromParticipantId,
                                                                           toDiscoveryEntry.getParticipantId(),
                                                                           subscriptionStop,
                                                                           messagingQos);
            message.setLocalMessage(toDiscoveryEntry.getIsLocal());
            logger.debug("UNREGISTER SUBSCRIPTION call proxy: subscriptionId: {}, messageId: {}, proxy participantId: {}, provider participantId: {}, domain {}, interfaceName {}, {}",
                         subscriptionStop.getSubscriptionId(),
                         message.getId(),
                         fromParticipantId,
                         toDiscoveryEntry.getParticipantId(),
                         toDiscoveryEntry.getDomain(),
                         toDiscoveryEntry.getInterfaceName(),
                         toDiscoveryEntry.getProviderVersion());
            messageSender.sendMessage(message);
        }

    }

    @Override
    public void sendSubscriptionPublication(String fromParticipantId,
                                            Set<String> toParticipantIds,
                                            SubscriptionPublication publication,
                                            MessagingQos messagingQos) {

        for (String toParticipantId : toParticipantIds) {
            MutableMessage message = messageFactory.createPublication(fromParticipantId,
                                                                      toParticipantId,
                                                                      publication,
                                                                      messagingQos);
            messageSender.sendMessage(message);
        }
    }

    public void sendReply(final String fromParticipantId,
                          final String toParticipantId,
                          Reply reply,
                          final long expiryDateMs,
                          Map<String, String> customHeaders,
                          final MessagingQosEffort effort,
                          boolean compress) throws IOException {
        MessagingQos messagingQos = new MessagingQos(expiryDateMs - System.currentTimeMillis(), effort);
        messagingQos.getCustomMessageHeaders().putAll(customHeaders);
        if (overrideCompress) {
            compress = true;
        }
        messagingQos.setCompress(compress);
        MutableMessage message = messageFactory.createReply(fromParticipantId, toParticipantId, reply, messagingQos);
        messageSender.sendMessage(message);
    }

    @Override
    public void sendSubscriptionReply(final String fromParticipantId,
                                      final String toParticipantId,
                                      SubscriptionReply subscriptionReply,
                                      MessagingQos messagingQos) {

        MutableMessage message = messageFactory.createSubscriptionReply(fromParticipantId,
                                                                        toParticipantId,
                                                                        subscriptionReply,
                                                                        messagingQos);
        messageSender.sendMessage(message);
    }

    private MessagingQosEffort getEffort(final ImmutableMessage message) {
        String effortString = message.getEffort();
        if (effortString == null) {
            return null;
        }
        try {
            return MessagingQosEffort.valueOf(effortString);
        } catch (IllegalArgumentException e) {
            logger.error("Received message ({}) with invalid effort: {}. Using default effort for reply message.",
                         message.getTrackingInfo(),
                         effortString);
            return null;
        }
    }

    @Override
    public void messageArrived(final ImmutableMessage message) {
        if (message == null) {
            logger.error("Received message was null");
            return;
        }

        if (!message.isTtlAbsolute()) {
            logger.error("Received message with relative ttl (not supported): {}", message.getTrackingInfo());
            return;
        }

        final long expiryDate = message.getTtlMs();
        final Map<String, String> customHeaders = message.getCustomHeaders();
        if (DispatcherUtils.isExpired(expiryDate)) {
            if (logger.isTraceEnabled()) {
                logger.trace("TTL expired, discarding message : {}", message);
            } else {
                logger.debug("TTL expired, discarding message : {}", message.getTrackingInfo());
            }
            return;
        }

        String payload;

        try {
            payload = new String(message.getUnencryptedBody(), StandardCharsets.UTF_8);
        } catch (EncodingException e) {
            logger.error("Error reading SMRF message. msgId: {}. from: {} to: {}. Discarding joynr message. Error:",
                         message.getId(),
                         message.getSender(),
                         message.getRecipient(),
                         e);
            return;
        }

        Message.MessageType type = message.getType();
        try {
            if (Message.MessageType.VALUE_MESSAGE_TYPE_REPLY.equals(type)) {
                Reply reply = objectMapper.readValue(payload, Reply.class);
                if (reply.getRequestReplyId().contains(StatelessAsyncIdCalculator.REQUEST_REPLY_ID_SEPARATOR)) {
                    addStatelessCallback(message, reply);
                }
                logger.trace("Parsed reply from message payload: {}", payload);
                handle(reply);
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY.equals(type)) {
                SubscriptionReply subscriptionReply = objectMapper.readValue(payload, SubscriptionReply.class);
                logger.trace("Parsed subscription reply from message payload: {}", payload);
                handle(subscriptionReply);
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST.equals(type)) {
                MessagingQosEffort effort = getEffort(message);
                final Request request = objectMapper.readValue(payload, Request.class);
                request.setCreatorUserId(message.getCreatorUserId());
                request.setContext(createMessageContext(message));
                logger.trace("Parsed request from message payload: {}", payload);
                handle(request,
                       message.getSender(),
                       message.getRecipient(),
                       expiryDate,
                       customHeaders,
                       effort,
                       message.isCompressed());
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY.equals(type)) {
                OneWayRequest oneWayRequest = objectMapper.readValue(payload, OneWayRequest.class);
                oneWayRequest.setCreatorUserId(message.getCreatorUserId());
                oneWayRequest.setContext(createMessageContext(message));
                logger.trace("Parsed one way request from message payload: {}", payload);
                handle(oneWayRequest, message.getRecipient(), expiryDate);
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST.equals(type)
                    || Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST.equals(type)
                    || Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST.equals(type)) {
                SubscriptionRequest subscriptionRequest = objectMapper.readValue(payload, SubscriptionRequest.class);
                logger.trace("Parsed subscription request from message payload: {}", payload);
                handle(subscriptionRequest, message.getSender(), message.getRecipient());
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP.equals(type)) {
                SubscriptionStop subscriptionStop = objectMapper.readValue(payload, SubscriptionStop.class);
                logger.trace("Parsed subscription stop from message payload: {}", payload);
                handle(subscriptionStop);
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION.equals(type)) {
                SubscriptionPublication publication = objectMapper.readValue(payload, SubscriptionPublication.class);
                logger.trace("Parsed publication from message payload: {}", payload);
                handle(publication);
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(type)) {
                MulticastPublication multicastPublication = objectMapper.readValue(payload, MulticastPublication.class);
                logger.trace("Parsed multicast publication from message payload: {}", payload);
                handle(multicastPublication);
            }
        } catch (IOException e) {
            logger.error("Error parsing payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding joynr message.",
                         message.getId(),
                         message.getSender(),
                         message.getRecipient(),
                         e.getMessage());
            return;
        }
    }

    private Map<String, Serializable> createMessageContext(ImmutableMessage message) {
        Map<String, Serializable> result = new HashMap<>();
        result.putAll(message.getContext());
        result.putAll(message.getCustomHeaders());
        return result;
    }

    private void addStatelessCallback(ImmutableMessage message, Reply reply) {
        String methodId = statelessAsyncIdCalculator.extractMethodIdFromRequestReplyId(reply.getRequestReplyId());
        reply.setStatelessAsyncCallbackMethodId(methodId);
        String statelessParticipantId = message.getRecipient();
        reply.setStatelessAsyncCallbackId(statelessAsyncIdCalculator.fromParticipantUuid(statelessParticipantId));
    }

    private void handle(final Request request,
                        final String fromParticipantId,
                        final String toParticipantId,
                        final long expiryDate,
                        final Map<String, String> customHeaders,
                        final MessagingQosEffort effort,
                        final boolean compress) {
        requestReplyManager.handleRequest(new ProviderCallback<Reply>() {
            @Override
            public void onSuccess(Reply reply) {
                try {
                    if (!DispatcherUtils.isExpired(expiryDate)) {
                        sendReply(toParticipantId,
                                  fromParticipantId,
                                  reply,
                                  expiryDate,
                                  customHeaders,
                                  effort,
                                  compress);
                    } else {
                        logger.error("Error: reply {} is not send to caller, as the expiryDate of the reply message {} has been reached.",
                                     reply,
                                     expiryDate);
                    }
                } catch (Exception error) {
                    logger.error("Error processing reply {}: error:", reply, error);
                }
            }

            @Override
            public void onFailure(JoynrException error) {
                // do not log ApplicationExceptions as errors: these are not a sign of a system error
                if (error instanceof JoynrRuntimeException) {
                    logger.error("Error processing request {}:", request, error);
                }
                Reply reply = new Reply(request.getRequestReplyId(), error);
                try {
                    sendReply(toParticipantId, fromParticipantId, reply, expiryDate, customHeaders, effort, compress);
                } catch (Exception e) {
                    logger.error("Error sending error reply {}:", reply, e);
                }
            }
        }, toParticipantId, request, expiryDate);
    }

    private void handle(Reply reply) {
        requestReplyManager.handleReply(reply);
    }

    private void handle(SubscriptionReply subscriptionReply) {
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
    }

    private void handle(OneWayRequest oneWayRequest, String toParticipantId, final long expiryDate) {
        requestReplyManager.handleOneWayRequest(toParticipantId, oneWayRequest, expiryDate);
    }

    private void handle(SubscriptionRequest subscriptionRequest,
                        final String fromParticipantId,
                        final String toParticipantId) {
        publicationManager.addSubscriptionRequest(fromParticipantId, toParticipantId, subscriptionRequest);
    }

    @Override
    public void error(ImmutableMessage message, Throwable error) {
        if (message == null) {
            logger.error("Error: ", error);
            return;
        }

        Message.MessageType type = message.getType();
        String payload;

        try {
            payload = new String(message.getUnencryptedBody(), StandardCharsets.UTF_8);
        } catch (EncodingException e) {
            logger.error("Error extracting payload for message with ID {}:", message.getId(), e);
            return;
        }

        try {
            if (type.equals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST)) {
                Request request = objectMapper.readValue(payload, Request.class);
                requestReplyManager.handleError(request, error);
            }
        } catch (IOException e) {
            logger.error("Error extracting payload for message with ID {}, raw payload: {}. Error: ",
                         message.getId(),
                         payload,
                         e);
        }
    }

    private Object[] getPublicationValues(Class<?>[] parameterTypes, List<?> publicizedValues) {
        if (parameterTypes.length != publicizedValues.size()) {
            throw new JoynrRuntimeException("number of received out parameter values do not match with the number of out parameter types.");
        }
        Object[] values = new Object[parameterTypes.length];
        for (int i = 0; i < parameterTypes.length; i++) {
            values[i] = objectMapper.convertValue(publicizedValues.get(i), parameterTypes[i]);
        }
        return values;
    }

    private void handle(final SubscriptionPublication publication) {
        try {
            String subscriptionId = publication.getSubscriptionId();
            if (subscriptionManager.isBroadcast(subscriptionId)) {
                Class<?>[] broadcastOutParameterTypes = subscriptionManager.getUnicastPublicationOutParameterTypes(subscriptionId);
                List<?> broadcastOutParamterValues = (List<?>) publication.getResponse();
                Object[] broadcastValues = getPublicationValues(broadcastOutParameterTypes, broadcastOutParamterValues);
                subscriptionManager.handleBroadcastPublication(subscriptionId, broadcastValues);
            } else {
                JoynrRuntimeException error = publication.getError();
                if (error != null) {
                    subscriptionManager.handleAttributePublicationError(subscriptionId, error);
                } else {
                    Class<?> receivedType = subscriptionManager.getAttributeType(subscriptionId);

                    Object attributeValue;
                    if (TypeReference.class.isAssignableFrom(receivedType)) {
                        TypeReference<?> typeRef = (TypeReference<?>) receivedType.newInstance();
                        attributeValue = objectMapper.convertValue(((List<?>) publication.getResponse()).get(0),
                                                                   typeRef);
                    } else {
                        attributeValue = objectMapper.convertValue(((List<?>) publication.getResponse()).get(0),
                                                                   receivedType);
                    }

                    subscriptionManager.handleAttributePublication(subscriptionId, attributeValue);
                }
            }
        } catch (Exception e) {
            logger.error("Error delivering publication: ", e);
        }
    }

    private void handle(MulticastPublication multicastPublication) {
        try {
            Object[] values = getPublicationValues(subscriptionManager.getMulticastPublicationOutParameterTypes(multicastPublication.getMulticastId()),
                                                   (List<?>) multicastPublication.getResponse());
            subscriptionManager.handleMulticastPublication(multicastPublication.getMulticastId(), values);
        } catch (Exception e) {
            logger.error("Error delivering multicast publication: {}", e);
        }
    }

    private void handle(SubscriptionStop subscriptionStop) {
        logger.debug("Subscription stop received, subscriptionId: {}", subscriptionStop.getSubscriptionId());
        publicationManager.stopPublication(subscriptionStop.getSubscriptionId());
    }

    @Override
    public void sendMulticast(String fromParticipantId,
                              MulticastPublication multicastPublication,
                              MessagingQos messagingQos) {
        MutableMessage message = messageFactory.createMulticast(fromParticipantId, multicastPublication, messagingQos);
        messageSender.sendMessage(message);
    }
}
