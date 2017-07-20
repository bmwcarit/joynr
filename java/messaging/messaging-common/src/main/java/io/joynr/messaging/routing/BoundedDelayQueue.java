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
package io.joynr.messaging.routing;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_MAX_MESSAGES_INQUEUE;

import java.util.concurrent.Delayed;
import java.util.concurrent.locks.Condition;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

public class BoundedDelayQueue<E extends Delayed> extends DelayQueue<E> {
    private Logger logger = LoggerFactory.getLogger(BoundedDelayQueue.class);

    final int maxElementCount;
    private final Condition spaceAvailable = lock.newCondition();

    @Inject
    public BoundedDelayQueue(@Named(PROPERTY_MAX_MESSAGES_INQUEUE) int maxElements) {
        this.maxElementCount = maxElements;
    }

    public void putBounded(E delayable) throws InterruptedException {
        try {
            logger.trace("acquiring lock");
            lock.lockInterruptibly();
            while (q.size() == maxElementCount) {
                logger.trace("waiting for space");
                spaceAvailable.await();
            }
            logger.trace("putting element");
            super.put(delayable);
        } finally {
            logger.trace("finished");
            lock.unlock();
        }
    }

    @Override
    public E take() throws InterruptedException {
        lock.lockInterruptibly();
        try {
            E delayable = super.take();
            spaceAvailable.signal();
            return delayable;
        } finally {
            lock.unlock();
        }
    }

    @Override
    public boolean isEmpty() {
        return q.isEmpty();
    }
}
