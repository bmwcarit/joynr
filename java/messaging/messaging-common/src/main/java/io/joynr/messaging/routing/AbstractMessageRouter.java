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
package io.joynr.messaging.routing;

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.TimeZone;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import javax.inject.Singleton;

import org.apache.commons.lang.time.FastDateFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
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
import io.joynr.statusmetrics.MessageWorkerStatus;
import io.joynr.statusmetrics.StatusReceiver;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.system.RoutingTypes.WebSocketClientAddress;

abstract public class AbstractMessageRouter implements MessageRouter, ShutdownListener {
    private static final Logger logger = LoggerFactory.getLogger(AbstractMessageRouter.class);
    private static final FastDateFormat dateFormatter = FastDateFormat.getInstance("dd/MM/yyyy HH:mm:ss:sss z",
                                                                                   TimeZone.getTimeZone("UTC"));
    protected final RoutingTable routingTable;
    private ScheduledExecutorService scheduler;
    private long sendMsgRetryIntervalMs;
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

    private final MessageQueue messageQueue;
    private final StatusReceiver statusReceiver;

    private List<MessageProcessedListener> messageProcessedListeners;
    private List<MessageWorker> messageWorkers;

    private static class ProxyInformation {
        public String participantId;
        public ShutdownListener shutdownListener;

        public ProxyInformation(String participantId, ShutdownListener shutdownListener) {
            this.participantId = participantId;
            this.shutdownListener = shutdownListener;
        }
    }

    // Map weak reference to proxy object -> {proxyParticipantId, shutdownListener}
    private final ConcurrentHashMap<WeakReference<Object>, ProxyInformation> proxyMap;
    private final ReferenceQueue<Object> garbageCollectedProxiesQueue;
    private final ShutdownNotifier shutdownNotifier;

    @Inject
    @Singleton
    // CHECKSTYLE:OFF
    public AbstractMessageRouter(RoutingTable routingTable,
                                 @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs,
                                 MessagingStubFactory messagingStubFactory,
                                 MessagingSkeletonFactory messagingSkeletonFactory,
                                 AddressManager addressManager,
                                 MulticastReceiverRegistry multicastReceiverRegistry,
                                 MessageQueue messageQueue,
                                 ShutdownNotifier shutdownNotifier,
                                 StatusReceiver statusReceiver) {
        // CHECKSTYLE:ON
        this.routingTable = routingTable;
        this.scheduler = scheduler;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.routingTableCleanupIntervalMs = routingTableCleanupIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
        this.messagingSkeletonFactory = messagingSkeletonFactory;
        this.addressManager = addressManager;
        this.multicastReceiverRegistry = multicastReceiverRegistry;
        this.messageQueue = messageQueue;
        this.statusReceiver = statusReceiver;
        this.proxyMap = new ConcurrentHashMap<WeakReference<Object>, ProxyInformation>();
        this.garbageCollectedProxiesQueue = new ReferenceQueue<Object>();
        this.shutdownNotifier = shutdownNotifier;
        shutdownNotifier.registerForShutdown(this);
        messageProcessedListeners = new ArrayList<MessageProcessedListener>();
        startMessageWorkerThreads(maxParallelSends);
        startRoutingTableCleanupThread();
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

                // remove Routing table entries for proxies which have been garbage collected
                Reference<? extends Object> r;
                synchronized (garbageCollectedProxiesQueue) {
                    r = garbageCollectedProxiesQueue.poll();
                }
                while (r != null) {
                    ProxyInformation proxyInformation = proxyMap.get(r);
                    logger.debug("removing garbage collected proxy participantId {}", proxyInformation.participantId);
                    removeNextHop(proxyInformation.participantId);
                    shutdownNotifier.unregister(proxyInformation.shutdownListener);
                    proxyMap.remove(r);
                    synchronized (garbageCollectedProxiesQueue) {
                        r = garbageCollectedProxiesQueue.poll();
                    }
                }
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

        addToRoutingTable(participantId, address, isGloballyVisible, expiryDateMs);
    }

    public void addToRoutingTable(String participantId, Address address, boolean isGloballyVisible, long expiryDateMs) {
        routingTable.put(participantId, address, isGloballyVisible, expiryDateMs);
    }

