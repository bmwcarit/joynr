/*
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
package itest.io.joynr.jeeintegration.messagepersister;

import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.persistence.MessagePersister;
import io.joynr.messaging.routing.DelayableImmutableMessage;

public class TestMessagePersister implements MessagePersister {
    private static final Logger logger = LoggerFactory.getLogger(TestMessagePersister.class);

    @Override
    public boolean persist(String messageQueueId, DelayableImmutableMessage message) {
        logger.info("Adding message {} to persistence for queue {}",
                    message.getMessage().toLogMessage(),
                    messageQueueId);
        return true;
    }

    @Override
    public Set<DelayableImmutableMessage> fetchAll(String messageQueueId) {
        logger.info("Fetching all persisted messages for queue {}", messageQueueId);
        return new HashSet<DelayableImmutableMessage>();
    }

    @Override
    public void remove(String messageQueueId, DelayableImmutableMessage message) {
        logger.info("Removing message {} from persistence for queue {}",
                    message.getMessage().toLogMessage(),
                    messageQueueId);
    }

}
