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
import java.util.Set;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.inject.Inject;
import javax.inject.Singleton;

import com.google.inject.name.Named;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingMulticastSubscriber;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingSkeletonFactory;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

abstract public class AbstractMessageRouter implements MessageRouter {
    private static final long TERMINATION_TIMEOUT = 5000;

    private Logger logger = LoggerFactory.getLogger(AbstractMessageRouter.class);
    private final RoutingTable routingTable;
    private static final DateFormat DateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
    private ScheduledExecutorService scheduler;
    private long sendMsgRetryIntervalMs;
    private MessagingStubFactory messagingStubFactory;
    private final MessagingSkeletonFactory messagingSkeletonFactory;
    private AddressManager addressManager;
    protected final MulticastReceiverRegistry multicastReceiverRegistry;

    @Inject
    @Singleton
    public AbstractMessageRouter(RoutingTable routingTable,
                                 @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                 MessagingStubFactory messagingStubFactory,
                                 MessagingSkeletonFactory messagingSkeletonFactory,
                                 AddressManager addressManager,
                                 MulticastReceiverRegistry multicastReceiverRegistry) {
        this.routingTable = routingTable;
        this.scheduler = scheduler;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
        this.messagingSkeletonFactory = messagingSkeletonFactory;
        this.addressManager = addressManager;
        this.multicastReceiverRegistry = multicastReceiverRegistry;
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
        routingTable.put(participantId, address, isGloballyVisible);
    }

    @Override
    public void route(final ImmutableMessage message) {
        checkExpiry(message);
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

    private void routeInternal(final ImmutableMessage message, final long delayMs, final int retriesCount) {
        try {
            logger.trace("Scheduling {} with delay {} and retries {}", new Object[]{ message, delayMs, retriesCount });
            schedule(new Runnable() {
                @Override
                public void run() {
                    logger.trace("Starting processing of message {}", message);
                    try {
                        checkExpiry(message);

                        Set<Address> addresses = getAddresses(message);
                        if (addresses.isEmpty()) {
                            throw new JoynrMessageNotSentException("Failed to send Request: No route for given participantId: "
                                    + message.getRecipient());
                        }
                        for (Address address : addresses) {
                            logger.trace(">>>>> SEND  {}", message.toLogMessage());

                            IMessagingStub messagingStub = messagingStubFactory.create(address);
                            messagingStub.transmit(message, createFailureAction(message, retriesCount));
                        }
                    } catch (Exception error) {
                        logger.error("error in scheduled message router thread: {}", error.getMessage());
                        FailureAction failureAction = createFailureAction(message, retriesCount);
                        failureAction.execute(error);
                    }
                }
            },
                     message.getId(),
                     delayMs,
                     TimeUnit.MILLISECONDS);
        } catch (RejectedExecutionException e) {
            logger.error("Execution rejected while scheduling SendSerializedMessageRequest ", e);
            throw new JoynrSendBufferFullException(e);
        }
    }

    private void checkExpiry(final ImmutableMessage message) {
        if (!message.isTtlAbsolute()) {
            throw new JoynrRuntimeException("Relative ttl not supported");
        }

        long currentTimeMillis = System.currentTimeMillis();
        long ttlExpirationDateMs = message.getTtlMs();

        if (ttlExpirationDateMs <= currentTimeMillis) {
            String errorMessage = MessageFormat.format("ttl must be greater than 0 / ttl timestamp must be in the future: now: {0} abs_ttl: {1}",
                                                       currentTimeMillis,
                                                       ttlExpirationDateMs);
            logger.error(errorMessage);
            throw new JoynrMessageNotSentException(errorMessage);
        }
    }

    private FailureAction createFailureAction(final ImmutableMessage message, final int retriesCount) {
        final FailureAction failureAction = new FailureAction() {
            final String messageId = message.getId();

            @Override
            public void execute(Throwable error) {
                if (error instanceof JoynrShutdownException) {
                    logger.warn("{}", error.getMessage());
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error(" ERROR SENDING:  aborting send of messageId: {}. Error: {}", new Object[]{ messageId,
                            error.getMessage() });
                    return;
                }
                logger.warn("PROBLEM SENDING, will retry. messageId: {}. Error: {} Message: {}", new Object[]{
                        messageId, error.getClass().getName(), error.getMessage() });

                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = sendMsgRetryIntervalMs;
                    delayMs += exponentialBackoff(delayMs, retriesCount);
                }

                try {
                    logger.error("Rescheduling messageId: {} with delay " + delayMs + " ms, TTL is: {} ms",
                                 messageId,
                                 DateFormatter.format(message.getTtlMs()));
                    routeInternal(message, delayMs, retriesCount + 1);
                    return;
                } catch (JoynrSendBufferFullException e) {
                    try {
                        logger.error("Rescheduling message: {} delayed {} ms because send buffer is full",
                                     delayMs,
                                     messageId);
                        Thread.sleep(delayMs);
                        this.execute(e);
                    } catch (InterruptedException e1) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                }
            }
        };
        return failureAction;
    }

    @Override
    public void shutdown() {
        scheduler.shutdown();
        try {
            if (!scheduler.awaitTermination(TERMINATION_TIMEOUT, TimeUnit.MILLISECONDS)) {
                logger.error("Message Scheduler did not shut down in time. Timedout out waiting for executor service to shutdown after {}ms.",
                             TERMINATION_TIMEOUT);
                logger.debug("Attempting to shutdown scheduler {} forcibly.", scheduler);
                scheduler.shutdownNow();
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            logger.error("Message Scheduler shutdown interrupted: {}", e.getMessage());
        }
    }

    private long exponentialBackoff(long delayMs, int retries) {
        logger.trace("TRIES: " + retries);
        long millis = delayMs + (long) ((2 ^ (retries)) * delayMs * Math.random());
        logger.trace("MILLIS: " + millis);
        return millis;
    }

}
