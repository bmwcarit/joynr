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
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.getImmutableMessageFromPublish;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.failIfCalledAction;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.feedMqttSkeletonWithMessages;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.feedMqttSkeletonWithRequests;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.getExpectToBeCalledAction;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.AdditionalAnswers.returnsFirstArg;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;

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
import org.mockito.ArgumentMatchers;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingSkeletonTest {
    private final int maxIncomingMqttRequests = 20;
    private final String ownTopic = "testOwnTopic";
    private final String testBackendUid = "testBackendUid";

    private MqttMessagingSkeleton subject;

    private String ownGbid = "testOwnGbid";

    @Mock
    private MessageRouter messageRouter;
    @Mock
    private MessageProcessedHandler mockMessageProcessedHandler;

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

    @Mock
    protected MqttMessageInProgressObserver mqttMessageInProgressObserver;

    @Before
    public void setup() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());
        when(mqttClientFactory.createReceiver(ownGbid)).thenReturn(mqttClientReceiver);
        when(mqttClientFactory.createSender(ownGbid)).thenReturn(mqttClientSender);
        when(mqttMessageInProgressObserver.canMessageBeAcknowledged(anyString())).thenReturn(true);
        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequests,
                                            messageRouter,
                                            mockMessageProcessedHandler,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>(),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable,
                                            testBackendUid,
                                            mqttMessageInProgressObserver);
        verify(mqttClientFactory).createReceiver(ownGbid);
        subject.init();
    }

    @Test
    public void testSkeletonCreatesAndStartsSenderAndReceiverForItsOwnGbid() {
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
        verify(mockMessageProcessedHandler).registerMessageProcessedListener(eq(subject));
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
        Mqtt5Publish publish = createTestRequestMessage();
        ImmutableMessage rqMessage = getImmutableMessageFromPublish(publish);

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        subject.transmit(publish, rqMessage.getPrefixedCustomHeaders(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(messageRouter).routeIn(captor.capture());

        assertArrayEquals(rqMessage.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testRegistrationOfReplyToAddress() throws Exception {
        Mqtt5Publish publish = createTestRequestMessage();
        ImmutableMessage rqMessage = getImmutableMessageFromPublish(publish);

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        subject.transmit(publish, rqMessage.getPrefixedCustomHeaders(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        final MqttAddress expectedAddress = new MqttAddress(ownGbid, "testTopic");
        verify(routingTable).put(rqMessage.getSender(), expectedAddress, true, rqMessage.getTtlMs());

        verify(messageRouter).routeIn(captor.capture());
        assertArrayEquals(rqMessage.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testSetCreatorUserIdIsCalled() throws Exception {
        ImmutableMessage rqMessage = createTestRequestMessage();

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        subject.transmit(rqMessage.getSerializedMessage(), rqMessage.getPrefixedCustomHeaders(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        final MqttAddress expectedAddress = new MqttAddress(ownGbid, "testTopic");
        verify(routingTable).put(rqMessage.getSender(), expectedAddress, true, rqMessage.getTtlMs());
        verify(messageRouter).routeIn(captor.capture());

        assertEquals(testBackendUid, captor.getValue().getCreatorUserId());

        assertArrayEquals(rqMessage.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testPrefixedExtraCustomHeaders() throws Exception {
        ImmutableMessage rqMessage = createTestRequestMessage();
        assertTrue(rqMessage.getPrefixedCustomHeaders().isEmpty());
        Map<String, String> prefixedExtraCustomHeaders = Map.of("c-key1", "v1", "key2", "v2", "c-key3", "v3");
        Map<String, String> expectedExtraCustomHeaders = Map.of("key1", "v1", "key3", "v3");

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        subject.transmit(rqMessage.getSerializedMessage(), prefixedExtraCustomHeaders, failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> captor = ArgumentCaptor.forClass(ImmutableMessage.class);
        final MqttAddress expectedAddress = new MqttAddress(ownGbid, "testTopic");
        verify(routingTable).put(rqMessage.getSender(), expectedAddress, true, rqMessage.getTtlMs());
        verify(messageRouter).routeIn(captor.capture());

        assertEquals(expectedExtraCustomHeaders, captor.getValue().getExtraCustomHeaders());

        assertArrayEquals(rqMessage.getSerializedMessage(), captor.getValue().getSerializedMessage());
    }

    @Test
    public void testRawMessageProcessorIsCalled() throws Exception {
        RawMessagingPreprocessor rawMessagingPreprocessorMock = mock(RawMessagingPreprocessor.class);
        when(rawMessagingPreprocessorMock.process(any(byte[].class),
                                                  ArgumentMatchers.<Optional<Map<String, Serializable>>> any())).then(returnsFirstArg());

        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequests,
                                            messageRouter,
                                            mockMessageProcessedHandler,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            rawMessagingPreprocessorMock,
                                            new HashSet<JoynrMessageProcessor>(),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable,
                                            "",
                                            mqttMessageInProgressObserver);

        Mqtt5Publish publish = createTestRequestMessage();
        ImmutableMessage rqMessage = getImmutableMessageFromPublish(publish);

        subject.transmit(publish, rqMessage.getPrefixedCustomHeaders(), failIfCalledAction);

        ArgumentCaptor<byte[]> argCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(rawMessagingPreprocessorMock).process(argCaptor.capture(),
                                                     ArgumentMatchers.<Optional<Map<String, Serializable>>> any());

        Assert.assertArrayEquals(rqMessage.getSerializedMessage(), argCaptor.getValue());
    }

    @Test
    public void testJoynrMessageProcessorIsCalled() throws Exception {
        JoynrMessageProcessor processorMock = mock(JoynrMessageProcessor.class);

        when(processorMock.processIncoming(any(ImmutableMessage.class))).then(returnsFirstArg());

        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequests,
                                            messageRouter,
                                            mockMessageProcessedHandler,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>(Arrays.asList(processorMock)),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable,
                                            "",
                                            mqttMessageInProgressObserver);

        Mqtt5Publish publish = createTestRequestMessage();
        ImmutableMessage rqMessage = getImmutableMessageFromPublish(publish);

        subject.transmit(publish, rqMessage.getPrefixedCustomHeaders(), failIfCalledAction);

        ArgumentCaptor<ImmutableMessage> argCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        verify(processorMock).processIncoming(argCaptor.capture());

        Assert.assertArrayEquals(rqMessage.getSerializedMessage(), argCaptor.getValue().getSerializedMessage());
    }

    /*
    @Test
    public void testFailureActionCalledForInvalidMessage() throws Exception {
        Semaphore semaphore = new Semaphore(0);
        Map<String, String> prefixedCustomHeaders = new HashMap<String, String>();
        subject.transmit("Invalid message which cannot be deserialized".getBytes(),
                         prefixedCustomHeaders,
                         getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
    }
     */
    @Test
    public void testFailureActionCalledAfterExceptionFromMessageRouter() throws Exception {
        Mqtt5Publish publish = createTestRequestMessage();
        ImmutableMessage rqMessage = getImmutableMessageFromPublish(publish);

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        doThrow(new JoynrRuntimeException()).when(messageRouter).routeIn(any(ImmutableMessage.class));

        Semaphore semaphore = new Semaphore(0);
        subject.transmit(publish, rqMessage.getPrefixedCustomHeaders(), getExpectToBeCalledAction(semaphore));

        assertTrue(semaphore.tryAcquire());
        verify(routingTable, times(1)).remove(rqMessage.getSender());
    }

    @Test
    public void testRequestsAreNotAcknowledgedWhenObserverForbidsIt() throws Exception {
        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        when(mqttMessageInProgressObserver.canMessageBeAcknowledged(anyString())).thenReturn(false);

        Mqtt5Publish publish = spy(createTestRequestMessage());
        Mqtt5Publish publish2 = spy(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY));

        subject.transmit(publish,
                         getImmutableMessageFromPublish(createTestRequestMessage()).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(publish2,
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        verify(messageRouter, times(2)).routeIn(any(ImmutableMessage.class));
        verify(publish, never()).acknowledge();
        verify(publish2, never()).acknowledge();
    }

    @Test
    public void testOtherMessagesAreAcceptedEvenWhenMaxForIncomingMqttRequestsIsReached() throws Exception {
        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        feedMqttSkeletonWithRequests(subject, maxIncomingMqttRequests);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests)).routeIn(any(ImmutableMessage.class));

        // Further non-request messages should still be accepted
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP),
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        assertEquals(0, subject.getDroppedMessagesCount());
        verify(messageRouter, times(maxIncomingMqttRequests + 8)).routeIn(any(ImmutableMessage.class));
    }

    @Test
    public void testOutstandingPublishesAreAcknowledgedOnTrigger() throws Exception {
        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        when(mqttMessageInProgressObserver.canMessageBeAcknowledged(anyString())).thenReturn(false);

        Mqtt5Publish publish = spy(createTestRequestMessage());
        Mqtt5Publish publish2 = spy(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY));

        subject.transmit(publish,
                         getImmutableMessageFromPublish(createTestRequestMessage()).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        subject.transmit(publish2,
                         getImmutableMessageFromPublish(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY)).getPrefixedCustomHeaders(),
                         failIfCalledAction);
        verify(messageRouter, times(2)).routeIn(any(ImmutableMessage.class));
        verify(publish, never()).acknowledge();
        verify(publish2, never()).acknowledge();
        subject.acknowledgeOutstandingPublishes();
        verify(publish, times(1)).acknowledge();
        verify(publish2, times(1)).acknowledge();
        // Publishes must not be acknowledged twice
        subject.acknowledgeOutstandingPublishes();
        verify(publish, times(1)).acknowledge();
        verify(publish2, times(1)).acknowledge();
    }

    @Test
    public void testNoMessagesAreDroppedWhenNoMaxForIncomingMqttRequestsIsSet() throws Exception {
        final int maxIncomingMqttRequestsNoLimit = 0;

        subject = new MqttMessagingSkeleton(ownTopic,
                                            maxIncomingMqttRequestsNoLimit,
                                            messageRouter,
                                            mockMessageProcessedHandler,
                                            mqttClientFactory,
                                            mqttTopicPrefixProvider,
                                            new NoOpRawMessagingPreprocessor(),
                                            new HashSet<JoynrMessageProcessor>(),
                                            mockJoynrStatusMetricsReceiver,
                                            ownGbid,
                                            routingTable,
                                            "",
                                            mqttMessageInProgressObserver);
        subject.init();

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        // number of incoming messages is arbitrarily selected
        feedMqttSkeletonWithRequests(subject, 2 * maxIncomingMqttRequests);
        feedMqttSkeletonWithMessages(subject,
                                     Message.MessageType.VALUE_MESSAGE_TYPE_REPLY,
                                     2 * maxIncomingMqttRequests);
        feedMqttSkeletonWithMessages(subject,
                                     Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST,
                                     2 * maxIncomingMqttRequests);

        verify(messageRouter, times(3 * 2 * maxIncomingMqttRequests)).routeIn(any(ImmutableMessage.class));
        assertEquals(0, subject.getDroppedMessagesCount());
    }

    @Test
    public void testFailureActionCalledForExpiredAbsoluteMessage() throws Exception {
        testFailureActionCalled(1, true, "Message is expired");
    }

    @Test
    public void testFailureActionCalledForExpiredRelativeMessage() throws Exception {
        testFailureActionCalled(1, false, "Relative ttl not supported");
    }

    @Test
    public void testFailureActionCalledForNotExpiredRelativeMessage() throws Exception {
        testFailureActionCalled(10000, false, "Relative ttl not supported");
    }

    private void testFailureActionCalled(long ttlMs, boolean ttlAbsolute, String failureText) throws Exception {
        Semaphore semaphore = new Semaphore(0);
        Mqtt5Publish publish = createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, ttlMs, ttlAbsolute);
        ImmutableMessage message = getImmutableMessageFromPublish(publish);
        Thread.sleep(5);

        subject.transmit(publish, message.getCustomHeaders(), getExpectToBeCalledAction(semaphore));
        assertTrue(failureText, semaphore.tryAcquire());
        assertEquals(0, subject.getCurrentCountOfUnprocessedMqttRequests());
        verify(messageRouter, never()).routeIn(any(ImmutableMessage.class));
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
        verify(routingTable,
               never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong(), any(Boolean.class));
    }
}
