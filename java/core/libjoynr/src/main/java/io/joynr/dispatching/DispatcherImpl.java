package io.joynr.dispatching;

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

import java.io.IOException;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.inject.Singleton;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.provider.ProviderCallback;
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
import joynr.types.DiscoveryEntryWithMetaInfo;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DispatcherImpl implements Dispatcher {

    private static final Logger logger = LoggerFactory.getLogger(DispatcherImpl.class);
    private final JoynrMessageFactory joynrMessageFactory;
    private RequestReplyManager requestReplyManager;
    private SubscriptionManager subscriptionManager;
    private PublicationManager publicationManager;
    private final MessageRouter messageRouter;
    private final MessageSender messageSender;
    private ObjectMapper objectMapper;

    @Inject
    @Singleton
    // CHECKSTYLE:OFF
    public DispatcherImpl(RequestReplyManager requestReplyManager,
                          SubscriptionManager subscriptionManager,
                          PublicationManager publicationManager,
                          MessageRouter messageRouter,
                          MessageSender messageSender,
                          JoynrMessageFactory joynrMessageFactory,
                          ObjectMapper objectMapper) {
        this.requestReplyManager = requestReplyManager;
        this.subscriptionManager = subscriptionManager;
        this.publicationManager = publicationManager;
        this.messageRouter = messageRouter;
        this.messageSender = messageSender;
        this.joynrMessageFactory = joynrMessageFactory;
        this.objectMapper = objectMapper;
    }

    // CHECKSTYLE:ON

    @Override
    public void sendSubscriptionRequest(String fromParticipantId,
                                        Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                        SubscriptionRequest subscriptionRequest,
                                        MessagingQos messagingQos) {
        for (DiscoveryEntryWithMetaInfo toDiscoveryEntry : toDiscoveryEntries) {
            JoynrMessage message = joynrMessageFactory.createSubscriptionRequest(fromParticipantId,
                                                                                 toDiscoveryEntry.getParticipantId(),
                                                                                 subscriptionRequest,
                                                                                 messagingQos);
            message.setLocalMessage(toDiscoveryEntry.getIsLocal());

            if (subscriptionRequest instanceof MulticastSubscriptionRequest) {
                String multicastId = ((MulticastSubscriptionRequest) subscriptionRequest).getMulticastId();
                messageRouter.addMulticastReceiver(multicastId, fromParticipantId, toDiscoveryEntry.getParticipantId());
            }
            messageSender.sendMessage(message);
        }
    }

    @Override
    public void sendSubscriptionStop(String fromParticipantId,
                                     Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                     SubscriptionStop subscriptionStop,
                                     MessagingQos messagingQos) {
        for (DiscoveryEntryWithMetaInfo toDiscoveryEntry : toDiscoveryEntries) {
            JoynrMessage message = joynrMessageFactory.createSubscriptionStop(fromParticipantId,
                                                                              toDiscoveryEntry.getParticipantId(),
                                                                              subscriptionStop,
                                                                              messagingQos);
            message.setLocalMessage(toDiscoveryEntry.getIsLocal());
            messageSender.sendMessage(message);
        }

    }

    @Override
    public void sendSubscriptionPublication(String fromParticipantId,
                                            Set<String> toParticipantIds,
                                            SubscriptionPublication publication,
                                            MessagingQos messagingQos) {

        for (String toParticipantId : toParticipantIds) {
            JoynrMessage message = joynrMessageFactory.createPublication(fromParticipantId,
                                                                         toParticipantId,
                                                                         publication,
                                                                         messagingQos);
            messageSender.sendMessage(message);
        }
    }

    public void sendReply(final String fromParticipantId,
                          final String toParticipantId,
                          Reply reply,
                          long expiryDateMs,
                          Map<String, String> customHeaders) throws IOException {
        MessagingQos messagingQos = new MessagingQos(expiryDateMs);
        messagingQos.getCustomMessageHeaders().putAll(customHeaders);
        JoynrMessage message = joynrMessageFactory.createReply(fromParticipantId, toParticipantId, reply, messagingQos);
        messageSender.sendMessage(message);
    }

    @Override
    public void sendSubscriptionReply(final String fromParticipantId,
                                      final String toParticipantId,
                                      SubscriptionReply subscriptionReply,
                                      MessagingQos messagingQos) {

        JoynrMessage message = joynrMessageFactory.createSubscriptionReply(fromParticipantId,
                                                                           toParticipantId,
                                                                           subscriptionReply,
                                                                           messagingQos);
        messageSender.sendMessage(message);
    }

    @Override
    public void messageArrived(final JoynrMessage message) {
        if (message == null) {
            logger.error("received message was null");
            return;
        }
        final long expiryDate = message.getExpiryDate();
        final Map<String, String> customHeaders = message.getCustomHeaders();
        if (DispatcherUtils.isExpired(expiryDate)) {
            logger.debug("TTL expired, discarding message : {}", message.toLogMessage());
            return;
        }

        String type = message.getType();
        try {
            if (JoynrMessage.MESSAGE_TYPE_REPLY.equals(type)) {
                Reply reply = objectMapper.readValue(message.getPayload(), Reply.class);
                logger.trace("Parsed reply from message payload :" + message.getPayload());
                handle(reply);
            } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REPLY.equals(type)) {
                SubscriptionReply subscriptionReply = objectMapper.readValue(message.getPayload(),
                                                                             SubscriptionReply.class);
                logger.trace("Parsed subscription reply from message payload :" + message.getPayload());
                handle(subscriptionReply);
            } else if (JoynrMessage.MESSAGE_TYPE_REQUEST.equals(type)) {
                final Request request = objectMapper.readValue(message.getPayload(), Request.class);
                request.setCreatorUserId(message.getCreatorUserId());
                request.setContext(message.getContext());
                logger.trace("Parsed request from message payload :" + message.getPayload());
                handle(request, message.getFrom(), message.getTo(), expiryDate, customHeaders);
            } else if (JoynrMessage.MESSAGE_TYPE_ONE_WAY.equals(type)) {
                OneWayRequest oneWayRequest = objectMapper.readValue(message.getPayload(), OneWayRequest.class);
                oneWayRequest.setCreatorUserId(message.getCreatorUserId());
                oneWayRequest.setContext(message.getContext());
                logger.trace("Parsed one way request from message payload :" + message.getPayload());
                handle(oneWayRequest, message.getTo(), expiryDate);
            } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST.equals(type)
                    || JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST.equals(type)
                    || JoynrMessage.MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST.equals(type)) {
                SubscriptionRequest subscriptionRequest = objectMapper.readValue(message.getPayload(),
                                                                                 SubscriptionRequest.class);
                logger.trace("Parsed subscription request from message payload :" + message.getPayload());
                handle(subscriptionRequest, message.getFrom(), message.getTo());
            } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP.equals(type)) {
                SubscriptionStop subscriptionStop = objectMapper.readValue(message.getPayload(), SubscriptionStop.class);
                logger.trace("Parsed subscription stop from message payload :" + message.getPayload());
                handle(subscriptionStop);
            } else if (JoynrMessage.MESSAGE_TYPE_PUBLICATION.equals(type)) {
                SubscriptionPublication publication = objectMapper.readValue(message.getPayload(),
                                                                             SubscriptionPublication.class);
                logger.trace("Parsed publication from message payload :" + message.getPayload());
                handle(publication);
            } else if (JoynrMessage.MESSAGE_TYPE_MULTICAST.equals(type)) {
                MulticastPublication multicastPublication = objectMapper.readValue(message.getPayload(),
                                                                                   MulticastPublication.class);
                logger.trace("Parsed multicast publication from message payload: {}", message.getPayload());
                handle(multicastPublication);
            }
        } catch (IOException e) {
            logger.error("Error parsing payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding joynr message.",
                         new String[]{ message.getFrom(), message.getFrom(), message.getId(), e.getMessage() });
            return;
        }
    }

    private void handle(final Request request,
                        final String fromParticipantId,
                        final String toParticipantId,
                        final long expiryDate,
                        final Map<String, String> customHeaders) {
        requestReplyManager.handleRequest(new ProviderCallback<Reply>() {
            @Override
            public void onSuccess(Reply reply) {
                try {
                    if (!DispatcherUtils.isExpired(expiryDate)) {
                        sendReply(toParticipantId, fromParticipantId, reply, expiryDate, customHeaders);
                    } else {
                        logger.error("Error: reply {} is not send to caller, as the expiryDate of the reply message {} has been reached.",
                                     reply,
                                     new Date(expiryDate));
                    }
                } catch (Exception error) {
                    logger.error("Error processing reply: \r\n {} : error : {}", reply, error);
                }
            }

            @Override
            public void onFailure(JoynrException error) {
                // do not log ApplicationExceptions as errors: these are not a sign of a system error
                if (error instanceof JoynrRuntimeException) {
                    logger.error("Error processing request: {}", request, error);
                }
                Reply reply = new Reply(request.getRequestReplyId(), error);
                try {
                    sendReply(toParticipantId, fromParticipantId, reply, expiryDate, customHeaders);
                } catch (Exception e) {
                    logger.error("Error sending error reply: \r\n {}", reply, e);
                }
            }
        },
                                          toParticipantId,
                                          request,
                                          expiryDate);
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
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN DispatcherImpl");
    }

    @Override
    public void error(JoynrMessage message, Throwable error) {
        if (message == null) {
            logger.error("error: ", error);
            return;
        }

        String type = message.getType();
        try {
            if (type.equals(JoynrMessage.MESSAGE_TYPE_REQUEST)) {
                Request request = objectMapper.readValue(message.getPayload(), Request.class);
                requestReplyManager.handleError(request, error);
            }
        } catch (IOException e) {
            logger.error("Error extracting payload for message " + message.getId() + ", raw payload: "
                                 + message.getPayload(),
                         e.getMessage());
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
            logger.error("Error delivering publication: {} : {}", e.getClass(), e.getMessage());
        }
    }

    private void handle(MulticastPublication multicastPublication) {
        try {
            Object[] values = getPublicationValues(subscriptionManager.getMulticastPublicationOutParameterTypes(multicastPublication.getMulticastId()),
                                                   (List<?>) multicastPublication.getResponse());
            subscriptionManager.handleMulticastPublication(multicastPublication.getMulticastId(), values);
        } catch (Exception e) {
            logger.error("Error delivering multicast publication: {} : {}", e.getClass(), e.getMessage());
            logger.trace("Full exception.", e);
        }
    }

    private void handle(SubscriptionStop subscriptionStop) {
        logger.info("Subscription stop received");
        publicationManager.stopPublication(subscriptionStop.getSubscriptionId());
    }

    @Override
    public void sendMulticast(String fromParticipantId,
                              MulticastPublication multicastPublication,
                              MessagingQos messagingQos) {
        JoynrMessage message = joynrMessageFactory.createMulticast(fromParticipantId,
                                                                   multicastPublication,
                                                                   messagingQos);
        messageSender.sendMessage(message);
    }
}
