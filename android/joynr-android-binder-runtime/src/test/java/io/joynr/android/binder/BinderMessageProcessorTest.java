/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.android.binder;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.atMostOnce;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Handler;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.google.inject.Injector;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.messaging.routing.MessageRouter;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;

@RunWith(AndroidJUnit4.class)
public class BinderMessageProcessorTest {

    private static final String RECIPIENT_ID = "recipientId";
    private static final String SENDER_ID = "senderId";

    private static byte[] createMessage(Message.MessageType type) throws Exception {
        MutableMessage mutableMessage = new MutableMessage();
        mutableMessage.setPayload("testPayload".getBytes());
        mutableMessage.setRecipient(RECIPIENT_ID);
        mutableMessage.setSender(SENDER_ID);
        mutableMessage.setType(type);
        return mutableMessage.getImmutableMessage().getSerializedMessage();
    }

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private Injector injector;

    private MockedStatic<AndroidBinderRuntime> androidBinderRuntimeMockedStatic;

    private BinderMessageProcessor binderMessageProcessor;

    private Handler injectorRuntimeAvailabilityHandler;

    private void createBinderMessageProcessor(boolean mainTransport) {
        // create test subject
        binderMessageProcessor = spy(new BinderMessageProcessor(mainTransport));
        injectorRuntimeAvailabilityHandler = binderMessageProcessor.createInjectorRuntimeAvailabilityHandler();
    }

    @Before
    public void setUp() {
        MockitoAnnotations.openMocks(this);
        androidBinderRuntimeMockedStatic = Mockito.mockStatic(AndroidBinderRuntime.class);
    }

    private void verifyReceivedFromGlobal(boolean expected) throws Exception {
        when(injector.getInstance(MessageRouter.class)).thenReturn(messageRouter);
        androidBinderRuntimeMockedStatic.when(AndroidBinderRuntime::getInjector).thenReturn(injector);

        for (Message.MessageType type : Message.MessageType.values()) {
            ImmutableMessage message = new ImmutableMessage(createMessage(type));
            binderMessageProcessor.processMessage(injector, message);
            ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);

            verify(messageRouter).routeIn(messageCaptor.capture());
            assertEquals(expected, messageCaptor.getValue().isReceivedFromGlobal());
            reset(messageRouter);
        }
    }

    @Test
    public void processMessage_doNotSetReceivedFromGlobalInCC_ReceivedFromGlobalIsFalse() throws Exception {
        createBinderMessageProcessor(false); //Re-initialize subject
        verifyReceivedFromGlobal(false);
    }

    @Test
    public void processMessage_doSetReceivedFromGlobalInApp_ReceivedFromGlobalIsTrue() throws Exception {
        createBinderMessageProcessor(true); //Re-initialize subject
        verifyReceivedFromGlobal(true);
    }

    @Test
    public void handleMessage_whenInjectorIsNotNull_callsProcessMessage() throws Exception {
        when(injector.getInstance(MessageRouter.class)).thenReturn(messageRouter);
        androidBinderRuntimeMockedStatic.when(AndroidBinderRuntime::getInjector).thenReturn(injector);

        createBinderMessageProcessor(true); //Re-initialize subject

        for (Message.MessageType type : Message.MessageType.values()) {
            ImmutableMessage message = new ImmutableMessage(createMessage(type));
            binderMessageProcessor.addToWaitingProcessing(message);
        }

        int queueLength = Message.MessageType.values().length;

        assertEquals(binderMessageProcessor.messagesWaitingProcessing.size(), queueLength);

        injectorRuntimeAvailabilityHandler.handleMessage(mock(android.os.Message.class));

        // all queued messages were handled
        assertEquals(binderMessageProcessor.messagesWaitingProcessing.size(), 0);

        verify(binderMessageProcessor, times(queueLength)).processMessage(any(), any());
        verify(binderMessageProcessor, never()).removeAndSendMessageDelayedToHandler();

    }

    @Test
    public void handleMessage_whenInjectorIsNull_callsRemoveAndSendMessageDelayedToHandler() throws Exception {
        createBinderMessageProcessor(true); //Re-initialize subject

        when(injector.getInstance(MessageRouter.class)).thenReturn(messageRouter);
        androidBinderRuntimeMockedStatic.when(AndroidBinderRuntime::getInjector).thenReturn(null);
        for (Message.MessageType type : Message.MessageType.values()) {
            ImmutableMessage message = new ImmutableMessage(createMessage(type));
            binderMessageProcessor.addToWaitingProcessing(message);
        }

        int queueLength = Message.MessageType.values().length;
        assertEquals(binderMessageProcessor.messagesWaitingProcessing.size(), queueLength);

        injectorRuntimeAvailabilityHandler.handleMessage(mock(android.os.Message.class));

        // No message was processed yet
        assertEquals(binderMessageProcessor.messagesWaitingProcessing.size(), queueLength);

        verify(binderMessageProcessor, never()).processMessage(any(), any());
        verify(binderMessageProcessor, atMostOnce()).removeAndSendMessageDelayedToHandler();

    }

    @After
    public void cleanup() {
        androidBinderRuntimeMockedStatic.close();
    }
}
