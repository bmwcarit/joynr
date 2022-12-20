/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import static org.junit.Assert.assertNotEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashSet;
import java.util.Set;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingSkeletonFactoryTest {

    private final int maxIncomingMqttRequests = 1;
    private final String[] gbids = new String[]{ "testGbid1", "testGbid2", "testGbid3" };
    private final Set<JoynrMessageProcessor> messageProcessors = new HashSet<>();

    @Mock
    private MqttAddress ownAddress;
    @Mock
    private MessageRouter messageRouter;
    @Mock
    private MessageProcessedHandler mockMessageProcessedHandler;
    @Mock
    private RoutingTable routingTable;
    @Mock
    private MqttClientFactory mqttClientFactory;
    @Mock
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;
    @Mock
    private RawMessagingPreprocessor rawMessagingPreprocessor;
    @Mock
    private JoynrStatusMetricsReceiver mockJoynrStatusMetricsReceiver;

    @Test
    public void getSkeletonReturnsCorrectSkeleton() {
        JoynrMqttClient mqttClient1 = mock(JoynrMqttClient.class);
        JoynrMqttClient mqttClient2 = mock(JoynrMqttClient.class);
        JoynrMqttClient mqttClient3 = mock(JoynrMqttClient.class);
        when(mqttClientFactory.createReceiver(gbids[0])).thenReturn(mqttClient1);
        when(mqttClientFactory.createReceiver(gbids[1])).thenReturn(mqttClient2);
        when(mqttClientFactory.createReceiver(gbids[2])).thenReturn(mqttClient3);
        when(mqttClientFactory.createSender(gbids[0])).thenReturn(mqttClient1);
        when(mqttClientFactory.createSender(gbids[1])).thenReturn(mqttClient2);
        when(mqttClientFactory.createSender(gbids[2])).thenReturn(mqttClient3);

        MqttMessagingSkeletonFactory factory = new MqttMessagingSkeletonFactory(gbids,
                                                                                ownAddress,
                                                                                maxIncomingMqttRequests,
                                                                                messageRouter,
                                                                                mockMessageProcessedHandler,
                                                                                mqttClientFactory,
                                                                                mqttTopicPrefixProvider,
                                                                                rawMessagingPreprocessor,
                                                                                messageProcessors,
                                                                                mockJoynrStatusMetricsReceiver,
                                                                                routingTable);

        MqttAddress testAddress = mock(MqttAddress.class);
        when(testAddress.getBrokerUri()).thenReturn(gbids[0]);
        MqttMessagingSkeleton skeleton1 = (MqttMessagingSkeleton) factory.getSkeleton(testAddress);
        when(testAddress.getBrokerUri()).thenReturn(gbids[1]);
        MqttMessagingSkeleton skeleton2 = (MqttMessagingSkeleton) factory.getSkeleton(testAddress);
        when(testAddress.getBrokerUri()).thenReturn(gbids[2]);
        MqttMessagingSkeleton skeleton3 = (MqttMessagingSkeleton) factory.getSkeleton(testAddress);

        assertNotEquals(skeleton1, skeleton2);
        assertNotEquals(skeleton1, skeleton3);
        assertNotEquals(skeleton2, skeleton3);

        verify(mqttClient1, times(0)).start();
        verify(mqttClient2, times(0)).start();
        verify(mqttClient3, times(0)).start();

        skeleton1.init();
        verify(mqttClient1, times(2)).start();
        verify(mqttClient2, times(0)).start();
        verify(mqttClient3, times(0)).start();

        reset(mqttClient1, mqttClient2, mqttClient3);

        skeleton2.init();
        verify(mqttClient1, times(0)).start();
        verify(mqttClient2, times(2)).start();
        verify(mqttClient3, times(0)).start();

        reset(mqttClient1, mqttClient2, mqttClient3);

        skeleton3.init();
        verify(mqttClient1, times(0)).start();
        verify(mqttClient2, times(0)).start();
        verify(mqttClient3, times(2)).start();
    }

    private void testInitAllSkeletons(IMessagingSkeletonFactory factory) {
        MqttAddress testAddress = mock(MqttAddress.class);
        when(testAddress.getBrokerUri()).thenReturn(gbids[0]);
        MqttMessagingSkeleton skeleton1 = (MqttMessagingSkeleton) factory.getSkeleton(testAddress);
        when(testAddress.getBrokerUri()).thenReturn(gbids[1]);
        MqttMessagingSkeleton skeleton2 = (MqttMessagingSkeleton) factory.getSkeleton(testAddress);
        when(testAddress.getBrokerUri()).thenReturn(gbids[2]);
        MqttMessagingSkeleton skeleton3 = (MqttMessagingSkeleton) factory.getSkeleton(testAddress);

        assertNotEquals(skeleton1, skeleton2);
        assertNotEquals(skeleton1, skeleton3);
        assertNotEquals(skeleton2, skeleton3);

        verify(mqttClientFactory).createReceiver(gbids[0]);
        verify(mqttClientFactory).createReceiver(gbids[1]);
        verify(mqttClientFactory).createReceiver(gbids[2]);

        factory.init();

        verify(mqttClientFactory).createSender(gbids[0]);
        verify(mqttClientFactory).createSender(gbids[1]);
        verify(mqttClientFactory).createSender(gbids[2]);
    }

    @Test
    public void initAllSkeletons() {
        JoynrMqttClient mqttClient = mock(JoynrMqttClient.class);
        when(mqttClientFactory.createReceiver(gbids[0])).thenReturn(mqttClient);
        when(mqttClientFactory.createReceiver(gbids[1])).thenReturn(mqttClient);
        when(mqttClientFactory.createReceiver(gbids[2])).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(gbids[0])).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(gbids[1])).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(gbids[2])).thenReturn(mqttClient);

        MqttMessagingSkeletonFactory factory = new MqttMessagingSkeletonFactory(gbids,
                                                                                ownAddress,
                                                                                maxIncomingMqttRequests,
                                                                                messageRouter,
                                                                                mockMessageProcessedHandler,
                                                                                mqttClientFactory,
                                                                                mqttTopicPrefixProvider,
                                                                                rawMessagingPreprocessor,
                                                                                messageProcessors,
                                                                                mockJoynrStatusMetricsReceiver,
                                                                                routingTable);
        testInitAllSkeletons(factory);
    }

    @Test
    public void initAllSkeletons_sharedSubscriptions() {
        JoynrMqttClient mqttClient = mock(JoynrMqttClient.class);
        when(mqttClientFactory.createReceiver(gbids[0])).thenReturn(mqttClient);
        when(mqttClientFactory.createReceiver(gbids[1])).thenReturn(mqttClient);
        when(mqttClientFactory.createReceiver(gbids[2])).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(gbids[0])).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(gbids[1])).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(gbids[2])).thenReturn(mqttClient);
        when(mqttClientFactory.createReplyReceiver(gbids[0])).thenReturn(mqttClient);
        when(mqttClientFactory.createReplyReceiver(gbids[1])).thenReturn(mqttClient);
        when(mqttClientFactory.createReplyReceiver(gbids[2])).thenReturn(mqttClient);

        final boolean backpressureEnabled = false;
        final int backpressureIncomingMqttRequestsUpperThreshold = 42;
        final int backpressureIncomingMqttRequestsLowerThreshold = 41;
        final MqttAddress replyToAddress = mock(MqttAddress.class);
        final String channelId = "testChannelId";
        SharedSubscriptionsMqttMessagingSkeletonFactory factory = new SharedSubscriptionsMqttMessagingSkeletonFactory(gbids,
                                                                                                                      ownAddress,
                                                                                                                      maxIncomingMqttRequests,
                                                                                                                      backpressureEnabled,
                                                                                                                      backpressureIncomingMqttRequestsUpperThreshold,
                                                                                                                      backpressureIncomingMqttRequestsLowerThreshold,
                                                                                                                      replyToAddress,
                                                                                                                      messageRouter,
                                                                                                                      mockMessageProcessedHandler,
                                                                                                                      mqttClientFactory,
                                                                                                                      channelId,
                                                                                                                      mqttTopicPrefixProvider,
                                                                                                                      rawMessagingPreprocessor,
                                                                                                                      messageProcessors,
                                                                                                                      mockJoynrStatusMetricsReceiver,
                                                                                                                      routingTable,
                                                                                                                      false);
        testInitAllSkeletons(factory);
    }

}
