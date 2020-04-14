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
package io.joynr.messaging.mqtt.paho.client;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;

import static org.junit.Assert.*;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.verify;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.JoynrThreadFactory;

public class MqttPahoClientFactoryTest {

    private MqttPahoClientFactory subject;

    private final int reconnectSleepMs = 100000; // no reconnect
    private final int timeToWaitMs = 135;
    private final int maxMsgsInflight = 136;
    private final int maxMsgSizeBytes = 137;
    private final boolean cleanSession = false;
    private final String mqttClientId = "testMqttClientId";

    private final String[] gbids = new String[]{ "testGbid1", "testGbid2", "testGbid3" };
    private HashMap<String, String> mqttGbidToBrokerUriMap;
    private HashMap<String, Integer> mqttGbidToKeepAliveTimerSecMap;
    private HashMap<String, Integer> mqttGbidToConnectionTimeoutSecMap;

    private ScheduledExecutorService executorService;

    @Mock
    private MqttClientIdProvider mqttClientIdProvider;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    List<MqttPahoClient> receivers = new ArrayList<>();
    List<MqttPahoClient> senders = new ArrayList<>();

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mqttGbidToBrokerUriMap = new HashMap<>();
        for (String gbid : gbids) {
            mqttGbidToBrokerUriMap.put(gbid, "ws://" + gbid + ".URI");
        }

        mqttGbidToKeepAliveTimerSecMap = new HashMap<>();
        for (int i = 0; i < gbids.length; i++) {
            mqttGbidToKeepAliveTimerSecMap.put(gbids[i], i);
        }

        mqttGbidToConnectionTimeoutSecMap = new HashMap<>();
        for (int i = 0; i < gbids.length; i++) {
            mqttGbidToConnectionTimeoutSecMap.put(gbids[i], i + 10);
        }

        String name = "TEST.joynr.scheduler.messaging.mqtt.paho.client";
        ThreadFactory joynrThreadFactory = new JoynrThreadFactory(name, true);
        executorService = Executors.newScheduledThreadPool(4, joynrThreadFactory);

