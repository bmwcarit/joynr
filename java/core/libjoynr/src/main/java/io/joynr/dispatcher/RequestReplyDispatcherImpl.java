package io.joynr.dispatcher;

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
import io.joynr.dispatcher.rpc.JsonRequestInterpreter;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.IMessageReceivers;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.pubsub.publication.PublicationManager;
import io.joynr.pubsub.subscription.SubscriptionListener;
import io.joynr.pubsub.subscription.SubscriptionManager;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Maps;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Default implementation of the Dispatcher interface.
 */
@Singleton
public class RequestReplyDispatcherImpl implements RequestReplyDispatcher {

    // TODO tune threadpool size for RequestReplyDispatcherImpl
    private static final int THREADPOOLSIZE = 10;

    private Map<String, PayloadListener<?>> oneWayRecipients = Maps.newHashMap();
    private Map<String, RequestCaller> requestCallerDirectory = Maps.newHashMap();
    private final ScheduledExecutorService scheduler;

    private DispatcherMessageQueues messageQueues = new DispatcherMessageQueues();
    private ReplyCallerDirectory replyCallerDirectory;

    private MessagingEndpointDirectory messagingEndpointDirectory;
    protected RequestReplySender messageSender;

    private JsonRequestInterpreter jsonRequestInterpreter;

    private static final Logger logger = LoggerFactory.getLogger(RequestReplyDispatcherImpl.class);

    private final MessageReceiver messageReceiver;

    private final ObjectMapper objectMapper;

    private PublicationManager publicationManager;

    private SubscriptionManager subscriptionManager;

    private boolean shutdown = false;

    private boolean registering = false;

    @Inject
    // CHECKSTYLE:OFF
    public RequestReplyDispatcherImpl(RequestReplySender messageSender,
                                      IMessageReceivers messageReceivers,
                                      MessageReceiver messageReceiver,
                                      MessagingEndpointDirectory messagingEndpointDirectory,
                                      ReplyCallerDirectory replyCallerDirectory,
                                      @Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                      ObjectMapper objectMapper,
                                      PublicationManager publicationManager,
                                      SubscriptionManager subscriptionManager,
                                      JsonRequestInterpreter jsonRequestInterpreter) {
        // CHECKSTYLE:ON
        this.messageSender = messageSender;
        this.messageReceiver = messageReceiver;

        this.messagingEndpointDirectory = messagingEndpointDirectory;
        this.replyCallerDirectory = replyCallerDirectory;
        this.objectMapper = objectMapper;
        this.publicationManager = publicationManager;
        this.subscriptionManager = subscriptionManager;
        this.jsonRequestInterpreter = jsonRequestInterpreter;
        ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("RequestReplyDispatcher-%d")
                                                                     .build();
        scheduler = Executors.newScheduledThreadPool(THREADPOOLSIZE, namedThreadFactory);

        // TODO would be better not to have this in the constructor to prevent
        // any race condition issues with messages being
        // received before the constructor is finished. Also would be good to
        // only start the long poll once someone really is
        // interested in incoming messages
        // messageReceiver.registerMessageListener(this);
        // messageReceivers.registerMessageReceiver(messageReceiver);
        messageReceivers.registerMessageReceiver(messageReceiver, channelId);
    }

    @Override
    public void addOneWayRecipient(final String participantId, PayloadListener<?> listener) {
        synchronized (oneWayRecipients) {
            oneWayRecipients.put(participantId, listener);
        }

        ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> messageList = messageQueues.getAndRemoveOneWayMessages(participantId);
        if (messageList != null) {
            for (ContentWithExpiryDate<JoynrMessage> messageItem : messageList) {
                if (!messageItem.isExpired()) {
                    deliverMessageToListener(listener, messageItem.getContent());
                }
            }
        }
    }

    @Override
    /**
     * Will start the message receiver if anyone is listening
     */
    public void addRequestCaller(String participantId, RequestCaller requestCaller) {

        synchronized (requestCallerDirectory) {
            requestCallerDirectory.put(participantId, requestCaller);
            startReceiver();
        }

        ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> messageList = messageQueues.getAndRemoveRequestMessages(participantId);
        if (messageList != null) {
            for (ContentWithExpiryDate<JoynrMessage> messageItem : messageList) {
                if (!messageItem.isExpired()) {
                    executeRequestAndRespond(requestCaller, messageItem.getContent());
                }
            }
        }
    }

