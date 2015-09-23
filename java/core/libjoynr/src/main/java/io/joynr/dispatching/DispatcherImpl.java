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
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.ReceiverStatusListener;
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
import joynr.system.routingtypes.ChannelAddress;

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
    private RequestReplyDispatcher requestReplyDispatcher;
    private SubscriptionManager subscriptionManager;
    private PublicationManager publicationManager;
    private final MessageRouter messageRouter;
    private PlatformSecurityManager securityManager;
    private ObjectMapper objectMapper;
    private AccessController accessController;
    private MessageReceiver messageReceiver;
    private boolean shutdown = false;
    private boolean registering = false;
    private RequestCallerDirectory requestCallerDirectory;
    private ReplyCallerDirectory replyCallerDirectory;
    private CallerDirectoryListener<RequestCaller> requestCallerDirectoryListener;
    private CallerDirectoryListener<ReplyCaller> replyCallerDirectoryListener;

    private class CallerDirectoryListenerImpl<T> implements CallerDirectoryListener<T> {

        @Override
        public void callerAdded(String participantId, T caller) {
            startReceiver();
        }

        public void callerRemoved(String participantId) {
        }
    }

    @Inject
    @Singleton
    // CHECKSTYLE:OFF
    public DispatcherImpl(RequestCallerDirectory requestCallerDirectory,
                          ReplyCallerDirectory replyCallerDirectory,
                          RequestReplyDispatcher requestReplyDispatcher,
                          SubscriptionManager subscriptionManager,
                          PublicationManager publicationManager,
                          MessageRouter messageRouter,
                          MessageReceiver messageReceiver,
                          PlatformSecurityManager securityManager,
                          AccessController accessController,
                          JoynrMessageFactory joynrMessageFactory,
                          ObjectMapper objectMapper) {
        this.requestCallerDirectory = requestCallerDirectory;
        this.replyCallerDirectory = replyCallerDirectory;
        this.requestReplyDispatcher = requestReplyDispatcher;
        this.subscriptionManager = subscriptionManager;
        this.publicationManager = publicationManager;
        this.messageRouter = messageRouter;
        this.messageReceiver = messageReceiver;
        this.securityManager = securityManager;
        this.accessController = accessController;
        this.joynrMessageFactory = joynrMessageFactory;
        this.objectMapper = objectMapper;
        requestCallerDirectoryListener = new CallerDirectoryListenerImpl<RequestCaller>();
        replyCallerDirectoryListener = new CallerDirectoryListenerImpl<ReplyCaller>();
        requestCallerDirectory.addListener(requestCallerDirectoryListener);
        replyCallerDirectory.addListener(replyCallerDirectoryListener);
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
                        final String replyToChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
                        final Request request = objectMapper.readValue(message.getPayload(), Request.class);
                        logger.debug("Parsed request from message payload :" + message.getPayload());
                        handle(request, message.getFrom(), message.getTo(), replyToChannelId, expiryDate);
                    }
                } else if (JoynrMessage.MESSAGE_TYPE_ONE_WAY.equals(type)) {
                    OneWay oneWayRequest = objectMapper.readValue(message.getPayload(), OneWay.class);
                    logger.debug("Parsed one way request from message payload :" + message.getPayload());
                    handle(oneWayRequest, message.getTo(), expiryDate);
                } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST.equals(type)
                        || JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST.equals(type)) {
                    // handle only if this message creator (userId) has permissions
                    if (accessController.hasConsumerPermission(message)) {
                        final String replyToChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
                        SubscriptionRequest subscriptionRequest = objectMapper.readValue(message.getPayload(),
                                                                                         SubscriptionRequest.class);
                        logger.debug("Parsed subscription request from message payload :" + message.getPayload());
                        handle(subscriptionRequest, message.getFrom(), message.getTo(), replyToChannelId);
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
                        final String replyToChannelId,
                        final long expiryDate) {
        addRequestorToMessageRouter(fromParticipantId, replyToChannelId);

        requestReplyDispatcher.handleRequest(new Callback<Reply>() {
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
        requestReplyDispatcher.handleReply(reply);
    }

    private void handle(OneWay oneWayRequest, String toParticipantId, final long expiryDate) {
        requestReplyDispatcher.handleOneWayRequest(toParticipantId, oneWayRequest, expiryDate);
    }

    private void handle(SubscriptionRequest subscriptionRequest,
                        final String fromParticipantId,
                        final String toParticipantId,
                        final String channelId) {
        addRequestorToMessageRouter(fromParticipantId, channelId);

        publicationManager.addSubscriptionRequest(fromParticipantId, toParticipantId, subscriptionRequest);
    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN RequestReplyDispatcher");
        requestCallerDirectory.removeListener(requestCallerDirectoryListener);
        replyCallerDirectory.removeListener(replyCallerDirectoryListener);
        shutdown = true;

        try {
            messageReceiver.shutdown(clear);
        } catch (Exception e) {
            logger.error("error shutting down messageReceiver");
        }

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
                requestReplyDispatcher.handleError(request, error);
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
                Class<?> receivedType = subscriptionManager.getAttributeType(subscriptionId);

                Object attributeValue;
                if (TypeReference.class.isAssignableFrom(receivedType)) {
                    TypeReference<?> typeRef = (TypeReference<?>) receivedType.newInstance();
                    attributeValue = objectMapper.convertValue(((List<?>) publication.getResponse()).get(0), typeRef);
                } else {
                    attributeValue = objectMapper.convertValue(((List<?>) publication.getResponse()).get(0),
                                                               receivedType);
                }

                subscriptionManager.handleAttributePublication(subscriptionId, attributeValue);
            }
        } catch (Exception e) {
            logger.error("Error delivering publication: {} : {}", e.getClass(), e.getMessage());
        }
    }

    private void handle(SubscriptionStop subscriptionStop) {
        logger.info("Subscription stop received");
        publicationManager.stopPublication(subscriptionStop.getSubscriptionId());
    }

    private void addRequestorToMessageRouter(String requestorParticipantId, String replyToChannelId) {
        if (replyToChannelId != null && !replyToChannelId.isEmpty()) {
            messageRouter.addNextHop(requestorParticipantId, new ChannelAddress(replyToChannelId));
        } else {
            /*
             * TODO make sure that all requests (ie not one-way) also have replyTo
             * set, otherwise log an error.
             * Caution: the replyToChannelId is not set in case of local communication
             */
        }
    }

    private void startReceiver() {
        if (shutdown) {
            throw new JoynrShutdownException("cannot start receiver: dispatcher is already shutting down");
        }

        synchronized (messageReceiver) {
            if (registering == false) {
                registering = true;

                if (!messageReceiver.isStarted()) {
                    // The messageReceiver gets the message off the wire and passes it on to the message Listener.
                    // Starting the messageReceiver triggers a registration with the channelUrlDirectory, thus causing
                    // reply messages to be sent back to this message Receiver. It is therefore necessary to register
                    // the message receiver before registering the message listener.

                    // NOTE LongPollMessageReceiver creates a channel synchronously before returning

                    // TODO this will lead to a unique messageReceiver => all servlets share one channelId
                    messageReceiver.start(DispatcherImpl.this, new ReceiverStatusListener() {

                        @Override
                        public void receiverStarted() {
                        }

                        @Override
                        // Exceptions that could not be resolved from within the receiver require a shutdown
                        public void receiverException(Throwable e) {
                            // clear == false means that offboard resources (registrations, existing channels etc are
                            // not affected
                            shutdown(false);
                        }
                    });

                }
            }
        }
    }
}
