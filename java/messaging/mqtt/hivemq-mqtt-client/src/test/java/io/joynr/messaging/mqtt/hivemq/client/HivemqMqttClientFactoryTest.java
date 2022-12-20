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
import static org.junit.Assert.assertNotNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.statusmetrics.ConnectionStatusMetrics;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.statusmetrics.ConnectionStatusMetricsImpl;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory.ResubscribeHandler;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory.DisconnectedListener;

import com.hivemq.client.mqtt.exceptions.ConnectionFailedException;
import com.hivemq.client.mqtt.lifecycle.MqttClientConnectedContext;
import com.hivemq.client.mqtt.lifecycle.MqttClientDisconnectedContext;
import com.hivemq.client.mqtt.mqtt5.exceptions.Mqtt5ConnAckException;
import com.hivemq.client.mqtt.mqtt5.exceptions.Mqtt5DisconnectException;

public class HivemqMqttClientFactoryTest {

    private HivemqMqttClientFactory factory;
    private final String[] gbids = new String[]{ "testGbid1", "testGbid2", "testGbid3" };
    private final String[] brokerUris = new String[]{ "tcp://mqttbroker1.com:1883", "tcp://mqttbroker2.com:8883",
            "tcp://mqttbroker3.com:8883" };
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
    @Mock
    private ShutdownNotifier mockShutdownNotifier;
    @Mock
    private IHivemqMqttClientTrustManagerFactory trustManagerFactory;
    @Mock
    private MqttClientDisconnectedContext mockMqttClientDisconnectedContext;
    @Mock
    private MqttClientConnectedContext mockMqttClientConnectedcontext;

    List<HivemqMqttClient> receivers = new ArrayList<>();
    List<HivemqMqttClient> replyReceivers = new ArrayList<>();
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

    private void createDefaultFactory(boolean separateConnections, boolean separateReplyReceiver) {
        factory = new HivemqMqttClientFactory(separateConnections,
                                              defaultMqttGbidToBrokerUriMap,
                                              defaultMqttGbidToKeepAliveTimerSecMap,
                                              defaultMqttGbidToConnectionTimeoutSecMap,
                                              defaultCleanSession,
                                              scheduledExecutorService,
                                              separateReplyReceiver,
                                              mockClientIdProvider,
                                              mockStatusReceiver,
                                              mockShutdownNotifier,
                                              trustManagerFactory);
    }

