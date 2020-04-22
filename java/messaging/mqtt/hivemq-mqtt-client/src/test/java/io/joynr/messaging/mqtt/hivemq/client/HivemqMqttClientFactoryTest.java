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

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.HashMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.statusmetrics.ConnectionStatusMetrics;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;

public class HivemqMqttClientFactoryTest {

    private HivemqMqttClientFactory factory;
    private final String[] gbids = new String[]{ "testGbid1", "testGbid2" };
    private final String[] brokerUris = new String[]{ "tcp://mqttbroker1.com:1883", "tcp://mqttbroker2.com:8883" };
    private HashMap<String, String> defaultMqttGbidToBrokerUriMap;
    private HashMap<String, Integer> defaultMqttGbidToKeepAliveTimerSecMap;
    private HashMap<String, Integer> defaultMqttGbidToConnectionTimeoutSecMap;
    private final boolean defaultCleanSession = false;
    private final ScheduledExecutorService scheduledExecutorService = new ScheduledThreadPoolExecutor(42);
    private final String defaultClientId = "HivemqMqttClientFactoryTest-" + System.currentTimeMillis();
    @Mock
    private MqttClientIdProvider mockClientIdProvider;
    @Mock
    private JoynrStatusMetricsReceiver mockStatusReceiver;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        defaultMqttGbidToBrokerUriMap = new HashMap<>();
        defaultMqttGbidToBrokerUriMap.put(gbids[0], brokerUris[0]);
        defaultMqttGbidToBrokerUriMap.put(gbids[1], brokerUris[1]);
        defaultMqttGbidToKeepAliveTimerSecMap = new HashMap<>();
        defaultMqttGbidToKeepAliveTimerSecMap.put(gbids[0], 31);
        defaultMqttGbidToKeepAliveTimerSecMap.put(gbids[1], 32);
        defaultMqttGbidToConnectionTimeoutSecMap = new HashMap<>();
        defaultMqttGbidToConnectionTimeoutSecMap.put(gbids[0], 41);
        defaultMqttGbidToConnectionTimeoutSecMap.put(gbids[1], 42);
        doReturn(defaultClientId).when(mockClientIdProvider).getClientId();
    }

    private void createDefaultFactory(boolean separateConnections) {
        factory = new HivemqMqttClientFactory(separateConnections,
                                              defaultMqttGbidToBrokerUriMap,
                                              defaultMqttGbidToKeepAliveTimerSecMap,
                                              defaultMqttGbidToConnectionTimeoutSecMap,
                                              defaultCleanSession,
                                              scheduledExecutorService,
                                              mockClientIdProvider,
                                              mockStatusReceiver);
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

}