    @Override
    public void route(final ImmutableMessage message) {
        checkExpiry(message);
        registerGlobalRoutingEntryIfRequired(message);
        routeInternal(message, 0, 0);
    }

    protected Set<Address> getAddresses(ImmutableMessage message) {
        return addressManager.getAddresses(message);
    }

    private void checkFoundAddresses(Set<Address> foundAddresses, ImmutableMessage message) {
        if (foundAddresses.isEmpty()) {
            if (Message.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
                // discard msg
                throw new JoynrMessageNotSentException("Failed to route multicast publication: No address found for given message: "
                        + message);
            } else if (Message.VALUE_MESSAGE_TYPE_PUBLICATION.equals(message.getType())) {
                // discard msg
                throw new JoynrMessageNotSentException("Failed to route publication: No address found for given message: "
                        + message);
            } else if (message.isReply()) {
                // discard msg
                throw new JoynrMessageNotSentException("Failed to route reply: No address found for given message: "
                        + message);
            } else {
                // any kind of request; retry routing
                throw new JoynrIllegalStateException("Unable to find address for recipient with participant ID "
                        + message.getRecipient());
            }
        }
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
            final long expiryDateMs = message.getTtlMs();

            routingTable.put(message.getSender(), address, isGloballyVisible, expiryDateMs);
        }
    }

    private void routeInternal(final ImmutableMessage message, long delayMs, final int retriesCount) {
        logger.trace("Scheduling message {} with delay {} and retries {}",
                     new Object[]{ message, delayMs, retriesCount });

        Set<Address> addresses = getAddresses(message);
        try {
            checkFoundAddresses(addresses, message);
        } catch (JoynrMessageNotSentException error) {
            logger.error(" ERROR SENDING: aborting send of messageId: {}. Error: {}",
                         new Object[]{ message.getId(), error.getMessage() });
            callMessageProcessedListeners(message.getId());
            return;
        } catch (Exception error) {
            logger.warn("PROBLEM SENDING, will retry. messageId: {}. Error: {} Message: {}",
                        new Object[]{ message.getId(), error.getClass().getName(), error.getMessage() });
            delayMs = createDelayWithExponentialBackoff(sendMsgRetryIntervalMs, retriesCount);
            logger.error("Rescheduling messageId: {} with delay {} ms, TTL is: {}",
                         message.getId(),
                         delayMs,
                         dateFormatter.format(message.getTtlMs()));
        }

        DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message,
                                                                                   delayMs,
                                                                                   addresses,
                                                                                   retriesCount);
        scheduleMessage(delayableMessage);
    }

    private void scheduleMessage(final DelayableImmutableMessage delayableMessage) {
        final String messageId = delayableMessage.getMessage().getId();
        if (maxRetryCount > -1) {
            final int retriesCount = delayableMessage.getRetriesCount();
            if (retriesCount > maxRetryCount) {
                logger.error("Max-retry-count (" + maxRetryCount + ") reached. Dropping message " + messageId);
                callMessageProcessedListeners(messageId);
                return;
            }
            if (retriesCount > 0) {
                logger.debug("Retry {}/{} sending message {}", retriesCount, maxRetryCount, messageId);
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
            String errorMessage = MessageFormat.format("ttl must be greater than 0 / ttl timestamp must be in the future: now: {0} ({1}) abs_ttl: {2} ({3}) msg_id: {4}",
                                                       currentTimeMillis,
                                                       dateFormatter.format(currentTimeMillis),
                                                       ttlExpirationDateMs,
                                                       dateFormatter.format(ttlExpirationDateMs),
                                                       message.getId());
            logger.error(errorMessage);
            callMessageProcessedListeners(message.getId());
            throw new JoynrMessageNotSentException(errorMessage);
        }
    }

    private FailureAction createFailureAction(final DelayableImmutableMessage delayableMessage) {
        final FailureAction failureAction = new FailureAction() {
            final String messageId = delayableMessage.getMessage().getId();
            private final AtomicBoolean failureActionExecutedOnce = new AtomicBoolean(false);

            @Override
            public void execute(Throwable error) {
                if (!failureActionExecutedOnce.compareAndSet(false, true)) {
                    logger.trace("Failure action for message with id {} already executed once. Ignoring further call.",
                                 messageId);
                    return;
                }
                if (error instanceof JoynrShutdownException) {
                    logger.warn("{}", error.getMessage());
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error(" ERROR SENDING:  aborting send of messageId: {}. Error: {}",
                                 new Object[]{ messageId, error.getMessage() });
                    callMessageProcessedListeners(messageId);
                    return;
                }
                logger.warn("PROBLEM SENDING, will retry. messageId: {}. Error: {} Message: {}",
                            new Object[]{ messageId, error.getClass().getName(), error.getMessage() });

                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = createDelayWithExponentialBackoff(sendMsgRetryIntervalMs,
                                                                delayableMessage.getRetriesCount());
                }

                if (Message.VALUE_MESSAGE_TYPE_MULTICAST.equals(delayableMessage.getMessage().getType())
                        || delayableMessage.getDestinationAddresses()
                                           .removeIf(address -> address instanceof WebSocketClientAddress)) {
                    // WebSocketClientAddresses have to be determined again in every retry because they change when a
                    // client is restarted, e.g. after a crash.
                    routeInternal(delayableMessage.getMessage(), delayMs, delayableMessage.getRetriesCount() + 1);
                    return;
                }

                delayableMessage.setDelay(delayMs);
                delayableMessage.setRetriesCount(delayableMessage.getRetriesCount() + 1);
                logger.error("Rescheduling messageId: {} with delay {} ms, TTL: {}, retries: {}",
                             messageId,
                             delayMs,
                             dateFormatter.format(delayableMessage.getMessage().getTtlMs()),
                             delayableMessage.getRetriesCount());
                try {
                    scheduleMessage(delayableMessage);
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
            private final AtomicInteger callCount = new AtomicInteger(numberOfCalls);

            @Override
            public void execute() {
                if (callCount.decrementAndGet() == 0) {
                    callMessageProcessedListeners(messageId);
                }
            }
        };
        return successAction;
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

    @Override
    public void registerProxy(Object proxy, String proxyParticipantId, ShutdownListener shutdownListener) {
        synchronized (garbageCollectedProxiesQueue) {
            proxyMap.putIfAbsent(new WeakReference<Object>(proxy, garbageCollectedProxiesQueue),
                                 new ProxyInformation(proxyParticipantId, shutdownListener));
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
                ImmutableMessage message;
                DelayableImmutableMessage delayableMessage = null;

                try {
                    statusReceiver.updateMessageWorkerStatus(number,
                                                             new MessageWorkerStatus(System.currentTimeMillis(), true));
                    delayableMessage = messageQueue.poll(1000, TimeUnit.MILLISECONDS);

                    if (delayableMessage == null) {
                        continue;
                    }

                    statusReceiver.updateMessageWorkerStatus(number,
                                                             new MessageWorkerStatus(System.currentTimeMillis(),
                                                                                     false));

                    message = delayableMessage.getMessage();
                    logger.trace("Starting processing of message {}", message);
                    checkExpiry(message);

                    Set<Address> addresses = delayableMessage.getDestinationAddresses();
                    if (addresses.isEmpty()) {
                        // try again immediately (message has been scheduled with delay for retry)
                        final int delayMs = 0;
                        routeInternal(message, delayMs, delayableMessage.getRetriesCount() + 1);
                        continue;
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
                    FailureAction failureAction = createFailureAction(delayableMessage);
                    for (Address address : addresses) {
                        logger.trace(">>>>> SEND message {} to address {}", message.getId(), address);

                        IMessagingStub messagingStub = messagingStubFactory.create(address);
                        messagingStub.transmit(message, messageProcessedAction, failureAction);
                    }
                } catch (InterruptedException e) {
                    logger.trace("Message Worker interrupted. Stopping.");
                    Thread.currentThread().interrupt();
                    return;
                } catch (Exception error) {
                    logger.error("error in scheduled message router thread: {}", error.getMessage());
                    FailureAction failureAction = createFailureAction(delayableMessage);
                    failureAction.execute(error);
                }
            }
            countDownLatch.countDown();
        }
    }
}