    private void createSendersAndReceivers() {
        for (int i = 0; i < gbids.length; i++) {
            receivers.add((HivemqMqttClient) factory.createReceiver(gbids[i]));
            replyReceivers.add((HivemqMqttClient) factory.createReplyReceiver(gbids[i]));
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

    private void assertReceiversAndReplyReceiversAreDifferent() {
        for (int i = 0; i < receivers.size(); i++) {
            for (int j = 0; j < receivers.size(); j++) {
                assertNotEquals(receivers.get(i), replyReceivers.get(j));
            }
        }
    }

    private void testcreateClientAddsConnectionStatusMetrics(boolean separateConnections,
                                                             boolean separateReplyReceiver,
                                                             int timesMetricsAdded) {
        createDefaultFactory(separateConnections, separateReplyReceiver);
        factory.createReceiver(gbids[0]);
        factory.createReplyReceiver(gbids[0]);
        factory.createSender(gbids[0]);
        verify(mockStatusReceiver,
               times(timesMetricsAdded)).addConnectionStatusMetrics(any(ConnectionStatusMetrics.class));
    }

    @Test
    public void createClientAddsConnectionStatusMetrics_noSeparateConnections() {
        testcreateClientAddsConnectionStatusMetrics(false, false, 1);
    }

    @Test
    public void createClientAddsConnectionStatusMetrics_noSeparateConnections_separateReplyReceiver() {
        testcreateClientAddsConnectionStatusMetrics(false, true, 2);
    }

    @Test
    public void createClientAddsConnectionStatusMetrics_separateConnections() {
        testcreateClientAddsConnectionStatusMetrics(true, false, 2);
    }

    @Test
    public void createClientAddsConnectionStatusMetrics_separateConnections_separateReplyReceiver() {
        testcreateClientAddsConnectionStatusMetrics(true, true, 3);
    }

    @Test
    public void testRegisterForShutdown() {
        createDefaultFactory(false, true);
        verify(mockShutdownNotifier).registerForShutdown(factory);
    }

    @Test
    public void testReceiversAndReplyReceiversAreTheSameWhenConfigured_withShutdown() {
        //Shutdown is added in this test to cover the branch where reply receivers should not be shut down a second time
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        for (int i = 0; i < receivers.size(); i++) {
            assertEquals(receivers.get(i), replyReceivers.get(i));
        }

        for (int i = 0; i < receivers.size(); i++) {
            assertFalse(receivers.get(i).isShutdown());
            assertFalse(replyReceivers.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < receivers.size(); i++) {
            assertTrue(receivers.get(i).isShutdown());
            assertTrue(replyReceivers.get(i).isShutdown());
        }
    }

    @Test
    public void testShutdown_singleConnection() {
        createDefaultFactory(false, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < receivers.size(); i++) {
            assertFalse(receivers.get(i).isShutdown());
            assertFalse(replyReceivers.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < receivers.size(); i++) {
            assertTrue(receivers.get(i).isShutdown());
            assertTrue(replyReceivers.get(i).isShutdown());
        }
    }

    @Test
    public void testShutdown_separateConnections() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < receivers.size(); i++) {
            assertFalse(receivers.get(i).isShutdown());
            assertFalse(senders.get(i).isShutdown());
            assertFalse(replyReceivers.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < receivers.size(); i++) {
            assertTrue(receivers.get(i).isShutdown());
            assertTrue(senders.get(i).isShutdown());
            assertTrue(replyReceivers.get(i).isShutdown());
        }
    }

    @Test
    public void initializeDisconnectListener() {
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        DisconnectedListener disconnectedListener = new DisconnectedListener(connectionStatusMetrics);
        assertNotNull(disconnectedListener);
    }

    @Test
    public void testOnDisconnectWithDisconnect() {
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        DisconnectedListener disconnectedListener = new DisconnectedListener(connectionStatusMetrics);
        Mqtt5DisconnectException disconnectException = mock(Mqtt5DisconnectException.class);
        when(mockMqttClientDisconnectedContext.getCause()).thenReturn(disconnectException);

        disconnectedListener.onDisconnected(mockMqttClientDisconnectedContext);

        assertFalse(connectionStatusMetrics.isConnected());
        assertEquals(1, connectionStatusMetrics.getConnectionDrops());
    }

    @Test
    public void testOnDisconnectWithConnAck() {
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        DisconnectedListener disconnectedListener = new DisconnectedListener(connectionStatusMetrics);
        Mqtt5ConnAckException connAckException = mock(Mqtt5ConnAckException.class);
        when(mockMqttClientDisconnectedContext.getCause()).thenReturn(connAckException);

        disconnectedListener.onDisconnected(mockMqttClientDisconnectedContext);

        assertFalse(connectionStatusMetrics.isConnected());
        assertEquals(1, connectionStatusMetrics.getConnectionDrops());
    }

    @Test
    public void testOnDisconnectWithNull() {
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        DisconnectedListener disconnectedListener = new DisconnectedListener(connectionStatusMetrics);
        when(mockMqttClientDisconnectedContext.getCause()).thenReturn(null);

        disconnectedListener.onDisconnected(mockMqttClientDisconnectedContext);

        assertFalse(connectionStatusMetrics.isConnected());
        assertEquals(1, connectionStatusMetrics.getConnectionDrops());
    }

    @Test
    public void testOnDisconnectWithConnectionFailed() {
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        DisconnectedListener disconnectedListener = new DisconnectedListener(connectionStatusMetrics);
        ConnectionFailedException connectionFailedException = mock(ConnectionFailedException.class);
        when(mockMqttClientDisconnectedContext.getCause()).thenReturn(connectionFailedException);

        disconnectedListener.onDisconnected(mockMqttClientDisconnectedContext);

        assertFalse(connectionStatusMetrics.isConnected());
        assertEquals(1, connectionStatusMetrics.getConnectionDrops());
    }

    @Test
    public void initializeResubscribeHandler() {
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        ResubscribeHandler resubscribeHandler = new ResubscribeHandler(connectionStatusMetrics);
        assertNotNull(resubscribeHandler);
    }

    @Test
    public void testResubscribeHandlerSetClient() {
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        ResubscribeHandler resubscribeHandler = new ResubscribeHandler(connectionStatusMetrics);
        HivemqMqttClient client = mock(HivemqMqttClient.class);
        resubscribeHandler.setClient(client);

        resubscribeHandler.onConnected(mockMqttClientConnectedcontext);

        verify(client).resubscribe();
        assertTrue(connectionStatusMetrics.isConnected());
    }
}
