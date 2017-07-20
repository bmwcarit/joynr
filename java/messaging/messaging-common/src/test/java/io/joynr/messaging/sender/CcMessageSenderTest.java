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
import static org.junit.Assert.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doAnswer;
import static org.hamcrest.CoreMatchers.instanceOf;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.messaging.routing.TransportReadyListener;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class CcMessageSenderTest extends MessageSenderTestBase {
    @Test
    public void testReplyToIsSet() throws Exception {
        MutableMessage message = createTestRequestMessage();
        final MqttAddress replyToAddress = new MqttAddress("testBrokerUri", "testTopic");
        ObjectMapper objectMapper = new ObjectMapper();
        String serializedReplyToAddress = objectMapper.writeValueAsString(replyToAddress);

        ReplyToAddressProvider replyToAddressProviderMock = Mockito.mock(ReplyToAddressProvider.class);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                assertEquals(1, invocation.getArguments().length);
                assertThat(invocation.getArguments()[0], instanceOf(TransportReadyListener.class));

                TransportReadyListener listener = (TransportReadyListener) invocation.getArguments()[0];
                listener.transportReady(replyToAddress);
                return null;
            }
        }).when(replyToAddressProviderMock).registerGlobalAddressesReadyListener(any(TransportReadyListener.class));

        CcMessageSender subject = new CcMessageSender(messageRouterMock, replyToAddressProviderMock);
        subject.sendMessage(message);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouterMock).route(argCaptor.capture());
        assertEquals(serializedReplyToAddress, argCaptor.getValue().getReplyTo());
    }
}
