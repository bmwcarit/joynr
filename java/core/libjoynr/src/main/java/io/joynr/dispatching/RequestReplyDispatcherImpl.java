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
import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import io.joynr.accesscontrol.AccessController;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.IMessageReceivers;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.Callback;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import io.joynr.security.PlatformSecurityManager;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.system.routingtypes.ChannelAddress;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Default implementation of the Dispatcher interface.
 */
@Singleton
public class RequestReplyDispatcherImpl implements RequestReplyDispatcher {

    private Map<String, PayloadListener<?>> oneWayRecipients = Maps.newHashMap();
    private Map<String, RequestCaller> requestCallerDirectory = Maps.newHashMap();

    private ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>> messageQueue = new ConcurrentHashMap<String, ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>>();

    private ReplyCallerDirectory replyCallerDirectory;

    private RoutingTable messagingEndpointDirectory;
    protected RequestReplySender messageSender;

    private RequestInterpreter requestInterpreter;

    private static final Logger logger = LoggerFactory.getLogger(RequestReplyDispatcherImpl.class);

    private final MessageReceiver messageReceiver;

    private final ObjectMapper objectMapper;

    private PublicationManager publicationManager;

    private SubscriptionManager subscriptionManager;

    private boolean shutdown = false;

    private boolean registering = false;
    private ScheduledExecutorService cleanupScheduler;
    private AccessController accessController;
    private PlatformSecurityManager securityManager;

    @Inject
    // CHECKSTYLE:OFF
    public RequestReplyDispatcherImpl(RequestReplySender messageSender,
                                      IMessageReceivers messageReceivers,
                                      MessageReceiver messageReceiver,
                                      RoutingTable messagingEndpointDirectory,
                                      ReplyCallerDirectory replyCallerDirectory,
                                      @Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                      ObjectMapper objectMapper,
                                      PublicationManager publicationManager,
                                      SubscriptionManager subscriptionManager,
                                      RequestInterpreter requestInterpreter,
                                      @Named(JOYNR_SCHEDULER_CLEANUP) ScheduledExecutorService cleanupScheduler,
                                      AccessController accessController,
                                      PlatformSecurityManager securityManager) {
        // CHECKSTYLE:ON
        this.messageSender = messageSender;
        this.messageReceiver = messageReceiver;

        this.messagingEndpointDirectory = messagingEndpointDirectory;
        this.replyCallerDirectory = replyCallerDirectory;
        this.objectMapper = objectMapper;
        this.publicationManager = publicationManager;
        this.subscriptionManager = subscriptionManager;
        this.requestInterpreter = requestInterpreter;
        this.cleanupScheduler = cleanupScheduler;
        this.accessController = accessController;
        this.securityManager = securityManager;

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

        ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> messageList = messageQueue.remove(participantId);
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

        ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> messageList = messageQueue.remove(participantId);
        if (messageList != null) {
            for (ContentWithExpiryDate<JoynrMessage> messageItem : messageList) {
                if (!messageItem.isExpired()) {
                    executeRequestAndReply(requestCaller, messageItem.getContent());
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
                        + message.getPayload(), e.getMessage());
            }
        }

    }

    @Override
    public void removeRequestCaller(String participantId) {
        synchronized (requestCallerDirectory) {
            requestCallerDirectory.remove(participantId);
        }
    }

