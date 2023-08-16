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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;

import io.joynr.runtime.ShutdownNotifier;
import org.junit.Test;

import io.joynr.messaging.mqtt.JoynrMqttClient;

public class HivemqMqttClientFactoryTest extends AbstractHiveMqttClientFactoryTest {

    @Test
    public void createReceiverAndSender_noSeparateConnections_callsCreateClientTwice() {
        createDefaultFactory(false, true);
        manageMockHivemqMqttClientCreator(0);
        factory.createReceiver(gbids[0]);
        factory.createSender(gbids[0]);
        factory.createReplyReceiver(gbids[0]);
        verify(mockHivemqMqttClientCreator,
               times(2)).createClient(eq(gbids[0]), any(), anyBoolean(), anyBoolean(), anyBoolean());
        // Specifying types of createClient
        verify(mockHivemqMqttClientCreator, times(1)).createClient(eq(gbids[0]), any(), eq(true), eq(true), eq(false));
        verify(mockHivemqMqttClientCreator, times(1)).createClient(eq(gbids[0]), any(), eq(true), eq(false), eq(true));
    }

    @Test
    public void createReceiverAndSender_separateConnections_callsCreateClientThreeTimes() {
        createDefaultFactory(true, true);
        manageMockHivemqMqttClientCreator(0);
        factory.createReceiver(gbids[0]);
        factory.createSender(gbids[0]);
        factory.createReplyReceiver(gbids[0]);
        verify(mockHivemqMqttClientCreator,
               times(3)).createClient(eq(gbids[0]), any(), anyBoolean(), anyBoolean(), anyBoolean());
        // Specifying types of createClient
        verify(mockHivemqMqttClientCreator, times(1)).createClient(eq(gbids[0]), any(), eq(false), eq(true), eq(false));
        verify(mockHivemqMqttClientCreator, times(1)).createClient(eq(gbids[0]), any(), eq(true), eq(false), eq(false));
        verify(mockHivemqMqttClientCreator, times(1)).createClient(eq(gbids[0]), any(), eq(true), eq(false), eq(true));
    }

    @Test
    public void createClients_noSeparateConnections_separateReplyReceivers() {
        createDefaultFactory(false, true);
        manageMockHivemqMqttClientCreator(0);

        JoynrMqttClient receiver = factory.createReceiver(gbids[0]);
        JoynrMqttClient sender = factory.createSender(gbids[0]);
        JoynrMqttClient replyReceiver = factory.createReplyReceiver(gbids[0]);
        assertNotNull(receiver);
        assertNotNull(sender);
        assertNotNull(replyReceiver);
        assertEquals(receiver, sender);
        assertNotEquals(sender, replyReceiver);
        assertNotEquals(receiver, replyReceiver);
    }

    @Test
    public void createClients_noSeparateConnections_noSeparateReplyReceivers() {
        createDefaultFactory(false, false);
        manageMockHivemqMqttClientCreator(0);

        JoynrMqttClient receiver = factory.createReceiver(gbids[0]);
        JoynrMqttClient sender = factory.createSender(gbids[0]);
        JoynrMqttClient replyReceiver = factory.createReplyReceiver(gbids[0]);
        assertNotNull(receiver);
        assertNotNull(sender);
        assertNotNull(replyReceiver);
        assertEquals(receiver, sender);
        assertEquals(sender, replyReceiver);
        assertEquals(receiver, replyReceiver);
    }

    @Test
    public void createClients_separateConnections_separateReplyReceivers() {
        createDefaultFactory(true, true);
        manageMockHivemqMqttClientCreator(0);

        JoynrMqttClient receiver = factory.createReceiver(gbids[0]);
        JoynrMqttClient sender = factory.createSender(gbids[0]);
        JoynrMqttClient replyReceiver = factory.createReplyReceiver(gbids[0]);
        assertNotNull(receiver);
        assertNotNull(sender);
        assertNotNull(replyReceiver);
        assertNotEquals(receiver, sender);
        assertNotEquals(sender, replyReceiver);
        assertNotEquals(receiver, replyReceiver);
    }

    @Test
    public void createClients_separateConnections_noSeparateReplyReceivers() {
        createDefaultFactory(true, false);
        manageMockHivemqMqttClientCreator(0);

        JoynrMqttClient receiver = factory.createReceiver(gbids[0]);
        JoynrMqttClient sender = factory.createSender(gbids[0]);
        JoynrMqttClient replyReceiver = factory.createReplyReceiver(gbids[0]);
        assertNotNull(receiver);
        assertNotNull(sender);
        assertNotNull(replyReceiver);
        assertNotEquals(receiver, sender);
    }

