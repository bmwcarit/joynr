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
package io.joynr.messaging.mqtt;

import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.createTestMessage;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.createTestRequestMessage;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.failIfCalledAction;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.feedMqttSkeletonWithMessages;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.feedMqttSkeletonWithRequests;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.getExpectToBeCalledAction;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.AdditionalAnswers.returnsFirstArg;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.Serializable;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.Semaphore;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingSkeletonTest {
    private final int maxIncomingMqttRequests = 20;
    private final String ownTopic = "testOwnTopic";

    private MqttMessagingSkeleton subject;

    private String ownGbid = "testOwnGbid";

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private RoutingTable routingTable;

    @Mock
    private MqttClientFactory mqttClientFactory;

    @Mock
    private JoynrMqttClient mqttClientReceiver;
    @Mock
    private JoynrMqttClient mqttClientSender;

    @Mock
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    @Mock
    private JoynrStatusMetricsReceiver mockJoynrStatusMetricsReceiver;

    @Before
    public void setup() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());
        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequests,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>(),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable);
        when(mqttClientFactory.createReceiver(ownGbid)).thenReturn(mqttClientReceiver);
        when(mqttClientFactory.createSender(ownGbid)).thenReturn(mqttClientSender);
        subject.init();
    }

    @Test
    public void testSkeletonCreatesAndStartsSenderAndReceiverForItsOwnGbid() {
        verify(mqttClientFactory).createReceiver(ownGbid);
        verify(mqttClientFactory).createSender(ownGbid);

        verify(mqttClientReceiver).start();
        verify(mqttClientSender).start();
    }

    @Test
    public void testSkeletonSubscribesToOwnTopic() {
        verify(mqttClientReceiver).subscribe(ownTopic + "/#");
        verify(mqttClientSender, times(0)).subscribe(anyString());
    }

    @Test
    public void testSkeletonRegistersItselfAsMessageProcessedListener() {
        verify(messageRouter).registerMessageProcessedListener(eq(subject));
    }

    @Test
    public void testSkeletonRegistersItselfAsMessageListener() {
        verify(mqttClientReceiver).setMessageListener(subject);
        verify(mqttClientSender, times(0)).setMessageListener(any(IMqttMessagingSkeleton.class));
    }

    @Test
    public void testSubscribeToMulticastWithTopicPrefix() {
        final String expectedPrefix = "testMulticastPrefix";
        final String multicastId = "multicastId";
        when(mqttTopicPrefixProvider.getMulticastTopicPrefix()).thenReturn(expectedPrefix);

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClientReceiver).subscribe(expectedPrefix + multicastId);

        subject.unregisterMulticastSubscription(multicastId);
        verify(mqttClientReceiver).unsubscribe(expectedPrefix + multicastId);
    }

    @Test
    public void testOnlySubscribeToMulticastIfNotAlreadySubscribed() {
        when(mqttTopicPrefixProvider.getMulticastTopicPrefix()).thenReturn("");
        String multicastId = "multicastId";

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClientReceiver).subscribe(eq(multicastId));
        reset(mqttClientReceiver);

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClientReceiver, never()).subscribe(anyString());
    }

    @Test
    public void testMultilevelWildcardTranslated() {
        when(mqttTopicPrefixProvider.getMulticastTopicPrefix()).thenReturn("");
        String multicastId = "one/two/*";

        subject.registerMulticastSubscription(multicastId);
        verify(mqttClientReceiver).subscribe(eq("one/two/#"));

        subject.unregisterMulticastSubscription(multicastId);
        verify(mqttClientReceiver).unsubscribe("one/two/#");
    }

    @Test
    public void testMessageRouterIsCalled() throws Exception {
        ImmutableMessage rqMessage = createTestRequestMessage();

        subject.transmit(rqMessage.getSerializedMessage(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouter).route(captor.capture());

        assertArrayEquals(rqMessage.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testRegistrationOfReplyToAddress() throws Exception {
        ImmutableMessage rqMessage = createTestRequestMessage();

        subject.transmit(rqMessage.getSerializedMessage(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        final MqttAddress expectedAddress = new MqttAddress(ownGbid, "testTopic");
        verify(routingTable).put(rqMessage.getSender(), expectedAddress, true, 100000L);

        verify(messageRouter).route(captor.capture());
        assertArrayEquals(rqMessage.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testRawMessageProcessorIsCalled() throws Exception {
        RawMessagingPreprocessor rawMessagingPreprocessorMock = mock(RawMessagingPreprocessor.class);
        when(rawMessagingPreprocessorMock.process(any(byte[].class),
                                                  Matchers.<Optional<Map<String, Serializable>>> any())).then(returnsFirstArg());

        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequests,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            rawMessagingPreprocessorMock,
                                            new HashSet<JoynrMessageProcessor>(),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable);

        ImmutableMessage rqMessage = createTestRequestMessage();

        subject.transmit(rqMessage.getSerializedMessage(), failIfCalledAction);

        ArgumentCaptor<byte[]> argCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(rawMessagingPreprocessorMock).process(argCaptor.capture(),
                                                     Matchers.<Optional<Map<String, Serializable>>> any());

        Assert.assertArrayEquals(rqMessage.getSerializedMessage(), argCaptor.getValue());
    }

    @Test
    public void testJoynrMessageProcessorIsCalled() throws Exception {
        JoynrMessageProcessor processorMock = mock(JoynrMessageProcessor.class);

        when(processorMock.processIncoming(any(ImmutableMessage.class))).then(returnsFirstArg());

        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequests,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>(Arrays.asList(processorMock)),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable);

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
        feedMqttSkeletonWithRequests(subject, maxIncomingMqttRequests);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        // As the limit is reached, further requests should be dropped
        subject.transmit(createTestRequestMessage().getSerializedMessage(), failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY).getSerializedMessage(),
                         failIfCalledAction);
        assertEquals(2, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));
    }

    @Test
    public void testMqttStatusReceiverIsNotifiedWhenMessageIsDropped() throws Exception {
        feedMqttSkeletonWithRequests(subject, maxIncomingMqttRequests);
        subject.transmit(createTestRequestMessage().getSerializedMessage(), failIfCalledAction);

        verify(mockJoynrStatusMetricsReceiver, times(1)).notifyMessageDropped();
    }

    @Test
    public void testOtherMessagesAreAcceptedEvenWhenMaxForIncomingMqttRequestsIsReached() throws Exception {
        feedMqttSkeletonWithRequests(subject, maxIncomingMqttRequests);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).route(any(ImmutableMessage.class));

        // Further non-request messages should still be accepted
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP).getSerializedMessage(),
                         failIfCalledAction);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests + 8)).route(any(ImmutableMessage.class));
    }

    @Test
    public void testRequestsAreAcceptedAgainWhenPreviousAreProcessedAfterMaxIncomingRequestsReached() throws Exception {
        ImmutableMessage rqMessage1 = createTestRequestMessage();
        final String messageId1 = rqMessage1.getId();
        subject.transmit(rqMessage1.getSerializedMessage(), failIfCalledAction);

        feedMqttSkeletonWithRequests(subject, maxIncomingMqttRequests - 1);
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

        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequestsNoLimit,
                                            messageRouter,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>(),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable);
        subject.init();

        // number of incoming messages is arbitrarily selected
        feedMqttSkeletonWithRequests(subject, 2 * maxIncomingMqttRequests);
        feedMqttSkeletonWithMessages(subject,
                                     Message.MessageType.VALUE_MESSAGE_TYPE_REPLY,
                                     2 * maxIncomingMqttRequests);
        feedMqttSkeletonWithMessages(subject,
                                     Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST,
                                     2 * maxIncomingMqttRequests);

        verify(messageRouter, times(3 * 2 * maxIncomingMqttRequests)).route(any(ImmutableMessage.class));
        assertEquals(0, subject.getDroppedMessagesCount());
    }
}
