/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.startsWith;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.RoutingTypesUtil;

public abstract class AbstractSharedSubscriptionsMqttMessagingSkeletonTest {

    protected int maxMqttMessagesInQueue = 20;
    protected String ownGbid = "testOwnGbid";
    @Mock
    protected MqttClientFactory mqttClientFactory;
    @Mock
    protected JoynrMqttClient mqttClient;
    @Mock
    protected JoynrMqttClient mqttReplyClient;
    protected final String ownTopic = "testOwnTopic";
    protected final String replyToTopic = "testReplyToTopic";
    @Mock
    protected MessageRouter messageRouter;
    @Mock
    protected MessageProcessedHandler messageProcessedHandler;
    @Mock
    protected RoutingTable routingTable;
    @Mock
    protected MqttTopicPrefixProvider mqttTopicPrefixProvider;
    @Mock
    protected JoynrStatusMetricsReceiver mockJoynrStatusMetrics;
    @Mock
    protected MqttMessageInProgressObserver mqttMessageInProgressObserver;
    protected SharedSubscriptionsMqttMessagingSkeleton subject;

    protected boolean separateReplyConnection;

    @Before
    public void setup() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());
    }

    protected void initMocks(boolean separateReplyConnection) {
        this.separateReplyConnection = separateReplyConnection;
        when(mqttClientFactory.createReceiver(ownGbid)).thenReturn(mqttClient);
        when(mqttClientFactory.createSender(ownGbid)).thenReturn(mqttClient);
        if (separateReplyConnection) {
            when(mqttClientFactory.createReplyReceiver(ownGbid)).thenReturn(mqttReplyClient);
        } else {
            when(mqttClientFactory.createReplyReceiver(ownGbid)).thenReturn(mqttClient);
        }
    }

    protected abstract void createSkeleton(String channelId);

    protected abstract void initAndSubscribe();

    private void createAndInitSkeleton(String channelId) {
        createSkeleton(channelId);
        initAndSubscribe();
    }

    @Test
    public void testSubscribesToSharedSubscription() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        initAndSubscribe();
        verify(mqttClient).subscribe(eq("$share/channelId/" + ownTopic + "/#"));
        verify(mqttClient).subscribe(eq(replyToTopic + "/#"));
    }

    @Test
    public void subscribeToSharedTopicSubscribesToSharedTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToSharedTopic();
        verify(mqttClient).subscribe(eq("$share/channelId/" + ownTopic + "/#"));
    }

    @Test
    public void subscribeToSharedTopicDoesNotSubscribeToReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToSharedTopic();
        verify(mqttClient, times(0)).subscribe(startsWith(replyToTopic));
    }

    @Test
    public void subscribeToReplyToTopicSubscribesToReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToReplyTopic();
        verify(mqttClient).subscribe(eq(replyToTopic + "/#"));
    }

    @Test
    public void subscribeToReplyToTopicSubscribesToReplyToTopic_separateReplyClient() {
        initMocks(true);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToReplyTopic();
        verify(mqttClient, times(0)).subscribe(any(String.class));
        verify(mqttReplyClient).subscribe(replyToTopic + "/#");
    }

    @Test
    public void subscribeToReplyToTopicDoesNotSubscribeToSharedTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribeToReplyTopic();
        verify(mqttClient, times(0)).subscribe(startsWith("$share/"));
    }

    @Test
    public void testChannelIdStrippedOfNonAlphaChars() {
        initMocks(false);
        createAndInitSkeleton("channel@123_bling$$");
        verify(mqttClient).subscribe(startsWith("$share/channelbling/"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalChannelId() {
        initMocks(false);
        createAndInitSkeleton("@123_$$-!");
    }

}
