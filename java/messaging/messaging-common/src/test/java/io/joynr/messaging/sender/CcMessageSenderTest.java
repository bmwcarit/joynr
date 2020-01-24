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

import static org.hamcrest.CoreMatchers.instanceOf;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.Optional;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.messaging.routing.TransportReadyListener;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

@RunWith(MockitoJUnitRunner.class)
public class CcMessageSenderTest extends MessageSenderTestBase {

    private ObjectMapper objectMapper = new ObjectMapper();
    private MqttAddress replyToAddress = new MqttAddress("testBrokerUri", "testTopic");
    private MqttAddress globalAddress = new MqttAddress("testBrokerUri", "globalTopic");

    @Before
    public void setUp() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, objectMapper);
    }

    @Test
    public void testReplyToIsSet() throws Exception {
        MutableMessage message = createTestRequestMessage();
        String serializedReplyToAddress = objectMapper.writeValueAsString(replyToAddress);

        testCorrectReplyToSetOnMessage(message, serializedReplyToAddress);
    }

    @Test
    public void testGlobalSetForStatelessAsync() throws Exception {
        MutableMessage message = createTestRequestMessage();
        message.setStatelessAsync(true);
        String globalSerialized = objectMapper.writeValueAsString(globalAddress);

        testCorrectReplyToSetOnMessage(message, globalSerialized);
    }

    private void testCorrectReplyToSetOnMessage(MutableMessage message, String expectedAddress) throws Exception {
        ReplyToAddressProvider replyToAddressProviderMock = mock(ReplyToAddressProvider.class);
        doAnswer(createTransportReadyCallback(replyToAddress)).when(replyToAddressProviderMock)
                                                              .registerGlobalAddressesReadyListener(any(TransportReadyListener.class));

        GlobalAddressProvider globalAddressProviderMock = mock(GlobalAddressProvider.class);
        doAnswer(createTransportReadyCallback(globalAddress)).when(globalAddressProviderMock)
                                                             .registerGlobalAddressesReadyListener(any(TransportReadyListener.class));

        CcMessageSender subject = new CcMessageSender(messageRouterMock,
                                                      replyToAddressProviderMock,
                                                      globalAddressProviderMock);
        subject.sendMessage(message);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouterMock).route(argCaptor.capture());
        assertEquals(expectedAddress, argCaptor.getValue().getReplyTo());
    }

    private Answer<Object> createTransportReadyCallback(MqttAddress address) {
        return (invocation) -> {
            assertEquals(1, invocation.getArguments().length);
            assertThat(invocation.getArguments()[0], instanceOf(TransportReadyListener.class));

            TransportReadyListener listener = (TransportReadyListener) invocation.getArguments()[0];
            listener.transportReady(Optional.of(address));
            return null;
        };
    }
}
