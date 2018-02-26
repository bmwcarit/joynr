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

        subject.transmit(rqMessage.getSerializedMessage(), failIfCalledAction);

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

        subject.transmit(rqMessage.getSerializedMessage(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(processorMock).processIncoming(argCaptor.capture());

        Assert.assertArrayEquals(rqMessage.getSerializedMessage(), argCaptor.getValue().getSerializedMessage());
    }

    @Test
    public void testFailureActionCalledForInvalidMessage() throws Exception {
        Semaphore semaphore = new Semaphore(0);
        subject.transmit("Invalid message which cannot be deserialized".getBytes(),
                         getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
    }

    @Test
    public void testFailureActionCalledAfterExceptionFromMessageRouter() throws Exception {
        ImmutableMessage rqMessage = createTestRequestMessage();

        doThrow(new JoynrRuntimeException()).when(messageRouter).route(any(ImmutableMessage.class));

        Semaphore semaphore = new Semaphore(0);
        subject.transmit(rqMessage.getSerializedMessage(), getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
    }

    @Test
    public void testFurtherRequestsAreDroppedWhenMaxForIncomingMqttRequestsIsReached() throws Exception {
        feedMqttSkeletonWithRequests(maxIncomingMqttRequests);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        // As the limit is reached, further requests should be dropped
        subject.transmit(createTestRequestMessage().getSerializedMessage(), failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_ONE_WAY).getSerializedMessage(),
                         failIfCalledAction);
        assertEquals(2, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));
    }

    @Test
    public void testOtherMessagesAreAcceptedEvenWhenMaxForIncomingMqttRequestsIsReached() throws Exception {
        feedMqttSkeletonWithRequests(maxIncomingMqttRequests);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        // Further non-request messages should still be accepted
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_REPLY).getSerializedMessage(), failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_MULTICAST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_PUBLICATION).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP).getSerializedMessage(),
                         failIfCalledAction);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests + 8)).route(any(ImmutableMessage.class));
    }

    @Test
    public void testRequestsAreAcceptedAgainWhenPreviousAreProcessedAfterMaxIncomingRequestsReached() throws Exception {
        ImmutableMessage rqMessage1 = createTestRequestMessage();
        final String messageId1 = rqMessage1.getId();
        subject.transmit(rqMessage1.getSerializedMessage(), failIfCalledAction);

        feedMqttSkeletonWithRequests(maxIncomingMqttRequests - 1);
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        // As the limit is reached, further requests should be dropped and not transmitted
        // until an already accepted request is marked as processed
        subject.transmit(createTestRequestMessage().getSerializedMessage(), failIfCalledAction);
        assertEquals(1, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        subject.messageProcessed(messageId1);

        subject.transmit(createTestRequestMessage().getSerializedMessage(), failIfCalledAction);
        verify(messageRouter, times(maxIncomingMqttRequests + 1)).route(any(ImmutableMessage.class));
    }

    @Test
    public void testNoMessagesAreDroppedWhenNoMaxForIncomingMqttRequestsIsSet() throws Exception {
        final int maxIncomingMqttRequestsNoLimit = 0;

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
        feedMqttSkeletonWithRequests(2 * maxIncomingMqttRequests);
        feedMqttSkeletonWithMessages(Message.VALUE_MESSAGE_TYPE_REPLY, 2 * maxIncomingMqttRequests);
        feedMqttSkeletonWithMessages(Message.VALUE_MESSAGE_TYPE_MULTICAST, 2 * maxIncomingMqttRequests);

        verify(messageRouter, times(3 * 2 * maxIncomingMqttRequests)).route(any(ImmutableMessage.class));
        assertEquals(0, subject.getDroppedMessagesCount());
    }

    private void feedMqttSkeletonWithRequests(int numRequests) throws Exception {
        feedMqttSkeletonWithMessages(Message.VALUE_MESSAGE_TYPE_REQUEST, numRequests);
    }

    private void feedMqttSkeletonWithMessages(String messageType, int numMessages) throws Exception {
        for (int i = 0; i < numMessages; i++) {
            subject.transmit(createTestMessage(messageType).getSerializedMessage(), failIfCalledAction);
        }
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
