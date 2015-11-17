package io.joynr.dispatching;

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

import io.joynr.accesscontrol.AccessController;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.proxy.Callback;
import io.joynr.security.PlatformSecurityManager;

import java.io.IOException;
import java.util.Date;
import java.util.List;

import javax.inject.Singleton;

import joynr.JoynrMessage;
import joynr.OneWay;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class DispatcherImpl implements Dispatcher {

    private static final Logger logger = LoggerFactory.getLogger(DispatcherImpl.class);
    private final JoynrMessageFactory joynrMessageFactory;
    private RequestReplyManager requestReplyManager;
    private SubscriptionManager subscriptionManager;
    private PublicationManager publicationManager;
    private final MessageRouter messageRouter;
    private PlatformSecurityManager securityManager;
    private ObjectMapper objectMapper;
    private AccessController accessController;

    @Inject
    @Singleton
    // CHECKSTYLE:OFF
    public DispatcherImpl(RequestReplyManager requestReplyManager,
                          SubscriptionManager subscriptionManager,
                          PublicationManager publicationManager,
                          MessageRouter messageRouter,
                          PlatformSecurityManager securityManager,
                          AccessController accessController,
                          JoynrMessageFactory joynrMessageFactory,
                          ObjectMapper objectMapper) {
        this.requestReplyManager = requestReplyManager;
        this.subscriptionManager = subscriptionManager;
        this.publicationManager = publicationManager;
        this.messageRouter = messageRouter;
        this.securityManager = securityManager;
        this.accessController = accessController;
        this.joynrMessageFactory = joynrMessageFactory;
        this.objectMapper = objectMapper;
    }

    // CHECKSTYLE:ON

    @Override
    public void sendSubscriptionRequest(String fromParticipantId,
                                        String toParticipantId,
                                        SubscriptionRequest subscriptionRequest,
                                        MessagingQos qosSettings,
                                        boolean broadcast) throws JoynrSendBufferFullException,
                                                          JoynrMessageNotSentException, JsonGenerationException,
                                                          JsonMappingException, IOException {
        JoynrMessage message = joynrMessageFactory.createSubscriptionRequest(fromParticipantId,
                                                                             toParticipantId,
                                                                             subscriptionRequest,
                                                                             DispatcherUtils.convertTtlToExpirationDate(qosSettings.getRoundTripTtl_ms()),
                                                                             broadcast);

        messageRouter.route(message);
    }

    @Override
    public void sendSubscriptionStop(String fromParticipantId,
                                     String toParticipantId,
                                     SubscriptionStop subscriptionStop,
                                     MessagingQos messagingQos) throws JoynrSendBufferFullException,
                                                               JoynrMessageNotSentException, JsonGenerationException,
                                                               JsonMappingException, IOException {
        JoynrMessage message = joynrMessageFactory.createSubscriptionStop(fromParticipantId,
                                                                          toParticipantId,
                                                                          subscriptionStop,
                                                                          DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms()));
        messageRouter.route(message);

    }

    @Override
    public void sendSubscriptionPublication(String fromParticipantId,
                                            String toParticipantId,
                                            SubscriptionPublication publication,
                                            MessagingQos qosSettings) throws JoynrSendBufferFullException,
                                                                     JoynrMessageNotSentException,
                                                                     JsonGenerationException, JsonMappingException,
                                                                     IOException {

        JoynrMessage message = joynrMessageFactory.createPublication(fromParticipantId,
                                                                     toParticipantId,
                                                                     publication,
                                                                     DispatcherUtils.convertTtlToExpirationDate(qosSettings.getRoundTripTtl_ms()));
        messageRouter.route(message);
    }

    public void sendReply(final String fromParticipantId, final String toParticipantId, Reply reply, long expiryDate)
                                                                                                                     throws JoynrSendBufferFullException,
                                                                                                                     JoynrMessageNotSentException,
                                                                                                                     JsonGenerationException,
                                                                                                                     JsonMappingException,
                                                                                                                     IOException {
        JoynrMessage message = joynrMessageFactory.createReply(fromParticipantId,
                                                               toParticipantId,
                                                               reply,
                                                               DispatcherUtils.convertTtlToExpirationDate(expiryDate));
        messageRouter.route(message);
    }

    @Override
    public void messageArrived(final JoynrMessage message) {
        if (message == null) {
            logger.error("received messaage was null");
            return;
        }
        if (!securityManager.validate(message)) {
            logger.error("unable to validate received message, discarding message: {}", message.toLogMessage());
            return;
        }
        final long expiryDate = message.getExpiryDate();
        if (DispatcherUtils.isExpired(expiryDate)) {
            logger.debug("TTL expired, discarding message : {}", message.toLogMessage());
            return;
        }

        String type = message.getType();
        try {
            if (JoynrMessage.MESSAGE_TYPE_REPLY.equals(type)) {
                Reply reply = objectMapper.readValue(message.getPayload(), Reply.class);
                logger.debug("Parsed reply from message payload :" + message.getPayload());
                handle(reply);
            } else {
                if (JoynrMessage.MESSAGE_TYPE_REQUEST.equals(type)) {
                    // handle only if this message creator (userId) has permissions
                    if (accessController.hasConsumerPermission(message)) {
                        final Request request = objectMapper.readValue(message.getPayload(), Request.class);
                        logger.debug("Parsed request from message payload :" + message.getPayload());
                        handle(request, message.getFrom(), message.getTo(), expiryDate);
                    }
                } else if (JoynrMessage.MESSAGE_TYPE_ONE_WAY.equals(type)) {
                    OneWay oneWayRequest = objectMapper.readValue(message.getPayload(), OneWay.class);
                    logger.debug("Parsed one way request from message payload :" + message.getPayload());
                    handle(oneWayRequest, message.getTo(), expiryDate);
                } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST.equals(type)
                        || JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST.equals(type)) {
                    // handle only if this message creator (userId) has permissions
                    if (accessController.hasConsumerPermission(message)) {
                        SubscriptionRequest subscriptionRequest = objectMapper.readValue(message.getPayload(),
                                                                                         SubscriptionRequest.class);
                        logger.debug("Parsed subscription request from message payload :" + message.getPayload());
                        handle(subscriptionRequest, message.getFrom(), message.getTo());
                    }
                } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP.equals(type)) {
                    SubscriptionStop subscriptionStop = objectMapper.readValue(message.getPayload(),
                                                                               SubscriptionStop.class);
                    logger.debug("Parsed subscription stop from message payload :" + message.getPayload());
                    handle(subscriptionStop);
                } else if (JoynrMessage.MESSAGE_TYPE_PUBLICATION.equals(type)) {
                    SubscriptionPublication publication = objectMapper.readValue(message.getPayload(),
                                                                                 SubscriptionPublication.class);
                    logger.debug("Parsed publication from message payload :" + message.getPayload());
                    handle(publication);
                }
            }
        } catch (Exception e) {
            logger.error("Error parsing payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding joynr message.",
                         new String[]{ message.getFrom(), message.getFrom(), message.getId(), e.getMessage() });
            return;
        }
    }

    private void handle(final Request request,
                        final String fromParticipantId,
                        final String toParticipantId,
                        final long expiryDate) {
        requestReplyManager.handleRequest(new Callback<Reply>() {
            @Override
            public void onSuccess(Reply reply) {
                try {
                    if (!DispatcherUtils.isExpired(expiryDate)) {
                        sendReply(toParticipantId, fromParticipantId, reply, expiryDate);
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
                logger.error("Error processing request: \r\n {} ; error: {}", request, error);
                Reply reply = new Reply(request.getRequestReplyId(), error);
                try {
                    sendReply(toParticipantId, fromParticipantId, reply, expiryDate);
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

    private void handle(OneWay oneWayRequest, String toParticipantId, final long expiryDate) {
        requestReplyManager.handleOneWayRequest(toParticipantId, oneWayRequest, expiryDate);
    }

    private void handle(SubscriptionRequest subscriptionRequest,
                        final String fromParticipantId,
                        final String toParticipantId) {
        publicationManager.addSubscriptionRequest(fromParticipantId, toParticipantId, subscriptionRequest);
    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN RequestReplyManager");
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
                    + message.getPayload(), e.getMessage());
        }

    }

    private void handle(final SubscriptionPublication publication) {
        try {
            String subscriptionId = publication.getSubscriptionId();
            if (subscriptionManager.isBroadcast(subscriptionId)) {
                Class<?>[] broadcastOutParameterTypes = subscriptionManager.getBroadcastOutParameterTypes(subscriptionId);
                List<?> broadcastOutParamterValues = (List<?>) publication.getResponse();
                if (broadcastOutParameterTypes.length != broadcastOutParamterValues.size()) {
                    throw new JoynrRuntimeException("number of received broadcast out parameter values do not match with number of broadcast out parameter types.");
                }
                Object[] broadcastValues = new Object[broadcastOutParameterTypes.length];
                for (int i = 0; i < broadcastOutParameterTypes.length; i++) {
                    broadcastValues[i] = objectMapper.convertValue(broadcastOutParamterValues.get(i),
                                                                   broadcastOutParameterTypes[i]);
                }
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

    private void handle(SubscriptionStop subscriptionStop) {
        logger.info("Subscription stop received");
        publicationManager.stopPublication(subscriptionStop.getSubscriptionId());
    }

}
