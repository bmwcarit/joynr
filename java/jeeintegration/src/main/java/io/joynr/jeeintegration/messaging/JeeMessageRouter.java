package io.joynr.jeeintegration.messaging;

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

import static java.lang.String.format;

import java.text.DateFormat;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;

import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.RoutingTable;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;

/**
 * The MessageRouter is responsible for routing messages to their destination, and internally queues message post
 * requests using an executor service.
 * <p>
 * This override of the normal {@link io.joynr.messaging.routing.MessageRouterImpl} is necessary, because the standard
 * implementation calls {@link ScheduledExecutorService#isShutdown()}, which results in an exception in a JEE
 * environment. Hence, this implementation overrides the {@link #route(JoynrMessage)} method and provides an
 * implementation which doesn't call <code>isShutdown()</code>.
 *
 * @author clive.jevons commissioned by MaibornWolff GmbH
 * @see io.joynr.messaging.routing.MessageRouterImpl
 */
@edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER", justification = "ensure that no new messages are scheduled when scheduler is shuting down")
public class JeeMessageRouter extends io.joynr.messaging.routing.MessageRouterImpl {

    private static final Logger LOG = LoggerFactory.getLogger(JeeMessageRouter.class);
    private static final DateFormat DateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
    private static final int UUID_TAIL = 32;
    private ScheduledExecutorService scheduler;
    private MessagingStubFactory messagingStubFactory;
    private long sendMsgRetryIntervalMs;

    @Inject
    public JeeMessageRouter(RoutingTable routingTable,
                            @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                            @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                            MessagingStubFactory messagingStubFactory) {
        super(routingTable, scheduler, sendMsgRetryIntervalMs, messagingStubFactory);
        if (LOG.isDebugEnabled()) {
            LOG.debug(format("Initialising with:%n\troutingTable: %s%n\tscheduler: %s%n\tsendMsgRetryIntervalMs: %d%n\tmessageStubFactory: %s",
                             routingTable,
                             scheduler,
                             sendMsgRetryIntervalMs,
                             messagingStubFactory));
        }
        this.scheduler = scheduler;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
    }

    @Override
    public void route(final JoynrMessage message) throws JoynrSendBufferFullException, JoynrMessageNotSentException {
        checkExpiry(message);
        routeInternal(message, 0, 0);
    }

    private void routeInternal(final JoynrMessage message, final long delayMs, final int retriesCount) {
        try {
            scheduler.schedule(new Runnable() {
                @Override
                public void run() {
                    try {
                        checkExpiry(message);

                        String toParticipantId = message.getTo();
                        Address address = getAddress(toParticipantId);
                        if (LOG.isDebugEnabled()) {
                            LOG.debug(format(">>>>> ROUTE message for address: %s", address));
                        }
                        if (address != null) {
                            String messageId = message.getId().substring(UUID_TAIL);
                            LOG.info(format(">>>>> SEND  ID:%s:%s from: %s to: %s header: %s",
                                            messageId,
                                            message.getType(),
                                            message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                                            message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                                            message.getHeader().toString()));
                            if (LOG.isDebugEnabled()) {
                                LOG.debug(format(">>>>> body  ID:%s:%s: %s",
                                                 messageId,
                                                 message.getType(),
                                                 message.getPayload()));
                            }

                        } else {
                            throw new JoynrMessageNotSentException("Failed to send Request: No route for given participantId: "
                                    + toParticipantId);
                        }

                        IMessaging messagingStub = messagingStubFactory.create(address);
                        FailureAction failureAction = createFailureAction(message, retriesCount);
                        messagingStub.transmit(message, failureAction);
                    } catch (Throwable error) {
                        LOG.error("error in scheduled message router thread: {}" + error.getMessage());
                        throw error;
                    }
                }
            },
                               delayMs,
                               TimeUnit.MILLISECONDS);
        } catch (RejectedExecutionException e) {
            LOG.error("Execution rejected while scheduling SendSerializedMessageRequest ", e);
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
            LOG.error(errorMessage);
            throw new JoynrMessageNotSentException(errorMessage);
        }
    }

    private FailureAction createFailureAction(final JoynrMessage message, final int retriesCount) {
        final FailureAction failureAction = new FailureAction() {
            final String messageId = message.getId();

            @Override
            public void execute(Throwable error) {
                if (error instanceof JoynrShutdownException) {
                    LOG.warn("{}", error.getMessage());
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    LOG.error(" ERROR SENDING:  aborting send of messageId: {} to Address: {}. Error: {}",
                              new Object[]{ messageId, getAddress(message.getTo()), error.getMessage() });
                    return;
                }
                LOG.error("ERROR SENDING: messageId: {} to Address: {}. Error: {}", new Object[]{ messageId,
                        getAddress(message.getTo()), error.getMessage() });

                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = sendMsgRetryIntervalMs;
                    delayMs += exponentialBackoff(delayMs, retriesCount);
                }

                try {
                    LOG.error("Rescheduling messageId: {} with delay " + delayMs + " ms, new TTL expiration date: {}",
                              messageId,
                              DateFormatter.format(message.getExpiryDate()));
                    routeInternal(message, delayMs, retriesCount + 1);
                    return;
                } catch (JoynrSendBufferFullException e) {
                    try {
                        LOG.error("Rescheduling message: {} delayed {} ms because send buffer is full",
                                  delayMs,
                                  messageId);
                        Thread.sleep(delayMs);
                        this.execute(e);
                    } catch (InterruptedException e1) {
                        return;
                    }
                }
            }
        };
        return failureAction;
    }

    private long exponentialBackoff(long delayMs, int retries) {
        LOG.debug("TRIES: " + retries);
        long millis = delayMs + (long) ((2 ^ (retries)) * delayMs * Math.random());
        LOG.debug("MILLIS: " + millis);
        return millis;
    }
}