    private void startReceiver() {
        if (shutdown) {
            throw new JoynrShutdownException("cannot start receiver: dispatcher is already shutting down");
        }

        synchronized (messageReceiver) {
            if (registering == false) {
                registering = true;
                messageReceiver.registerMessageListener(RequestReplyDispatcherImpl.this);

                if (!messageReceiver.isStarted()) {
                    // The messageReceiver gets the message off the wire and passes it on to the message Listener.
                    // Starting the messageReceiver triggers a registration with the channelUrlDirectory, thus causing
                    // reply messages to be sent back to this message Receiver. It is therefore necessary to register
                    // the message receiver before registering the message listener.

                    // NOTE LongPollMessageReceiver creates a channel synchronously before returning

                    // TODO this will lead to a unique messageReceiver => all servlets share one channelId
                    messageReceiver.startReceiver(new ReceiverStatusListener() {

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

    @Override
    public void addReplyCaller(String requestReplyId, ReplyCaller replyCaller, long expiryDateAsMs) {
        ExpiryDate expiryDate = DispatcherUtils.convertTtlToExpirationDate(expiryDateAsMs);
        replyCallerDirectory.putReplyCaller(requestReplyId, replyCaller, expiryDate);
        startReceiver();

    }

    @Override
    public void removeReplyCaller(String requestReplyId) {
        replyCallerDirectory.getAndRemoveReplyCaller(requestReplyId);

    }

    @Override
    public void error(JoynrMessage message, Throwable error) {
        if (message == null) {
            logger.error("error: ", error);
            return;
        }

        if (message.getType().equals(JoynrMessage.MESSAGE_TYPE_REQUEST)) {
            // TODO when request and reply manager are divided from dispatcher, they are responsible for the error
            // handling
            Request payload;
            try {
                payload = objectMapper.readValue(message.getPayload(), Request.class);
                String requestReplyId = payload.getRequestReplyId();
                if (requestReplyId != null) {
                    ReplyCaller replyCaller = replyCallerDirectory.getAndRemoveReplyCaller(requestReplyId);
                    if (replyCaller != null) {
                        replyCaller.error(error);
                    }
                }
            } catch (IOException e) {
                logger.error("Error extracting payload for message " + message.getId() + ", raw payload: "
                                     + message.getPayload(),
                             e.getMessage());
            }
        }

    }

    public void removeRequestCaller(String participantId) {
        synchronized (requestCallerDirectory) {
            requestCallerDirectory.remove(participantId);
        }
        publicationManager.stopPublicationByProviderId(participantId);

    }

    @Override
    public void removeListener(final String participantId) {
        synchronized (oneWayRecipients) {
            oneWayRecipients.remove(participantId);
        }
    }

    @Override
    public void messageArrived(final JoynrMessage message) {
        if (message != null) {
            long incomingExpiryDate = message.getExpiryDate();
            if (!DispatcherUtils.isExpired(incomingExpiryDate)) {
                String type = message.getType();
                if (JoynrMessage.MESSAGE_TYPE_REPLY.equals(type)) {
                    handleReplyMessageReceived(message);
                } else if (JoynrMessage.MESSAGE_TYPE_REQUEST.equals(type)) {
                    handleRequestMessageReceived(message);
                } else if (JoynrMessage.MESSAGE_TYPE_ONE_WAY.equals(type)) {
                    handleOneWayMessageReceived(message);
                } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST.equals(type)) {
                    handleSubscriptionRequestReceived(message);
                } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP.equals(type)) {
                    handleSubscriptionStopReceived(message);
                } else if (JoynrMessage.MESSAGE_TYPE_PUBLICATION.equals(type)) {
                    handlePublicationReceived(message);
                }
            } else {
                logger.debug("TTL expired, discarding message : {}", message.toLogMessage());
            }
        }
    }

    private void handlePublicationReceived(final JoynrMessage message) {
        logger.info("Publication received");
        scheduler.execute(new Runnable() {

            @Override
            public void run() {
                deliverPublication(message);
            }
        });

    }

    @SuppressWarnings("unchecked")
    private void deliverPublication(JoynrMessage message) {
        SubscriptionPublication publication;
        try {
            publication = objectMapper.readValue(message.getPayload(), SubscriptionPublication.class);
            String subscriptionId = publication.getSubscriptionId();
            Class<? extends TypeReference<?>> attributeType = subscriptionManager.getAttributeTypeReference(subscriptionId);

            TypeReference<?> typeRef = attributeType.newInstance();
            Object receivedObject = objectMapper.convertValue(publication.getResponse(), typeRef);
            @SuppressWarnings("unchecked")
            SubscriptionListener listener = subscriptionManager.getSubscriptionListener(subscriptionId);
            if (listener == null) {
                logger.error("No subscription listener found for incoming publication!");
            } else {
                subscriptionManager.touchSubscriptionState(subscriptionId);
                listener.receive(receivedObject);
            }

        } catch (Exception e) {
            logger.error("Error delivering publication: {} : {}", e.getClass(), e.getMessage());
        }
    }

    private void handleSubscriptionStopReceived(JoynrMessage message) {
        logger.info("Subscription stop received");
        try {

            SubscriptionStop subscriptionStop = objectMapper.readValue(message.getPayload(), SubscriptionStop.class);
            final String subscriptionId = subscriptionStop.getSubscriptionId();

            scheduler.execute(new Runnable() {

                @Override
                public void run() {
                    publicationManager.stopPublication(subscriptionId);
                }
            });
        } catch (Exception e) {
            logger.error("Error delivering subscription stop: {}", e.getMessage());
        }

    }

    private void handleSubscriptionRequestReceived(final JoynrMessage message) {

        final String toParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID);
        final String fromParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID);
        if (requestCallerDirectory.containsKey(toParticipantId)) {
            try {
                scheduler.execute(new Runnable() {

                    @Override
                    public void run() {
                        try {
                            RequestCaller requestCaller = null;
                            requestCaller = requestCallerDirectory.get(toParticipantId);
                            SubscriptionRequest subscriptionRequest = objectMapper.readValue(message.getPayload(),
                                                                                             SubscriptionRequest.class);
                            String replyChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
                            messagingEndpointDirectory.put(fromParticipantId,
                                                           new JoynrMessagingEndpointAddress(replyChannelId));

                            publicationManager.addSubscriptionRequest(fromParticipantId,
                                                                      toParticipantId,
                                                                      subscriptionRequest,
                                                                      requestCaller,
                                                                      messageSender);
                        } catch (Throwable e) {
                            logger.error("Error processing message: \r\n {}", message, e);

                        }
                    }
                });
            } catch (Throwable e) {
                logger.error("Error processing message: \r\n {}", message.toLogMessage(), e);

            }

        } else {
            // TODO handle unkown participantID
            logger.debug("Received subscriptionRequest for unkown participant. Discarding request.");
        }

    }

    private void handleOneWayMessageReceived(final JoynrMessage message) {
        String toParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID);
        synchronized (oneWayRecipients) {
            final PayloadListener<?> listener = oneWayRecipients.get(toParticipantId);

            if (listener != null) {
                deliverMessageToListener(listener, message);
            } else {
                messageQueues.putOneWayMessage(toParticipantId,
                                               message,
                                               ExpiryDate.fromAbsolute(message.getExpiryDate()));
            }
        }
    }

    @SuppressWarnings("unchecked")
    private void deliverMessageToListener(final PayloadListener listener, final JoynrMessage message) {
        try {
            scheduler.execute(new Runnable() {
                @Override
                public void run() {
                    try {
                        Object extractPayload = objectMapper.readValue(message.getPayload(), Object.class);
                        listener.receive(extractPayload);
                    } catch (Exception e) {
                        logger.error("error: {} unable to extract payload from message {}",
                                     e.getMessage(),
                                     message.getId());
                    }
                }
            });
        } catch (RejectedExecutionException e) {
            logger.error("Exception while scheduling listener: ", e);
        }
    }

    private void handleRequestMessageReceived(final JoynrMessage message) {

        String fromParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID);
        String toParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID);
        String replyToChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
        if (replyToChannelId != null && !replyToChannelId.isEmpty()) {
            messagingEndpointDirectory.put(fromParticipantId, new JoynrMessagingEndpointAddress(replyToChannelId));
        }
        // TODO make sure that all requests (ie not one-way) also have replyTo
        // set, otherwise log an error

