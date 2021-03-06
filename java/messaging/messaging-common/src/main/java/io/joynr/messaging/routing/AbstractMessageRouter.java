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
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.TimeZone;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.inject.Singleton;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

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
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Message.MessageType;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.LocalAddress;

abstract public class AbstractMessageRouter implements MessageRouter, MulticastReceiverRegistrar, ShutdownListener {
    final static Set<Message.MessageType> MESSAGE_TYPE_REQUESTS = new HashSet<MessageType>(Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                                                                         Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                                                                                                         Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                                                                                                         Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST));

    final static Set<Message.MessageType> MESSAGE_TYPE_REPLIES = new HashSet<MessageType>(Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY,
                                                                                                        Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY));

    private static final Logger logger = LoggerFactory.getLogger(AbstractMessageRouter.class);
    private final SimpleDateFormat dateFormatter = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss:sss z");
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

    private List<MessageProcessedListener> messageProcessedListeners;
    private List<MessageWorker> messageWorkers;

    protected static class ProxyInformation {
        public String participantId;
        public ShutdownListener shutdownListener;
        public final Set<String> providerParticipantIds;

        public ProxyInformation(String participantId, ShutdownListener shutdownListener) {
            this.participantId = participantId;
            this.shutdownListener = shutdownListener;
            this.providerParticipantIds = new HashSet<String>();
        }
    }

    // Map weak reference to proxy object -> {proxyParticipantId, shutdownListener}
    private final ConcurrentHashMap<WeakReference<Object>, ProxyInformation> proxyMap;
    private final ReferenceQueue<Object> garbageCollectedProxiesQueue;
    private final ShutdownNotifier shutdownNotifier;
    private final ConcurrentHashMap<String, ProxyInformation> proxyParticipantIdToProxyInformationMap;

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
                                 ShutdownNotifier shutdownNotifier) {
        // CHECKSTYLE:ON
        dateFormatter.setTimeZone(TimeZone.getTimeZone("UTC"));
        this.routingTable = routingTable;
        this.scheduler = scheduler;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.routingTableCleanupIntervalMs = routingTableCleanupIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
        this.messagingSkeletonFactory = messagingSkeletonFactory;
        this.addressManager = addressManager;
        this.multicastReceiverRegistry = multicastReceiverRegistry;
        this.messageQueue = messageQueue;
        this.proxyMap = new ConcurrentHashMap<WeakReference<Object>, ProxyInformation>();
        this.proxyParticipantIdToProxyInformationMap = new ConcurrentHashMap<String, ProxyInformation>();
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
                    logger.debug("Removing garbage collected proxy participantId {}", proxyInformation.participantId);
                    removeNextHop(proxyInformation.participantId);
                    for (String providerParticipantId : proxyInformation.providerParticipantIds) {
                        removeNextHop(providerParticipantId);
                    }
                    shutdownNotifier.unregister(proxyInformation.shutdownListener);
                    proxyMap.remove(r);
                    proxyParticipantIdToProxyInformationMap.remove(proxyInformation.participantId);
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
        routeInternal(message, 0, 0);
    }

    protected Set<String> getRecipients(ImmutableMessage message) {
        return addressManager.getParticipantIdsForImmutableMessage(message);
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

        Set<String> recipients = getRecipients(message);
        if (recipients.isEmpty()) {
            //This can only happen in case of a multicast. Otherwise the participantId is taken directly from the ImmutableMessage.
            String errormessage = "Failed to route multicast publication: No recipient found for given message: "
                    + message.getTrackingInfo();
            logger.error("ERROR SENDING: aborting send. Error:", errormessage);
            finalizeMessageProcessing(message, false);
        }

        for (String recipient : recipients) {
            DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message,
                                                                                       delayMs,
                                                                                       recipient,
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

    private boolean isExpired(final ImmutableMessage message) {
        if (!message.isTtlAbsolute()) {
            // relative ttl is not supported
            return true;
        }
        return (message.getTtlMs() <= System.currentTimeMillis());
    }

    private void checkExpiry(final ImmutableMessage message) {
        if (!message.isTtlAbsolute()) {
            finalizeMessageProcessing(message, false);
            throw new JoynrRuntimeException("Relative ttl not supported");
        }

        if (isExpired(message)) {
            long currentTimeMillis = System.currentTimeMillis();
            String errorMessage = MessageFormat.format("Received expired message: (now ={0}). Dropping the message {1}",
                                                       currentTimeMillis,
                                                       message.getTrackingInfo());
            logger.trace(errorMessage);
            finalizeMessageProcessing(message, false);
            throw new JoynrMessageExpiredException(errorMessage);
        }
    }

    protected ImmutableMessage createReplyMessageWithError(ImmutableMessage requestMessage,
                                                           JoynrRuntimeException error) {
        // implemented only in sub classes
        return null;
    }

    private FailureAction createFailureAction(final DelayableImmutableMessage delayableMessage) {
        final FailureAction failureAction = new FailureAction() {
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
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error("ERROR SENDING: Aborting send of message {}, Error:",
                                 messageNotSent.getTrackingInfo(),
                                 error);

                    if (!isExpired(messageNotSent)
                            && messageNotSent.getType().equals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST)) {
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
                    delayMs = createDelayWithExponentialBackoff(sendMsgRetryIntervalMs,
                                                                delayableMessage.getRetriesCount());
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
        return failureAction;
    }

    protected void finalizeMessageProcessing(final ImmutableMessage message, boolean isMessageRoutingsuccessful) {
        if (message.isMessageProcessed()) {
            return;
        }
        message.messageProcessed();
        decreaseReferenceCountsForMessage(message, isMessageRoutingsuccessful);
        synchronized (messageProcessedListeners) {
            for (MessageProcessedListener messageProcessedListener : messageProcessedListeners) {
                messageProcessedListener.messageProcessed(message.getId());
            }
        }
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
            ProxyInformation proxyInformation = new ProxyInformation(proxyParticipantId, shutdownListener);
            if (proxyParticipantIdToProxyInformationMap.putIfAbsent(proxyParticipantId, proxyInformation) == null) {
                logger.debug("registerProxy called for {}", proxyParticipantId);
                proxyMap.put(new WeakReference<Object>(proxy, garbageCollectedProxiesQueue), proxyInformation);
            } else {
                throw new JoynrIllegalStateException("The proxy with " + proxyParticipantId
                        + " has already been registered.");
            }
        }
    }

    @Override
    public void registerProxyProviderParticipantIds(String proxyParticipantId, Set<String> providerParticipantIds) {
        if (proxyParticipantId == null || proxyParticipantId.isEmpty()) {
            throw new JoynrIllegalStateException("Proxy participant id is null or has an empty value."
                    + "Registration of proxy's provider participant ids failed.");
        }

        if (providerParticipantIds == null || providerParticipantIds.isEmpty()) {
            throw new JoynrIllegalStateException("Set of the provider participant ids is null or empty."
                    + "Registration of proxy's provider participant ids failed.");
        } else {
            if (providerParticipantIds.contains(null) || providerParticipantIds.contains("")) {
                throw new JoynrIllegalStateException("Set of the provider participant ids has an entry with an empty or null value."
                        + "Registration of proxy's provider participant ids failed.");
            }
        }

        proxyParticipantIdToProxyInformationMap.computeIfPresent(proxyParticipantId, (key, oldVal) -> {
            if (oldVal.providerParticipantIds.isEmpty()) {
                oldVal.providerParticipantIds.addAll(providerParticipantIds);
                return oldVal;
            } else {
                throw new JoynrIllegalStateException("The proxy with " + proxyParticipantId
                        + " already has registered providers. Registration of proxy's provider participant ids failed.");
            }
        });
    }

    private long createDelayWithExponentialBackoff(long sendMsgRetryIntervalMs, int retries) {
        long millis = sendMsgRetryIntervalMs + (long) ((2 ^ (retries)) * sendMsgRetryIntervalMs * Math.random());
        if (maxDelayMs >= sendMsgRetryIntervalMs && millis > maxDelayMs) {
            millis = maxDelayMs;
        }
        logger.trace("Created delay of {}ms in retry {}", millis, retries);
        return millis;
    }

    private void decreaseReferenceCountsForMessage(final ImmutableMessage message, boolean isMessageRoutingSuccessful) {
        MessageType type = message.getType();
        if (!isMessageRoutingSuccessful && MESSAGE_TYPE_REQUESTS.contains(type)) {
            if (!proxyParticipantIdToProxyInformationMap.containsKey(message.getSender())
                    && !(routingTable.get(message.getSender()) instanceof LocalAddress)) {
                // Sender (global / remote proxy) address is not required anymore to send a reply message
                routingTable.remove(message.getSender());
            }
        } else if (MESSAGE_TYPE_REPLIES.contains(type)
                && !proxyParticipantIdToProxyInformationMap.containsKey(message.getRecipient())
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
                    checkExpiry(message);

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
                        final long delayMs = createDelayWithExponentialBackoff(sendMsgRetryIntervalMs,
                                                                               delayableMessage.getRetriesCount() + 1);
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
