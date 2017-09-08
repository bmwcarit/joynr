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
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.AdditionalAnswers.returnsFirstArg;
import static org.junit.Assert.fail;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertTrue;

import java.util.HashSet;
import java.util.concurrent.Semaphore;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageRouter;
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
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Sets;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingSkeletonTest {

    private final int repeatedMqttMessageIgnorePeriodMs = 1000;
    private final int maxMqttMessagesInQueue = 20;
    private MqttMessagingSkeleton subject;

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

    private FailureAction getExpectToBeCalledAction(final Semaphore semaphore) {
        return new FailureAction() {
            @Override
            public void execute(Throwable error) {
                semaphore.release();
            }
        };
    }

    @Before
    public void setup() {
        subject = new MqttMessagingSkeleton(ownAddress,
                                            repeatedMqttMessageIgnorePeriodMs,
                                            maxMqttMessagesInQueue,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>());
        when(mqttClientFactory.create()).thenReturn(mqttClient);
        subject.init();
        verify(mqttClient).subscribe(anyString());
        reset(mqttClient);
    }

    @Test
    public void testSkeletonRegistersItselfAsMessageProcessedListener() {
        verify(messageRouter).registerMessageProcessedListener(eq(subject));
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
                                            repeatedMqttMessageIgnorePeriodMs,
                                            maxMqttMessagesInQueue,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            preprocessor,
                                            new HashSet<JoynrMessageProcessor>());

        ImmutableMessage message = createTestMessage();

        subject.transmit(message.getSerializedMessage(), 42, 0, failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouter).route(captor.capture());

        assertArrayEquals(message.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testJoynrMessageProcessorIsCalled() throws Exception {
        JoynrMessageProcessor processorMock = mock(JoynrMessageProcessor.class);

        when(processorMock.processIncoming(any(ImmutableMessage.class))).then(returnsFirstArg());

        subject = new MqttMessagingSkeleton(ownAddress,
                                            repeatedMqttMessageIgnorePeriodMs,
                                            maxMqttMessagesInQueue,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            Sets.newHashSet(processorMock));

        ImmutableMessage message = createTestMessage();

        subject.transmit(message.getSerializedMessage(), 42, 0, failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(processorMock).processIncoming(argCaptor.capture());

        Assert.assertArrayEquals(message.getSerializedMessage(), argCaptor.getValue().getSerializedMessage());
    }

    @Test
    public void testClientNotifiedWhenMessageIsProcessed() throws Exception {
        final int mqttMessageId = -753;
        final int mqttQos = 1;
        ImmutableMessage message = createTestMessage();
        final String messageId = message.getId();

        subject.transmit(message.getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);

        Thread.sleep(50);
        verify(mqttClient, times(0)).sendMqttAck(anyInt(), anyInt());

        subject.messageProcessed(messageId);
        verify(mqttClient).sendMqttAck(mqttMessageId, mqttQos);
    }

    @Test
    public void testClientNotifiedForInvalidMessage() throws Exception {
        final int mqttMessageId = -333;
        final int mqttQos = 1;

        Semaphore semaphore = new Semaphore(0);
        subject.transmit("Invalid message which cannot be deserialized".getBytes(),
                         mqttMessageId,
                         mqttQos,
                         getExpectToBeCalledAction(semaphore));

        verify(mqttClient).sendMqttAck(mqttMessageId, mqttQos);
        assertTrue(semaphore.tryAcquire());
    }

    @Test
    public void testClientNotifiedAfterExceptionFromMessageRouter() throws Exception {
        final int mqttMessageId = -44;
        final int mqttQos = 1;

        ImmutableMessage message = createTestMessage();

        doThrow(new JoynrRuntimeException()).when(messageRouter).route(any(ImmutableMessage.class));

        Semaphore semaphore = new Semaphore(0);
        subject.transmit(message.getSerializedMessage(), mqttMessageId, mqttQos, getExpectToBeCalledAction(semaphore));

        verify(mqttClient).sendMqttAck(mqttMessageId, mqttQos);
        assertTrue(semaphore.tryAcquire());
    }

    @Test
    public void testClientNotifiedOnlyOnceWithExceptionFromMessageRouter() throws Exception {
        final int mqttMessageId = 476;
        final int mqttQos = 1;
        ImmutableMessage message = createTestMessage();
        final String messageId = message.getId();

        doThrow(new JoynrRuntimeException()).when(messageRouter).route(any(ImmutableMessage.class));

        Semaphore semaphore = new Semaphore(0);
        subject.transmit(message.getSerializedMessage(), mqttMessageId, mqttQos, getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
        verify(mqttClient, times(1)).sendMqttAck(anyInt(), anyInt());

        subject.messageProcessed(messageId);
        verify(mqttClient, times(1)).sendMqttAck(mqttMessageId, mqttQos);

        subject.messageProcessed(messageId);
        verify(mqttClient, times(1)).sendMqttAck(mqttMessageId, mqttQos);
    }

    @Test
    public void testClientNotifiedOnlyOnceWithoutExceptionFromMessageRouter() throws Exception {
        final int mqttMessageId = 1453;
        final int mqttQos = 1;
        ImmutableMessage message = createTestMessage();
        final String messageId = message.getId();

        subject.transmit(message.getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);

        Thread.sleep(50);
        verify(mqttClient, times(0)).sendMqttAck(mqttMessageId, mqttQos);

        subject.messageProcessed(messageId);
        verify(mqttClient, times(1)).sendMqttAck(mqttMessageId, mqttQos);

        subject.messageProcessed(messageId);
        verify(mqttClient, times(1)).sendMqttAck(mqttMessageId, mqttQos);
    }

    private void transmitDuplicatedMessageForRepeatMqttMessageIgnorePeriodTests(ImmutableMessage message1,
                                                                                ImmutableMessage message2,
                                                                                int sleepDurationMs) throws Exception {
        final int mqttMessageId = 1487;
        final int mqttMessageId2 = 1516;
        final int mqttQos = 1;

        subject.transmit(message1.getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);
        subject.messageProcessed(message1.getId());
        subject.transmit(message1.getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);

        Thread.sleep(sleepDurationMs);

        subject.transmit(message2.getSerializedMessage(), mqttMessageId2, mqttQos, failIfCalledAction);
        subject.messageProcessed(message2.getId());

        subject.transmit(message1.getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);
    }

    @Test
    public void testDuplicatedMessageDroppedWithinRepeatedMqttMessageIgnorePeriod() throws Exception {
        ImmutableMessage message1 = createTestMessage();
        ImmutableMessage message2 = createTestMessage();

        final int toleranceMs = 300;
        final int sleepDurationMs = repeatedMqttMessageIgnorePeriodMs - toleranceMs;
        assertTrue(sleepDurationMs > 0);

        transmitDuplicatedMessageForRepeatMqttMessageIgnorePeriodTests(message1, message2, sleepDurationMs);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouter, times(2)).route(captor.capture());

        assertArrayEquals(message1.getSerializedMessage(), captor.getAllValues().get(0).getSerializedMessage());
        assertArrayEquals(message2.getSerializedMessage(), captor.getAllValues().get(1).getSerializedMessage());
    }

    @Test
    public void testDuplicatedMessageNotDroppedAfterRepeatedMqttMessageIgnorePeriod() throws Exception {
        ImmutableMessage message1 = createTestMessage();
        ImmutableMessage message2 = createTestMessage();

        final int toleranceMs = 50;
        final int sleepDurationMs = repeatedMqttMessageIgnorePeriodMs + toleranceMs;

        transmitDuplicatedMessageForRepeatMqttMessageIgnorePeriodTests(message1, message2, sleepDurationMs);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouter, times(3)).route(captor.capture());

        assertArrayEquals(message1.getSerializedMessage(), captor.getAllValues().get(0).getSerializedMessage());
        assertArrayEquals(message2.getSerializedMessage(), captor.getAllValues().get(1).getSerializedMessage());
        assertArrayEquals(message1.getSerializedMessage(), captor.getAllValues().get(2).getSerializedMessage());
    }

    private ImmutableMessage createTestMessage() throws Exception {
        MutableMessage message = new MutableMessage();

        ObjectMapper objectMapper = new ObjectMapper();
        MqttAddress address = new MqttAddress("testBrokerUri", "testTopic");

        message.setSender("someSender");
        message.setRecipient("someRecipient");
        message.setTtlAbsolute(true);
        message.setTtlMs(100000);
        message.setPayload(new byte[]{ 0, 1, 2 });
        message.setType(Message.VALUE_MESSAGE_TYPE_REQUEST);
        message.setReplyTo(objectMapper.writeValueAsString(address));

        return message.getImmutableMessage();
    }
}
