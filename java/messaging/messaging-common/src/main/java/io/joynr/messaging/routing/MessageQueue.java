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

import com.google.inject.Inject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.DelayQueue;
import java.util.concurrent.TimeUnit;

/**
 * This class holds the queued messages which are to be processed in the {@link MessageRouter}.
 */
public class MessageQueue {

    private static final Logger logger = LoggerFactory.getLogger(MessageQueue.class);

    private final DelayQueue<DelayableImmutableMessage> delayableImmutableMessages;

    @Inject
    public MessageQueue(DelayQueue<DelayableImmutableMessage> delayableImmutableMessages) {
        this.delayableImmutableMessages = new DelayQueue<>(delayableImmutableMessages);
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
        return delayableImmutableMessages.poll(timeout, unit);
    }
}
