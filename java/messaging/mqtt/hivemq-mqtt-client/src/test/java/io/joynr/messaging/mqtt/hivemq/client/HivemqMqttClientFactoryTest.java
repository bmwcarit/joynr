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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.statusmetrics.ConnectionStatusMetrics;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;

public class HivemqMqttClientFactoryTest {

    private HivemqMqttClientFactory factory;
    private final String[] gbids = new String[]{ "testGbid1", "testGbid2", "testGbid3" };
    private final String[] brokerUris = new String[]{ "tcp://mqttbroker1.com:1883", "tcp://mqttbroker2.com:8883",
            "tcp://mqttbroker3.com:8883" };
    private HashMap<String, String> defaultMqttGbidToBrokerUriMap;
    private HashMap<String, Integer> defaultMqttGbidToKeepAliveTimerSecMap;
    private HashMap<String, Integer> defaultMqttGbidToConnectionTimeoutSecMap;
    private final int defaultMaxMessageSize = 0;
    private final boolean defaultCleanSession = false;
    private final ScheduledExecutorService scheduledExecutorService = new ScheduledThreadPoolExecutor(42);
    private final String defaultClientId = "HivemqMqttClientFactoryTest-" + System.currentTimeMillis();
    @Mock
    private MqttClientIdProvider mockClientIdProvider;
    @Mock
    private JoynrStatusMetricsReceiver mockStatusReceiver;
    @Mock
    private ShutdownNotifier mockShutdownNotifier;

    List<HivemqMqttClient> receivers = new ArrayList<>();
    List<HivemqMqttClient> senders = new ArrayList<>();

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        defaultMqttGbidToBrokerUriMap = new HashMap<>();
        defaultMqttGbidToBrokerUriMap.put(gbids[0], brokerUris[0]);
        defaultMqttGbidToBrokerUriMap.put(gbids[1], brokerUris[1]);
        defaultMqttGbidToBrokerUriMap.put(gbids[2], brokerUris[2]);
        defaultMqttGbidToKeepAliveTimerSecMap = new HashMap<>();
        defaultMqttGbidToKeepAliveTimerSecMap.put(gbids[0], 31);
        defaultMqttGbidToKeepAliveTimerSecMap.put(gbids[1], 32);
        defaultMqttGbidToKeepAliveTimerSecMap.put(gbids[2], 33);
        defaultMqttGbidToConnectionTimeoutSecMap = new HashMap<>();
        defaultMqttGbidToConnectionTimeoutSecMap.put(gbids[0], 41);
        defaultMqttGbidToConnectionTimeoutSecMap.put(gbids[1], 42);
        defaultMqttGbidToConnectionTimeoutSecMap.put(gbids[2], 43);
        doReturn(defaultClientId).when(mockClientIdProvider).getClientId();
    }

    private void createDefaultFactory(boolean separateConnections) {
        factory = new HivemqMqttClientFactory(separateConnections,
                                              defaultMqttGbidToBrokerUriMap,
                                              defaultMqttGbidToKeepAliveTimerSecMap,
                                              defaultMqttGbidToConnectionTimeoutSecMap,
                                              defaultMaxMessageSize,
                                              defaultCleanSession,
                                              scheduledExecutorService,
                                              mockClientIdProvider,
                                              mockStatusReceiver,
                                              mockShutdownNotifier);
    }

    private void createSendersAndReceivers() {
        for (int i = 0; i < gbids.length; i++) {
            receivers.add((HivemqMqttClient) factory.createReceiver(gbids[i]));
            senders.add((HivemqMqttClient) (HivemqMqttClient) factory.createSender(gbids[i]));
            for (int j = 0; j < i; j++) {
                assertNotEquals(receivers.get(i), receivers.get(j));
                assertNotEquals(senders.get(i), senders.get(j));
            }
        }
    }

    private void assertSendersAndReceiversAreDifferent() {
        for (int i = 0; i < receivers.size(); i++) {
            for (int j = 0; j < receivers.size(); j++) {
                assertNotEquals(receivers.get(i), senders.get(j));
            }
        }
    }

    private void assertSendersAndReceiversAreTheSame() {
        for (int i = 0; i < receivers.size(); i++) {
            assertEquals(receivers.get(i), senders.get(i));
        }
    }

    @Test
    public void createClientAddsConnectionStatusMetrics_noSeparateConnections() {
        createDefaultFactory(false);
        factory.createReceiver(gbids[0]);
        factory.createSender(gbids[0]);
        verify(mockStatusReceiver, times(1)).addConnectionStatusMetrics(any(ConnectionStatusMetrics.class));
    }

    @Test
    public void createClientAddsConnectionStatusMetrics_separateConnections() {
        createDefaultFactory(true);
        factory.createReceiver(gbids[0]);
        factory.createSender(gbids[0]);
        verify(mockStatusReceiver, times(2)).addConnectionStatusMetrics(any(ConnectionStatusMetrics.class));
    }

    @Test
    public void createdClientsHaveCorrectMaxMessageSize() throws Exception {
        final int maxMessageSize = 100;
        factory = new HivemqMqttClientFactory(true,
                                              defaultMqttGbidToBrokerUriMap,
                                              defaultMqttGbidToKeepAliveTimerSecMap,
                                              defaultMqttGbidToConnectionTimeoutSecMap,
                                              maxMessageSize,
                                              defaultCleanSession,
                                              scheduledExecutorService,
                                              mockClientIdProvider,
                                              mockStatusReceiver,
                                              mockShutdownNotifier);

        JoynrMqttClient receiver = factory.createReceiver(gbids[0]);
        assertTrue(receiver instanceof HivemqMqttClient);
        if (receiver instanceof HivemqMqttClient) {
            HivemqMqttClient client = (HivemqMqttClient) receiver;
            Field maxMessageSizeBytesField = client.getClass().getDeclaredField("maxMsgSizeBytes");
            maxMessageSizeBytesField.setAccessible(true);
            assertEquals(maxMessageSize, (int) (maxMessageSizeBytesField.get(client)));
        }

        JoynrMqttClient sender = factory.createSender(gbids[0]);
        assertTrue(sender instanceof HivemqMqttClient);
        if (sender instanceof HivemqMqttClient) {
            HivemqMqttClient client = (HivemqMqttClient) sender;
            Field maxMessageSizeBytesField = client.getClass().getDeclaredField("maxMsgSizeBytes");
            maxMessageSizeBytesField.setAccessible(true);
            assertEquals(maxMessageSize, (int) (maxMessageSizeBytesField.get(client)));
        }
    }

    @Test
    public void testRegisterForShutdown() {
        createDefaultFactory(false);
        verify(mockShutdownNotifier).registerForShutdown(factory);
    }

    @Test
    public void testShutdown_singleConnection() {
        createDefaultFactory(false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();

        for (int i = 0; i < receivers.size(); i++) {
            assertFalse(receivers.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < receivers.size(); i++) {
            assertTrue(receivers.get(i).isShutdown());
        }
    }

    @Test
    public void testShutdown_separateConnections() {
        createDefaultFactory(true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();

        for (int i = 0; i < receivers.size(); i++) {
            assertFalse(receivers.get(i).isShutdown());
            assertFalse(senders.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < receivers.size(); i++) {
            assertTrue(receivers.get(i).isShutdown());
            assertTrue(senders.get(i).isShutdown());
        }
    }

}
