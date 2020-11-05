/*-
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static io.joynr.util.JoynrUtil.createUuidString;
import static java.util.stream.Collectors.toSet;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.anyBoolean;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.TimeUnit;
import java.util.stream.Stream;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.Spy;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.persistence.MessagePersister;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

@RunWith(MockitoJUnitRunner.class)
public class MessageQueueTest {

    @Mock
    private DelayableImmutableMessage mockMessage;

    @Mock
    private DelayableImmutableMessage mockDelayableMessage2_multicast;

    @Mock
    private ImmutableMessage mockImmutableMessage2_multicast;

    @Mock
    private Set<Address> multicastDestinationAddresses;

    @Mock
    private DelayableImmutableMessage mockDelayableMessage3_request;

    @Mock
    private ImmutableMessage mockImmutableMessage3_request;

    @Mock
    private MessageQueue.MaxTimeoutHolder maxTimeoutHolderMock;

    @Spy
    private DelayQueue<DelayableImmutableMessage> delayQueue = new DelayQueue<>();

    @Mock
    private MessagePersister messagePersisterMock;

    @Mock
    RoutingTable routingTableMock;

    private String generatedMessageQueueId;
    private MessageQueue subject;

    private final long shutdownMaxTimeout = 50;
    private final String sender = "fromParticipantId";
    private final String brokerUri = "testBrokerUri";
    private final String topic = "testTopic";
    private final MqttAddress replyToAddress = new MqttAddress(brokerUri, topic);

    @Before
    public void setup() throws Exception {
        generatedMessageQueueId = createUuidString();

        // configure mocks
        when(maxTimeoutHolderMock.getTimeout()).thenReturn(shutdownMaxTimeout);

        Set<DelayableImmutableMessage> mockedMessages = Stream.of(mockDelayableMessage2_multicast,
                                                                  mockDelayableMessage3_request)
                                                              .collect(toSet());
        when(messagePersisterMock.fetchAll(eq(generatedMessageQueueId))).thenReturn(mockedMessages);

        when(mockDelayableMessage2_multicast.getMessage()).thenReturn(mockImmutableMessage2_multicast);
        when(mockDelayableMessage3_request.getMessage()).thenReturn(mockImmutableMessage3_request);

        when(mockImmutableMessage2_multicast.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST);
        when(mockDelayableMessage2_multicast.getDestinationAddresses()).thenReturn(multicastDestinationAddresses);
        when(mockImmutableMessage3_request.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);
        ObjectMapper objectMapper = new ObjectMapper();
        when(mockImmutableMessage3_request.getReplyTo()).thenReturn(objectMapper.writeValueAsString(replyToAddress));
        when(mockImmutableMessage3_request.getTtlMs()).thenReturn(42l);
        when(mockImmutableMessage3_request.getSender()).thenReturn(sender);

        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());
        // create test subject
        subject = new MessageQueue(delayQueue,
                                   maxTimeoutHolderMock,
                                   generatedMessageQueueId,
                                   messagePersisterMock,
                                   routingTableMock);
        drainQueue();
    }

    private void drainQueue() {
        // directly after creation MessageQueue contains
        // two messages which are fetched from the MessagePersister mock
        try {
            subject.poll(1, TimeUnit.SECONDS);
            subject.poll(1, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            // Ignore
        }
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

        // Then the operation blocked for max just over shutdownMaxTimeout millis
        assertTrue("Expected stop to block for maximum of around " + shutdownMaxTimeout + "ms. Actual: " + timeTaken,
                   timeTaken >= shutdownMaxTimeout && timeTaken < shutdownMaxTimeout + 20);
    }

    @Test
    public void testMessagePersisterCalledWhenAddingMessage() {
        // Given the MessageQueue and a mock message

        // When we add a message to the MessageQueue
        subject.put(mockMessage);

        // Then the message persister was asked if it wanted to persist
        verify(messagePersisterMock).persist(eq(generatedMessageQueueId), eq(mockMessage));
        // ... and the message was also added to the in-memory queue
        verify(delayQueue).put(eq(mockMessage));
    }

    @Test
    public void testPollRemovesMessageFromDelayQueueAndMessagePersister() throws Exception {
        // Given the MessageQueue and a mock message

        // When we add a message to the MessageQueue
        subject.put(mockMessage);
        // and poll
        final long timeOut = 1;
        final TimeUnit timeUnit = TimeUnit.SECONDS;
        subject.poll(timeOut, timeUnit);

        // Then the message is removed from the message persister
        verify(messagePersisterMock).remove(eq(generatedMessageQueueId), eq(mockMessage));
        // ... and the in-memory queue is polled once for the message above and twice for the messages in the setup method
        verify(delayQueue, times(3)).poll(timeOut, timeUnit);
    }

    private void testFetchMessagesFromPersistence() {
        // Given a mocked MessagePersister and a set containing messages which is returned when fetching
        // When the MessageQueue is created (see the setup method)

        // Then the messages were fetched from the MessagePersistence
        verify(messagePersisterMock).fetchAll(eq(generatedMessageQueueId));
    }

    @Test
    public void testFetchMessagesFromPersistence_AddToQueue() throws Exception {
        testFetchMessagesFromPersistence();
        // ... and added to the queue
        ArgumentCaptor<DelayableImmutableMessage> argumentCaptor = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(delayQueue, times(2)).put(argumentCaptor.capture());
        List<DelayableImmutableMessage> passedArguments = argumentCaptor.getAllValues();
        assertEquals(2, passedArguments.size());
        assertTrue(passedArguments.contains(mockDelayableMessage2_multicast));
        assertTrue(passedArguments.contains(mockDelayableMessage3_request));

    }

    @Test
    public void testFetchMessagesFromPersistence_AddReplyToAddressToRoutingTable() throws Exception {
        testFetchMessagesFromPersistence();

        // ... and one routing entry was added (replyTo address of mockDelayableMessage3_request)
        verify(mockImmutableMessage2_multicast, times(0)).getReplyTo();
        verify(mockImmutableMessage3_request).getReplyTo();
        verify(routingTableMock).put(anyString(), Mockito.any(Address.class), anyBoolean(), anyLong());
        verify(routingTableMock).put(sender, replyToAddress, true, mockImmutableMessage3_request.getTtlMs());
    }

    @Test
    public void testFetchMessagesFromPersistence_clearDestinationAddressesOfMulticast() throws Exception {
        testFetchMessagesFromPersistence();

        verify(mockDelayableMessage2_multicast).getDestinationAddresses();
        verify(mockDelayableMessage3_request, times(0)).getDestinationAddresses();
        verify(multicastDestinationAddresses).clear();

    }

    @Test
    public void testFetchMessagesFromPersistence_setDelay() throws Exception {
        testFetchMessagesFromPersistence();

        final long startupGracePeriodMs = 1000;
        verify(mockDelayableMessage2_multicast).setDelay(startupGracePeriodMs);
        verify(mockDelayableMessage3_request).setDelay(startupGracePeriodMs);
    }

}
