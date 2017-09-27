package io.joynr.messaging.routing;

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

import java.text.DateFormat;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import javax.inject.Singleton;

import com.google.inject.Inject;
import com.google.inject.name.Named;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingMulticastSubscriber;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.SuccessAction;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

abstract public class AbstractMessageRouter implements MessageRouter, ShutdownListener {
    private Logger logger = LoggerFactory.getLogger(AbstractMessageRouter.class);
    private final RoutingTable routingTable;
    private static final DateFormat DateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
    private ScheduledExecutorService scheduler;
    private long sendMsgRetryIntervalMs;
    private long routingTableGracePeriodMs;
    private long routingTableCleanupIntervalMs;
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT)
    private long maxRetryCount = ConfigurableMessagingSettings.DEFAULT_ROUTING_MAX_RETRY_COUNT;
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS)
    private long maxDelayMs = ConfigurableMessagingSettings.DEFAULT_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF;
    private MessagingStubFactory messagingStubFactory;
    private final MessagingSkeletonFactory messagingSkeletonFactory;
    private AddressManager addressManager;
    protected final MulticastReceiverRegistry multicastReceiverRegistry;

    private DelayQueue<DelayableImmutableMessage> messageQueue;

    private List<MessageProcessedListener> messageProcessedListeners;
    private List<ScheduledFuture<?>> workerFutures;

    @Inject
    @Singleton
    // CHECKSTYLE:OFF
    public AbstractMessageRouter(RoutingTable routingTable,
                                 @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_GRACE_PERIOD_MS) long routingTableGracePeriodMs,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs,
                                 MessagingStubFactory messagingStubFactory,
                                 MessagingSkeletonFactory messagingSkeletonFactory,
                                 AddressManager addressManager,
                                 MulticastReceiverRegistry multicastReceiverRegistry,
                                 DelayQueue<DelayableImmutableMessage> messageQueue,
                                 ShutdownNotifier shutdownNotifier) {
        // CHECKSTYLE:ON
        this.routingTable = routingTable;
        this.scheduler = scheduler;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.routingTableGracePeriodMs = routingTableGracePeriodMs;
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
    }

    private void startMessageWorkerThreads(int numberOfWorkThreads) {
        workerFutures = new ArrayList<ScheduledFuture<?>>(numberOfWorkThreads);
        for (int i = 0; i < numberOfWorkThreads; i++) {
            ScheduledFuture<?> messageWorkerFuture = scheduler.schedule(new MessageWorker(i), 0, TimeUnit.MILLISECONDS);
            if (messageWorkerFuture == null) {
                logger.warn("scheduling messageWorker-" + i + "returned a null future. Cancel at shutdown not possible");
                continue;
            }
            workerFutures.add(messageWorkerFuture);
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
        performSubscriptionOperation(multicastId, providerParticipantId, new SubscriptionOperation() {
            @Override
            public void perform(IMessagingMulticastSubscriber messagingMulticastSubscriber) {
                messagingMulticastSubscriber.unregisterMulticastSubscription(multicastId);
            }
        });
    }

    private static interface SubscriptionOperation {
        void perform(IMessagingMulticastSubscriber messagingMulticastSubscriber);
    }

    private void performSubscriptionOperation(String multicastId,
                                              String providerParticipantId,
                                              SubscriptionOperation operation) {
        Address providerAddress = routingTable.get(providerParticipantId);
        IMessagingSkeleton messagingSkeleton = messagingSkeletonFactory.getSkeleton(providerAddress);
        if (messagingSkeleton != null && messagingSkeleton instanceof IMessagingMulticastSubscriber) {
            operation.perform((IMessagingMulticastSubscriber) messagingSkeleton);
        } else {
            logger.trace("No messaging skeleton found for address {}, not performing multicast subscription.",
                         providerAddress);
        }
    }

    @Override
    public void addNextHop(String participantId, Address address, boolean isGloballyVisible) {
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        routingTable.put(participantId, address, isGloballyVisible, expiryDateMs, isSticky);
    }

    @Override
    public void route(final ImmutableMessage message) {
        checkExpiry(message);
        registerGlobalRoutingEntryIfRequired(message);
        routeInternal(message, 0, 0);
    }

    protected void schedule(Runnable runnable, String messageId, long delay, TimeUnit timeUnit) {
        if (scheduler.isShutdown()) {
            JoynrShutdownException joynrShutdownEx = new JoynrShutdownException("MessageScheduler is shutting down already. Unable to send message [messageId: "
                    + messageId + "].");
            throw joynrShutdownEx;
        }
        scheduler.schedule(runnable, delay, timeUnit);
    }

    protected Set<Address> getAddresses(ImmutableMessage message) {
        return addressManager.getAddresses(message);
    }

    private void registerGlobalRoutingEntryIfRequired(final ImmutableMessage message) {
        if (!message.isReceivedFromGlobal()) {
            return;
        }

        String messageType = message.getType();

        if (!messageType.equals(Message.VALUE_MESSAGE_TYPE_REQUEST)
                && !messageType.equals(Message.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                && !messageType.equals(Message.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST)
                && !messageType.equals(Message.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)) {
            return;
        }

        String replyTo = message.getReplyTo();
        if (replyTo != null && !replyTo.isEmpty()) {
            Address address = RoutingTypesUtil.fromAddressString(replyTo);

            // If the message was received from global, the sender is globally visible by definition.
            final boolean isGloballyVisible = true;

            long expiryDateMs;
            try {
                expiryDateMs = Math.addExact(message.getTtlMs(), routingTableGracePeriodMs);
            } catch (ArithmeticException e) {
                expiryDateMs = Long.MAX_VALUE;
            }

            final boolean isSticky = false;
            routingTable.put(message.getSender(), address, isGloballyVisible, expiryDateMs, isSticky);
        }
    }

    private void routeInternal(final ImmutableMessage message, final long delayMs, final int retriesCount) {
        logger.trace("Scheduling {} with delay {} and retries {}", new Object[]{ message, delayMs, retriesCount });
        DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message, delayMs, retriesCount);
        if (maxRetryCount > -1) {
            if (retriesCount > maxRetryCount) {
                logger.error("Max-retry-count (" + maxRetryCount + ") reached. Dropping message " + message);
                callMessageProcessedListeners(message.getId());
                return;
            }
            if (retriesCount > 0) {
                logger.debug("Retry {}/{} sending message {}", retriesCount, maxRetryCount, message);
            }
        }
        messageQueue.put(delayableMessage);
    }

    private void checkExpiry(final ImmutableMessage message) {
        if (!message.isTtlAbsolute()) {
            callMessageProcessedListeners(message.getId());
            throw new JoynrRuntimeException("Relative ttl not supported");
        }

        long currentTimeMillis = System.currentTimeMillis();
        long ttlExpirationDateMs = message.getTtlMs();

        if (ttlExpirationDateMs <= currentTimeMillis) {
            String errorMessage = MessageFormat.format("ttl must be greater than 0 / ttl timestamp must be in the future: now: {0} abs_ttl: {1}",
                                                       currentTimeMillis,
                                                       ttlExpirationDateMs);
            logger.error(errorMessage);
            callMessageProcessedListeners(message.getId());
            throw new JoynrMessageNotSentException(errorMessage);
        }
    }

    private FailureAction createFailureAction(final ImmutableMessage message, final int retriesCount) {
        final FailureAction failureAction = new FailureAction() {
            final String messageId = message.getId();
            private boolean failureActionExecutedOnce = false;

            @Override
            public void execute(Throwable error) {
                synchronized (this) {
                    if (failureActionExecutedOnce) {
                        logger.trace("Failure action for message with id {} already executed once. Ignoring further call.",
                                     messageId);
                        return;
                    }
                    failureActionExecutedOnce = true;
                }
                if (error instanceof JoynrShutdownException) {
                    logger.warn("{}", error.getMessage());
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error(" ERROR SENDING:  aborting send of messageId: {}. Error: {}", new Object[]{ messageId,
                            error.getMessage() });
                    callMessageProcessedListeners(messageId);
                    return;
                }
                logger.warn("PROBLEM SENDING, will retry. messageId: {}. Error: {} Message: {}", new Object[]{
                        messageId, error.getClass().getName(), error.getMessage() });

                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = createDelayWithExponentialBackoff(sendMsgRetryIntervalMs, retriesCount);
                }

                logger.error("Rescheduling messageId: {} with delay " + delayMs + " ms, TTL is: {} ms",
                             messageId,
                             DateFormatter.format(message.getTtlMs()));
                try {
                    routeInternal(message, delayMs, retriesCount + 1);
                } catch (Exception e) {
                    logger.warn("Rescheduling of message failed (messageId {})", messageId);
                    callMessageProcessedListeners(messageId);
                }
                return;
            }
        };
        return failureAction;
    }

    private void callMessageProcessedListeners(final String messageId) {
        synchronized (messageProcessedListeners) {
            for (MessageProcessedListener messageProcessedListener : messageProcessedListeners) {
                messageProcessedListener.messageProcessed(messageId);
            }
        }
    }

    private SuccessAction createMessageProcessedAction(final String messageId, final int numberOfCalls) {
        final SuccessAction successAction = new SuccessAction() {
            private int callCount = numberOfCalls;

            @Override
            public void execute() {
                callCount--;
                if (callCount == 0) {
                    callMessageProcessedListeners(messageId);
                }
            }
        };
        return successAction;
    }

    @Override
    public void shutdown() {
        for (ScheduledFuture<?> workerFuture : workerFutures) {
            workerFuture.cancel(true);
        }
    }

    private long createDelayWithExponentialBackoff(long sendMsgRetryIntervalMs, int retries) {
        logger.trace("TRIES: " + retries);
        long millis = sendMsgRetryIntervalMs + (long) ((2 ^ (retries)) * sendMsgRetryIntervalMs * Math.random());
        if (maxDelayMs >= sendMsgRetryIntervalMs && millis > maxDelayMs) {
            millis = maxDelayMs;
            logger.trace("set MILLIS to " + millis + " since maxDelayMs is " + maxDelayMs);
        }
        logger.trace("MILLIS: " + millis);
        return millis;
    }

    class MessageWorker implements Runnable {
        private int number;

        public MessageWorker(int number) {
            this.number = number;
        }

        @Override
        public void run() {
            while (!Thread.interrupted()) {
                Thread.currentThread().setName("joynrMessageWorker-" + number);
                ImmutableMessage message = null;
                DelayableImmutableMessage delayableMessage;
                int retriesCount = 0;
                try {
                    delayableMessage = messageQueue.take();
                    retriesCount = delayableMessage.getRetriesCount();
                    message = delayableMessage.getMessage();
                    logger.trace("Starting processing of message {}", message);
                    checkExpiry(message);

                    Set<Address> addresses = getAddresses(message);
                    if (addresses.isEmpty()) {
                        throw new JoynrMessageNotSentException("Failed to send Message: No route for given participantId: "
                                + message.getRecipient());
                    }
                    SuccessAction messageProcessedAction = createMessageProcessedAction(message.getId(),
                                                                                        addresses.size());
                    // If multiple stub calls for a multicast to multiple destination addresses fail, the failure
                    // action is called for each failing stub call. Hence, the same failureAction has to be used.
                    // Otherwise, the message is rescheduled multiple times and the message queue is flooded with
                    // entries for the same message until the transmission of every entry is successful for all
                    // recipients. Also the recipients are flooded with the same message.
                    // Open issue:
                    // If only some stub calls fail, the rescheduled message will be sent to all its recipients again,
                    // no matter if an earlier transmission attempt was already successful or not.
                    FailureAction failureAction = createFailureAction(message, retriesCount);
                    for (Address address : addresses) {
                        logger.trace(">>>>> SEND  {} to address {}", message, address);

                        IMessagingStub messagingStub = messagingStubFactory.create(address);
                        messagingStub.transmit(message, messageProcessedAction, failureAction);
                    }
                } catch (InterruptedException e) {
                    logger.trace("Message Worker interrupted. Stopping.");
                    Thread.currentThread().interrupt();
                    return;
                } catch (Exception error) {
                    logger.error("error in scheduled message router thread: {}", error.getMessage());
                    FailureAction failureAction = createFailureAction(message, retriesCount);
                    failureAction.execute(error);
                }
            }
        }
    }
}
