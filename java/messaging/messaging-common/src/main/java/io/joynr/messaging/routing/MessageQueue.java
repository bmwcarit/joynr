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

import java.util.concurrent.DelayQueue;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

/**
 * This class holds the queued messages which are to be processed in the {@link AbstractMessageRouter} and offers
 * the ability to {@link #waitForQueueToDrain() attempt to wait for the queue to drain}.
 */
public class MessageQueue {

    private static final Logger logger = LoggerFactory.getLogger(MessageQueue.class);

    public static final String PROPERTY_MESSAGE_QUEUE_SHUTDOWN_MAX_TIMEOUT = "io.joynr.messaging.queue.shutdown.timeout";
    private static final long STARTUP_GRACE_PERIOD_MS = 1000;

    private DelayQueue<DelayableImmutableMessage> delayableImmutableMessages;
    private final long shutdownTimeoutMs;
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
                        RoutingTable routingTable) {
        this.delayableImmutableMessages = delayableImmutableMessages;
        this.shutdownTimeoutMs = maxTimeoutHolder.getTimeout();
        this.routingTable = routingTable;
    }

    /**
     * Call this method to wait for the queue to drain if it still contains any messages. The timeout is set by
     * the {@link #PROPERTY_MESSAGE_QUEUE_SHUTDOWN_MAX_TIMEOUT} property, which defaults to five seconds.
     */
    void waitForQueueToDrain() {
        int remainingMessages = delayableImmutableMessages.size();
        logger.info("Joynr message queue stopping. Contains {} remaining messages.", remainingMessages);
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
            logger.info("Joynr message queue successfully emptied.");
        } else {
            logger.info("Joynr message queue still contained {} messages at shutdown.", remainingMessages);
        }
    }

    /**
     * Add the passed in message to the queue of messages to be processed.
     *
     * @param delayableImmutableMessage the message to add.
     */
    public void put(DelayableImmutableMessage delayableImmutableMessage) {
        delayableImmutableMessages.put(delayableImmutableMessage);
    }

    /**
     * Polls the message queue for a period no longer than the timeout specified for a new message.
     *
     * @param timeout the maximum time to wait for a message to become available.
     * @param unit the time unit of measurement for <code>timeout</code>
     * @return a new message if one is available, or <code>null</code> if none became available within the specified time limit.
     * @throws InterruptedException if the thread was interrupted while waiting for a message to become available.
     */
    public DelayableImmutableMessage poll(long timeout, TimeUnit unit) throws InterruptedException {
        DelayableImmutableMessage message = delayableImmutableMessages.poll(timeout, unit);
        return message;
    }
}
