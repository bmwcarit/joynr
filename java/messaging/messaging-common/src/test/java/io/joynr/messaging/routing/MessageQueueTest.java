/*-
 * #%L
 * %%
 * Copyright (C) 2020-2023 BMW Car IT GmbH
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

import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.junit.MockitoJUnitRunner;

import java.lang.reflect.Field;
import java.util.Collection;
import java.util.HashSet;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

@RunWith(MockitoJUnitRunner.class)
public class MessageQueueTest {

    @Mock
    private DelayableImmutableMessage mockMessage;

    @Spy
    private DelayQueue<DelayableImmutableMessage> delayQueue = new DelayQueue<>();

    private MessageQueue subject;

    private final String sender = "fromParticipantId";
    private final String brokerUri = "testBrokerUri";
    private final String topic = "testTopic";
    private final MqttAddress replyToAddress = new MqttAddress(brokerUri, topic);

    @Before
    public void setup() throws Exception {
        // configure mocks
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());
        // create test subject
        subject = new MessageQueue(delayQueue);
    }

    @Test
    public void testPutAndRetrieveMessage() throws Exception {
        // Given a message and an empty queue

        // When I put the message to the queue, and then retrieve it
        subject.put(mockMessage);
        DelayableImmutableMessage result = subject.poll(0, TimeUnit.MILLISECONDS);

        // Then I got back the message I put in
        assertNotNull(result);
        assertEquals(mockMessage, result);
    }

    @Test
    public void testPollAndDelayedPut() throws Exception {
        // Given a message and an empty queue

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
        assertTrue("returned from poll before put", countDownLatch.getCount() > 0);
        subject.put(mockMessage);
        assertTrue("poll did not return within 1 second", countDownLatch.await(1, TimeUnit.SECONDS));

        // Then I was returned the message I put
        assertEquals(1, resultContainer.size());
        assertEquals(mockMessage, resultContainer.iterator().next());
    }

    @Test
    public void testPollRemovesMessageFromDelayQueue() throws Exception {
        // Given the MessageQueue and a mock message

        // When we add a message to the MessageQueue
        subject.put(mockMessage);
        // and poll
        final long timeOut = 1;
        final TimeUnit timeUnit = TimeUnit.SECONDS;
        subject.poll(timeOut, timeUnit);

        // The in-memory queue is polled once for the message above
        verify(delayQueue, times(1)).poll(timeOut, timeUnit);
    }
}
