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
package io.joynr.messaging.routing;

import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Optional;
import java.util.Set;
import java.util.TimeZone;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.inject.Singleton;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.accesscontrol.AccessController;
import io.joynr.accesscontrol.HasConsumerPermissionCallback;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingMulticastSubscriber;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Message.MessageType;
import joynr.MutableMessage;
import joynr.Reply;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.LocalAddress;

public class CcMessageRouter
        implements MessageRouter, MessageProcessedHandler, MulticastReceiverRegistrar, ShutdownListener {

    final static Set<Message.MessageType> MESSAGE_TYPE_REQUESTS = new HashSet<MessageType>(Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                                                                         Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                                                                                                         Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                                                                                                         Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST));

    final static Set<Message.MessageType> MESSAGE_TYPE_REPLIES = new HashSet<MessageType>(Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY,
                                                                                                        Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY));

    private static final Logger logger = LoggerFactory.getLogger(CcMessageRouter.class);
    private final SimpleDateFormat dateFormatter = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss:sss z");
    protected final RoutingTable routingTable;
    private ScheduledExecutorService scheduler;
    private long routingTableCleanupIntervalMs;
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT)
    private long maxRetryCount = ConfigurableMessagingSettings.DEFAULT_ROUTING_MAX_RETRY_COUNT;
    private MessagingStubFactory messagingStubFactory;
    private final MessagingSkeletonFactory messagingSkeletonFactory;
    private AddressManager addressManager;
    protected final MulticastReceiverRegistry multicastReceiverRegistry;

    private final MessageQueue messageQueue;

    private List<MessageProcessedListener> messageProcessedListeners;
    private List<MessageWorker> messageWorkers;

    private AccessController accessController;
    private boolean enableAccessControl;
    private ObjectMapper objectMapper;

    @Inject
    @Singleton
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public CcMessageRouter(RoutingTable routingTable,
                           @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                           @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                           @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs,
                           MessagingStubFactory messagingStubFactory,
                           MessagingSkeletonFactory messagingSkeletonFactory,
                           AddressManager addressManager,
                           MulticastReceiverRegistry multicastReceiverRegistry,
                           AccessController accessController,
                           @Named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE) boolean enableAccessControl,
                           MessageQueue messageQueue,
                           ShutdownNotifier shutdownNotifier,
                           ObjectMapper objectMapper) {
        dateFormatter.setTimeZone(TimeZone.getTimeZone("UTC"));
        this.routingTable = routingTable;
        this.scheduler = scheduler;
        this.routingTableCleanupIntervalMs = routingTableCleanupIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
        this.messagingSkeletonFactory = messagingSkeletonFactory;
        this.addressManager = addressManager;
        this.multicastReceiverRegistry = multicastReceiverRegistry;
        this.messageQueue = messageQueue;
        shutdownNotifier.registerForShutdown(this);
        messageProcessedListeners = new ArrayList<MessageProcessedListener>();
        startMessageWorkerThreads(maxParallelSends);
        startRoutingTableCleanupThread();

        this.accessController = accessController;
        this.enableAccessControl = enableAccessControl;
        this.objectMapper = objectMapper;
    }

    @Override
    public void routeIn(ImmutableMessage message) {
        route(message);
    }

    @Override
    public void routeOut(ImmutableMessage message) {
        route(message);
    }

    @Override
    public void registerMessageProcessedListener(MessageProcessedListener messageProcessedListener) {
        synchronized (messageProcessedListeners) {
            messageProcessedListeners.add(messageProcessedListener);
        }
    }

    @Override
    public void unregisterMessageProcessedListener(MessageProcessedListener messageProcessedListener) {
        synchronized (messageProcessedListeners) {
            messageProcessedListeners.remove(messageProcessedListener);
        }
    }

    @Override
    public void messageProcessed(String messageId) {
        synchronized (messageProcessedListeners) {
            for (MessageProcessedListener messageProcessedListener : messageProcessedListeners) {
                messageProcessedListener.messageProcessed(messageId);
            }
        }
    }

    @Override
    public void removeNextHop(String participantId) {
        routingTable.remove(participantId);
    }

    @Override
    public boolean resolveNextHop(String participantId) {
        return routingTable.containsKey(participantId);
    }

    @Override
    public void addMulticastReceiver(final String multicastId,
                                     String subscriberParticipantId,
                                     String providerParticipantId) {
        if (!routingTable.containsKey(providerParticipantId)) {
            logger.error("The provider {} is not known, multicast receiver will not be added.", providerParticipantId);
            throw new JoynrIllegalStateException("The provider " + providerParticipantId
                    + " is not known, multicast receiver will not be added.");
        }
        logger.trace("Adding multicast receiver {} for multicast {} on provider {}",
                     subscriberParticipantId,
                     multicastId,
                     providerParticipantId);
        multicastReceiverRegistry.registerMulticastReceiver(multicastId, subscriberParticipantId);
        performSubscriptionOperation(multicastId, providerParticipantId, new SubscriptionOperation() {
            @Override
            public void perform(IMessagingMulticastSubscriber messagingMulticastSubscriber) {
                messagingMulticastSubscriber.registerMulticastSubscription(multicastId);
            }
        });
    }

    @Override
    public void removeMulticastReceiver(final String multicastId,
                                        String subscriberParticipantId,
                                        String providerParticipantId) {
        multicastReceiverRegistry.unregisterMulticastReceiver(multicastId, subscriberParticipantId);
        if (!routingTable.containsKey(providerParticipantId)) {
            logger.error("The provider {} is not known, multicast receiver cannot be removed.", providerParticipantId);
            throw new JoynrIllegalStateException("The provider " + providerParticipantId
                    + " is not known, multicast receiver will not be added.");
        }
        performSubscriptionOperation(multicastId, providerParticipantId, new SubscriptionOperation() {
            @Override
            public void perform(IMessagingMulticastSubscriber messagingMulticastSubscriber) {
                messagingMulticastSubscriber.unregisterMulticastSubscription(multicastId);
            }
        });
    }

    @Override
    public void addNextHop(String participantId, Address address, boolean isGloballyVisible) {
        final long expiryDateMs = Long.MAX_VALUE;
        if (!routingTable.put(participantId, address, isGloballyVisible, expiryDateMs)) {
            throw (new JoynrRuntimeException("unable to addNextHop, as RoutingTable.put failed"));
        }
    }

    @Override
    public void prepareForShutdown() {
        messageQueue.waitForQueueToDrain();
    }

    @Override
    public void shutdown() {
        CountDownLatch countDownLatch = new CountDownLatch(messageWorkers.size());
        for (MessageWorker worker : messageWorkers) {
            worker.stopWorker(countDownLatch);
        }
        try {
            countDownLatch.await(1500, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            logger.error("Interrupted while waiting for message workers to stop.", e);
        }
    }

    protected void route(final ImmutableMessage message) {
        if (enableAccessControl) {
            logger.debug("CcMessageRouter hasConsumerPermission CONSUMER ACCESS: {}", message);
            accessController.hasConsumerPermission(message, new HasConsumerPermissionCallback() {
                @Override
                public void hasConsumerPermission(boolean hasPermission) {
                    if (hasPermission) {
                        try {
                            routeInternal(message, 0, 0);
                        } catch (Exception e) {
                            logger.error("Error processing message. Message {} is dropped: {}",
                                         message.getId(),
                                         e.getMessage());
                            finalizeMessageProcessing(message, false);
                        }
                    } else {
                        logger.warn("Dropping message {} from {} to {} because of insufficient access rights",
                                    message.getId(),
                                    message.getSender(),
                                    message.getRecipient());
                        finalizeMessageProcessing(message, false);
                    }
                }
            });
        } else {
            routeInternal(message, 0, 0);
        }
    }

    protected ImmutableMessage createReplyMessageWithError(ImmutableMessage requestMessage,
                                                           JoynrRuntimeException error) {
        try {
            String deserializedPayload = new String(requestMessage.getUnencryptedBody(), StandardCharsets.UTF_8);
            final Request request = objectMapper.readValue(deserializedPayload, Request.class);
            String requestReplyId = request.getRequestReplyId();

            MutableMessage replyMessage = new MutableMessage();
            replyMessage.setType(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY);
            if (requestMessage.getEffort() != null) {
                replyMessage.setEffort(requestMessage.getEffort());
            }
            replyMessage.setSender(requestMessage.getRecipient());
            replyMessage.setRecipient(requestMessage.getSender());
            replyMessage.setTtlAbsolute(true);
            replyMessage.setTtlMs(requestMessage.getTtlMs());
            Reply reply = new Reply(requestReplyId, error);
            String serializedPayload = objectMapper.writeValueAsString(reply);
            replyMessage.setPayload(serializedPayload.getBytes(StandardCharsets.UTF_8));
            Map<String, String> customHeaders = new HashMap<>();
            customHeaders.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId);
            replyMessage.setCustomHeaders(customHeaders);
            replyMessage.setCompressed(requestMessage.isCompressed());
            return replyMessage.getImmutableMessage();
        } catch (Exception e) {
            logger.error("Failed to prepare ReplyMessageWithError for msgId: {}. from: {} to: {}. Reason: {}",
                         requestMessage.getId(),
                         requestMessage.getSender(),
                         requestMessage.getRecipient(),
                         e.getMessage());
            return null;
        }
    }

    protected void finalizeMessageProcessing(final ImmutableMessage message, boolean isMessageRoutingsuccessful) {
        if (message.isMessageProcessed()) {
            return;
        }
        message.messageProcessed();
        decreaseReferenceCountsForMessage(message, isMessageRoutingsuccessful);
        messageProcessed(message.getId());
    }

    private void startMessageWorkerThreads(int numberOfWorkThreads) {
        messageWorkers = new ArrayList<MessageWorker>(numberOfWorkThreads);
        for (int i = 0; i < numberOfWorkThreads; i++) {
            MessageWorker messageWorker = new MessageWorker(i);
            scheduler.schedule(messageWorker, 0, TimeUnit.MILLISECONDS);
            messageWorkers.add(messageWorker);
        }
    }

    private void startRoutingTableCleanupThread() {
        scheduler.scheduleWithFixedDelay(new Runnable() {
            @Override
            public void run() {
                routingTable.purge();
            }
        }, routingTableCleanupIntervalMs, routingTableCleanupIntervalMs, TimeUnit.MILLISECONDS);
    }

    private static interface SubscriptionOperation {
        void perform(IMessagingMulticastSubscriber messagingMulticastSubscriber);
    }

    private void performSubscriptionOperation(String multicastId,
                                              String providerParticipantId,
                                              SubscriptionOperation operation) {
        Address providerAddress = routingTable.get(providerParticipantId);
        Optional<IMessagingSkeleton> messagingSkeleton = messagingSkeletonFactory.getSkeleton(providerAddress);
        if (messagingSkeleton.isPresent() && messagingSkeleton.get() instanceof IMessagingMulticastSubscriber) {
            operation.perform((IMessagingMulticastSubscriber) messagingSkeleton.get());
        } else {
            logger.trace("No messaging skeleton found for address {}, not performing multicast subscription.",
                         providerAddress);
        }
    }

    private void checkFoundAddress(Optional<Address> foundAddress, ImmutableMessage message) {
        if (!foundAddress.isPresent()) {
            if (Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
                // discard msg
                throw new JoynrMessageNotSentException("Failed to route multicast publication: No address found for given message.");
            } else if (Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION.equals(message.getType())) {
                // discard msg
                throw new JoynrMessageNotSentException("Failed to route publication: No address found for given message.");
            } else if (message.isReply()) {
                // discard msg
                throw new JoynrMessageNotSentException("Failed to route reply: No address found for given message.");
            } else {
                // any kind of request; retry routing
                throw new JoynrIllegalStateException("Unable to find addresses for message.");
            }
        }
    }

    private void routeInternal(final ImmutableMessage message, long delayMs, final int retriesCount) {
        logger.trace("Scheduling message {} with delay {} and retries {}", message, delayMs, retriesCount);

        Map<Address, Set<String>> recipientMap = addressManager.getParticipantIdMap(message);
        if (recipientMap.isEmpty()) {
            //This can only happen in case of a multicast. Otherwise the participantId is taken directly from the ImmutableMessage.
            String errormessage = "Failed to route multicast publication: No recipient found for given message: "
                    + message.getTrackingInfo();
            logger.error("ERROR SENDING: aborting send. Error: {}", errormessage);
            finalizeMessageProcessing(message, false);
        }

        for (Entry<Address, Set<String>> entry : recipientMap.entrySet()) {
            // Schedule only a single message per destination address to prevent duplication of messages.
            // This can only happen for multicast messages with multiple recipients.
            DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message,
                                                                                       delayMs,
                                                                                       entry.getValue(),
                                                                                       retriesCount);
            scheduleMessage(delayableMessage);
        }
    }

    private void scheduleMessage(final DelayableImmutableMessage delayableMessage) {
        final int retriesCount = delayableMessage.getRetriesCount();
        if (maxRetryCount > -1) {
            if (retriesCount > maxRetryCount) {
                logger.error("Max-retry-count ({}) reached. Dropping message {}",
                             maxRetryCount,
                             delayableMessage.getMessage().getTrackingInfo());
                finalizeMessageProcessing(delayableMessage.getMessage(), false);
                return;
            }
        }
        if (retriesCount > 0) {
            logger.debug("Retry {}/{} sending message {}",
                         retriesCount,
                         maxRetryCount,
                         delayableMessage.getMessage().getTrackingInfo());
        }
        messageQueue.put(delayableMessage);
    }

    private FailureAction createFailureAction(final DelayableImmutableMessage delayableMessage) {
        return new FailureAction() {
            private final AtomicBoolean failureActionExecutedOnce = new AtomicBoolean(false);

            @Override
            public void execute(Throwable error) {
                ImmutableMessage messageNotSent = delayableMessage.getMessage();

                if (!failureActionExecutedOnce.compareAndSet(false, true)) {
                    logger.trace("Failure action for message {} already executed once. Ignoring further call.",
                                 messageNotSent.getTrackingInfo());
                    return;
                }
                if (error instanceof JoynrShutdownException) {
                    logger.warn("Caught JoynrShutdownException while handling message {}:",
                                messageNotSent.getTrackingInfo(),
                                error);
                    return;
                } else if (error instanceof JoynrMessageExpiredException) {
                    logger.error("ERROR SENDING: Aborting send of message {}, Error:",
                                 messageNotSent.getTrackingInfo(),
                                 error);
                    finalizeMessageProcessing(messageNotSent, false);
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error("ERROR SENDING: Aborting send of message {}, Error:",
                                 messageNotSent.getTrackingInfo(),
                                 error);

                    if (!MessageRouterUtil.isExpired(messageNotSent)
                            && Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST.equals(messageNotSent.getType())) {
                        ImmutableMessage replyMessage = createReplyMessageWithError(messageNotSent,
                                                                                    (JoynrMessageNotSentException) error);
                        if (replyMessage != null) {
                            routeInternal(replyMessage, 0, 0);
                            // pretend success to not decrease the routing entry refCnt before the reply message is processed
                            finalizeMessageProcessing(messageNotSent, true);
                            return;
                        }
                    }
                    finalizeMessageProcessing(messageNotSent, false);
                    return;
                }
                logger.warn("PROBLEM SENDING, will retry. message: {}, Error:",
                            messageNotSent.getTrackingInfo(),
                            error);

                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = MessageRouterUtil.createDelayWithExponentialBackoff(delayableMessage.getRetriesCount());
                }
                delayableMessage.setDelay(delayMs);
                delayableMessage.setRetriesCount(delayableMessage.getRetriesCount() + 1);
                logger.warn("Rescheduling message {} with delay {} ms, TTL: {}, retries: {}",
                            messageNotSent.getTrackingInfo(),
                            delayMs,
                            dateFormatter.format(delayableMessage.getMessage().getTtlMs()),
                            delayableMessage.getRetriesCount());
                try {
                    scheduleMessage(delayableMessage);
                } catch (Exception e) {
                    logger.warn("Rescheduling of message {} failed", messageNotSent.getTrackingInfo());
                    finalizeMessageProcessing(messageNotSent, false);
                }
            }
        };
    }

    private SuccessAction createMessageProcessedAction(final ImmutableMessage message) {
        final SuccessAction successAction = new SuccessAction() {

            @Override
            public void execute() {
                /* In case of a multicast, the listener is already called when the message is sent to the first recipient,
                 * i.e. the message is not fully processed if it has multiple recipients.
                 * This is not a problem because the MessageProcessedListener is part of the backpressure mechanism
                 * which ignores multicast messages (only request messages are counted), see
                 * MqttMessagingSkeleton.transmit and SharedSubscriptionsMqttMessagingSkeleton.requestAccepted.
                 *
                 * Also, since a multicast does not lead to a decrease of the reference count, calling this multiple times
                 * won't be an issue there.*/
                finalizeMessageProcessing(message, true);
            }
        };
        return successAction;
    }

    private void decreaseReferenceCountsForMessage(final ImmutableMessage message, boolean isMessageRoutingSuccessful) {
        MessageType type = message.getType();
        if (!isMessageRoutingSuccessful && MESSAGE_TYPE_REQUESTS.contains(type)) {
            if (!(routingTable.get(message.getSender()) instanceof LocalAddress)) {
                // Sender (global / remote proxy) address is not required anymore to send a reply message
                routingTable.remove(message.getSender());
            }
        } else if (MESSAGE_TYPE_REPLIES.contains(type)
                && !(routingTable.get(message.getRecipient()) instanceof LocalAddress)) {
            if (type == MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY
                    && !(routingTable.get(message.getSender()) instanceof InProcessAddress)) {
                return;
            }
            // Recipient (global / remote proxy) address is not required anymore after the reply message has been sent
            routingTable.remove(message.getRecipient());
        }
    }

    class MessageWorker implements Runnable {
        private Logger logger = LoggerFactory.getLogger(MessageWorker.class);
        private int number;
        private volatile CountDownLatch countDownLatch;
        private volatile boolean stopped;

        public MessageWorker(int number) {
            this.number = number;
            countDownLatch = null;
            stopped = false;
        }

        void stopWorker(CountDownLatch countDownLatch) {
            this.countDownLatch = countDownLatch;
            stopped = true;
        }

        @Override
        public void run() {
            Thread.currentThread().setName("joynrMessageWorker-" + number);

            while (!stopped) {
                DelayableImmutableMessage delayableMessage = null;
                FailureAction failureAction = null;

                try {
                    delayableMessage = messageQueue.poll(1000, TimeUnit.MILLISECONDS);

                    if (delayableMessage == null) {
                        continue;
                    }

                    ImmutableMessage message = delayableMessage.getMessage();
                    logger.trace("Starting processing of message {}", message);

                    MessageRouterUtil.checkExpiry(message);

                    Optional<Address> optionalAddress = addressManager.getAddressForDelayableImmutableMessage(delayableMessage);
                    try {
                        checkFoundAddress(optionalAddress, message);
                    } catch (JoynrMessageNotSentException error) {
                        logger.error("ERROR SENDING: aborting send of message: {}. Error:",
                                     message.getTrackingInfo(),
                                     error);
                        finalizeMessageProcessing(message, false);
                        continue;
                    } catch (Exception error) {
                        logger.debug("ERROR SENDING: retrying send of message. Error:", error);
                        final long delayMs = MessageRouterUtil.createDelayWithExponentialBackoff(delayableMessage.getRetriesCount()
                                + 1);
                        delayableMessage.setDelay(delayMs);
                        delayableMessage.setRetriesCount(delayableMessage.getRetriesCount() + 1);
                        scheduleMessage(delayableMessage);
                        continue;
                    }

                    SuccessAction messageProcessedAction = createMessageProcessedAction(message);
                    failureAction = createFailureAction(delayableMessage);
                    Address address = optionalAddress.get();
                    logger.trace(">>>>> SEND message {} to address {}", message.getId(), address);

                    IMessagingStub messagingStub = messagingStubFactory.create(address);
                    messagingStub.transmit(message, messageProcessedAction, failureAction);
                } catch (InterruptedException e) {
                    logger.trace("Message Worker interrupted. Stopping.");
                    Thread.currentThread().interrupt();
                    return;
                } catch (Exception error) {
                    if (delayableMessage == null) {
                        logger.error("Error in scheduled message router thread. delayableMessage == null, continuing. Error:",
                                     error);
                        continue;
                    }
                    logger.error("Error in scheduled message router thread:", error);
                    if (failureAction == null) {
                        failureAction = createFailureAction(delayableMessage);
                    }
                    failureAction.execute(error);
                }
            }
            countDownLatch.countDown();
        }
    }
}
