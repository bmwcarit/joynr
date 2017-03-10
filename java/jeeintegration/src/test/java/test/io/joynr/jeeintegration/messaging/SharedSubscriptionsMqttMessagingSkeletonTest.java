package test.io.joynr.jeeintegration.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.HashSet;

import org.junit.Before;
import org.junit.Test;

import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.jeeintegration.messaging.SharedSubscriptionsMqttMessagingSkeleton;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageSerializerFactory;
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
    private MessageRouter messageRouter;

    @Mock
    private MqttMessageSerializerFactory mqttMessageSerializerFactory;

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
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                               messageRouter,
                                                               mqttClientFactory,
                                                               mqttMessageSerializerFactory,
                                                               "channelId",
                                                               "receiverId",
                                                               new NoOpRawMessagingPreprocessor(),
                                                               new HashSet<JoynrMessageProcessor>());
        subject.init();
        verify(mqttClient).subscribe(eq("$share:channelId:ownTopic/#"));
        verify(mqttClient).subscribe(eq("replyto/ownTopic/receiverId/#"));
    }

    @Test
    public void testChannelIdStrippedOfNonAlphaChars() {
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                               messageRouter,
                                                               mqttClientFactory,
                                                               mqttMessageSerializerFactory,
                                                               "channel@123_bling$$",
                                                               "receiverId",
                                                               new NoOpRawMessagingPreprocessor(),
                                                               new HashSet<JoynrMessageProcessor>());
        subject.init();
        verify(mqttClient).subscribe(startsWith("$share:channelbling:"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalChannelId() {
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownAddress,
                                                               messageRouter,
                                                               mqttClientFactory,
                                                               mqttMessageSerializerFactory,
                                                               "@123_$$-!",
                                                               "receiverId",
                                                               new NoOpRawMessagingPreprocessor(),
                                                               new HashSet<JoynrMessageProcessor>());
        subject.init();
    }

}
