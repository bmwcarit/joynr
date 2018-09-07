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
package io.joynr.messaging.persistence;

import java.util.Set;

import io.joynr.messaging.routing.DelayableImmutableMessage;

/**
 * You can provide an implementation of this interface in order to be given the chance to persist messages being
 * added to the {@link io.joynr.messaging.routing.MessageQueue} so that if the joynr runtime quits unexpectedly,
 * the messages in the queue are not lost and can be restored on the next start of the runtime.
 */
public interface MessagePersister {

    /**
     * Called with the message to be persisted. If the implementation does not want to persist the message, it
     * should return <code>false</code>. Implementations of this method can block until the persisting was
     * successful, but be aware that this might impact performance.
     *
     * @param messageQueueId the unique ID of the message queue for which the message is being persisted
     * @param message the message which is a candidate for persisting
     * @return <code>true</code> if the message is persisted, <code>false</code> if not.
     */
    boolean persist(String messageQueueId, DelayableImmutableMessage message);

    /**
     * Fetches all messages from persistence which have not yet been {@link #remove(String,DelayableImmutableMessage)}.
     * The {@link io.joynr.messaging.routing.MessageQueue} will call this during startup to fetch any messages which were
     * persisted but not consumed during a previous lifetime of the joynr runtime in order to re-add them to the queue.
     *
     * @param messageQueueId the ID of the message queue for which to fetch the persisted messages.
     * @return the set of messages which were previously added by the IDed message queue, but were not consumed.
     */
    Set<DelayableImmutableMessage> fetchAll(String messageQueueId);

    /**
     * Called once the message queue has actually processed the message previously added to the persistence by calling
     * {@link #persist(String,DelayableImmutableMessage)}.
     * The implementation of this method should ensure that the message is actually removed
     * from the persistence, and should block until this has been done successfully.
     *
     * @param messageQueueId the ID of the message queue which consumed the message.
     * @param message the message which has been consumed by the queue and should be removed from persistence.
     */
    void remove(String messageQueueId, DelayableImmutableMessage message);
}
