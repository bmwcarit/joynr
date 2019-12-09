/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

import java.util.Set;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.persistence.MessagePersister;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;

/**
 * This class holds the queued messages which are to be processed in the {@link AbstractMessageRouter} and offers
 * the ability to {@link #waitForQueueToDrain() attempt to wait for the queue to drain}.
 */
public class MessageQueue {

    private static final Logger logger = LoggerFactory.getLogger(MessageQueue.class);

    public static final String MESSAGE_QUEUE_ID = "io.joynr.messaging.queue.id";
    public static final String PROPERTY_MESSAGE_QUEUE_SHUTDOWN_MAX_TIMEOUT = "io.joynr.messaging.queue.shutdown.timeout";
    private static final long STARTUP_GRACE_PERIOD_MS = 1000;

    private DelayQueue<DelayableImmutableMessage> delayableImmutableMessages;
    private final long shutdownTimeoutMs;
    private final String messageQueueId;
    private final MessagePersister messagePersister;
    private final RoutingTable routingTable;

    /**
     * Helper class to enable constructor injection of an optionally configured timeout value.
     */
    public static class MaxTimeoutHolder {
        @Inject(optional = true)
        @Named(PROPERTY_MESSAGE_QUEUE_SHUTDOWN_MAX_TIMEOUT)
        private Long timeout = 5000L;

        public long getTimeout() {
            return timeout;
        }
    }

    @Inject
    public MessageQueue(DelayQueue<DelayableImmutableMessage> delayableImmutableMessages,
                        MaxTimeoutHolder maxTimeoutHolder,
                        @Named(MESSAGE_QUEUE_ID) String messageQueueId,
                        MessagePersister messagePersister,
                        RoutingTable routingTable) {
        this.delayableImmutableMessages = delayableImmutableMessages;
        this.shutdownTimeoutMs = maxTimeoutHolder.getTimeout();
        this.messageQueueId = messageQueueId;
        this.messagePersister = messagePersister;
        this.routingTable = routingTable;
        fetchAndQueuePersistedMessages(delayableImmutableMessages, messageQueueId);
    }

    private void registerReplyToAddress(DelayableImmutableMessage delayableImmutableMessage) {
        ImmutableMessage message = delayableImmutableMessage.getMessage();
        Message.MessageType messageType = message.getType();

        if (!messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST)
                && !messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                && !messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST)
                && !messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)) {
            return;
        }

        String replyTo = message.getReplyTo();
        if (replyTo != null && !replyTo.isEmpty()) {
            Address address = RoutingTypesUtil.fromAddressString(replyTo);
            // Set participant globally visible because the actual visibility is unknown
            // (the transient flag isReceivedFromGlobal is not persisted)
            final boolean isGloballyVisible = true;
            final long expiryDateMs = message.getTtlMs();

            routingTable.put(message.getSender(), address, isGloballyVisible, expiryDateMs);
        }
    }

    private void restoreMessage(DelayableImmutableMessage delayableImmutableMessage) {
        if (delayableImmutableMessage.getMessage().getType() == Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST) {
            // remove all addresses to trigger new address resolution to get also InProcessAddresses which are not persisted
            delayableImmutableMessage.getDestinationAddresses().clear();
        }
        // give potential recipients some time to register
        delayableImmutableMessage.setDelay(STARTUP_GRACE_PERIOD_MS);
        delayableImmutableMessages.put(delayableImmutableMessage);
    }

    private void fetchAndQueuePersistedMessages(DelayQueue<DelayableImmutableMessage> delayableImmutableMessages,
                                                String messageQueueId) {
        Set<DelayableImmutableMessage> persistedFromLastRun = messagePersister.fetchAll(messageQueueId);
        if (persistedFromLastRun != null) {
            persistedFromLastRun.forEach(this::registerReplyToAddress);
            persistedFromLastRun.forEach(this::restoreMessage);
        }
    }

    /**
     * Call this method to wait for the queue to drain if it still contains any messages. The timeout is set by
     * the {@link #PROPERTY_MESSAGE_QUEUE_SHUTDOWN_MAX_TIMEOUT} property, which defaults to five seconds.
     */
    void waitForQueueToDrain() {
        int remainingMessages = delayableImmutableMessages.size();
        logger.info("joynr message queue stopping. Contains {} remaining messages.", remainingMessages);
        if (remainingMessages > 0) {
            long shutdownStart = System.currentTimeMillis();
            while (System.currentTimeMillis() - shutdownStart < shutdownTimeoutMs) {
                if (delayableImmutableMessages.size() == 0) {
                    break;
                }
                try {
                    Thread.sleep(5);
                } catch (InterruptedException e) {
                    logger.error("Interrupted while waiting for joynr message queue to drain.");
                    e.printStackTrace();
                }
            }
        }
        remainingMessages = delayableImmutableMessages.size();
        if (remainingMessages == 0) {
            logger.info("joynr message queue successfully emptied.");
        } else {
            logger.info("joynr message queue still contained " + remainingMessages + " messages at shutdown.");
        }
    }

    /**
     * Add the passed in message to the queue of messages to be processed.
     * Also offer the message for persisting.
     *
     * @param delayableImmutableMessage the message to add.
     */
    public void put(DelayableImmutableMessage delayableImmutableMessage) {
        if (messagePersister.persist(messageQueueId, delayableImmutableMessage)) {
            logger.trace("Message {} was persisted for messageQueueId {}",
                         delayableImmutableMessage.getMessage(),
                         messageQueueId);
        } else {
            logger.trace("Message {} was not persisted for messageQueueId {}",
                         delayableImmutableMessage.getMessage(),
                         messageQueueId);
        }
        delayableImmutableMessages.put(delayableImmutableMessage);
    }

    /**
     * Polls the message queue for a period no longer than the timeout specified for a new message.
     *
     * If a message is successfully obtained, before returning it, a message persister is called in order to remove
     * this message from the persistence (in case it was persisted at all).
     *
     * @param timeout the maximum time to wait for a message to become available.
     * @param unit the time unit of measurement for <code>timeout</code>
     * @return a new message if one is available, or <code>null</code> if none became available within the specified time limit.
     * @throws InterruptedException if the thread was interrupted while waiting for a message to become available.
     */
    public DelayableImmutableMessage poll(long timeout, TimeUnit unit) throws InterruptedException {
        DelayableImmutableMessage message = delayableImmutableMessages.poll(timeout, unit);
        if (message != null) {
            messagePersister.remove(messageQueueId, message);
        }
        return message;
    }
}
