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
package io.joynr.messaging.sender;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.smrf.EncodingException;
import joynr.ImmutableMessage;
import joynr.MutableMessage;

@RunWith(MockitoJUnitRunner.class)
public class AbstractMessageSenderTest extends MessageSenderTestBase {
    private class TestMessageSender extends AbstractMessageSender {
        public TestMessageSender(MessageRouter messageRouter) {
            super(messageRouter);
        }

        public void setReplyToAddress(String replyToAddress, String globalAddress) {
            super.setReplyToAddress(replyToAddress, globalAddress);
        }
    }

    TestMessageSender subject;

    @Before
    public void setUp() {
        subject = new TestMessageSender(messageRouterMock);
    }

    @Test(expected = JoynrRuntimeException.class)
    @SuppressWarnings("unchecked")
    public void testSerializationExceptionIsHandled() throws Exception {
        MutableMessage message = createTestRequestMessage();
        message = spy(message);

        when(message.getImmutableMessage()).thenThrow(EncodingException.class);

        subject.setReplyToAddress("someReplyTo", "someGlobal");
        subject.sendMessage(message);
    }

    @Test
    public void testLocalMessageGetsNoReplyTo() {
        MutableMessage message = createTestRequestMessage();
        message.setLocalMessage(true);

        subject.setReplyToAddress("someReplyTo", "someGlobal");
        subject.sendMessage(message);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouterMock).routeOut(argCaptor.capture());
        assertEquals(null, argCaptor.getValue().getReplyTo());
    }

    @Test
    public void testReplyToMessageQueue() throws Exception {
        MutableMessage message = createTestRequestMessage();
        message = spy(message);

        subject.sendMessage(message);
        verify(messageRouterMock, never()).routeOut(any(ImmutableMessage.class));

        ImmutableMessage immutableMessageMock = Mockito.mock(ImmutableMessage.class);
        when(message.getImmutableMessage()).thenReturn(immutableMessageMock);

        String expectedReplyTo = "expectedReplyTo";
        subject.setReplyToAddress(expectedReplyTo, "someGlobal");

        verify(messageRouterMock, times(1)).routeOut(immutableMessageMock);
    }

    @Test
    public void testStatelessAsyncReplyToGlobal() throws Exception {
        MutableMessage message = createTestRequestMessage();
        message.setStatelessAsync(true);

        subject.setReplyToAddress("replyTo", "global");
        subject.sendMessage(message);

        assertEquals("global", message.getReplyTo());
    }
}
