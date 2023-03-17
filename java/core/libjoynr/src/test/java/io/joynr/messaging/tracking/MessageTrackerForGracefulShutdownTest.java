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
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

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

    private final String messageId = "messageId123";

    private final Message.MessageType oneWayRequestType = Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY;

    private final long shutdownMaxTimeout = 50;

    @Mock
    private ImmutableMessage immutableMessage;

    @Before
    public void setUp() {
        initMocks(this);
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

    @Test
    public void testUnsupportedMessageTypeForRegister() {
        when(immutableMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);
        messageTracker.register(immutableMessage);
        int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(0, registerNumber);
    }

    @Test
    public void testUnsupportedMessageTypeForUnregister() {
        messageTracker.register(immutableMessage);
        int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(1, registerNumber);

        when(immutableMessage.getType()).thenReturn(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY);
        messageTracker.unregister(immutableMessage);
        registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(1, registerNumber);
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
        int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(1, registerNumber);
    }

    @Test
    public void testUnregisterMessage() {
        messageTracker.register(immutableMessage);
        messageTracker.unregister(immutableMessage);
        int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(0, registerNumber);
    }

    @Test
    public void testRegisterMessageIfExists() {
        messageTracker.register(immutableMessage);
        messageTracker.register(immutableMessage);
        int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(1, registerNumber);
    }

    @Test
    public void testUnregisterMessageIfNotExists() {
        messageTracker.unregister(immutableMessage);
        int registerNumber = messageTracker.getNumberOfRegisteredMessages();
        assertEquals(0, registerNumber);
    }

    @Test
    public void testGetNumberOfRegisteredMessages() {
        int numberOfMessages = 3;
        for (int i = 1; i <= numberOfMessages; i++) {
            when(immutableMessage.getId()).thenReturn(messageId + "-" + i);
            messageTracker.register(immutableMessage);
        }
        assertEquals(numberOfMessages, messageTracker.getNumberOfRegisteredMessages());
    }

    private MessageTrackerForGracefulShutdown getMessageTracker() {
        return new MessageTrackerForGracefulShutdown(new ShutdownNotifier(), new ObjectMapper());
    }

}
