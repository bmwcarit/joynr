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

import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TimeZone;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.SuccessAction;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import joynr.ImmutableMessage;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

/**
 * MessageRouter implementation which adds hops to its parent and tries to resolve unknown addresses at its parent
 */
@Singleton
public class LibJoynrMessageRouter implements MessageRouter, MulticastReceiverRegistrar, ShutdownListener {
    private static final Logger logger = LoggerFactory.getLogger(LibJoynrMessageRouter.class);
    private final SimpleDateFormat dateFormatter = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss:sss z");
    private ScheduledExecutorService scheduler;
    private long sendMsgRetryIntervalMs;
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT)
    private long maxRetryCount = ConfigurableMessagingSettings.DEFAULT_ROUTING_MAX_RETRY_COUNT;
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS)
    private long maxDelayMs = ConfigurableMessagingSettings.DEFAULT_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF;
    private MessagingStubFactory messagingStubFactory;

    private final MessageQueue incomingMessageQueue;
    private final MessageQueue outgoingMessageQueue;

    private List<IncomingMessageWorker> incomingMessageWorkers;
    private List<OutgoingMessageWorker> outgoingMessageWorkers;

    private static interface DeferrableRegistration {
        void register();
    }

    private static class ParticipantIdAndIsGloballyVisibleHolder {
        final String participantId;
        final boolean isGloballyVisible;

        public ParticipantIdAndIsGloballyVisibleHolder(String participantId, boolean isGloballyVisible) {
            this.participantId = participantId;
            this.isGloballyVisible = isGloballyVisible;
        }
    }

    private Address parentRouterMessagingAddress;
    private RoutingProxy parentRouter;
    private Address incomingAddress;
    private Dispatcher dispatcher;
    private Set<ParticipantIdAndIsGloballyVisibleHolder> deferredParentHopsParticipantIds = new HashSet<>();
    private Map<String, DeferrableRegistration> deferredMulticastRegistrations = new HashMap<>();
    private boolean ready = false;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LibJoynrMessageRouter(@Named(SystemServicesSettings.LIBJOYNR_MESSAGING_ADDRESS) Address incomingAddress,
                                 @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                                 MessagingStubFactory messagingStubFactory,
                                 MessageQueue incomingMessageQueue,
                                 MessageQueue outgoingMessageQueue,
                                 ShutdownNotifier shutdownNotifier,
                                 Dispatcher dispatcher) {
        // CHECKSTYLE:ON
        dateFormatter.setTimeZone(TimeZone.getTimeZone("UTC"));
        this.scheduler = scheduler;
        this.dispatcher = dispatcher;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
        this.incomingMessageQueue = incomingMessageQueue;
        this.outgoingMessageQueue = outgoingMessageQueue;
        shutdownNotifier.registerForShutdown(this);
        if (maxParallelSends < 2) {
            maxParallelSends = 2;
        }
        startMessageWorkerThreads(maxParallelSends);
        this.incomingAddress = incomingAddress;
    }

    @Override
    public boolean resolveNextHop(String participantId) {
        return true;
    }

    private void startMessageWorkerThreads(int numberOfWorkThreads) {
        final int numberOfIncomingMessageWorkers = Math.floorDiv(numberOfWorkThreads, 2);
        incomingMessageWorkers = new ArrayList<IncomingMessageWorker>(numberOfIncomingMessageWorkers);
        for (int i = 0; i < numberOfIncomingMessageWorkers; i++) {
            IncomingMessageWorker messageWorker = new IncomingMessageWorker(i);
            scheduler.schedule(messageWorker, 0, TimeUnit.MILLISECONDS);
            incomingMessageWorkers.add(messageWorker);
        }

        final int numberOfOutgoingMessageWorkers = numberOfWorkThreads - numberOfIncomingMessageWorkers;
        outgoingMessageWorkers = new ArrayList<OutgoingMessageWorker>(numberOfOutgoingMessageWorkers);
        for (int i = 0; i < numberOfOutgoingMessageWorkers; i++) {
            OutgoingMessageWorker messageWorker = new OutgoingMessageWorker(i);
            scheduler.schedule(messageWorker, 0, TimeUnit.MILLISECONDS);
            outgoingMessageWorkers.add(messageWorker);
        }
    }

    @Override
    public void addNextHop(final String participantId, final Address address, final boolean isGloballyVisible) {
        synchronized (this) {
            if (!ready) {
                deferredParentHopsParticipantIds.add(new ParticipantIdAndIsGloballyVisibleHolder(participantId,
                                                                                                 isGloballyVisible));
                return;
            }
        }
        addNextHopToParent(participantId, isGloballyVisible);
    }

    @Override
    public void removeNextHop(final String participantId) {
        if (parentRouter != null) {
            removeNextHopFromParent(participantId);
        }
    }

    private void addNextHopToParent(String participantId, boolean isGloballyVisible) {
        logger.trace("Adding next hop with participantId {} to parent router", participantId);
        if (incomingAddress instanceof WebSocketAddress) {
            parentRouter.addNextHop(participantId, (WebSocketAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof WebSocketClientAddress) {
            parentRouter.addNextHop(participantId, (WebSocketClientAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof BinderAddress) {
            parentRouter.addNextHop(participantId, (BinderAddress) incomingAddress, isGloballyVisible);
        } else {
            throw new ProviderRuntimeException("Failed to add next hop to parent: unknown address type "
                    + incomingAddress.getClass().getSimpleName());
        }
    }

    private void removeNextHopFromParent(String participantId) {
        logger.trace("Removing next hop with participantId {} from parent router", participantId);
        parentRouter.removeNextHop(participantId);
    }

    @Override
    public void addMulticastReceiver(final String multicastId,
                                     final String subscriberParticipantId,
                                     final String providerParticipantId) {
        logger.trace("Adding multicast receiver {} for multicast {} on provider {}",
                     subscriberParticipantId,
                     multicastId,
                     providerParticipantId);
        DeferrableRegistration registerWithParent = new DeferrableRegistration() {
            @Override
            public void register() {
                parentRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
            }
        };
        synchronized (this) {
            if (!ready) {
                deferredMulticastRegistrations.put(multicastId + subscriberParticipantId + providerParticipantId,
                                                   registerWithParent);
                return;
            }
        }
        registerWithParent.register();
    }

    @Override
    public void removeMulticastReceiver(String multicastId,
                                        String subscriberParticipantId,
                                        String providerParticipantId) {
        synchronized (this) {
            if (!ready) {
                deferredMulticastRegistrations.remove(multicastId + subscriberParticipantId + providerParticipantId);
                return;
            }
        }
        parentRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
    }

    public void setParentRouter(RoutingProxy parentRouter,
                                Address parentRouterMessagingAddress,
                                String parentRoutingProviderParticipantId,
                                String routingProxyParticipantId) {
        this.parentRouterMessagingAddress = parentRouterMessagingAddress;
        this.parentRouter = parentRouter;

        // because the routing provider is local, therefore isGloballyVisible is false
        final boolean isGloballyVisible = false;
        addNextHopToParent(routingProxyParticipantId, isGloballyVisible);
        synchronized (this) {
            for (ParticipantIdAndIsGloballyVisibleHolder participantIds : deferredParentHopsParticipantIds) {
                addNextHopToParent(participantIds.participantId, participantIds.isGloballyVisible);
            }
            deferredParentHopsParticipantIds.clear();
            for (DeferrableRegistration registerWithParent : deferredMulticastRegistrations.values()) {
                registerWithParent.register();
            }
            deferredMulticastRegistrations.clear();
            ready = true;
        }
    }

    /**
     * Sets the address which will be registered at the parent router for the next hop
     * to contact this child message router
     * @param incomingAddress address of this libjoynr instance. Used by the cluster controller's
     *                        message router to forward messages
     */
    public void setIncomingAddress(Address incomingAddress) {
        this.incomingAddress = incomingAddress;
    }

    @Override
    public void routeIn(ImmutableMessage message) {
        checkExpiry(message);
        DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message, 0, "incoming", 0);
        incomingMessageQueue.put(delayableMessage);
    }

    @Override
    public void routeOut(ImmutableMessage message) {
        checkExpiry(message);
        logger.trace("Scheduling outgoing message {} with delay {} and retries {}", message, 0, 0);
        DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message, 0, "outgoing", 0);
        scheduleOutgoingMessage(delayableMessage);
    }

    private void scheduleOutgoingMessage(DelayableImmutableMessage delayableMessage) {
        final int retriesCount = delayableMessage.getRetriesCount();
        if (maxRetryCount > -1) {
            if (retriesCount > maxRetryCount) {
                logger.error("Max-retry-count ({}) reached. Dropping message {}",
                             maxRetryCount,
                             delayableMessage.getMessage().getTrackingInfo());
                return;
            }
        }
        if (retriesCount > 0) {
            logger.debug("Retry {}/{} sending message {}",
                         retriesCount,
                         maxRetryCount,
                         delayableMessage.getMessage().getTrackingInfo());
        }
        outgoingMessageQueue.put(delayableMessage);
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
                    scheduleOutgoingMessage(delayableMessage);
                } catch (Exception e) {
                    logger.warn("Rescheduling of message {} failed", messageNotSent.getTrackingInfo());
                }
            }
        };
        return failureAction;
    }

    private SuccessAction createMessageProcessedAction(final ImmutableMessage message) {
        final SuccessAction successAction = new SuccessAction() {

            @Override
            public void execute() {
            }
        };
        return successAction;
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
            throw new JoynrRuntimeException("Relative ttl not supported");
        }

        if (isExpired(message)) {
            long currentTimeMillis = System.currentTimeMillis();
            String errorMessage = MessageFormat.format("Received expired message: (now ={0}). Dropping the message {1}",
                                                       currentTimeMillis,
                                                       message.getTrackingInfo());
            logger.trace(errorMessage);
            throw new JoynrMessageExpiredException(errorMessage);
        }
    }

    @Override
    public void prepareForShutdown() {
        incomingMessageQueue.waitForQueueToDrain();
        outgoingMessageQueue.waitForQueueToDrain();
    private long createDelayWithExponentialBackoff(long sendMsgRetryIntervalMs, int retries) {
        long millis = sendMsgRetryIntervalMs + (long) ((2 ^ (retries)) * sendMsgRetryIntervalMs * Math.random());
        if (maxDelayMs >= sendMsgRetryIntervalMs && millis > maxDelayMs) {
            millis = maxDelayMs;
        }
        logger.trace("Created delay of {}ms in retry {}", millis, retries);
        return millis;
    }

    @Override
    public void shutdown() {
        CountDownLatch countDownLatch = new CountDownLatch(incomingMessageWorkers.size()
                + outgoingMessageWorkers.size());
        for (IncomingMessageWorker worker : incomingMessageWorkers) {
            worker.stopWorker(countDownLatch);
        }
        for (OutgoingMessageWorker worker : outgoingMessageWorkers) {
            worker.stopWorker(countDownLatch);
        }
        try {
            countDownLatch.await(1500, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            logger.error("Interrupted while waiting for message workers to stop.", e);
        }
    }

    class IncomingMessageWorker implements Runnable {
        private Logger logger = LoggerFactory.getLogger(IncomingMessageWorker.class);
        private int number;
        private volatile CountDownLatch countDownLatch;
        private volatile boolean stopped;

        public IncomingMessageWorker(int number) {
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
            Thread.currentThread().setName("joynrIncomingMessageWorker-" + number);

            while (!stopped) {
                DelayableImmutableMessage delayableMessage = null;

                try {
                    delayableMessage = incomingMessageQueue.poll(1000, TimeUnit.MILLISECONDS);

                    if (delayableMessage == null) {
                        continue;
                    }

                    ImmutableMessage message = delayableMessage.getMessage();
                    logger.trace("Starting processing of incoming message {}", message);
                    checkExpiry(message);
                    dispatcher.messageArrived(message);
                } catch (InterruptedException e) {
                    logger.trace("Message Worker interrupted. Stopping.");
                    Thread.currentThread().interrupt();
                    return;
                } catch (Exception error) {
                    if (delayableMessage == null) {
                        logger.error("Error in scheduled incoming message router thread. delayableMessage == null, continuing. Error:",
                                     error);
                    }
                    logger.error("Error in scheduled incoming message router thread:", error);
                    continue;
                }
            }
            countDownLatch.countDown();
        }
    }

    class OutgoingMessageWorker implements Runnable {
        private Logger logger = LoggerFactory.getLogger(OutgoingMessageWorker.class);
        private int number;
        private volatile CountDownLatch countDownLatch;
        private volatile boolean stopped;

        public OutgoingMessageWorker(int number) {
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
            Thread.currentThread().setName("joynrOutgoingMessageWorker-" + number);

            while (!stopped) {
                DelayableImmutableMessage delayableMessage = null;
                FailureAction failureAction = null;

                try {
                    delayableMessage = outgoingMessageQueue.poll(1000, TimeUnit.MILLISECONDS);

                    if (delayableMessage == null) {
                        continue;
                    }

                    ImmutableMessage message = delayableMessage.getMessage();
                    logger.trace("Starting processing of outgoing message {}", message);
                    checkExpiry(message);

                    SuccessAction messageProcessedAction = createMessageProcessedAction(message);
                    failureAction = createFailureAction(delayableMessage);
                    logger.trace(">>>>> SEND message {}", message.getId());

                    IMessagingStub messagingStub = messagingStubFactory.create(parentRouterMessagingAddress);
                    messagingStub.transmit(message, messageProcessedAction, failureAction);
                } catch (InterruptedException e) {
                    logger.trace("Message Worker interrupted. Stopping.");
                    Thread.currentThread().interrupt();
                    return;
                } catch (Exception error) {
                    if (delayableMessage == null) {
                        logger.error("Error in scheduled outgoing message router thread. delayableMessage == null, continuing. Error:",
                                     error);
                        continue;
                    }
                    logger.error("Error in scheduled outgoing message router thread:", error);
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
