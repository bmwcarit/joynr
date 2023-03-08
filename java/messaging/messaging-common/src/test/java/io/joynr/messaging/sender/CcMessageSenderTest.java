/*
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
package io.joynr.messaging.sender;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.Properties;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.junit.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.system.RoutingTypes.Address;
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
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(MessageRouter.class).toInstance(messageRouterMock);
                bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.GLOBAL_ADDRESS))
                                   .toInstance(globalAddress);
                bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.REPLY_TO_ADDRESS))
                                   .toInstance(replyToAddress);
            }
        }, new JoynrPropertiesModule(new Properties()));
        CcMessageSender subject = injector.getInstance(CcMessageSender.class);
        subject.sendMessage(message);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouterMock).routeOut(argCaptor.capture());
        assertEquals(expectedAddress, argCaptor.getValue().getReplyTo());
    }
}
