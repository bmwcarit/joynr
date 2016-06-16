package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;

import java.text.DateFormat;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.annotation.CheckForNull;
import javax.inject.Inject;
import javax.inject.Singleton;

import joynr.JoynrMessage;
import joynr.system.RoutingAbstractProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BrowserAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.CommonApiDbusAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.name.Named;

public class MessageRouterImpl extends RoutingAbstractProvider implements MessageRouter {
    private static final long TERMINATION_TIMEOUT = 5000;

    private Logger logger = LoggerFactory.getLogger(MessageRouterImpl.class);
    private final RoutingTable routingTable;
    private static final int UUID_TAIL = 32;
    private static final DateFormat DateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
    private ScheduledExecutorService scheduler;
    private long sendMsgRetryIntervalMs;
    private MessagingStubFactory messagingStubFactory;

    @Inject
    @Singleton
    public MessageRouterImpl(RoutingTable routingTable,
                             @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                             @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                             MessagingStubFactory messagingStubFactory) {
        this.routingTable = routingTable;
        this.scheduler = scheduler;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
    }

    protected Promise<DeferredVoid> addNextHopInternal(String participantId, Address address) {
        routingTable.put(participantId, address);
        final DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, ChannelAddress channelAddress) {
        return addNextHopInternal(participantId, channelAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, MqttAddress mqttAddress) {
        return addNextHopInternal(participantId, mqttAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, CommonApiDbusAddress commonApiDbusAddress) {
        return addNextHopInternal(participantId, commonApiDbusAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, BrowserAddress browserAddress) {
        return addNextHopInternal(participantId, browserAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, WebSocketAddress webSocketAddress) {
        return addNextHopInternal(participantId, webSocketAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, WebSocketClientAddress webSocketClientAddress) {
        return addNextHopInternal(participantId, webSocketClientAddress);
    }

    @Override
    public Promise<DeferredVoid> removeNextHop(String participantId) {
        routingTable.remove(participantId);
        DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<ResolveNextHopDeferred> resolveNextHop(String participantId) {
        ResolveNextHopDeferred deferred = new ResolveNextHopDeferred();
        deferred.resolve(routingTable.containsKey(participantId));
        return new Promise<ResolveNextHopDeferred>(deferred);
    }

    @Override
    public void addNextHop(String participantId, Address address) {
        addNextHopInternal(participantId, address);
    }

    @CheckForNull
    protected Address getAddress(String toParticipantId) {
        Address address = null;
        if (toParticipantId != null && routingTable.containsKey(toParticipantId)) {
            address = routingTable.get(toParticipantId);
        }
        logger.trace("Participant with ID {} has address {}", new Object[]{ toParticipantId, address });
        return address;
    }

    @Override
    public void route(final JoynrMessage message) {
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

    private void routeInternal(final JoynrMessage message, final long delayMs, final int retriesCount) {
        try {
            logger.debug("Scheduling {} with delay {} and retries {}", new Object[]{ message, delayMs, retriesCount });
            schedule(new Runnable() {
                @Override
                public void run() {
                    logger.debug("Staring processing of message {}", message);
                    try {
                        checkExpiry(message);

                        String toParticipantId = message.getTo();
                        Address address = getAddress(toParticipantId);
                        if (address != null) {
                            String messageId = message.getId().substring(UUID_TAIL);
                            logger.info(">>>>> SEND  ID:{}:{} from: {} to: {} header: {}", new String[]{ messageId,
                                    message.getType(),
                                    message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                                    message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                                    message.getHeader().toString() });
                            logger.debug(">>>>> body  ID:{}:{}: {}", new String[]{ messageId, message.getType(),
                                    message.getPayload() });

                        } else {
                            throw new JoynrMessageNotSentException("Failed to send Request: No route for given participantId: "
                                    + toParticipantId);
                        }

                        IMessaging messagingStub = messagingStubFactory.create(address);
                        messagingStub.transmit(message, createFailureAction(message, retriesCount));
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

    private void checkExpiry(final JoynrMessage message) {
        long currentTimeMillis = System.currentTimeMillis();
        long ttlExpirationDateMs = message.getExpiryDate();

        if (ttlExpirationDateMs <= currentTimeMillis) {
            String errorMessage = MessageFormat.format("ttl must be greater than 0 / ttl timestamp must be in the future: now: {0} abs_ttl: {1}",
                                                       currentTimeMillis,
                                                       ttlExpirationDateMs);
            logger.error(errorMessage);
            throw new JoynrMessageNotSentException(errorMessage);
        }
    }

    private FailureAction createFailureAction(final JoynrMessage message, final int retriesCount) {
        final FailureAction failureAction = new FailureAction() {
            final String messageId = message.getId();

            @Override
            public void execute(Throwable error) {
                if (error instanceof JoynrShutdownException) {
                    logger.warn("{}", error.getMessage());
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error(" ERROR SENDING:  aborting send of messageId: {} to Address: {}. Error: {}",
                                 new Object[]{ messageId, getAddress(message.getTo()), error.getMessage() });
                    return;
                }
                logger.warn("PROBLEM SENDING, will retry. messageId: {} to Address: {}. Error: {} Message: {}",
                            new Object[]{ messageId, getAddress(message.getTo()), error.getClass().getName(),
                                    error.getMessage() });

                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = sendMsgRetryIntervalMs;
                    delayMs += exponentialBackoff(delayMs, retriesCount);
                }

                try {
                    logger.error("Rescheduling messageId: {} with delay " + delayMs
                                         + " ms, new TTL expiration date: {}",
                                 messageId,
                                 DateFormatter.format(message.getExpiryDate()));
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
            scheduler.awaitTermination(TERMINATION_TIMEOUT, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            logger.error("Message Scheduler did not shut down in time: {}", e.getMessage());
        }
    }

    private long exponentialBackoff(long delayMs, int retries) {
        logger.debug("TRIES: " + retries);
        long millis = delayMs + (long) ((2 ^ (retries)) * delayMs * Math.random());
        logger.debug("MILLIS: " + millis);
        return millis;
    }

}
