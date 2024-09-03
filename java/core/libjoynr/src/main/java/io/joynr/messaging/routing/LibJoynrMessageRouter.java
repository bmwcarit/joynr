/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TimeZone;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Future;
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
import io.joynr.messaging.tracking.MessageTrackerForGracefulShutdown;
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
    private final ScheduledExecutorService scheduler;
    @SuppressWarnings("FieldCanBeLocal")
    @Inject(optional = true)
    @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_MAX_RETRY_COUNT)
    private long maxRetryCount = ConfigurableMessagingSettings.DEFAULT_ROUTING_MAX_RETRY_COUNT;

    private final MessageQueue messageQueue;

    private List<LibJoynrMessageWorkable> messageWorkers;
    private List<Future<?>> messageWorkerFutures;

    private interface QueuedParentRoutingUpdate {
        void execute();
    }

    private interface QueuedMulticastRegistration {
        void register();
    }

    private final IMessagingStub outgoingMessagingStub;
    private RoutingProxy parentRouter;
    private final Address incomingAddress;
    private final Dispatcher dispatcher;
    private final List<QueuedParentRoutingUpdate> queuedParentRoutingUpdates = new ArrayList<>();
    private final Map<String, QueuedMulticastRegistration> queuedMulticastRegistrations = new HashMap<>();
    private volatile boolean ready = false;
    private final MessageTrackerForGracefulShutdown messageTracker;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LibJoynrMessageRouter(final @Named(SystemServicesSettings.LIBJOYNR_MESSAGING_ADDRESS) Address incomingAddress,
                                 final @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                 final @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                                 final MessagingStubFactory messagingStubFactory,
                                 final MessageQueue messageQueue,
                                 final ShutdownNotifier shutdownNotifier,
                                 final Dispatcher dispatcher,
                                 final MessageTrackerForGracefulShutdown messageTracker,
                                 final @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address parentRouterMessagingAddress) {
        dateFormatter.setTimeZone(TimeZone.getTimeZone("UTC"));
        this.scheduler = scheduler;
        this.dispatcher = dispatcher;
        this.messageQueue = messageQueue;
        shutdownNotifier.registerForShutdown(this);
        final int threadCount = maxParallelSends < 2 ? 2 : maxParallelSends;
        startMessageWorkerThreads(threadCount);
        this.incomingAddress = incomingAddress;
        this.messageTracker = messageTracker;
        this.outgoingMessagingStub = messagingStubFactory.create(parentRouterMessagingAddress);
    }

    @Override
    public boolean resolveNextHop(final String participantId) {
        return true;
    }

    private void startMessageWorkerThreads(final int numberOfWorkThreads) {
        messageWorkers = new ArrayList<>(numberOfWorkThreads);
        messageWorkerFutures = new ArrayList<>(numberOfWorkThreads);
        for (int i = 0; i < numberOfWorkThreads; i++) {
            final MessageWorker messageWorker = new MessageWorker(i);
            final Future<?> future = scheduler.schedule(messageWorker, 0, TimeUnit.MILLISECONDS);
            messageWorkerFutures.add(future);
            messageWorkers.add(messageWorker);
        }
    }

    private void cancelMessageWorkerFutures() {
        logger.error("Cancelling all message worker futures");
        messageWorkerFutures.forEach(future -> future.cancel(true));
    }

    @Override
    public void addNextHop(final String participantId, final Address address, final boolean isGloballyVisible) {
        if (!ready) {
            // lazy synchronization: synchronization is only required if message router is not yet ready, i.e.
            // proxy for parent router is not ready. Once ready == true, it will never be set to false again.
            synchronized (this) {
                if (!ready) {
                    final QueuedParentRoutingUpdate queuedAdd = () -> addNextHopToParent(participantId,
                                                                                         isGloballyVisible);
                    logger.debug("Queuing addNextHop for participantId {}", participantId);
                    queuedParentRoutingUpdates.add(queuedAdd);
                    return;
                }
            }
        }
        addNextHopToParent(participantId, isGloballyVisible);
    }

    @Override
    public void removeNextHop(final String participantId) {
        if (!ready) {
            // lazy synchronization: synchronization is only required if message router is not yet ready, i.e.
            // proxy for parent router is not ready. Once ready == true, it will never be set to false again.
            synchronized (this) {
                if (!ready) {
                    final QueuedParentRoutingUpdate queuedAdd = () -> removeNextHopFromParent(participantId);
                    logger.debug("Queuing removeNextHop for participantId {}", participantId);
                    queuedParentRoutingUpdates.add(queuedAdd);
                    return;
                }
            }
        }
        removeNextHopFromParent(participantId);
    }

    private void addNextHopToParent(final String participantId, final boolean isGloballyVisible) {
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

    private void removeNextHopFromParent(final String participantId) {
        logger.trace("Removing next hop with participantId {} from parent router", participantId);
        parentRouter.removeNextHop(participantId);
    }

    @Override
    public void addMulticastReceiver(final String multicastId,
                                     final String subscriberParticipantId,
                                     final String providerParticipantId) {
        final QueuedMulticastRegistration registerWithParent = () -> {
            logger.trace("Adding multicast receiver {} for multicast {} on provider {}",
                         subscriberParticipantId,
                         multicastId,
                         providerParticipantId);
            parentRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        };
        if (!ready) {
            // lazy synchronization: synchronization is only required if message router is not yet ready, i.e.
            // proxy for parent router is not ready. Once ready == true, it will never be set to false again.
            synchronized (this) {
                if (!ready) {
                    logger.debug("Queuing addMulticastReceiver for participantId {} for multicast {} on provider {}",
                                 subscriberParticipantId,
                                 multicastId,
                                 providerParticipantId);
                    queuedMulticastRegistrations.put(multicastId + subscriberParticipantId + providerParticipantId,
                                                     registerWithParent);
                    return;
                }
            }
        }
        registerWithParent.register();
    }

    @Override
    public void removeMulticastReceiver(final String multicastId,
                                        final String subscriberParticipantId,
                                        final String providerParticipantId) {
        logger.trace("Removing multicast receiver {} for multicast {} on provider {}",
                     subscriberParticipantId,
                     multicastId,
                     providerParticipantId);
        if (!ready) {
            // lazy synchronization: synchronization is only required if message router is not yet ready, i.e.
            // proxy for parent router is not ready. Once ready == true, it will never be set to false again.
            synchronized (this) {
                if (!ready) {
                    queuedMulticastRegistrations.remove(multicastId + subscriberParticipantId + providerParticipantId);
                    return;
                }
            }
        }
        parentRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
    }

    public void setParentRouter(final RoutingProxy parentRouter, final String routingProxyParticipantId) {
        this.parentRouter = parentRouter;

        // because the routing provider is local, therefore isGloballyVisible is false
        final boolean isGloballyVisible = false;
        addNextHopToParent(routingProxyParticipantId, isGloballyVisible);
        synchronized (this) {
            queuedParentRoutingUpdates.forEach(QueuedParentRoutingUpdate::execute);
            queuedParentRoutingUpdates.clear();
            queuedMulticastRegistrations.values().forEach(QueuedMulticastRegistration::register);
            queuedMulticastRegistrations.clear();
            ready = true;
        }
    }

    @Override
    public void routeIn(final ImmutableMessage message) {
        messageTracker.register(message);
        final DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message,
                                                                                         0,
                                                                                         Set.of("incoming"),
                                                                                         0);
        messageQueue.put(delayableMessage);
    }

    @Override
    public void routeOut(final ImmutableMessage message) {
        logger.trace("Scheduling outgoing message {} with delay {} and retries {}", message, 0, 0);
        messageTracker.register(message);
        final DelayableImmutableMessage delayableMessage = new DelayableImmutableMessage(message,
                                                                                         0,
                                                                                         Set.of("outgoing"),
                                                                                         0);
        scheduleOutgoingMessage(delayableMessage);
    }

    private void scheduleOutgoingMessage(final DelayableImmutableMessage delayableMessage) {
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
        messageQueue.put(delayableMessage);
    }

    private FailureAction createFailureAction(final DelayableImmutableMessage delayableMessage) {
        return new FailureAction() {
            private final AtomicBoolean failureActionExecutedOnce = new AtomicBoolean(false);

            @Override
            public void execute(final Throwable error) {
                final ImmutableMessage messageNotSent = delayableMessage.getMessage();

                if (!failureActionExecutedOnce.compareAndSet(false, true)) {
                    logger.trace("Failure action for message {} already executed once. Ignoring further call.",
                                 messageNotSent.getTrackingInfo());
                    messageTracker.unregister(messageNotSent);
                    return;
                }
                if (error instanceof JoynrShutdownException) {
                    logger.warn("Caught JoynrShutdownException while handling message {}:",
                                messageNotSent.getTrackingInfo(),
                                error);
                    messageTracker.unregister(messageNotSent);
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error("ERROR SENDING: Aborting send of message {}, Error:",
                                 messageNotSent.getTrackingInfo(),
                                 error);
                    messageTracker.unregister(messageNotSent);
                    return;
                }
                logger.warn("PROBLEM SENDING, will retry. message: {}, Error:",
                            messageNotSent.getTrackingInfo(),
                            error);

                final long delayMs;
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
                    scheduleOutgoingMessage(delayableMessage);
                } catch (final Exception e) {
                    logger.warn("Rescheduling of message {} failed", messageNotSent.getTrackingInfo());
                }
            }
        };
    }

    private SuccessAction createMessageProcessedAction(final ImmutableMessage message) {
        return () -> messageTracker.unregister(message);
    }

    private void checkExpiry(final ImmutableMessage message) {
        if (!message.isTtlAbsolute()) {
            throw new JoynrRuntimeException("Relative ttl not supported");
        }

        if (MessageRouterUtil.isExpired(message)) {
            final long currentTimeMillis = System.currentTimeMillis();
            final String errorMessage = MessageFormat.format("Received expired message: (now ={0}). Dropping the message {1}",
                                                             currentTimeMillis,
                                                             message.getTrackingInfo());
            logger.trace(errorMessage);
            throw new JoynrMessageExpiredException(errorMessage);
        }
    }

    @Override
    public void shutdown() {
        // tell all message workers to stop
        final CountDownLatch countDownLatch = new CountDownLatch(messageWorkers.size());
        for (final LibJoynrMessageWorkable worker : messageWorkers) {
            worker.stopWorker(countDownLatch);
        }
        try {
            if (!countDownLatch.await(1500, TimeUnit.MILLISECONDS)) {
                logger.error("FAILURE: waiting for message workers to stop timed out");
                cancelMessageWorkerFutures();
            }
        } catch (final InterruptedException e) {
            logger.error("Interrupted while waiting for message workers to stop.", e);
            Thread.currentThread().interrupt();
        }
    }

    class MessageWorker implements LibJoynrMessageWorkable {
        private final Logger logger = LoggerFactory.getLogger(MessageWorker.class);
        private final int number;
        private CountDownLatch countDownLatch;
        private volatile boolean stopped;

        public MessageWorker(final int number) {
            this.number = number;
            countDownLatch = null;
            stopped = false;
        }

        @Override
        public void stopWorker(final CountDownLatch countDownLatch) {
            this.countDownLatch = countDownLatch;
            stopped = true;
        }

        void handleOutgoingMessage(final DelayableImmutableMessage delayableMessage) {
            FailureAction failureAction = null;
            try {
                final ImmutableMessage message = delayableMessage.getMessage();
                logger.trace("Starting processing of outgoing message {}", message);
                checkExpiry(message);

                final SuccessAction messageProcessedAction = createMessageProcessedAction(message);
                failureAction = createFailureAction(delayableMessage);
                logger.trace(">>>>> SEND message {}", message.getId());

                outgoingMessagingStub.transmit(message, messageProcessedAction, failureAction);
                messageTracker.unregister(message);
            } catch (final Exception error) {
                logger.error("Error in scheduled MessageWorker thread while processing outgoing message:", error);
                if (failureAction == null) {
                    failureAction = createFailureAction(delayableMessage);
                }
                failureAction.execute(error);
            }
        }

        void handleIncomingMessage(final DelayableImmutableMessage delayableMessage) {
            try {
                final ImmutableMessage message = delayableMessage.getMessage();
                logger.trace("Starting processing of incoming message {}", message);
                checkExpiry(message);
                dispatcher.messageArrived(message);
                messageTracker.unregister(message);
            } catch (final Exception error) {
                logger.error("Error in scheduled MessageWorker thread while processing incoming message:", error);
            }
        }

        @Override
        public void run() {
            Thread.currentThread().setName("joynrMessageWorker-" + number);

            while (!stopped) {
                DelayableImmutableMessage delayableMessage;

                try {
                    delayableMessage = messageQueue.poll(1000, TimeUnit.MILLISECONDS);
                    if (delayableMessage != null) {
                        // Since there is no longer a MessageRouter in libJoynrRuntime
                        // all messages are sent to / received from the ClusterController
                        // (even if consumer and provider are using same libJoynrRuntime)
                        // which means incoming messages all have the receivedFromGlobal
                        // flag set since this is the case for everything received from
                        // cluster controller.
                        if (delayableMessage.getMessage().isReceivedFromGlobal()) {
                            handleIncomingMessage(delayableMessage);
                        } else {
                            handleOutgoingMessage(delayableMessage);
                        }
                    }
                } catch (final InterruptedException e) {
                    logger.trace("MessageWorker interrupted. Stopping.");
                    Thread.currentThread().interrupt();
                    return;
                }
            }
            countDownLatch.countDown();
        }
    }
}
