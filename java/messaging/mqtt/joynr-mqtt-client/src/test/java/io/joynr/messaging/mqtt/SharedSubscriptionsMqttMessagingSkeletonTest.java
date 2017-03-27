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

import static org.mockito.Matchers.eq;
import static org.mockito.Matchers.startsWith;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;

import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageSerializerFactory;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.mqtt.SharedSubscriptionsMqttMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Unit tests for {@link SharedSubscriptionsMqttMessagingSkeleton}.
 */
@RunWith(MockitoJUnitRunner.class)
public class SharedSubscriptionsMqttMessagingSkeletonTest {

    @Mock
    private MqttClientFactory mqttClientFactory;

    @Mock
    private JoynrMqttClient mqttClient;

    @Mock
    private MqttAddress ownAddress;

    @Mock
    private MqttAddress replyToAddress;

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private MqttMessageSerializerFactory mqttMessageSerializerFactory;

    @Mock
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;

    private SharedSubscriptionsMqttMessagingSkeleton subject;

    @Captor
    private ArgumentCaptor<String> stringCaptor;

    @Before
    public void setup() {
        when(mqttClientFactory.create()).thenReturn(mqttClient);
    }

    @Test
    public void testSubscribesToSharedSubscription() {
        when(ownAddress.getTopic()).thenReturn("ownTopic");
        final String replyToAddressTopic = "replyToAddressTopic";
        when(replyToAddress.getTopic()).thenReturn(replyToAddressTopic);
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                               replyToAddress,
                                                               messageRouter,
                                                               mqttClientFactory,
                                                               mqttMessageSerializerFactory,
                                                               "channelId",
                                                               mqttTopicPrefixProvider);
        subject.init();
        verify(mqttClient).subscribe(eq("$share:channelId:ownTopic/#"));
        verify(mqttClient).subscribe(eq(replyToAddressTopic + "/#"));
    }

    @Test
    public void testChannelIdStrippedOfNonAlphaChars() {
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                               replyToAddress,
                                                               messageRouter,
                                                               mqttClientFactory,
                                                               mqttMessageSerializerFactory,
                                                               "channel@123_bling$$",
                                                               mqttTopicPrefixProvider);
        subject.init();
        verify(mqttClient).subscribe(startsWith("$share:channelbling:"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalChannelId() {
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                               replyToAddress,
                                                               messageRouter,
                                                               mqttClientFactory,
                                                               mqttMessageSerializerFactory,
                                                               "@123_$$-!",
                                                               mqttTopicPrefixProvider);
        subject.init();
    }

}
