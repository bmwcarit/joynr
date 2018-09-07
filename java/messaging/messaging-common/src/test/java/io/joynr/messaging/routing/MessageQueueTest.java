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

import static java.util.stream.Collectors.toSet;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.TimeUnit;
import java.util.stream.Stream;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.persistence.MessagePersister;

@RunWith(MockitoJUnitRunner.class)
public class MessageQueueTest {

    @Mock
    private DelayableImmutableMessage mockMessage;

    @Mock
    private MessageQueue.MaxTimeoutHolder maxTimeoutHolderMock;

    @Spy
    private DelayQueue<DelayableImmutableMessage> delayQueue = new DelayQueue<>();

    @Mock
    private MessagePersister messagePersisterMock;

    private String generatedMessageQueueId;
    private MessageQueue subject;

    @Before
    public void setup() {
        when(maxTimeoutHolderMock.getTimeout()).thenReturn(50L);

        generatedMessageQueueId = UUID.randomUUID().toString();
        subject = new MessageQueue(delayQueue, maxTimeoutHolderMock, generatedMessageQueueId, messagePersisterMock);
    }

    @Test
    public void testPutAndRetrieveMessage() throws Exception {
        // Given a message

        // When I put the message to the queue, and then retrieve it
        subject.put(mockMessage);
        DelayableImmutableMessage result = subject.poll(0, TimeUnit.MILLISECONDS);

        // Then I got back the message I put in
        assertNotNull(result);
        assertEquals(mockMessage, result);
    }

    @Test
    public void testPollAndDelayedPut() throws Exception {
        // Given a message

        // When I poll for a message for max 1 sec, and I then put a message 5ms after
        Collection<DelayableImmutableMessage> resultContainer = new HashSet<>();
        CountDownLatch countDownLatch = new CountDownLatch(1);
        new Thread(() -> {
            try {
                resultContainer.add(subject.poll(1, TimeUnit.SECONDS));
            } catch (InterruptedException e) {
                // Ignore
            } finally {
                countDownLatch.countDown();
            }
        }).start();
        Thread.sleep(5);
        subject.put(mockMessage);
        countDownLatch.await();

        // Then I was returned the message I put
        assertEquals(1, resultContainer.size());
        assertEquals(mockMessage, resultContainer.iterator().next());
    }

    @Test
    public void testShutdownImmediatelyWithEmptyQueue() {
        // Given an empty queue

        // When I shutdown the message queue
        long beforeStop = System.currentTimeMillis();
        subject.waitForQueueToDrain();
        long timeTaken = System.currentTimeMillis() - beforeStop;

        // Then the call returned almost immediately
        assertTrue(timeTaken < 5);
    }

    @Test
    public void testShutdownBlocksUntilQueueEmpty() {
        // Given an item in the queue
        subject.put(mockMessage);

        // When I stop the message queue, and poll ten millis later
        new Thread(() -> {
            try {
                Thread.sleep(10);
                subject.poll(1, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                // Ignore
            }
        }).start();
        long beforeStop = System.currentTimeMillis();
        subject.waitForQueueToDrain();
        long timeTaken = System.currentTimeMillis() - beforeStop;

        // Then the shutdown blocked for roughly 10 milli-seconds or more, but not the max 5 sec timeout
        assertTrue("Expected call to take at least a second. Actual: " + timeTaken, timeTaken >= 10);
        assertTrue("Expected call to not take more then fifty millis. Actual: " + timeTaken, timeTaken < 50);
    }

    @Test
    public void testShutdownBlocksMaxTimeIfQueueNotEmptied() {
        // Given an item in the queue, and a max shutdown of 50ms (see setup)
        subject.put(mockMessage);

        // When I stop the queue, but do NOT remove the item
        long beforeStop = System.currentTimeMillis();
        subject.waitForQueueToDrain();
        long timeTaken = System.currentTimeMillis() - beforeStop;

        // Then the operation blocked for max just over 50 millis
        assertTrue("Expected stop to block for maximum of around 50ms. Actual: " + timeTaken,
                   timeTaken >= 50 && timeTaken < 70);
    }

}
