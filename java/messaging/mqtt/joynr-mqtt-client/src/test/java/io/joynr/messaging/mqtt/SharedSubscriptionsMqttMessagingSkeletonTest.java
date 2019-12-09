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

import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.createTestMessage;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.failIfCalledAction;
import static io.joynr.messaging.mqtt.MqttMessagingSkeletonTestUtil.feedMqttSkeletonWithRequests;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Matchers.startsWith;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.HashSet;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import joynr.Message;
import joynr.system.RoutingTypes.RoutingTypesUtil;

/**
 * Unit tests for {@link SharedSubscriptionsMqttMessagingSkeleton}.
 */
@RunWith(MockitoJUnitRunner.class)
public class SharedSubscriptionsMqttMessagingSkeletonTest {
    private int maxMqttMessagesInQueue = 20;
    private boolean backpressureEnabled = false;
    private int backpressureIncomingMqttRequestsUpperThreshold = 80;
    private int backpressureIncomingMqttRequestsLowerThreshold = 20;

    private String ownGbid = "testOwnGbid";

    @Mock
    private MqttClientFactory mqttClientFactory;

    @Mock
    private JoynrMqttClient mqttClient;

    private final String ownTopic = "testOwnTopic";
    private final String replyToTopic = "testReplyToTopic";

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private RoutingTable routingTable;

    @Mock
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    @Mock
    private MqttStatusReceiver mqttStatusReceiver;

    private SharedSubscriptionsMqttMessagingSkeleton subject;