        if (requestCallerDirectory.containsKey(toParticipantId)) {
            executeRequestAndRespond(requestCallerDirectory.get(toParticipantId), message);

        } else {
            messageQueues.putRequestMessage(toParticipantId, message, ExpiryDate.fromAbsolute(message.getExpiryDate()));
            logger.info("No requestCaller found for participantId: {} queuing request message.", toParticipantId);
        }
    }

    private void executeRequestAndRespond(final RequestCaller requestCaller, final JoynrMessage message) {
        try {
            // TODO add timeout to the runnable to make sure a requestCaller can not block the thread for ever
            scheduler.execute(new Runnable() {

                @Override
                public void run() {
                    try {
                        // TODO shall be moved to request manager and not handled by dispatcher
                        Request request = objectMapper.readValue(message.getPayload(), Request.class);

                        Reply reply = jsonRequestInterpreter.execute(requestCaller, request);
                        // String replyMcid =
                        // message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_TO);
                        String originalReceiverParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID);
                        String originalSenderParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID);
                        long expiryDate = Long.parseLong(message.getHeader().get(JoynrMessage.HEADER_NAME_EXPIRY_DATE));
                        if (expiryDate > System.currentTimeMillis()) {
                            try {
                                messageSender.sendReply(originalReceiverParticipantId,
                                                        originalSenderParticipantId,
                                                        reply,
                                                        ExpiryDate.fromAbsolute(expiryDate));
                            } catch (JoynrSendBufferFullException e) {
                                // TODO React on exception thrown by sendReply
                                logger.error("Responder could not reply due to a JoynSendBufferFullException: ", e);
                            } catch (JoynrMessageNotSentException e) {
                                // TODO React on JoynrMessageNotSentException
                                // thrown by sendReply
                                logger.error("Responder could not reply due to a JoynrMessageNotSentException: ", e);
                            } catch (JoynrCommunicationException e) {
                                // TODO React on JoynCommunicationException
                                // thrown by sendReply
                                logger.error("Responder could not reply due to a JoynCommunicationException: ", e);
                            }

                        } else {
                            logger.error("Expiry Date exceeded. Reply discarded: messageId: {} requestReplyId: {}",
                                         message.getId(),
                                         reply.getRequestReplyId());
                        }
                        // } catch (IOException e) {
                        // logger.error("Error processing message: \r\n {}",
                        // message, e);
                        // } catch (ClassNotFoundException e) {
                        // e.printStackTrace();
                        // TODO: how can the exception be passed on, outside of
                        // the runnable, and to whom?
                    } catch (Throwable e) {
                        logger.error("Error processing message: \r\n {}", message, e);

                    }
                }
            });
        } catch (RejectedExecutionException e) {
            logger.error("Exception while scheduling responder: ", e);
        }
    }

    private void handleReplyMessageReceived(final JoynrMessage message) {
        // TODO shall be moved to (not yet existing) ReplyHandler
        try {
            final Reply reply = objectMapper.readValue(message.getPayload(), Reply.class);
            final ReplyCaller callBack = replyCallerDirectory.getAndRemoveReplyCaller(reply.getRequestReplyId());
            if (callBack == null) {
                logger.warn("No reply caller found for id: " + reply.getRequestReplyId());
                return;
            }

            final String serializedPayload = message.getPayload();
            logger.debug("Parsed response from json with payload :" + serializedPayload);
            scheduler.execute(new Runnable() {

                public void run() {
                    try {
                        callBack.messageCallBack(reply);
                    } catch (Exception e) {
                        logger.error("error extracting reply payload from messageId: {} requestReplyId: {}",
                                     message.getId(),
                                     reply.getRequestReplyId());
                    }
                }
            });
        } catch (IOException e1) {
            logger.error("Exception while creating JsonReply object: ", e1);
        } catch (RejectedExecutionException e) {
            logger.error("Exception while scheduling callback: ", e);
        }
    }

    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN Dispatcher");
        shutdown = true;

        try {
            messageReceiver.shutdown(clear);
        } catch (Exception e) {
            logger.error("error shutting down messageReceiver");
        }

        try {
            scheduler.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down scheduler");
        }

        try {
            scheduler.awaitTermination(1000, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            scheduler.shutdownNow();
        }

        try {
            messageQueues.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down messageQueues");
        }

        try {
            replyCallerDirectory.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down replyCallerDirectory");
        }
    }

}
