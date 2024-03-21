/*-
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
package io.joynr.messaging.tracking;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.openMocks;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;

@RunWith(MockitoJUnitRunner.class)
public class MessageTrackerForGracefulShutdownTest {

    private MessageTrackerForGracefulShutdown messageTracker;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    private final String messageId = "messageId123";

    private final Message.MessageType oneWayRequestType = Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY;

    @Mock
    private ImmutableMessage immutableMessage;

    @Before
    public void setUp() {
        openMocks(this);
        messageTracker = getMessageTracker();
        when(immutableMessage.getType()).thenReturn(oneWayRequestType);
        when(immutableMessage.getId()).thenReturn(messageId);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testRegisterMessageWithNull() {
        messageTracker.register(null);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testUnregisterMessageWithNull() {
        messageTracker.unregister(null);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testUnregisterAfterExpiredReplyCallerWithNull() {
        messageTracker.unregisterAfterReplyCallerExpired(null);
    }

    @Test
    public void testUnsupportedMessageTypeForRegister() {
        when(immutableMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);
        messageTracker.register(immutableMessage);
        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());
    }

    @Test
    public void testUnsupportedMessageTypeForUnregister() {
        messageTracker.register(immutableMessage);
        assertEquals(1, messageTracker.getNumberOfRegisteredMessages());

        when(immutableMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);
        messageTracker.unregister(immutableMessage);
        assertEquals(1, messageTracker.getNumberOfRegisteredMessages());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testIdIsNullForRegistration() {
        when(immutableMessage.getId()).thenReturn(null);
        messageTracker.register(immutableMessage);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testIdIsNullForUnregistration() {
        when(immutableMessage.getId()).thenReturn(null);
        messageTracker.unregister(immutableMessage);
    }

    @Test
    public void testRegisterMessage() {
        messageTracker.register(immutableMessage);
        final int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(1, registerNumber);
    }

    @Test
    public void testUnregisterMessage() {
        messageTracker.register(immutableMessage);
        messageTracker.unregister(immutableMessage);
        final int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(0, registerNumber);
    }

    @Test
    public void testUnregisterAfterExpiredReplyCaller() {
        final String requestReplyId = "requestReplyId";
        final Map<String, String> customHeader = new HashMap<>();
        customHeader.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId);

        when(immutableMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);
        when(immutableMessage.getCustomHeaders()).thenReturn(customHeader);
        messageTracker.register(immutableMessage);
        final int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        messageTracker.unregisterAfterReplyCallerExpired(requestReplyId);
        final int afterUnregisterNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(1, registerNumber);
        assertEquals(0, afterUnregisterNumber);
    }

    @Test
    public void testRegisterMessageIfExists() {
        messageTracker.register(immutableMessage);
        messageTracker.register(immutableMessage);
        final int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(1, registerNumber);
    }

    @Test
    public void testUnregisterMessageIfNotExists() {
        messageTracker.unregister(immutableMessage);
        final int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(0, registerNumber);
    }

    @Test
    public void testGetNumberOfRegisteredMessages() {
        final int numberOfMessages = 3;
        for (int i = 1; i <= numberOfMessages; i++) {
            when(immutableMessage.getId()).thenReturn(messageId + "-" + i);
            messageTracker.register(immutableMessage);
        }
        assertEquals(numberOfMessages, messageTracker.getNumberOfRegisteredMessages());
    }

    @Test
    public void testRegisterShutdownListener() throws InterruptedException {
        messageTracker = getMessageTracker(shutdownNotifier);

        final CountDownLatch cdl = new CountDownLatch(1);
        mockShutdownListenerRegistration(cdl);

        shutdownNotifier.registerMessageTrackerShutdownListener(messageTracker);
        assertTrue(cdl.await(1000, TimeUnit.MILLISECONDS));
        verify(shutdownNotifier, times(1)).shutdown();
    }

    @Test
    public void testRegisterPrepareForShutdownListener() throws InterruptedException {
        messageTracker = getMessageTracker(shutdownNotifier);

        final CountDownLatch cdl = new CountDownLatch(1);
        mockPrepareForShutdownListenerRegistration(cdl);

        shutdownNotifier.registerMessageTrackerPrepareForShutdownListener(messageTracker);
        assertTrue(cdl.await(1000, TimeUnit.MILLISECONDS));
        verify(shutdownNotifier, times(1)).prepareForShutdown();
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterForShutdown_throws() {
        final ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerForShutdown(getMessageTracker());
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterToBeShutdownAsLast_throws() {
        final ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerToBeShutdownAsLast(getMessageTracker());
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterPrepareForShutdownListener_throws() {
        final ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerPrepareForShutdownListener(getMessageTracker());
    }

    @Test
    public void testPrepareForShutdownImmediatelyWithEmptyQueue() {
        final long timeTaken = invokePrepareForShutdown();

        assertTrue(timeTaken < 5);
    }

    @Test
    public void testPrepareForShutdownBlocksMaxTimeIfQueueNotEmptied() {
        messageTracker.register(immutableMessage);

        // Message is registered but there is no call to unregister
        final long timeTaken = invokePrepareForShutdown();

        final long prepareForShutdownTimeoutMs = 5000;
        assertTrue("prepareForShutdown should take up to " + prepareForShutdownTimeoutMs + "ms. Actual: " + timeTaken,
                   timeTaken >= prepareForShutdownTimeoutMs && timeTaken < prepareForShutdownTimeoutMs + 50);
    }

    @Test
    public void testPrepareForShutdownBlocksUntilQueueEmpty() {
        messageTracker.register(immutableMessage);

        waitAndUnregister(10L);

        final long timeTaken = invokePrepareForShutdown();

        // Then the shutdown blocked for roughly 10 milliseconds or more, but not the max 5 sec timeout
        assertTrue("Expected call to take at least 10 ms second. Actual: " + timeTaken, timeTaken >= 10);
        assertTrue("Expected call to not take more than 50 ms. Actual: " + timeTaken, timeTaken < 50);
    }

    @Test
    public void testPrepareForShutdownThatTimesOut() {
        setMessageTrackerPrepareForShutdownTimeout(1);

        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());
        messageTracker.register(immutableMessage);
        assertEquals(1, messageTracker.getNumberOfRegisteredMessages());

        final long timeElapsed = invokePrepareForShutdown();

        assertEquals(1, messageTracker.getNumberOfRegisteredMessages());
        assertTrue(timeElapsed >= 1000);
    }

    @Test
    public void testPrepareForShutdownThatDoesNotTimeOut() {
        setMessageTrackerPrepareForShutdownTimeout(2);

        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());

        final long timeElapsed = invokePrepareForShutdown();

        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());
        assertTrue(timeElapsed < 1000);
    }

    @Test
    public void testMessageQueueIsEmptiedWhilePreparingForShutdown() {
        setMessageTrackerPrepareForShutdownTimeout(2);

        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());
        messageTracker.register(immutableMessage);
        assertEquals(1, messageTracker.getNumberOfRegisteredMessages());

        waitAndUnregister(1000L);

        final long timeElapsed = invokePrepareForShutdown();

        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());
        assertTrue(timeElapsed < 2000);
    }

    @Test
    public void testRequestReplyRegistration() {
        final String requestMessageId = "requestMessageId";
        final String replyMessageId = "replyMessageId";
        final String requestReplyId = "requestReplyId";

        final ImmutableMessage request = mock(ImmutableMessage.class);
        final Map<String, String> requestHeaders = new HashMap<>();
        requestHeaders.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId);
        when(request.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST);
        when(request.getId()).thenReturn(requestMessageId);
        when(request.getCustomHeaders()).thenReturn(requestHeaders);

        final ImmutableMessage reply = mock(ImmutableMessage.class);
        final Map<String, String> replyHeaders = new HashMap<>();
        replyHeaders.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId);
        when(reply.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY);
        when(reply.getId()).thenReturn(replyMessageId);
        when(reply.getCustomHeaders()).thenReturn(replyHeaders);

        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());
        messageTracker.register(request);
        assertEquals(1, messageTracker.getNumberOfRegisteredMessages());
        messageTracker.register(reply);
        assertEquals(2, messageTracker.getNumberOfRegisteredMessages());
        messageTracker.unregister(request);
        assertEquals(1, messageTracker.getNumberOfRegisteredMessages());
        messageTracker.unregister(reply);
        assertEquals(0, messageTracker.getNumberOfRegisteredMessages());
    }

    private void waitAndUnregister(final long waitInMs) {
        new Thread(() -> {
            try {
                Thread.sleep(waitInMs);
                messageTracker.unregister(immutableMessage);
            } catch (final InterruptedException e) {
                fail("Unexpected exception: " + e.getMessage());
            }
        }).start();
    }

    private long invokePrepareForShutdown() {
        final long before = System.currentTimeMillis();
        messageTracker.prepareForShutdown();
        final long after = System.currentTimeMillis();

        return after - before;
    }

    private void setMessageTrackerPrepareForShutdownTimeout(final int seconds) {
        try {
            final Field field = MessageTrackerForGracefulShutdown.class.getDeclaredField("prepareForShutdownTimeoutSec");
            field.setAccessible(true);
            field.set(messageTracker, seconds);
        } catch (final NoSuchFieldException | IllegalAccessException e) {
            fail("Unexpected exception: " + e.getMessage());
        }
    }

    private MessageTrackerForGracefulShutdown getMessageTracker() {
        return getMessageTracker(new ShutdownNotifier());
    }

    private MessageTrackerForGracefulShutdown getMessageTracker(final ShutdownNotifier shutdownNotifier) {
        return new MessageTrackerForGracefulShutdown(shutdownNotifier, new ObjectMapper());
    }

    private void mockPrepareForShutdownListenerRegistration(final CountDownLatch latch) {
        doAnswer(invocation -> {
            shutdownNotifier.prepareForShutdown();
            latch.countDown();
            return null;
        }).when(shutdownNotifier).registerMessageTrackerPrepareForShutdownListener(messageTracker);
    }

    private void mockShutdownListenerRegistration(final CountDownLatch latch) {
        doAnswer(invocation -> {
            shutdownNotifier.shutdown();
            latch.countDown();
            return null;
        }).when(shutdownNotifier).registerMessageTrackerShutdownListener(messageTracker);
    }
}
