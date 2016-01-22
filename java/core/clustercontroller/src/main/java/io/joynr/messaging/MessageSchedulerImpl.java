package io.joynr.messaging;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;

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

import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.http.HttpMessageSender;
import io.joynr.messaging.serialize.MessageSerializerFactory;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;

import java.text.DateFormat;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * The MessageScheduler queues message post requests in a single threaded executor. The executor is blocked until the
 * connection is established, from there on the request is async. If there are already too much connections open, the
 * executor is blocked until one of the connections is closed. Resend attempts are scheduled by a cached thread pool
 * executor.
 */
@edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER", justification = "ensure that no new messages are scheduled when scheduler is shuting down")
@Singleton
public class MessageSchedulerImpl implements MessageScheduler {
    private static final long TERMINATION_TIMEOUT = 5000;
    private static final Logger logger = LoggerFactory.getLogger(MessageSchedulerImpl.class);
    private static final DateFormat DateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
    private final HttpMessageSender httpMessageSender;
    private ScheduledExecutorService scheduler;
    private MessagingSettings settings;
    private MessageSerializerFactory messageSerializerFactory;

    @Inject
    public MessageSchedulerImpl(@Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                HttpMessageSender httpMessageSender,
                                MessagingSettings settings,
                                MessageSerializerFactory messageSerializerFactory) {
        this.httpMessageSender = httpMessageSender;
        this.scheduler = scheduler;
        this.settings = settings;
        this.messageSerializerFactory = messageSerializerFactory;
    }

    @Override
    public synchronized void scheduleMessage(final Address address, final JoynrMessage message) {
        long currentTimeMillis = System.currentTimeMillis();
        long ttlExpirationDate_ms = message.getExpiryDate();

        if (ttlExpirationDate_ms <= currentTimeMillis) {
            String errorMessage = MessageFormat.format("ttl must be greater than 0 / ttl timestamp must be in the future: now: {0} abs_ttl: {1}",
                                                       currentTimeMillis,
                                                       ttlExpirationDate_ms);
            logger.error(errorMessage);
            throw new JoynrMessageNotSentException(errorMessage);
        }

        final MessageContainer messageContainer;
        JoynrMessageSerializer messageSerializer = messageSerializerFactory.create(address);
        String serializedMessage = messageSerializer.serialize(message);
        messageContainer = new MessageContainer(address, message, serializedMessage, ttlExpirationDate_ms);

        rescheduleMessage(messageContainer, 0);
    }

    private void rescheduleMessage(final MessageContainer messageContainer, long delayMs) {
        logger.trace("scheduleMessage messageId: {} address {}",
                     messageContainer.getMessageId(),
                     messageContainer.getAddress());

        synchronized (scheduler) {
            if (scheduler.isShutdown()) {
                JoynrShutdownException joynrShutdownEx = new JoynrShutdownException("MessageScheduler is shutting down already. Unable to send message [messageId: "
                        + messageContainer.getMessageId() + "].");
                throw joynrShutdownEx;
            }

            if (messageContainer.isExpired()) {
                logger.error("SEND failed for messageId: {}, expiryDate: {} TTL expired.",
                             messageContainer.getMessageId(),
                             messageContainer.getExpiryDate());
                return;
            }

            final FailureAction failureAction = createFailureAction(messageContainer);

            try {
                scheduler.schedule(new Runnable() {
                    @Override
                    public void run() {
                        httpMessageSender.sendMessage(messageContainer, failureAction);
                    }
                }, delayMs, TimeUnit.MILLISECONDS);
            } catch (RejectedExecutionException e) {
                logger.error("Execution rejected while scheduling SendSerializedMessageRequest ", e);
                throw new JoynrSendBufferFullException(e);
            }
        }
    }

    private FailureAction createFailureAction(final MessageContainer messageContainer) {
        final FailureAction failureAction = new FailureAction() {
            final String messageId = messageContainer.getMessageId();
            final Address address = messageContainer.getAddress();
            final long sendMsgRetryIntervalMs = settings.getSendMsgRetryIntervalMs();

            @Override
            public void execute(Throwable error) {
                if (error instanceof JoynrShutdownException) {
                    logger.warn("{}", error.getMessage());
                    return;
                }
                logger.error("!!!! ERROR SENDING: messageId: {} to Address: {}. Error: {}", new String[]{ messageId,
                        address.toString(), error.getMessage() });

                messageContainer.incrementRetries();
                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = sendMsgRetryIntervalMs;
                    delayMs += exponentialBackoff(delayMs, messageContainer.getRetries());
                }

                try {
                    logger.error("Rescheduling messageId: {} with delay " + delayMs
                                         + " ms, new TTL expiration date: {}",
                                 messageId,
                                 DateFormatter.format(messageContainer.getExpiryDate()));
                    rescheduleMessage(messageContainer, delayMs);
                    return;
                } catch (JoynrSendBufferFullException e) {
                    try {
                        logger.error("Rescheduling message: {} delayed {} ms because send buffer is full",
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

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.messaging.MessageScheduler#shutdown()
     */
    @Override
    public synchronized void shutdown() throws InterruptedException {
        synchronized (scheduler) {
            scheduler.shutdown();
        }

        // TODO serialize messages that could not be resent because of shutdown? Or somehow notify sender?
        // List<Runnable> awaitingScheduling = scheduler.shutdownNow();
        // List<Runnable> awaitingResend = executionQueue.shutdownNow();
        scheduler.awaitTermination(TERMINATION_TIMEOUT, TimeUnit.MILLISECONDS);
    }

    private long exponentialBackoff(long delayMs, int retries) {
        logger.debug("TRIES: " + retries);
        long millis = delayMs + (long) ((2 ^ (retries)) * delayMs * Math.random());
        logger.debug("MILLIS: " + millis);
        return millis;
    }
}
