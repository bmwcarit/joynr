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
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Intent;

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
import joynr.Message.MessageType;
import joynr.MutableMessage;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.google.inject.Injector;

@RunWith(AndroidJUnit4.class)
public class BinderMessagingSkeletonTest {

    private static final String RECIPIENT_ID = "recipientId";
    private static final String SENDER_ID = "senderId";

    private static byte[] createMessage(MessageType type) throws Exception {
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

    @Mock
    private BinderMessagingSkeleton.MainTransportFlagBearer mainTransportFlagBearer;

    @Mock
    private Intent mockIntent;

    private MockedStatic<AndroidBinderRuntime> androidBinderRuntimeMockedStatic;

    private BinderMessagingSkeleton binderMessagingSkeleton;

    private BinderMessageProcessor binderMessageProcessor;

    private final String creatorUserId = "creatorUserIdTest";

    @Before
    public void setUp() {
        MockitoAnnotations.openMocks(this);
        androidBinderRuntimeMockedStatic = Mockito.mockStatic(AndroidBinderRuntime.class);
        createBinderService();
    }

    private void createBinderService() {
        // create test subject
        when(mockIntent.getStringExtra(any())).thenReturn(creatorUserId);
        binderMessageProcessor = spy(new BinderMessageProcessor(mainTransportFlagBearer.isMainTransport()));
        binderMessagingSkeleton = spy(new BinderMessagingSkeleton(mockIntent));
        binderMessagingSkeleton.setBinderMessageProcessor(binderMessageProcessor);
    }

    @Test
    public void transmit_whenInjectorIsNotNull_CallsProcessMessage() throws Exception {
        when(injector.getInstance(MessageRouter.class)).thenReturn(messageRouter);
        androidBinderRuntimeMockedStatic.when(AndroidBinderRuntime::getInjector).thenReturn(injector);

        for (MessageType type : MessageType.values()) {
            binderMessagingSkeleton.transmit(createMessage(type));
            ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);

            verify(binderMessageProcessor).processMessage(any(), messageCaptor.capture());
            assertEquals(false, messageCaptor.getValue().isReceivedFromGlobal());
            reset(messageRouter);
            reset(binderMessageProcessor);
        }
    }

    @Test
    public void transmit_whenInjectorIsNotNull_SetCreatorUserId() throws Exception {
        when(injector.getInstance(MessageRouter.class)).thenReturn(messageRouter);
        androidBinderRuntimeMockedStatic.when(AndroidBinderRuntime::getInjector).thenReturn(injector);

        for (MessageType type : MessageType.values()) {
            binderMessagingSkeleton.transmit(createMessage(type));
            ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);

            verify(binderMessageProcessor).processMessage(any(), messageCaptor.capture());
            assertEquals(creatorUserId, messageCaptor.getValue().getCreatorUserId());
            reset(messageRouter);
            reset(binderMessageProcessor);
        }
    }

    @Test
    public void transmit_whenInjectorIsNull_CallsRemoveAndSendMessageDelayedToHandler() throws Exception {
        when(injector.getInstance(MessageRouter.class)).thenReturn(messageRouter);
        androidBinderRuntimeMockedStatic.when(AndroidBinderRuntime::getInjector).thenReturn(null);

        for (MessageType type : MessageType.values()) {
            binderMessagingSkeleton.transmit(createMessage(type));

            verify(binderMessageProcessor).removeAndSendMessageDelayedToHandler();
            reset(messageRouter);
            reset(binderMessageProcessor);
        }
    }

    @After
    public void cleanup() {
        androidBinderRuntimeMockedStatic.close();
    }

}