    @Before
    public void setup() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());
        when(mqttClientFactory.createReceiver(ownGbid)).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(ownGbid)).thenReturn(mqttClient);
    }

    private void createAndInitSkeleton(String channelId) {
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownTopic,
                                                               maxMqttMessagesInQueue,
                                                               backpressureEnabled,
                                                               backpressureIncomingMqttRequestsUpperThreshold,
                                                               backpressureIncomingMqttRequestsLowerThreshold,
                                                               replyToTopic,
                                                               messageRouter,
                                                               mqttClientFactory,
                                                               channelId,
                                                               mqttTopicPrefixProvider,
                                                               new NoOpRawMessagingPreprocessor(),
                                                               new HashSet<JoynrMessageProcessor>(),
                                                               mqttStatusReceiver,
                                                               ownGbid,
                                                               routingTable);
        subject.init();
        verify(mqttClient).subscribe(startsWith("$share/"));
    }

    private void triggerAndVerifySharedSubscriptionsTopicUnsubscribeAndSubscribeCycle(String expectedChannelId,
                                                                                      int expectedTotalUnsubscribeCallCount,
                                                                                      int expectedTotalSubscribeCallCount) throws Exception {
        final String expectedSharedSubscriptionsTopicPrefix = "$share/" + expectedChannelId + "/";

        final int mqttRequestsToHitUpperThreshold = (maxMqttMessagesInQueue
                * backpressureIncomingMqttRequestsUpperThreshold) / 100;
        final int mqttRequestsToHitLowerThreshold = (maxMqttMessagesInQueue
                * backpressureIncomingMqttRequestsLowerThreshold) / 100;

        // fill up with requests and verify unsubscribe
        List<String> messageIds = feedMqttSkeletonWithRequests(subject, mqttRequestsToHitUpperThreshold + 1);
        verify(mqttClient,
               times(expectedTotalUnsubscribeCallCount)).unsubscribe(startsWith(expectedSharedSubscriptionsTopicPrefix));

        // finish processing of as many requests needed to drop below lower threshold
        final int numOfRequestsToProcess = mqttRequestsToHitUpperThreshold - mqttRequestsToHitLowerThreshold + 2;
        for (int i = 0; i < numOfRequestsToProcess; i++) {
            subject.messageProcessed(messageIds.get(i));
        }

        // verify that now subscribe is triggered again
        verify(mqttClient,
               times(expectedTotalSubscribeCallCount)).subscribe(startsWith(expectedSharedSubscriptionsTopicPrefix));

        // cleanup: process the rest of the messages in order to have 0 pending requests
        for (int i = numOfRequestsToProcess; i < messageIds.size(); i++) {
            subject.messageProcessed(messageIds.get(i));
        }
    }

    @Test
    public void testSubscribesToSharedSubscription() {
        createAndInitSkeleton("channelId");
        verify(mqttClient).subscribe(eq("$share/channelId/" + ownTopic + "/#"));
        verify(mqttClient).subscribe(eq(replyToTopic + "/#"));
    }

    @Test
    public void testChannelIdStrippedOfNonAlphaChars() {
        createAndInitSkeleton("channel@123_bling$$");
        verify(mqttClient).subscribe(startsWith("$share/channelbling/"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalChannelId() {
        createAndInitSkeleton("@123_$$-!");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersNoMaxMqttMessagesInQueue() {
        backpressureEnabled = true;
        maxMqttMessagesInQueue = 0;

        createAndInitSkeleton("channelId");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersInvalidUpperThreshold() {
        backpressureEnabled = true;
        backpressureIncomingMqttRequestsUpperThreshold = 101;

        createAndInitSkeleton("channelId");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersInvalidLowerThreshold() {
        backpressureEnabled = true;
        backpressureIncomingMqttRequestsLowerThreshold = -1;

        createAndInitSkeleton("channelId");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidBackpressureParametersLowerThresholdAboveUpper() {
        backpressureEnabled = true;
        backpressureIncomingMqttRequestsUpperThreshold = 70;
        backpressureIncomingMqttRequestsLowerThreshold = 75;

        createAndInitSkeleton("channelId");
    }

    @Test
    public void testBackpressureTriggersUnsubscribeWhenUpperThresholdHit() throws Exception {
        backpressureEnabled = true;
        createAndInitSkeleton("channelIdBackpressure");

        final int mqttRequestsToHitUpperThreshold = (maxMqttMessagesInQueue
                * backpressureIncomingMqttRequestsUpperThreshold) / 100;
        feedMqttSkeletonWithRequests(subject, mqttRequestsToHitUpperThreshold - 1);

        // just below threshold, still no unsubscribe call expected
        verify(mqttClient, times(0)).unsubscribe(any(String.class));

        // messages that are not of request type should not trigger unsubscribe as well
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY).getSerializedMessage(),
                         failIfCalledAction);
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST).getSerializedMessage(),
                         failIfCalledAction);
        verify(mqttClient, times(0)).unsubscribe(any(String.class));

        // a further request should hit the threshold value and trigger an unsubscribe
        subject.transmit(createTestMessage(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST).getSerializedMessage(),
                         failIfCalledAction);
        verify(mqttClient).unsubscribe(startsWith("$share/channelIdBackpressure/"));
    }

    @Test
    public void testBackpressureTriggersResubscribeWhenDroppedBelowLowerThreshold() throws Exception {
        backpressureEnabled = true;
        final String channelId = "channelIdBackpressureOneCycle";
        createAndInitSkeleton(channelId);

        final int expectedTotalUnsubscribeCallCount = 1;
        final int expectedTotalSubscribeCallCount = 2; // one call is from the init method of the skeleton
        triggerAndVerifySharedSubscriptionsTopicUnsubscribeAndSubscribeCycle(channelId,
                                                                             expectedTotalUnsubscribeCallCount,
                                                                             expectedTotalSubscribeCallCount);
    }

    @Test
    public void testBackpressureTriggersUnsubscribeAndSubscribeRepeatedly() throws Exception {
        backpressureEnabled = true;
        final String channelId = "channelIdBackpressureMultipleCycles";
        createAndInitSkeleton(channelId);

        final int numCycles = 10;
        for (int i = 1; i <= numCycles; i++) {
            final int expectedTotalUnsubscribeCallCount = i;
            final int expectedTotalSubscribeCallCount = i + 1; // one call is from the init method of the skeleton
            triggerAndVerifySharedSubscriptionsTopicUnsubscribeAndSubscribeCycle(channelId,
                                                                                 expectedTotalUnsubscribeCallCount,
                                                                                 expectedTotalSubscribeCallCount);
        }
    }
}
