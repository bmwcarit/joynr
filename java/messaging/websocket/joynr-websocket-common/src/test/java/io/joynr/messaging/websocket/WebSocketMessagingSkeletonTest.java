/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.messaging.websocket;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashSet;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.name.Names;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton.MainTransportFlagBearer;
import joynr.ImmutableMessage;
import joynr.Message.MessageType;
import joynr.MutableMessage;
import joynr.system.RoutingTypes.WebSocketAddress;

@RunWith(MockitoJUnitRunner.class)
public class WebSocketMessagingSkeletonTest {
    private static final FailureAction NO_FAILURE_EXPECTED = new FailureAction() {
        @Override
        public void execute(Throwable error) {
            fail("Unexpected FailureAction call: " + error.toString());
        }
    };

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

    private WebSocketMessagingSkeleton subject;

    @Mock
    private MainTransportFlagBearer mainTransportFlagBearer;

    @Mock
    private MessageRouter messageRouter;

    @Before
    public void setUp() throws Exception {
        // create test subject
        subject = new WebSocketMessagingSkeleton(Mockito.mock(WebSocketAddress.class),
                                                 Mockito.mock(WebSocketEndpointFactory.class),
                                                 messageRouter,
                                                 mainTransportFlagBearer,
                                                 new HashSet<JoynrMessageProcessor>());

    }

    private void verifyReceivedFromGlobal(boolean expected) throws Exception {
        for (MessageType type : MessageType.values()) {
            subject.transmit(createMessage(type), NO_FAILURE_EXPECTED);
            ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
            verify(messageRouter).routeIn(messageCaptor.capture());
            assertEquals(expected, messageCaptor.getValue().isReceivedFromGlobal());
            reset(messageRouter);
        }
    }

    @Test
    public void doNotSetReceivedFromGlobalInCC() throws Exception {
        verifyReceivedFromGlobal(false);
    }

    @Test
    public void doSetReceivedFromGlobalInApp() throws Exception {
        when(mainTransportFlagBearer.isMainTransport()).thenReturn(true);
        setUp(); //Re-initialize subject
        verifyReceivedFromGlobal(true);
    }

}