    @Test
    public void testRegisterForShutdown() {
        createDefaultFactory(false, true);
        verify(mockShutdownNotifier).registerHivemqMqttShutdownListener(factory);
    }

    @Test
    public void testRegisterPrepareForShutdownListener() {
        createDefaultFactory(false, true);
        verify(mockShutdownNotifier).registerHivemqMqttPrepareForShutdownListener(factory);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterForShutdown_throws() {
        createDefaultFactory(false, true);
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerForShutdown(factory);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterToBeShutdownAsLast_throws() {
        createDefaultFactory(false, true);
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerToBeShutdownAsLast(factory);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterPrepareForShutdownListener_throws() {
        createDefaultFactory(false, true);
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerPrepareForShutdownListener(factory);
    }

    @Test
    public void testReceiversAndReplyReceiversAreTheSameWhenConfigured_withShutdown() {
        //Shutdown is added in this test to cover the branch where reply receivers should not be shut down a second time
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        for (int i = 0; i < receivers.size(); i++) {
            assertEquals(receivers.get(i), replyReceivers.get(i));
        }

        for (final JoynrMqttClient receiver : receivers) {
            receiver.start();
            assertFalse(receiver.isShutdown());
        }

        factory.shutdown();

        for (final JoynrMqttClient receiver : receivers) {
            assertTrue(receiver.isShutdown());
        }
    }

    @Test
    public void testShutdown_singleConnection_separateReplyReceiver() {
        createDefaultFactory(false, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            receivers.get(i).start();
            replyReceivers.get(i).start();
            assertFalse(receivers.get(i).isShutdown());
            assertFalse(replyReceivers.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < gbids.length; i++) {
            assertTrue(receivers.get(i).isShutdown());
            assertTrue(replyReceivers.get(i).isShutdown());
        }
    }

    @Test
    public void testShutdown_singleConnection_noSeparateReplyReceiver() {
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            receivers.get(i).start();
            assertFalse(receivers.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < gbids.length; i++) {
            assertTrue(receivers.get(i).isShutdown());
        }
    }

    @Test
    public void testShutdown_separateConnections_separateReplyReceiver() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            receivers.get(i).start();
            senders.get(i).start();
            replyReceivers.get(i).start();
            assertFalse(receivers.get(i).isShutdown());
            assertFalse(senders.get(i).isShutdown());
            assertFalse(replyReceivers.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < gbids.length; i++) {
            assertTrue(receivers.get(i).isShutdown());
            assertTrue(senders.get(i).isShutdown());
            assertTrue(replyReceivers.get(i).isShutdown());
        }
    }

    @Test
    public void testShutdown_separateConnections_noSeparateReplyReceiver() {
        createDefaultFactory(true, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            receivers.get(i).start();
            senders.get(i).start();
            assertFalse(receivers.get(i).isShutdown());
            assertFalse(senders.get(i).isShutdown());
        }

        factory.shutdown();

        for (int i = 0; i < gbids.length; i++) {
            assertTrue(receivers.get(i).isShutdown());
            assertTrue(senders.get(i).isShutdown());
        }
    }

    @Test
    public void testStop_clientsStarted_singleConnection_separateReplyReceiver() {
        createDefaultFactory(false, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            JoynrMqttClient replyReceiver = replyReceivers.get(i);
            sender.start();
            replyReceiver.start();
            assertFalse(sender.isShutdown());
            assertFalse(replyReceiver.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).disconnect();
            verify(replyReceivers.get(i), times(1)).disconnect();
            verify(senders.get(i), times(0)).shutdown();
            verify(replyReceivers.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStop_clientsStarted_singleConnection_noSeparateReplyReceiver() {
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            sender.start();
            assertFalse(sender.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).disconnect();
            verify(senders.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStop_clientsNotStarted_singleConnection_separateReplyReceiver() {
        createDefaultFactory(false, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            JoynrMqttClient replyReceiver = replyReceivers.get(i);
            assertTrue(sender.isShutdown());
            assertTrue(replyReceiver.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).disconnect();
            verify(replyReceivers.get(i), times(1)).disconnect();
            verify(senders.get(i), times(0)).shutdown();
            verify(replyReceivers.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStop_clientsNotStarted_singleConnection_noSeparateReplyReceiver() {
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            assertTrue(sender.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).disconnect();
            verify(senders.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStop_clientsStarted_separateConnections_separateReplyReceiver() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient clientReceiver = receivers.get(i);
            clientReceiver.start();
            assertFalse(clientReceiver.isShutdown());

            JoynrMqttClient clientSender = senders.get(i);
            clientSender.start();
            assertFalse(clientSender.isShutdown());

            JoynrMqttClient replyReceiver = replyReceivers.get(i);
            replyReceiver.start();
            assertFalse(replyReceiver.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(receivers.get(i), times(1)).disconnect();
            verify(senders.get(i), times(1)).disconnect();
            verify(replyReceivers.get(i), times(1)).disconnect();
            verify(receivers.get(i), times(0)).shutdown();
            verify(senders.get(i), times(0)).shutdown();
            verify(replyReceivers.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStop_clientsStarted_separateConnections_noSeparateReplyReceiver() {
        createDefaultFactory(true, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient clientReceiver = receivers.get(i);
            clientReceiver.start();
            assertFalse(clientReceiver.isShutdown());

            JoynrMqttClient clientSender = senders.get(i);
            clientSender.start();
            assertFalse(clientSender.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(receivers.get(i), times(1)).disconnect();
            verify(senders.get(i), times(1)).disconnect();
            verify(receivers.get(i), times(0)).shutdown();
            verify(senders.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStop_clientsNotStarted_separateConnections_separateReplyReceiver() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient clientReceiver = receivers.get(i);
            assertTrue(clientReceiver.isShutdown());

            JoynrMqttClient clientSender = senders.get(i);
            assertTrue(clientSender.isShutdown());

            JoynrMqttClient replyReceiver = replyReceivers.get(i);
            assertTrue(replyReceiver.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(receivers.get(i), times(1)).disconnect();
            verify(senders.get(i), times(1)).disconnect();
            verify(replyReceivers.get(i), times(1)).disconnect();
            verify(receivers.get(i), times(0)).shutdown();
            verify(senders.get(i), times(0)).shutdown();
            verify(replyReceivers.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStop_clientsNotStarted_separateConnections_noSeparateReplyReceiver() {
        createDefaultFactory(true, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient clientReceiver = receivers.get(i);
            assertTrue(clientReceiver.isShutdown());

            JoynrMqttClient clientSender = senders.get(i);
            assertTrue(clientSender.isShutdown());
        }

        factory.stop();

        for (int i = 0; i < gbids.length; i++) {
            verify(receivers.get(i), times(1)).disconnect();
            verify(senders.get(i), times(1)).disconnect();
            verify(receivers.get(i), times(0)).shutdown();
            verify(senders.get(i), times(0)).shutdown();
        }
    }

    @Test
    public void testStart_clientsNotStarted_singleConnection_separateReplyReceiver() {
        createDefaultFactory(false, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            assertTrue(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            JoynrMqttClient receiver = receivers.get(i);
            assertTrue(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();

            JoynrMqttClient replyReceiver = replyReceivers.get(i);
            assertTrue(replyReceiver.isShutdown());
            doNothing().when((HivemqMqttClient) replyReceiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(0)).connect();
            verify(receivers.get(i), times(0)).connect();
            verify(replyReceivers.get(i), times(0)).connect();
        }
    }

    @Test
    public void testStart_clientNotStarted_singleConnection_noSeparateReplyReceiver() {
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            assertTrue(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            JoynrMqttClient receiver = receivers.get(i);
            assertTrue(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(0)).connect();
            verify(receivers.get(i), times(0)).connect();
        }
    }

    @Test
    public void testStart_clientsStarted_singleConnection_separateReplyReceiver() {
        createDefaultFactory(false, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            JoynrMqttClient receiver = receivers.get(i);
            JoynrMqttClient replyReceiver = replyReceivers.get(i);

            sender.start();
            receiver.start();
            replyReceiver.start();

            assertFalse(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            assertFalse(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();

            assertFalse(replyReceiver.isShutdown());
            doNothing().when((HivemqMqttClient) replyReceiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).connect();
            verify(receivers.get(i), times(1)).connect();
            verify(replyReceivers.get(i), times(1)).connect();
        }
    }

    @Test
    public void testStart_clientStarted_singleConnection_noSeparateReplyReceiver() {
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            JoynrMqttClient receiver = receivers.get(i);

            sender.start();
            receiver.start();

            assertFalse(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            assertFalse(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).connect();
            verify(receivers.get(i), times(1)).connect();
        }
    }

    @Test
    public void testStart_clientsNotStarted_separateConnections_separateReplyReceiver() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            assertTrue(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            JoynrMqttClient receiver = receivers.get(i);
            assertTrue(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();

            JoynrMqttClient replyReceiver = replyReceivers.get(i);
            assertTrue(replyReceiver.isShutdown());
            doNothing().when((HivemqMqttClient) replyReceiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(0)).connect();
            verify(receivers.get(i), times(0)).connect();
            verify(replyReceivers.get(i), times(0)).connect();
        }
    }

    @Test
    public void testStart_clientsNotStarted_separateConnections_noSeparateReplyReceiver() {
        createDefaultFactory(true, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            assertTrue(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            JoynrMqttClient receiver = receivers.get(i);
            assertTrue(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(0)).connect();
            verify(receivers.get(i), times(0)).connect();
        }
    }

    @Test
    public void testStart_clientsStarted_separateConnections_separateReplyReceiver() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            JoynrMqttClient receiver = receivers.get(i);
            JoynrMqttClient replyReceiver = replyReceivers.get(i);

            sender.start();
            receiver.start();
            replyReceiver.start();

            assertFalse(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            assertFalse(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();

            assertFalse(replyReceiver.isShutdown());
            doNothing().when((HivemqMqttClient) replyReceiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).connect();
            verify(receivers.get(i), times(1)).connect();
            verify(replyReceivers.get(i), times(1)).connect();
        }
    }

    @Test
    public void testStart_clientsStarted_separateConnections_noSeparateReplyReceiver() {
        createDefaultFactory(true, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreTheSame();

        for (int i = 0; i < gbids.length; i++) {
            JoynrMqttClient sender = senders.get(i);
            JoynrMqttClient receiver = receivers.get(i);

            sender.start();
            receiver.start();

            assertFalse(sender.isShutdown());
            doNothing().when((HivemqMqttClient) sender).connect();

            assertFalse(receiver.isShutdown());
            doNothing().when((HivemqMqttClient) receiver).connect();
        }

        factory.start();

        for (int i = 0; i < gbids.length; i++) {
            verify(senders.get(i), times(1)).connect();
            verify(receivers.get(i), times(1)).connect();
        }
    }

    @Test
    public void testConnect_canConnectFalse_clientsDontConnect() {
        HivemqMqttClientFactory clientFactory = new HivemqMqttClientFactory(true,
                                                                            mockClientIdProvider,
                                                                            mockShutdownNotifier,
                                                                            mockHivemqMqttClientCreator,
                                                                            false,
                                                                            true,
                                                                            false);

        manageMockHivemqMqttClientCreator(0);

        HivemqMqttClient receiver = (HivemqMqttClient) clientFactory.createReceiver(gbids[0]);
        receiver.setMessageListener(mockMqttMessagingSkeleton1);
        HivemqMqttClient sender = (HivemqMqttClient) clientFactory.createSender(gbids[0]);
        sender.setMessageListener(mockMqttMessagingSkeleton2);
        HivemqMqttClient replyReceiver = (HivemqMqttClient) clientFactory.createReplyReceiver(gbids[0]);
        replyReceiver.setMessageListener(mockMqttMessagingSkeleton3);

        clientFactory.connect(receiver);
        clientFactory.connect(sender);
        clientFactory.connect(replyReceiver);

        verify(sender, times(0)).connect();
        verify(receiver, times(0)).connect();
        verify(replyReceiver, times(0)).connect();
    }

    @Test
    public void testConnect_canConnectTrue_clientsConnect() {
        HivemqMqttClientFactory clientFactory = new HivemqMqttClientFactory(true,
                                                                            mockClientIdProvider,
                                                                            mockShutdownNotifier,
                                                                            mockHivemqMqttClientCreator,
                                                                            true,
                                                                            true,
                                                                            false);

        manageMockHivemqMqttClientCreator(0);

        HivemqMqttClient receiver = (HivemqMqttClient) clientFactory.createReceiver(gbids[0]);
        receiver.setMessageListener(mockMqttMessagingSkeleton1);
        HivemqMqttClient sender = (HivemqMqttClient) clientFactory.createSender(gbids[0]);
        sender.setMessageListener(mockMqttMessagingSkeleton2);
        HivemqMqttClient replyReceiver = (HivemqMqttClient) clientFactory.createReplyReceiver(gbids[0]);
        replyReceiver.setMessageListener(mockMqttMessagingSkeleton3);

        doNothing().when(receiver).registerPublishCallback();
        doNothing().when(sender).registerPublishCallback();
        doNothing().when(replyReceiver).registerPublishCallback();

        clientFactory.connect(sender);
        clientFactory.connect(receiver);
        clientFactory.connect(replyReceiver);

        verify(sender, times(1)).connect();
        verify(receiver, times(1)).connect();
        verify(replyReceiver, times(1)).connect();
    }

    @Test
    public void testStop_canConnectTrue_ClientsDisconnect() {
        HivemqMqttClientFactory clientFactory = new HivemqMqttClientFactory(true,
                                                                            mockClientIdProvider,
                                                                            mockShutdownNotifier,
                                                                            mockHivemqMqttClientCreator,
                                                                            true,
                                                                            true,
                                                                            false);

        manageMockHivemqMqttClientCreator(0);

        HivemqMqttClient receiver = (HivemqMqttClient) clientFactory.createReceiver(gbids[0]);
        receiver.setMessageListener(mockMqttMessagingSkeleton1);
        HivemqMqttClient sender = (HivemqMqttClient) clientFactory.createSender(gbids[0]);
        sender.setMessageListener(mockMqttMessagingSkeleton2);
        HivemqMqttClient replyReceiver = (HivemqMqttClient) clientFactory.createReplyReceiver(gbids[0]);
        replyReceiver.setMessageListener(mockMqttMessagingSkeleton3);

        receivers.add(receiver);
        senders.add(sender);
        replyReceivers.add(replyReceiver);

        clientFactory.stop();

        verify(sender, times(1)).disconnect();
        verify(receiver, times(1)).disconnect();
        verify(replyReceiver, times(1)).disconnect();
        verify(sender, times(0)).shutdown();
        verify(receiver, times(0)).shutdown();
        verify(replyReceiver, times(0)).shutdown();
    }

    @Test
    public void testStop_canConnectFalse_ClientsDontDisconnect() {
        HivemqMqttClientFactory clientFactory = new HivemqMqttClientFactory(true,
                                                                            mockClientIdProvider,
                                                                            mockShutdownNotifier,
                                                                            mockHivemqMqttClientCreator,
                                                                            false,
                                                                            true,
                                                                            false);

        manageMockHivemqMqttClientCreator(0);

        HivemqMqttClient receiver = (HivemqMqttClient) clientFactory.createReceiver(gbids[0]);
        receiver.setMessageListener(mockMqttMessagingSkeleton1);
        HivemqMqttClient sender = (HivemqMqttClient) clientFactory.createSender(gbids[0]);
        sender.setMessageListener(mockMqttMessagingSkeleton2);
        HivemqMqttClient replyReceiver = (HivemqMqttClient) clientFactory.createReplyReceiver(gbids[0]);
        replyReceiver.setMessageListener(mockMqttMessagingSkeleton3);

        receivers.add(receiver);
        senders.add(sender);
        replyReceivers.add(replyReceiver);

        clientFactory.stop();

        verify(sender, times(0)).disconnect();
        verify(receiver, times(0)).disconnect();
        verify(replyReceiver, times(0)).disconnect();
        verify(sender, times(0)).shutdown();
        verify(receiver, times(0)).shutdown();
        verify(replyReceiver, times(0)).shutdown();
    }

    @Test
    public void testShutdown_canConnectTrue_ClientsDisconnect() {
        HivemqMqttClientFactory clientFactory = new HivemqMqttClientFactory(true,
                                                                            mockClientIdProvider,
                                                                            mockShutdownNotifier,
                                                                            mockHivemqMqttClientCreator,
                                                                            true,
                                                                            true,
                                                                            false);

        manageMockHivemqMqttClientCreator(0);

        HivemqMqttClient receiver = (HivemqMqttClient) clientFactory.createReceiver(gbids[0]);
        receiver.setMessageListener(mockMqttMessagingSkeleton1);
        HivemqMqttClient sender = (HivemqMqttClient) clientFactory.createSender(gbids[0]);
        sender.setMessageListener(mockMqttMessagingSkeleton2);
        HivemqMqttClient replyReceiver = (HivemqMqttClient) clientFactory.createReplyReceiver(gbids[0]);
        replyReceiver.setMessageListener(mockMqttMessagingSkeleton3);

        receivers.add(receiver);
        senders.add(sender);
        replyReceivers.add(replyReceiver);

        clientFactory.shutdown();

        verify(sender, times(1)).disconnect();
        verify(receiver, times(1)).disconnect();
        verify(replyReceiver, times(1)).disconnect();
        verify(sender, times(1)).shutdown();
        verify(receiver, times(1)).shutdown();
        verify(replyReceiver, times(1)).shutdown();
    }

    @Test
    public void testShutdown_canConnectFalse_ClientsDontDisconnect() {
        HivemqMqttClientFactory clientFactory = new HivemqMqttClientFactory(true,
                                                                            mockClientIdProvider,
                                                                            mockShutdownNotifier,
                                                                            mockHivemqMqttClientCreator,
                                                                            false,
                                                                            true,
                                                                            false);

        manageMockHivemqMqttClientCreator(0);

        HivemqMqttClient receiver = (HivemqMqttClient) clientFactory.createReceiver(gbids[0]);
        receiver.setMessageListener(mockMqttMessagingSkeleton1);
        HivemqMqttClient sender = (HivemqMqttClient) clientFactory.createSender(gbids[0]);
        sender.setMessageListener(mockMqttMessagingSkeleton2);
        HivemqMqttClient replyReceiver = (HivemqMqttClient) clientFactory.createReplyReceiver(gbids[0]);
        replyReceiver.setMessageListener(mockMqttMessagingSkeleton3);

        receivers.add(receiver);
        senders.add(sender);
        replyReceivers.add(replyReceiver);

        clientFactory.shutdown();

        verify(sender, times(0)).disconnect();
        verify(receiver, times(0)).disconnect();
        verify(replyReceiver, times(0)).disconnect();
        verify(sender, times(1)).shutdown();
        verify(receiver, times(1)).shutdown();
        verify(replyReceiver, times(1)).shutdown();
    }

    @Test
    public void testStartConnectsClientsThatAreNotShutdown() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        final Set<JoynrMqttClient> activeClients = new HashSet<>();
        final Set<JoynrMqttClient> shutdownClients = new HashSet<>();
        final int shutdownIndex = 1;
        for (int i = 0; i < gbids.length; i++) {
            final JoynrMqttClient sender = senders.get(i);
            final JoynrMqttClient receiver = receivers.get(i);
            final JoynrMqttClient replyReceiver = replyReceivers.get(i);
            if (i == shutdownIndex) {
                shutdownClients(sender, receiver, replyReceiver);
                shutdownClients.add(sender);
                shutdownClients.add(receiver);
                shutdownClients.add(replyReceiver);
            } else {
                startClients(sender, receiver, replyReceiver);
                activeClients.add(sender);
                activeClients.add(receiver);
                activeClients.add(replyReceiver);
            }
        }

        factory.start();

        activeClients.forEach(client -> verify(client, times(1)).connect());
        shutdownClients.forEach(client -> verify(client, never()).connect());
    }

    @Test
    public void testPrepareForShutdownDoesNotShutdownClientsIfSeparateConnectionsTurnedOff() {
        createDefaultFactory(false, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreTheSame();
        assertReceiversAndReplyReceiversAreTheSame();

        factory.prepareForShutdown();

        Stream.of(senders.stream(), receivers.stream(), replyReceivers.stream())
              .flatMap(i -> i)
              .forEach(client -> verify(client, never()).shutdown());
    }

    @Test
    public void testPrepareForShutdownShutdownsReceivingClientsIfSeparateConnectionsTurnedOnAndSeparateReplyReceiverDisabled() {
        createDefaultFactory(true, false);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreTheSame();

        factory.prepareForShutdown();

        Stream.of(receivers.stream(), replyReceivers.stream())
              .flatMap(i -> i)
              .forEach(client -> verify(client, times(1)).shutdown());
        senders.forEach(client -> verify(client, never()).shutdown());
    }

    @Test
    public void testPrepareForShutdownShutdownsReceivingClientsIfSeparateConnectionsTurnedOnAndSeparateReplyReceiverEnabled() {
        createDefaultFactory(true, true);
        createSendersAndReceivers();

        assertSendersAndReceiversAreDifferent();
        assertReceiversAndReplyReceiversAreDifferent();

        factory.prepareForShutdown();

        receivers.forEach(client -> verify(client, times(1)).shutdown());
        Stream.of(senders.stream(), replyReceivers.stream())
              .flatMap(i -> i)
              .forEach(client -> verify(client, never()).shutdown());
    }

    private void createDefaultFactory(final boolean separateConnections, final boolean separateReplyReceiver) {
        createDefaultFactory(separateConnections, separateReplyReceiver, false, true);
    }
}