        when(mqttClientIdProvider.getClientId()).thenReturn(mqttClientId);
    }

    @After
    public void tearDown() {
        if (subject != null) {
            subject.shutdown();
        }
    }

    private void createFactory(boolean separateConnections) {
        subject = new MqttPahoClientFactory(reconnectSleepMs,
                                            mqttGbidToBrokerUriMap,
                                            mqttGbidToKeepAliveTimerSecMap,
                                            mqttGbidToConnectionTimeoutSecMap,
                                            timeToWaitMs,
                                            maxMsgsInflight,
                                            maxMsgSizeBytes,
                                            cleanSession,
                                            separateConnections,
                                            executorService,
                                            mqttClientIdProvider,
                                            shutdownNotifier);
    }

    private void createSendersAndReceivers() {
        receivers.add((MqttPahoClient) subject.createReceiver(gbids[0]));
        receivers.add((MqttPahoClient) subject.createReceiver(gbids[1]));
        receivers.add((MqttPahoClient) subject.createReceiver(gbids[2]));

        assertNotEquals(receivers.get(0), receivers.get(1));
        assertNotEquals(receivers.get(0), receivers.get(2));
        assertNotEquals(receivers.get(1), receivers.get(2));

        senders.add((MqttPahoClient) (MqttPahoClient) subject.createSender(gbids[0]));
        senders.add((MqttPahoClient) (MqttPahoClient) subject.createSender(gbids[1]));
        senders.add((MqttPahoClient) (MqttPahoClient) subject.createSender(gbids[2]));

        assertNotEquals(senders.get(0), senders.get(1));
        assertNotEquals(senders.get(0), senders.get(2));
        assertNotEquals(senders.get(1), senders.get(2));
    }

    private void assertSendersAndReceiversAreDifferent() {
        assertNotEquals(receivers.get(0), senders.get(0));
        assertNotEquals(receivers.get(0), senders.get(1));
        assertNotEquals(receivers.get(0), senders.get(2));

        assertNotEquals(receivers.get(1), senders.get(0));
        assertNotEquals(receivers.get(1), senders.get(1));
        assertNotEquals(receivers.get(1), senders.get(2));

        assertNotEquals(receivers.get(2), senders.get(0));
        assertNotEquals(receivers.get(2), senders.get(1));
        assertNotEquals(receivers.get(2), senders.get(2));
    }

    private void assertSendersAndReceiversAreTheSame() {
        assertEquals(receivers.get(0), senders.get(0));
        assertEquals(receivers.get(1), senders.get(1));
        assertEquals(receivers.get(2), senders.get(2));
    }

    @Test
    public void testRegisterForShutdown() {
        createFactory(false);
        verify(shutdownNotifier).registerForShutdown(subject);
    }

    @Test
    public void testCreate_singleConnection() {
        createFactory(false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();

        assertEquals(mqttClientId, receivers.get(0).getMqttClient().getClientId());
        assertEquals(mqttClientId, receivers.get(1).getMqttClient().getClientId());
        assertEquals(mqttClientId, receivers.get(2).getMqttClient().getClientId());

        assertEquals(mqttGbidToBrokerUriMap.get(gbids[0]), receivers.get(0).getMqttClient().getServerURI());
        assertEquals(mqttGbidToBrokerUriMap.get(gbids[1]), receivers.get(1).getMqttClient().getServerURI());
        assertEquals(mqttGbidToBrokerUriMap.get(gbids[2]), receivers.get(2).getMqttClient().getServerURI());
    }

    @Test
    public void testCreate_separateConnections() {
        createFactory(true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();

        assertEquals(mqttClientId + "Sub", receivers.get(0).getMqttClient().getClientId());
        assertEquals(mqttClientId + "Sub", receivers.get(1).getMqttClient().getClientId());
        assertEquals(mqttClientId + "Sub", receivers.get(2).getMqttClient().getClientId());

        assertEquals(mqttClientId + "Pub", senders.get(0).getMqttClient().getClientId());
        assertEquals(mqttClientId + "Pub", senders.get(1).getMqttClient().getClientId());
        assertEquals(mqttClientId + "Pub", senders.get(2).getMqttClient().getClientId());

        assertEquals(mqttGbidToBrokerUriMap.get(gbids[0]), receivers.get(0).getMqttClient().getServerURI());
        assertEquals(mqttGbidToBrokerUriMap.get(gbids[1]), receivers.get(1).getMqttClient().getServerURI());
        assertEquals(mqttGbidToBrokerUriMap.get(gbids[2]), receivers.get(2).getMqttClient().getServerURI());

        assertEquals(mqttGbidToBrokerUriMap.get(gbids[0]), senders.get(0).getMqttClient().getServerURI());
        assertEquals(mqttGbidToBrokerUriMap.get(gbids[1]), senders.get(1).getMqttClient().getServerURI());
        assertEquals(mqttGbidToBrokerUriMap.get(gbids[2]), senders.get(2).getMqttClient().getServerURI());
    }

    @Test
    public void testShutdown_singleConnection() {
        createFactory(false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();

        assertFalse(receivers.get(0).isShutdown());
        assertFalse(receivers.get(1).isShutdown());
        assertFalse(receivers.get(2).isShutdown());

        subject.shutdown();

        assertTrue(receivers.get(0).isShutdown());
        assertTrue(receivers.get(1).isShutdown());
        assertTrue(receivers.get(2).isShutdown());
    }

    @Test
    public void testShutdown_separateConnections() {
        createFactory(true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();

        assertFalse(receivers.get(0).isShutdown());
        assertFalse(receivers.get(1).isShutdown());
        assertFalse(receivers.get(2).isShutdown());
        assertFalse(senders.get(0).isShutdown());
        assertFalse(senders.get(1).isShutdown());
        assertFalse(senders.get(2).isShutdown());

        subject.shutdown();

        assertTrue(receivers.get(0).isShutdown());
        assertTrue(receivers.get(1).isShutdown());
        assertTrue(receivers.get(2).isShutdown());
        assertTrue(senders.get(0).isShutdown());
        assertTrue(senders.get(1).isShutdown());
        assertTrue(senders.get(2).isShutdown());
    }

}
