/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessageInProgressObserverTest {

    @Mock
    private MqttMessagingSkeleton mqttMessagingSkeletonMock;

    private MqttMessageInProgressObserver createTestSubject(boolean backpressureEnabled,
                                                            int maxIncomingMqttRequests,
                                                            int reEnableMessageAcknowledgementTreshold,
                                                            int receiveMaximum,
                                                            String[] gbids) {
        return new MqttMessageInProgressObserver(backpressureEnabled,
                                                 maxIncomingMqttRequests,
                                                 reEnableMessageAcknowledgementTreshold,
                                                 receiveMaximum,
                                                 gbids,
                                                 true);
    }

    @Test
    public void testAcknowledgementAlwaysAllowedWhenBackpressureDisabled() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(false,
                                                                  maxIncomingMqttRequests,
                                                                  20,
                                                                  receiveMaximum,
                                                                  new String[]{ "gbid1" });
        for (int i = 1; i <= (maxIncomingMqttRequests * 2); i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
    }

    @Test
    public void testMessagingskeletonNotNotifiedWhenBackpressureDisabled() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementTreshold = 20;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(false,
                                                                  maxIncomingMqttRequests,
                                                                  reEnableMessageAcknowledgementTreshold,
                                                                  receiveMaximum,
                                                                  new String[]{ "gbid1" });
        for (int i = 1; i <= maxIncomingMqttRequests; i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
        for (int i = 1; i <= maxIncomingMqttRequests; i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        verify(mqttMessagingSkeletonMock, never()).acknowledgeOutstandingPublishes();
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenReceiveMaximumEqualToMaximumIncomingRequests() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 100;
        createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum, new String[]{ "gbid1" });
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenReceiveMaximumGreaterThanMaximumIncomingRequests() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 101;
        createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum, new String[]{ "gbid1" });
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenReceiveMaximumEqualToMaximumIncomingRequests_multipleGbids() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 50;
        createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum, new String[]{ "gbid1", "gbid2" });
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenReceiveMaximumGreaterThanMaximumIncomingRequests_multipleGbids() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 51;
        createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum, new String[]{ "gbid1", "gbid2" });
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenMaximumIncomingRequestsZero() {
        int maxIncomingMqttRequests = 0;
        int receiveMaximum = 101;
        createTestSubject(true, maxIncomingMqttRequests, 20, receiveMaximum, new String[]{ "gbid1" });
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenLowerBackpressureThresholdTooHigh() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementTreshold = 81;
        int receiveMaximum = 20;
        createTestSubject(true,
                          maxIncomingMqttRequests,
                          reEnableMessageAcknowledgementTreshold,
                          receiveMaximum,
                          new String[]{ "gbid1" });
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionThrownWhenLowerBackpressureThresholdBelowZero() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementTreshold = -1;
        int receiveMaximum = 20;
        createTestSubject(true,
                          maxIncomingMqttRequests,
                          reEnableMessageAcknowledgementTreshold,
                          receiveMaximum,
                          new String[]{ "gbid1" });
    }

    @Test
    public void testAcknowledgementDisallowedWhenUpperThresholdReached() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(true,
                                                                  maxIncomingMqttRequests,
                                                                  20,
                                                                  receiveMaximum,
                                                                  new String[]{ "gbid1" });
        for (int i = 1; i <= (maxIncomingMqttRequests - receiveMaximum); i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
        assertFalse(subject.canMessageBeAcknowledged("latestMessage"));
    }

    @Test
    public void testAcknowledgementDisallowedWhenUpperThresholdReached_multipleGbids() {
        int maxIncomingMqttRequests = 100;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(true,
                                                                  maxIncomingMqttRequests,
                                                                  20,
                                                                  receiveMaximum,
                                                                  new String[]{ "gbid1", "gbid2" });
        for (int i = 1; i <= (maxIncomingMqttRequests - 2 * receiveMaximum); i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
        assertFalse(subject.canMessageBeAcknowledged("latestMessage"));
    }

    @Test
    public void testAcknowledgementReEnabledWhenLowerThresholdReached() {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementThreshold = 20;
        int receiveMaximum = 20;
        MqttMessageInProgressObserver subject = createTestSubject(true,
                                                                  maxIncomingMqttRequests,
                                                                  reEnableMessageAcknowledgementThreshold,
                                                                  receiveMaximum,
                                                                  new String[]{ "gbid1" });
        subject.registerMessagingSkeleton(mqttMessagingSkeletonMock);
        // Feed the test subject with messageIds until the backpressure threshold is reached
        for (int i = 1; i <= (maxIncomingMqttRequests - receiveMaximum); i++) {
            assertTrue(subject.canMessageBeAcknowledged(Integer.toString(i)));
        }
        String latestMessageId = "latestMessage";
        String secondLatestMessageId = "secondLatestMessage";
        // After the backpressure threshold is reached, newly incoming requests will not be acknowledged
        assertFalse(subject.canMessageBeAcknowledged(latestMessageId));
        // This is the case until the lower threshold is reached
        for (int i = 1; i <= 20; i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        assertFalse(subject.canMessageBeAcknowledged(secondLatestMessageId));
        // Finalize an arbitrary amount of messages without reaching the lower backpressure threshold
        for (int i = 21; i <= (maxIncomingMqttRequests - receiveMaximum
                - reEnableMessageAcknowledgementThreshold); i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        // New incoming messages will still not be acknowledged
        verify(mqttMessagingSkeletonMock, never()).acknowledgeOutstandingPublishes();
        // Messages should not be able to be acknowledged a second time
        for (int i = 1; i <= (maxIncomingMqttRequests - receiveMaximum
                - reEnableMessageAcknowledgementThreshold); i++) {
            subject.decrementMessagesInProgress(Integer.toString(i));
        }
        subject.decrementMessagesInProgress(secondLatestMessageId);
        verify(mqttMessagingSkeletonMock, never()).acknowledgeOutstandingPublishes();
        // As soon as the lower threshold is reached, acknowledgement of the outstanding publishes is triggered
        subject.decrementMessagesInProgress(latestMessageId);
        verify(mqttMessagingSkeletonMock, times(1)).acknowledgeOutstandingPublishes();
    }

    @Test
    public void testMultithreadedBehaviour() throws Exception {
        int maxIncomingMqttRequests = 100;
        int reEnableMessageAcknowledgementThreshold = 20;
        int receiveMaximum = 20;
        final AtomicBoolean failed = new AtomicBoolean(false);
        final AtomicBoolean firstThreadWasBlocked = new AtomicBoolean(false);
        final AtomicBoolean secondThreadWasBlocked = new AtomicBoolean(false);
        MqttMessageInProgressObserver subject = createTestSubject(true,
                                                                  maxIncomingMqttRequests,
                                                                  reEnableMessageAcknowledgementThreshold,
                                                                  receiveMaximum,
                                                                  new String[]{ "gbid1", "gbid2" });
        //We only need one mock to test the expected behaviour, even if we have two threads
        subject.registerMessagingSkeleton(mqttMessagingSkeletonMock);
        final AtomicInteger currentlyUnacknowledgedMessages1 = new AtomicInteger(0);
        final AtomicInteger currentlyUnacknowledgedMessages2 = new AtomicInteger(0);
        Thread skeletonThread1 = new Thread() {
            public void run() {
                int messagesToSend = 100;
                int messagesSent = 0;
                while (messagesSent < messagesToSend) {
                    try {
                        Thread.sleep(2);
                    } catch (InterruptedException v) {
                    }
                    // Sanity check
                    if (currentlyUnacknowledgedMessages1.get() > receiveMaximum) {
                        failed.set(true);
                        break;
                        // When receiveMaximum unacknowledged messages are reached, no more messages will come in
                    } else if (currentlyUnacknowledgedMessages1.get() == receiveMaximum) {
                        continue;
                    } else if (!subject.canMessageBeAcknowledged("skeleton1_" + Integer.toString(messagesSent))) {
                        firstThreadWasBlocked.set(true);
                        currentlyUnacknowledgedMessages1.incrementAndGet();
                    }
                    messagesSent++;
                }
            }
        };

        Thread skeletonThread2 = new Thread() {
            public void run() {
                int messagesToSend = 100;
                int messagesSent = 0;
                while (messagesSent < messagesToSend) {
                    try {
                        Thread.sleep(2);
                    } catch (InterruptedException v) {
                    }
                    // Sanity check
                    if (currentlyUnacknowledgedMessages2.get() > receiveMaximum) {
                        failed.set(true);
                        break;
                        // When receiveMaximum unacknowledged messages are reached, no more messages will come in
                    } else if (currentlyUnacknowledgedMessages2.get() == receiveMaximum) {
                        continue;
                    } else if (!subject.canMessageBeAcknowledged("skeleton2_" + Integer.toString(messagesSent))) {
                        secondThreadWasBlocked.set(true);
                        currentlyUnacknowledgedMessages2.incrementAndGet();
                    }
                    messagesSent++;
                }
            }
        };

        CountDownLatch allMessagesProcessedLatch = new CountDownLatch(1);
        Thread processorThread = new Thread() {
            public void run() {
                int messagesToProcess = 100;
                int messagesProcessed = 0;
                while (messagesProcessed < messagesToProcess) {
                    try {
                        Thread.sleep(4);
                    } catch (InterruptedException v) {
                    }
                    subject.decrementMessagesInProgress("skeleton1_" + Integer.toString(messagesProcessed));
                    try {
                        Thread.sleep(4);
                    } catch (InterruptedException v) {
                    }
                    subject.decrementMessagesInProgress("skeleton2_" + Integer.toString(messagesProcessed));
                    messagesProcessed++;
                }
                allMessagesProcessedLatch.countDown();
            }
        };

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                currentlyUnacknowledgedMessages1.set(0);
                currentlyUnacknowledgedMessages2.set(0);
                return null;
            }
        }).when(mqttMessagingSkeletonMock).acknowledgeOutstandingPublishes();

        skeletonThread1.start();
        skeletonThread2.start();
        processorThread.start();
        assertTrue(allMessagesProcessedLatch.await(10000, TimeUnit.MILLISECONDS));
        assertFalse(failed.get());
        assertTrue(firstThreadWasBlocked.get());
        assertTrue(secondThreadWasBlocked.get());
    }

}
