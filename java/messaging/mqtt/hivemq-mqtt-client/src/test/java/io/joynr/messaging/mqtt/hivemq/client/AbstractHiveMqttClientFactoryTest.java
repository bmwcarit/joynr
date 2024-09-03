/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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

import com.hivemq.client.mqtt.mqtt5.Mqtt5ClientConfig;
import com.hivemq.client.mqtt.mqtt5.Mqtt5RxClient;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttMessagingSkeleton;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.statusmetrics.ConnectionStatusMetricsImpl;
import org.junit.Before;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory.RECEIVER_PREFIX;
import static io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory.REPLY_RECEIVER_PREFIX;
import static io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory.SENDER_PREFIX;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;

public abstract class AbstractHiveMqttClientFactoryTest {
    protected HivemqMqttClientFactory factory;
    protected final String[] gbids = new String[]{ "testGbid1", "testGbid2", "testGbid3" };
    protected final String[] GBIDS_FOR_SHUTDOWN = new String[]{ "testGbid4", "testGbid5" };
    protected final String defaultClientId = "HivemqMqttClientFactoryTest-" + System.currentTimeMillis();
    @Mock
    protected MqttClientIdProvider mockClientIdProvider;
    @Mock
    protected ShutdownNotifier mockShutdownNotifier;
    @Mock
    protected Mqtt5RxClient mqtt5RxClient;
    @Mock
    protected Mqtt5ClientConfig mockClientConfig;
    @Mock
    protected ConnectionStatusMetricsImpl connectionStatusMetricsImpl;
    @Mock
    protected HivemqMqttClientCreator mockHivemqMqttClientCreator;
    @Mock
    protected MqttMessagingSkeleton mockMqttMessagingSkeleton1;
    @Mock
    protected MqttMessagingSkeleton mockMqttMessagingSkeleton2;
    @Mock
    protected MqttMessagingSkeleton mockMqttMessagingSkeleton3;
    protected final List<JoynrMqttClient> receivers = new ArrayList<>();
    protected final List<JoynrMqttClient> replyReceivers = new ArrayList<>();
    protected final List<JoynrMqttClient> senders = new ArrayList<>();

    @Before
    public void setUp() {
        MockitoAnnotations.openMocks(this);
        doReturn(defaultClientId).when(mockClientIdProvider).getClientId();
        doReturn(mockClientConfig).when(mqtt5RxClient).getConfig();
    }

    protected void createDefaultFactory(final boolean separateConnections,
                                        final boolean separateReplyReceiver,
                                        final boolean sharedSubscriptions,
                                        final boolean canConnect) {
        factory = new HivemqMqttClientFactory(separateConnections,
                                              mockClientIdProvider,
                                              mockShutdownNotifier,
                                              mockHivemqMqttClientCreator,
                                              canConnect,
                                              separateReplyReceiver,
                                              sharedSubscriptions);
    }

    protected void createSendersAndReceivers() {
        for (int i = 0; i < gbids.length; i++) {

            manageMockHivemqMqttClientCreator(i);

            HivemqMqttClient client1 = (HivemqMqttClient) factory.createReceiver(gbids[i]);
            client1.setMessageListener(mockMqttMessagingSkeleton1);
            HivemqMqttClient client2 = (HivemqMqttClient) factory.createSender(gbids[i]);
            client2.setMessageListener(mockMqttMessagingSkeleton2);
            HivemqMqttClient client3 = (HivemqMqttClient) factory.createReplyReceiver(gbids[i]);
            client3.setMessageListener(mockMqttMessagingSkeleton3);

            doNothing().when(client1).registerPublishCallback();
            doNothing().when(client2).registerPublishCallback();
            doNothing().when(client3).registerPublishCallback();

            receivers.add(client1);
            senders.add(client2);
            replyReceivers.add(client3);

            for (int j = 0; j < i; j++) {
                assertNotEquals(receivers.get(i), receivers.get(j));
                assertNotEquals(senders.get(i), senders.get(j));
                assertNotEquals(replyReceivers.get(i), replyReceivers.get(j));
            }
        }
    }

    protected void manageMockHivemqMqttClientCreator(final int index) {
        manageMockHivemqMqttClientCreator(gbids[index]);
    }

    private JoynrMqttClient createClient(final String gbid, final boolean isReceiver, final boolean isSender) {
        return new HivemqMqttClient(mqtt5RxClient,
                                    0,
                                    false,
                                    0,
                                    0,
                                    0,
                                    isReceiver,
                                    isSender,
                                    false,
                                    gbid,
                                    connectionStatusMetricsImpl);
    }

    private void mockCreateClient(final String gbid,
                                  final String clientId,
                                  final boolean isReceiver,
                                  final boolean isSender,
                                  final boolean isReplyReceiver) {
        final JoynrMqttClient client = createClient(gbid, isReceiver, isSender);
        doReturn(spy(client)).when(mockHivemqMqttClientCreator)
                             .createClient(gbid, clientId, isReceiver, isSender, isReplyReceiver);
    }

    protected void manageMockHivemqMqttClientCreator(final String gbid) {
        mockCreateClient(gbid, defaultClientId, true, true, true);
        mockCreateClient(gbid, defaultClientId, true, true, false);
        mockCreateClient(gbid, defaultClientId + RECEIVER_PREFIX, true, false, false);
        mockCreateClient(gbid, defaultClientId + SENDER_PREFIX, false, true, false);
        mockCreateClient(gbid, defaultClientId + RECEIVER_PREFIX, true, false, true);
        mockCreateClient(gbid, defaultClientId + REPLY_RECEIVER_PREFIX, true, false, true);
        mockCreateClient(gbid, defaultClientId + REPLY_RECEIVER_PREFIX, true, true, true);
    }

    protected void assertSendersAndReceiversAreTheSame() {
        assertListsAreTheSame(receivers, senders);
    }

    protected void assertReceiversAndReplyReceiversAreTheSame() {
        assertListsAreTheSame(receivers, replyReceivers);
    }

    protected void assertListsAreTheSame(final List<JoynrMqttClient> left, final List<JoynrMqttClient> right) {
        assertEquals(left, right);
    }

    protected void assertSendersAndReceiversAreDifferent() {
        assertListsAreDifferent(receivers, senders);
    }

    protected void assertReceiversAndReplyReceiversAreDifferent() {
        assertListsAreDifferent(receivers, replyReceivers);
    }

    protected void assertListsAreDifferent(final List<JoynrMqttClient> left, final List<JoynrMqttClient> right) {
        for (int i = 0; i < left.size(); i++) {
            for (int j = 0; j < left.size(); j++) {
                assertNotEquals(left.get(i), right.get(j));
            }
        }
    }

    protected void shutdownClients(final JoynrMqttClient... clients) {
        Arrays.stream(clients).distinct().forEach(this::shutdownClient);
    }

    protected void shutdownClient(final JoynrMqttClient client) {
        client.shutdown();
        assertTrue(client.isShutdown());
    }

    protected void startClients(final JoynrMqttClient... clients) {
        Arrays.stream(clients).forEach(this::startClient);
    }

    protected void startClient(final JoynrMqttClient client) {
        client.start();
        assertFalse(client.isShutdown());
        doNothing().when((HivemqMqttClient) client).connect();
    }
}
