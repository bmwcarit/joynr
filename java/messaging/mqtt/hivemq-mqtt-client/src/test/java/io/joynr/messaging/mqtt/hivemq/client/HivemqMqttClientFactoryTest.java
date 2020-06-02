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
package io.joynr.messaging.mqtt.hivemq.client;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;

import joynr.system.RoutingTypes.MqttAddress;

public class HivemqMqttClientFactoryTest {

    private HivemqMqttClientFactory factory;
    private final String brokerUri = "tcp://mqttbroker.com:1883";
    private final MqttAddress ownAddress = new MqttAddress(brokerUri, "sometopic");
    private int defaultKeepAliveTimerSec = 30;
    private final int defaultConnectionTimeoutSec = 40;
    private final int defaultMaxMessageSize = 0;
    private final boolean defaultCleanSession = false;
    private final ScheduledExecutorService scheduledExecutorService = new ScheduledThreadPoolExecutor(42);
    private final String defaultClientId = "HivemqMqttClientFactoryTest-" + System.currentTimeMillis();
    @Mock
    private MqttClientIdProvider mockClientIdProvider;
    @Mock
    private MqttStatusReceiver mockStatusReceiver;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        doReturn(defaultClientId).when(mockClientIdProvider).getClientId();
    }

    @Test
    public void createdClientsHaveCorrectMaxMessageSize() throws Exception {
        final int maxMessageSize = 100;
        factory = new HivemqMqttClientFactory(ownAddress,
                                              true,
                                              defaultKeepAliveTimerSec,
                                              maxMessageSize,
                                              defaultCleanSession,
                                              scheduledExecutorService,
                                              defaultConnectionTimeoutSec,
                                              mockClientIdProvider,
                                              mockStatusReceiver);

        JoynrMqttClient receiver = factory.createReceiver();
        assertTrue(receiver instanceof HivemqMqttClient);
        if (receiver instanceof HivemqMqttClient) {
            HivemqMqttClient client = (HivemqMqttClient) receiver;
            Field maxMessageSizeBytesField = client.getClass().getDeclaredField("maxMsgSizeBytes");
            maxMessageSizeBytesField.setAccessible(true);
            assertEquals(maxMessageSize, (int) (maxMessageSizeBytesField.get(client)));
        }

        JoynrMqttClient sender = factory.createSender();
        assertTrue(sender instanceof HivemqMqttClient);
        if (sender instanceof HivemqMqttClient) {
            HivemqMqttClient client = (HivemqMqttClient) sender;
            Field maxMessageSizeBytesField = client.getClass().getDeclaredField("maxMsgSizeBytes");
            maxMessageSizeBytesField.setAccessible(true);
            assertEquals(maxMessageSize, (int) (maxMessageSizeBytesField.get(client)));
        }
    }

}