    @Override
    public void removeListener(final String participantId) {
        synchronized (oneWayRecipients) {
            oneWayRecipients.remove(participantId);
        }
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
        if (DispatcherUtils.isExpired(message.getExpiryDate())) {
            logger.debug("TTL expired, discarding message : {}", message.toLogMessage());
            return;
        }

        String type = message.getType();
        if (JoynrMessage.MESSAGE_TYPE_REPLY.equals(type)) {
            handleReplyMessageReceived(message);
        } else if (JoynrMessage.MESSAGE_TYPE_REQUEST.equals(type)) {
            handleRequestMessageReceived(message);
        } else if (JoynrMessage.MESSAGE_TYPE_ONE_WAY.equals(type)) {
            handleOneWayMessageReceived(message);
        } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST.equals(type)
                || JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST.equals(type)) {
            handleSubscriptionRequestReceived(message);
        } else if (JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP.equals(type)) {
            handleSubscriptionStopReceived(message);
        } else if (JoynrMessage.MESSAGE_TYPE_PUBLICATION.equals(type)) {
            handlePublicationReceived(message);
        }
    }

    private void handlePublicationReceived(final JoynrMessage message) {
        logger.info("Publication received");
        deliverPublication(message);
    }

    private void deliverPublication(JoynrMessage message) {
        SubscriptionPublication publication;
        try {
            publication = objectMapper.readValue(message.getPayload(), SubscriptionPublication.class);
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
                callBroadcastListener(subscriptionId, broadcastValues);
            } else {
                Class<?> receivedType = subscriptionManager.getAttributeType(subscriptionId);

                Object receivedObject;
                if (TypeReference.class.isAssignableFrom(receivedType)) {
                    TypeReference<?> typeRef = (TypeReference<?>) receivedType.newInstance();
                    receivedObject = objectMapper.convertValue(((List<?>) publication.getResponse()).get(0), typeRef);
                } else {
                    receivedObject = objectMapper.convertValue(((List<?>) publication.getResponse()).get(0),
                                                               receivedType);
                }

                subscriptionManager.touchSubscriptionState(subscriptionId);
                callSubscriptionListener(subscriptionId, receivedObject);
            }
        } catch (Exception e) {
            logger.error("Error delivering publication: {} : {}", e.getClass(), e.getMessage());
        }
    }

    private <T> void callSubscriptionListener(String subscriptionId, T attributeValue) {
        AttributeSubscriptionListener<T> listener = subscriptionManager.getSubscriptionListener(subscriptionId);
        if (listener == null) {
            logger.error("No subscription listener found for incoming publication!");
        } else {
            listener.onReceive(attributeValue);
        }
    }

    private void callBroadcastListener(String subscriptionId, Object[] broadcastValues) throws NoSuchMethodException,
                                                                                       IllegalAccessException,
                                                                                       InvocationTargetException {
        BroadcastSubscriptionListener broadcastSubscriptionListener = subscriptionManager.getBroadcastSubscriptionListener(subscriptionId);

        Class<?>[] broadcastTypes = getParameterTypesForBroadcastPublication(broadcastValues);
        Method receive = broadcastSubscriptionListener.getClass().getDeclaredMethod("onReceive", broadcastTypes);
        if (!receive.isAccessible()) {
            receive.setAccessible(true);
        }
        receive.invoke(broadcastSubscriptionListener, broadcastValues);
    }

    private Class<?>[] getParameterTypesForBroadcastPublication(Object[] broadcastValues) {
        List<Class<?>> parameterTypes = new ArrayList<Class<?>>(broadcastValues.length);
        for (int i = 0; i < broadcastValues.length; i++) {
            parameterTypes.add(broadcastValues[i].getClass());
        }
        return parameterTypes.toArray(new Class<?>[parameterTypes.size()]);
    }

    private void handleSubscriptionStopReceived(JoynrMessage message) {
        logger.info("Subscription stop received");
        try {

            SubscriptionStop subscriptionStop = objectMapper.readValue(message.getPayload(), SubscriptionStop.class);
            final String subscriptionId = subscriptionStop.getSubscriptionId();
            publicationManager.stopPublication(subscriptionId);
        } catch (Exception e) {
            logger.error("Error delivering subscription stop: {}", e.getMessage());
        }

    }

    private void handleSubscriptionRequestReceived(final JoynrMessage message) {

        // handle only if this message creator (userId) has permissions
        if (accessController.hasConsumerPermission(message)) {

            final String toParticipantId = message.getTo();
            final String fromParticipantId = message.getFrom();
            if (requestCallerDirectory.containsKey(toParticipantId)) {
                RequestCaller requestCaller = null;
                requestCaller = requestCallerDirectory.get(toParticipantId);
                SubscriptionRequest subscriptionRequest;
                try {
                    subscriptionRequest = objectMapper.readValue(message.getPayload(), SubscriptionRequest.class);
                    String replyChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
                    messagingEndpointDirectory.put(fromParticipantId, new ChannelAddress(replyChannelId));

                    publicationManager.addSubscriptionRequest(fromParticipantId,
                                                              toParticipantId,
                                                              subscriptionRequest,
                                                              requestCaller);
                } catch (JsonParseException e) {
                    logger.error("Error parsing request payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding request.",
                                 new String[]{ fromParticipantId, toParticipantId, message.getId(), e.getMessage() });
                } catch (JsonMappingException e) {
                    logger.error("Error parsing request payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding request.",
                                 new String[]{ fromParticipantId, toParticipantId, message.getId(), e.getMessage() });
                } catch (IOException e) {
                    logger.error("Error parsing request payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding request.",
                                 new String[]{ fromParticipantId, toParticipantId, message.getId(), e.getMessage() });
                }

            } else {
                // TODO handle unknown participantID
                logger.debug("Received subscriptionRequest for unknown participant. Discarding request.");
            }
        }

    }

    private void handleOneWayMessageReceived(final JoynrMessage message) {
        String toParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID);
        synchronized (oneWayRecipients) {
            final PayloadListener<?> listener = oneWayRecipients.get(toParticipantId);

            if (listener != null) {
                deliverMessageToListener(listener, message);
            } else {
                putMessage(toParticipantId, message, ExpiryDate.fromAbsolute(message.getExpiryDate()));
            }
        }
    }

    @SuppressWarnings("unchecked")
    private void deliverMessageToListener(final PayloadListener listener, final JoynrMessage message) {
        try {
            Object extractPayload = objectMapper.readValue(message.getPayload(), Object.class);
            listener.receive(extractPayload);
        } catch (JsonParseException e) {
            logger.error("Error parsing oneway payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding oneway.",
                         new String[]{ message.getFrom(), message.getFrom(), message.getId(), e.getMessage() });
        } catch (JsonMappingException e) {
            logger.error("Error parsing oneway payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding oneway.",
                         new String[]{ message.getFrom(), message.getFrom(), message.getId(), e.getMessage() });
        } catch (IOException e) {
            logger.error("Error parsing oneway payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding oneway.",
                         new String[]{ message.getFrom(), message.getFrom(), message.getId(), e.getMessage() });
        }
    }

    private void handleRequestMessageReceived(final JoynrMessage message) {
        // handle only if this message creator (userId) has permissions
        if (accessController.hasConsumerPermission(message)) {
            String fromParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID);
            String toParticipantId = message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID);
            String replyToChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
            if (replyToChannelId != null && !replyToChannelId.isEmpty()) {
                messagingEndpointDirectory.put(fromParticipantId, new ChannelAddress(replyToChannelId));
            }

            // TODO make sure that all requests (ie not one-way) also have replyTo
            // set, otherwise log an error

            if (requestCallerDirectory.containsKey(toParticipantId)) {
                executeRequestAndReply(requestCallerDirectory.get(toParticipantId), message);

            } else {
                putMessage(toParticipantId, message, ExpiryDate.fromAbsolute(message.getExpiryDate()));
                logger.info("No requestCaller found for participantId: {} queuing request message.", toParticipantId);
            }
        }
    }

    private void executeRequestAndReply(final RequestCaller requestCaller, final JoynrMessage message) {
        try {
            // TODO shall be moved to request manager and not handled by dispatcher
            Request request = objectMapper.readValue(message.getPayload(), Request.class);
            logger.debug("executing request from message: {} request: {}", message.getId(), request.getRequestReplyId());

            requestInterpreter.execute(new Callback<Reply>() {

                @Override
                public void onSuccess(Reply reply) {
                    try {
                        if (!DispatcherUtils.isExpired(message.getExpiryDate())) {
                            sendReply(message, reply);
                        } else {
                            logger.error("Error: reply {} is not send to caller, as the expiryDate of the requesting message {} has been reached.",
                                         reply,
                                         new Date(message.getExpiryDate()));
                        }
                    } catch (Exception e) {
                        logger.error("Error processing message: \r\n {} : error : {}", message, e);
                    }
                }

                @Override
                public void onFailure(JoynrException error) {
                    logger.error("Error processing message: \r\n {} ; error: {}", message, error);
                }
            },
                                       requestCaller,
                                       request);
        } catch (Throwable e) {
            logger.error("Error processing message: \r\n {}", message, e);
        }

    }

    private void sendReply(final JoynrMessage message, Reply reply) throws JsonGenerationException,
                                                                   JsonMappingException, IOException {
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
    }

    private void handleReplyMessageReceived(final JoynrMessage message) {
        // TODO shall be moved to (not yet existing) ReplyHandler

        Reply reply;
        try {
            reply = objectMapper.readValue(message.getPayload(), Reply.class);
        } catch (Exception e) {
            logger.error("Error parsing reply payload. msgId: {}. from: {} to: {}. Reason: {}. Discarding reply.",
                         new String[]{ message.getFrom(), message.getFrom(), message.getId(), e.getMessage() });
            return;
        }

        final ReplyCaller callBack = replyCallerDirectory.getAndRemoveReplyCaller(reply.getRequestReplyId());
        if (callBack == null) {
            logger.warn("No reply caller found for id: " + reply.getRequestReplyId());
            return;
        }

        final String serializedPayload = message.getPayload();
        logger.debug("Parsed response from json with payload :" + serializedPayload);

        callBack.messageCallBack(reply);
    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN Dispatcher");
        shutdown = true;

        try {
            messageReceiver.shutdown(clear);
        } catch (Exception e) {
            logger.error("error shutting down messageReceiver");
        }

        try {
            replyCallerDirectory.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down replyCallerDirectory");
        }
    }

    private void putMessage(final String participantId, JoynrMessage message, ExpiryDate incomingTtlExpirationDate_ms) {

        if (!messageQueue.containsKey(participantId)) {
            ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>> newMessageList = new ConcurrentLinkedQueue<ContentWithExpiryDate<JoynrMessage>>();
            messageQueue.putIfAbsent(participantId, newMessageList);
        }
        final ContentWithExpiryDate<JoynrMessage> messageItem = new ContentWithExpiryDate<JoynrMessage>(message,
                                                                                                        incomingTtlExpirationDate_ms);
        messageQueue.get(participantId).add(messageItem);
        cleanupScheduler.schedule(new Runnable() {

            @Override
            public void run() {
                messageQueue.get(participantId).remove(messageItem);
                JoynrMessage message = messageItem.getContent();
                logger.warn("TTL DISCARD. msgId: {} from: {} to: {} because it has expired. ", new String[]{
                        message.getId(), message.getFrom(), message.getTo() });

            }
        }, incomingTtlExpirationDate_ms.getRelativeTtl(), TimeUnit.MILLISECONDS);
    }
}
