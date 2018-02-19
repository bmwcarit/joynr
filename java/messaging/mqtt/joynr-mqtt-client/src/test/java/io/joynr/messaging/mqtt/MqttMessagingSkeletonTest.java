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
package io.joynr.messaging.mqtt;

import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.anyMap;
import static org.mockito.Matchers.any;
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
import org.junit.Ignore;
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

    private final int maxMqttMessagesInQueue = 20;
    private final boolean backpressureEnabled = true;
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
                                            maxMqttMessagesInQueue,
                                            backpressureEnabled,
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
                                            maxMqttMessagesInQueue,
                                            backpressureEnabled,
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
                                            maxMqttMessagesInQueue,
                                            backpressureEnabled,
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
    public void testFailureActionCalledForInvalidMessage() throws Exception {
        final int mqttMessageId = -333;
        final int mqttQos = 1;

        Semaphore semaphore = new Semaphore(0);
        subject.transmit("Invalid message which cannot be deserialized".getBytes(),
                         mqttMessageId,
                         mqttQos,
                         getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
    }

    @Test
    public void testFailureActionCalledAfterExceptionFromMessageRouter() throws Exception {
        final int mqttMessageId = -44;
        final int mqttQos = 1;

        ImmutableMessage message = createTestMessage();

        doThrow(new JoynrRuntimeException()).when(messageRouter).route(any(ImmutableMessage.class));

        Semaphore semaphore = new Semaphore(0);
        subject.transmit(message.getSerializedMessage(), mqttMessageId, mqttQos, getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
    }

    @Ignore("Will be reactivated with the new message dropping mechanism")
    @Test
    public void testMessagesAreRejectedWhenMaxMqttMessagesInQueueIsReached() throws Exception {
        final int mqttMessageId = 1517;
        final int mqttQos = 1;

        for (int i = 0; i < maxMqttMessagesInQueue; i++) {
            subject.transmit(createTestMessage().getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);
        }
        verify(messageRouter, times(maxMqttMessagesInQueue)).route(any(ImmutableMessage.class));

        // As the queue is full, further messages should not be transmitted
        subject.transmit(createTestMessage().getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);
        verify(messageRouter, times(maxMqttMessagesInQueue)).route(any(ImmutableMessage.class));
    }

    @Ignore("Will be reactivated with the new message dropping mechanism")
    @Test
    public void testMessagesAreAcceptedAgainWhenMessageIsProcessedAfterMaxMessagesInQueueReached() throws Exception {
        final int mqttQos = 1;
        final int mqttMessageId1_willBeProcessed = 1618;
        final int mqttMessageId2_fillingUpQueue = 1648;
        final int mqttMessageId3_willBeRejected = 1789;
        final int mqttMessageId4_willBeAcceptedAsQueueHasFreeSlotAgain = 2017;

        ImmutableMessage message1 = createTestMessage();
        final String messageId1 = message1.getId();
        subject.transmit(message1.getSerializedMessage(), mqttMessageId1_willBeProcessed, mqttQos, failIfCalledAction);

        for (int i = 0; i < maxMqttMessagesInQueue - 1; i++) {
            subject.transmit(createTestMessage().getSerializedMessage(),
                             mqttMessageId2_fillingUpQueue,
                             mqttQos,
                             failIfCalledAction);
        }
        verify(messageRouter, times(maxMqttMessagesInQueue)).route(any(ImmutableMessage.class));

        // As the queue is full, further messages should not be transmitted
        // until an already accepted message is marked as processed
        subject.transmit(createTestMessage().getSerializedMessage(),
                         mqttMessageId3_willBeRejected,
                         mqttQos,
                         failIfCalledAction);
        verify(messageRouter, times(maxMqttMessagesInQueue)).route(any(ImmutableMessage.class));

        subject.messageProcessed(messageId1);

        subject.transmit(createTestMessage().getSerializedMessage(),
                         mqttMessageId4_willBeAcceptedAsQueueHasFreeSlotAgain,
                         mqttQos,
                         failIfCalledAction);
        verify(messageRouter, times(maxMqttMessagesInQueue + 1)).route(any(ImmutableMessage.class));
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
