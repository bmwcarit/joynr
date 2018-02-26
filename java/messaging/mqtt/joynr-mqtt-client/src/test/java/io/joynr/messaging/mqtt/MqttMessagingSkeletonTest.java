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
import static org.junit.Assert.assertEquals;
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

    private final int maxIncomingMqttRequests = 20;
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
                                            maxIncomingMqttRequests,
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
                                            maxIncomingMqttRequests,
                                            backpressureEnabled,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            preprocessor,
                                            new HashSet<JoynrMessageProcessor>());

        ImmutableMessage rqMessage = createTestRequestMessage();

        subject.transmit(rqMessage.getSerializedMessage(), 42, 0, failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouter).route(captor.capture());

        assertArrayEquals(rqMessage.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testJoynrMessageProcessorIsCalled() throws Exception {
        JoynrMessageProcessor processorMock = mock(JoynrMessageProcessor.class);

        when(processorMock.processIncoming(any(ImmutableMessage.class))).then(returnsFirstArg());

        subject = new MqttMessagingSkeleton(ownAddress,
                                            maxIncomingMqttRequests,
                                            backpressureEnabled,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            Sets.newHashSet(processorMock));

        ImmutableMessage rqMessage = createTestRequestMessage();

        subject.transmit(rqMessage.getSerializedMessage(), 42, 0, failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(processorMock).processIncoming(argCaptor.capture());

        Assert.assertArrayEquals(rqMessage.getSerializedMessage(), argCaptor.getValue().getSerializedMessage());
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

        ImmutableMessage rqMessage = createTestRequestMessage();

        doThrow(new JoynrRuntimeException()).when(messageRouter).route(any(ImmutableMessage.class));

        Semaphore semaphore = new Semaphore(0);
        subject.transmit(rqMessage.getSerializedMessage(), mqttMessageId, mqttQos, getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
    }

    @Ignore("Will be reactivated with the new message dropping mechanism")
    @Test
    public void testMessagesAreRejectedWhenMaxMqttMessagesInQueueIsReached() throws Exception {
        final int mqttMessageId = 1517;
        final int mqttQos = 1;

        for (int i = 0; i < maxIncomingMqttRequests; i++) {
            subject.transmit(createTestRequestMessage().getSerializedMessage(),
                             mqttMessageId,
                             mqttQos,
                             failIfCalledAction);
        }
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        // As the queue is full, further messages should not be transmitted
        subject.transmit(createTestRequestMessage().getSerializedMessage(), mqttMessageId, mqttQos, failIfCalledAction);
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));
    }

    @Ignore("Will be reactivated with the new message dropping mechanism")
    @Test
    public void testMessagesAreAcceptedAgainWhenMessageIsProcessedAfterMaxMessagesInQueueReached() throws Exception {
        final int mqttQos = 1;
        final int mqttMessageId1_willBeProcessed = 1618;
        final int mqttMessageId2_fillingUpQueue = 1648;
        final int mqttMessageId3_willBeRejected = 1789;
        final int mqttMessageId4_willBeAcceptedAsQueueHasFreeSlotAgain = 2017;

        ImmutableMessage rqMessage1 = createTestRequestMessage();
        final String messageId1 = rqMessage1.getId();
        subject.transmit(rqMessage1.getSerializedMessage(), mqttMessageId1_willBeProcessed, mqttQos, failIfCalledAction);

        for (int i = 0; i < maxIncomingMqttRequests - 1; i++) {
            subject.transmit(createTestRequestMessage().getSerializedMessage(),
                             mqttMessageId2_fillingUpQueue,
                             mqttQos,
                             failIfCalledAction);
        }
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        // As the queue is full, further messages should not be transmitted
        // until an already accepted message is marked as processed
        subject.transmit(createTestRequestMessage().getSerializedMessage(),
                         mqttMessageId3_willBeRejected,
                         mqttQos,
                         failIfCalledAction);
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        subject.messageProcessed(messageId1);

        subject.transmit(createTestRequestMessage().getSerializedMessage(),
                         mqttMessageId4_willBeAcceptedAsQueueHasFreeSlotAgain,
                         mqttQos,
                         failIfCalledAction);
        verify(messageRouter, times(maxIncomingMqttRequests + 1)).route(any(ImmutableMessage.class));
    }

    @Test
    public void testNoMessagesAreDroppedWhenNoMaxForIncomingMqttRequestsIsSet() throws Exception {
        final int maxIncomingMqttRequestsNoLimit = 0;
        final int mqttMessageId = 1517;
        final int mqttQos = 1;

        subject = new MqttMessagingSkeleton(ownAddress,
                                            maxIncomingMqttRequestsNoLimit,
                                            backpressureEnabled,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>());
        subject.init();

        // number of incoming messages is arbitrarily selected
        for (int i = 0; i < (2 * maxIncomingMqttRequests); i++) {
            subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_REQUEST).getSerializedMessage(),
                             mqttMessageId,
                             mqttQos,
                             failIfCalledAction);
            subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_REPLY).getSerializedMessage(),
                             mqttMessageId,
                             mqttQos,
                             failIfCalledAction);
            subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_MULTICAST).getSerializedMessage(),
                             mqttMessageId,
                             mqttQos,
                             failIfCalledAction);
        }

        verify(messageRouter, times(3 * 2 * maxIncomingMqttRequests)).route(any(ImmutableMessage.class));
        assertEquals(0, subject.getDroppedMessagesCount());
    }

    private ImmutableMessage createTestRequestMessage() throws Exception {
        return createTestMessage(Message.VALUE_MESSAGE_TYPE_REQUEST);
    }

    private ImmutableMessage createTestMessage(String messageType) throws Exception {
        MutableMessage message = new MutableMessage();

        ObjectMapper objectMapper = new ObjectMapper();
        MqttAddress address = new MqttAddress("testBrokerUri", "testTopic");

        message.setSender("someSender");
        message.setRecipient("someRecipient");
        message.setTtlAbsolute(true);
        message.setTtlMs(100000);
        message.setPayload(new byte[]{ 0, 1, 2 });
        message.setType(messageType);
        message.setReplyTo(objectMapper.writeValueAsString(address));

        return message.getImmutableMessage();
    }
}
