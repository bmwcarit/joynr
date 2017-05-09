package io.joynr.messaging.mqtt;

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

import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.anyMap;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashSet;

import static org.mockito.AdditionalAnswers.returnsFirstArg;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.JoynrMessageSerializer;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.serialize.JsonSerializer;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.system.RoutingTypes.MqttAddress;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.Assert;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Sets;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingSkeletonTest {

    private MqttMessagingSkeleton subject;
    private JoynrMessageSerializer messageSerializer;

    @Mock
    private MqttAddress ownAddress;

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private MqttClientFactory mqttClientFactory;

    @Mock
    private JoynrMqttClient mqttClient;

    @Mock
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    private FailureAction failIfCalledAction = new FailureAction() {
        @Override
        public void execute(Throwable error) {
            fail("failure action was erroneously called");
        }
    };

    @Before
    public void setup() {
        ObjectMapper objectMapper = new ObjectMapper();
        messageSerializer = new JsonSerializer(objectMapper);

        subject = new MqttMessagingSkeleton(ownAddress,
                                            messageRouter,
                                            mqttClientFactory,
                                            messageSerializer,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>());
        when(mqttClientFactory.create()).thenReturn(mqttClient);
        subject.init();
        verify(mqttClient).subscribe(anyString());
        reset(mqttClient);
    }

    @Test
    public void testSubscribeToMulticastWithTopicPrefix() {
        final String expectedPrefix = "testMulticastPrefix";
        final String multicastId = "multicastId";
        when(mqttTopicPrefixProvider.getMulticastTopicPrefix()).thenReturn(expectedPrefix);

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClient).subscribe(expectedPrefix + multicastId);

        subject.unregisterMulticastSubscription(multicastId);
        verify(mqttClient).unsubscribe(expectedPrefix + multicastId);
    }

    @Test
    public void testOnlySubscribeToMulticastIfNotAlreadySubscribed() {
        when(mqttTopicPrefixProvider.getMulticastTopicPrefix()).thenReturn("");
        String multicastId = "multicastId";

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClient).subscribe(eq(multicastId));
        reset(mqttClient);

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClient, never()).subscribe(anyString());
    }

    @Test
    public void testMultilevelWildcardTranslated() {
        when(mqttTopicPrefixProvider.getMulticastTopicPrefix()).thenReturn("");
        String multicastId = "one/two/*";

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClient).subscribe(eq("one/two/#"));

        subject.unregisterMulticastSubscription(multicastId);
        verify(mqttClient).unsubscribe("one/two/#");
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testMessageRouterIsCalled() throws Exception {
        RawMessagingPreprocessor preprocessor = mock(RawMessagingPreprocessor.class);
        when(preprocessor.process(any(byte[].class), anyMap())).then(returnsFirstArg());
        subject = new MqttMessagingSkeleton(ownAddress,
                                            messageRouter,
                                            mqttClientFactory,
                                            messageSerializer,
                                            mqttTopicPrefixProvider,
                                            preprocessor,
                                            new HashSet<JoynrMessageProcessor>());

        ObjectMapper objectMapper = new ObjectMapper();
        MqttAddress address = new MqttAddress("testBrokerUri", "testTopic");

        ImmutableMessage message = Mockito.mock(ImmutableMessage.class);
        when(message.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_REQUEST);
        when(message.getReplyTo()).thenReturn(objectMapper.writeValueAsString(address));

        subject.transmit(message, failIfCalledAction);
        verify(messageRouter).route(message);
    }

    @Test
    public void testJoynrMessageProcessorIsCalled() throws Exception {
        JoynrMessageProcessor processorMock = mock(JoynrMessageProcessor.class);

        when(processorMock.processIncoming(Mockito.any(ImmutableMessage.class))).then(returnsFirstArg());

        subject = new MqttMessagingSkeleton(ownAddress,
                                            messageRouter,
                                            mqttClientFactory,
                                            messageSerializer,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            Sets.newHashSet(processorMock));

        byte[] payload = new byte[]{ 0, 1, 2 };
        MutableMessage message = new MutableMessage();
        message.setSender("someSender");
        message.setRecipient("someRecipient");
        message.setTtlAbsolute(true);
        message.setTtlMs(100000);
        message.setPayload(payload);
        message.setType(Message.VALUE_MESSAGE_TYPE_REQUEST);

        subject.transmit(message.getImmutableMessage().getSerializedMessage(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(processorMock).processIncoming(argCaptor.capture());

        Assert.assertArrayEquals(payload, argCaptor.getValue().getUnencryptedBody());
    }
}
