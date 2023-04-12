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

import io.joynr.messaging.mqtt.JoynrMqttClient;
import org.junit.Test;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Stream;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

public class HivemqMqttClientFactoryWorkflowTest extends AbstractHiveMqttClientFactoryTest {

    private final List<JoynrMqttClient> shutDownSenders = new ArrayList<>();
    private final List<JoynrMqttClient> shutDownReceivers = new ArrayList<>();
    private final List<JoynrMqttClient> shutDownReplyReceivers = new ArrayList<>();

    @Test
    public void testWorkflow_noSeparateConnections_noSeparateReplyReceiver_noSharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(false, false, false, false);
    }

    @Test
    public void testWorkflow_noSeparateConnections_noSeparateReplyReceiver_noSharedSubscriptions_canConnect() throws Exception {
        testWorkflow(false, false, false, true);
    }

    @Test
    public void testWorkflow_noSeparateConnections_noSeparateReplyReceiver_sharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(false, false, true, false);
    }

    @Test
    public void testWorkflow_noSeparateConnections_noSeparateReplyReceiver_sharedSubscriptions_canConnect() throws Exception {
        testWorkflow(false, false, true, true);
    }

    @Test
    public void testWorkflow_noSeparateConnections_separateReplyReceiver_noSharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(false, true, false, false);
    }

    @Test
    public void testWorkflow_noSeparateConnections_separateReplyReceiver_noSharedSubscriptions_canConnect() throws Exception {
        testWorkflow(false, true, false, true);
    }

    @Test
    public void testWorkflow_noSeparateConnections_separateReplyReceiver_sharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(false, true, true, false);
    }

    @Test
    public void testWorkflow_noSeparateConnections_separateReplyReceiver_sharedSubscriptions_canConnect() throws Exception {
        testWorkflow(false, true, true, true);
    }

    @Test
    public void testWorkflow_separateConnections_noSeparateReplyReceiver_noSharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(true, false, false, false);
    }

    @Test
    public void testWorkflow_separateConnections_noSeparateReplyReceiver_noSharedSubscriptions_canConnect() throws Exception {
        testWorkflow(true, false, false, true);
    }

    @Test
    public void testWorkflow_separateConnections_noSeparateReplyReceiver_sharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(true, false, true, false);
    }

    @Test
    public void testWorkflow_separateConnections_noSeparateReplyReceiver_sharedSubscriptions_canConnect() throws Exception {
        testWorkflow(true, false, true, true);
    }

    @Test
    public void testWorkflow_separateConnections_separateReplyReceiver_noSharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(true, true, false, false);
    }

    @Test
    public void testWorkflow_separateConnections_separateReplyReceiver_noSharedSubscriptions_canConnect() throws Exception {
        testWorkflow(true, true, false, true);
    }

    @Test
    public void testWorkflow_separateConnections_separateReplyReceiver_sharedSubscriptions_cannotConnect() throws Exception {
        testWorkflow(true, true, true, false);
    }

    @Test
    public void testWorkflow_separateConnections_separateReplyReceiver_sharedSubscriptions_canConnect() throws Exception {
        testWorkflow(true, true, true, true);
    }

    private boolean shouldSendersAndReceiversBeDifferent(final boolean separateConnections,
                                                         final boolean separateReplyReceiver,
                                                         final boolean sharedSubscriptions) {
        return separateConnections || (sharedSubscriptions && separateReplyReceiver);
    }

    private void testWorkflow(final boolean separateConnections,
                              final boolean separateReplyReceiver,
                              final boolean sharedSubscriptions,
                              final boolean canConnect) throws Exception {
        createDefaultFactory(separateConnections, separateReplyReceiver, sharedSubscriptions, canConnect);
        createSendersAndReceivers();
        assertEquals(getFactoryCanConnectValue(), canConnect);

        createAndShutdownConnections();
        verifyConnectionInstances(separateConnections, separateReplyReceiver, sharedSubscriptions);

        startAndConnectClients(canConnect, replyReceivers, receivers, senders);

        startFactoryAndVerifyConnectInvocationCount(canConnect, 1);

        stopFactoryAndVerifyDisconnectInvocationCount(separateConnections, separateReplyReceiver);

        startFactoryAndVerifyConnectInvocationCount(canConnect, 2);

        factory.prepareForShutdown();
        verifyShutdownInvocationCount(separateConnections, separateReplyReceiver, sharedSubscriptions, 1, 0);

        factory.prepareForShutdown();
        verifyShutdownInvocationCount(separateConnections, separateReplyReceiver, sharedSubscriptions, 2, 0);

        factory.shutdown();
        verifyShutdownInvocationCount(separateConnections, separateReplyReceiver, sharedSubscriptions, 2, 1);

        factory.shutdown();
        verifyShutdownInvocationCount(separateConnections, separateReplyReceiver, sharedSubscriptions, 2, 2);
    }

    private void verifyShutdownInvocationCount(final boolean separateConnections,
                                               final boolean separateReplyReceiver,
                                               final boolean sharedSubscriptions,
                                               final int prepareForShutdownInvocationCount,
                                               final int shutdownInvocationCount) {
        if (separateConnections) {
            verifyShutdownInvocationCount(prepareForShutdownInvocationCount + shutdownInvocationCount, receivers);
            verifyShutdownInvocationCount(1 + prepareForShutdownInvocationCount + shutdownInvocationCount,
                                          shutDownReceivers);

            verifyShutdownInvocationCount(shutdownInvocationCount, senders);
            verifyShutdownInvocationCount(1 + shutdownInvocationCount, shutDownSenders);
            if (separateReplyReceiver) {
                verifyShutdownInvocationCount(shutdownInvocationCount, replyReceivers);
                verifyShutdownInvocationCount(1 + shutdownInvocationCount, shutDownReplyReceivers);
            } else {
                verifyShutdownInvocationCount(prepareForShutdownInvocationCount + shutdownInvocationCount,
                                              replyReceivers);
                verifyShutdownInvocationCount(1 + prepareForShutdownInvocationCount + shutdownInvocationCount,
                                              shutDownReplyReceivers);
            }
        } else {
            if (sharedSubscriptions && separateReplyReceiver) {
                verifyShutdownInvocationCount(prepareForShutdownInvocationCount + shutdownInvocationCount, receivers);
                verifyShutdownInvocationCount(shutdownInvocationCount, senders, replyReceivers);
                verifyShutdownInvocationCount(1 + prepareForShutdownInvocationCount + shutdownInvocationCount,
                                              shutDownReceivers);
                verifyShutdownInvocationCount(1 + shutdownInvocationCount, shutDownSenders, shutDownReplyReceivers);
            } else {
                verifyShutdownInvocationCount(shutdownInvocationCount, receivers, senders, replyReceivers);
                verifyShutdownInvocationCount(1 + shutdownInvocationCount,
                                              shutDownReceivers,
                                              shutDownSenders,
                                              shutDownReplyReceivers);
            }
        }
    }

    private void stopFactoryAndVerifyDisconnectInvocationCount(final boolean separateConnections, final boolean separateReplyReceiver) throws Exception {
        factory.stop();
        assertFalse(getFactoryCanConnectValue());
        verifyDisconnectInvokedOnce(senders, shutDownSenders);
        if (separateConnections) {
            verifyDisconnectInvokedOnce(receivers, shutDownReceivers);
        }
        if (separateReplyReceiver) {
            verifyDisconnectInvokedOnce(replyReceivers, shutDownReplyReceivers);
        }
    }

    private void startFactoryAndVerifyConnectInvocationCount(final boolean canConnect, final int factoryStartInvocationCount) throws Exception {
        factory.start();
        assertTrue(getFactoryCanConnectValue());

        if (canConnect) {
            verifyConnectInvocationCount(factoryStartInvocationCount + 1, senders, receivers, replyReceivers);
        } else {
            verifyConnectInvocationCount(factoryStartInvocationCount, senders, receivers, replyReceivers);
        }

        verifyConnectNotInvoked(shutDownSenders, shutDownReceivers, shutDownReplyReceivers);
    }

    private void createAndShutdownConnections() {
        Arrays.stream(GBIDS_FOR_SHUTDOWN).forEach(gbid -> {
            manageMockHivemqMqttClientCreator(gbid);
            final JoynrMqttClient receiver = factory.createReceiver(gbid);
            final JoynrMqttClient sender = factory.createSender(gbid);
            final JoynrMqttClient replyReceiver = factory.createReplyReceiver(gbid);
            shutdownClients(sender, receiver, replyReceiver);
            shutDownSenders.add(factory.createSender(gbid));
            shutDownReceivers.add(factory.createReceiver(gbid));
            shutDownReplyReceivers.add(factory.createReplyReceiver(gbid));
        });
    }

    private void verifyConnectionInstances(final boolean separateConnections,
                                           final boolean separateReplyReceiver,
                                           final boolean sharedSubscriptions) {
        if (shouldSendersAndReceiversBeDifferent(separateConnections, separateReplyReceiver, sharedSubscriptions)) {
            assertSendersAndReceiversAreDifferent();
        } else {
            assertSendersAndReceiversAreTheSame();
        }
        if (separateReplyReceiver) {
            assertReceiversAndReplyReceiversAreDifferent();
        } else {
            assertReceiversAndReplyReceiversAreTheSame();
        }
    }

    @SafeVarargs
    private void verifyShutdownInvocationCount(final int times, final List<JoynrMqttClient>... clientLists) {
        for (List<JoynrMqttClient> clientList : clientLists) {
            verifyShutdownInvocationCount(times, clientList);
        }
    }

    private void verifyShutdownInvocationCount(final int times, final List<JoynrMqttClient> clients) {
        clients.forEach(client -> verifyShutdownInvocationCount(times, client));
    }

    private void verifyShutdownInvocationCount(final int times, final JoynrMqttClient client) {
        verify(client, times(times)).shutdown();
    }

    @SafeVarargs
    private void verifyConnectNotInvoked(final List<JoynrMqttClient>... clientLists) {
        streamDistinct(clientLists).forEach(this::verifyConnectNotInvoked);
    }

    private void verifyConnectNotInvoked(final JoynrMqttClient client) {
        verify(client, never()).connect();
    }

    @SafeVarargs
    private void verifyConnectInvokedOnce(final List<JoynrMqttClient>... clientLists) {
        verifyConnectInvocationCount(1, clientLists);
    }

    @SafeVarargs
    private void verifyConnectInvokedTwice(final List<JoynrMqttClient>... clientLists) {
        verifyConnectInvocationCount(2, clientLists);
    }

    @SafeVarargs
    private void verifyConnectInvocationCount(final int times, final List<JoynrMqttClient>... clientLists) {
        streamDistinct(clientLists).forEach(client -> verifyConnectInvocationCount(times, client));
    }

    private void verifyConnectInvocationCount(final int times, final JoynrMqttClient client) {
        verify(client, times(times)).connect();
    }

    @SafeVarargs
    private void verifyDisconnectInvokedOnce(final List<JoynrMqttClient>... clientLists) {
        streamDistinct(clientLists).forEach(this::verifyDisconnectInvokedOnce);
    }

    private void verifyDisconnectInvokedOnce(final JoynrMqttClient client) {
        verify(client, times(1)).disconnect();
    }

    private boolean getFactoryCanConnectValue() throws Exception {
        final Field field = factory.getClass().getDeclaredField("canConnect");
        field.setAccessible(true);
        return (boolean) field.get(factory);
    }

    @SafeVarargs
    private void startAndConnectClients(final boolean isConnectExpected, final List<JoynrMqttClient>... clientLists) {
        streamDistinct(clientLists).forEach(client -> startAndConnectClient(isConnectExpected, client));
    }

    @SafeVarargs
    private Stream<JoynrMqttClient> streamDistinct(final List<JoynrMqttClient>... clientLists) {
        return Arrays.stream(clientLists).flatMap(List::stream).distinct();
    }

    private void startAndConnectClient(final boolean isConnectExpected, final JoynrMqttClient client) {
        startClient(client);
        factory.connect(client);
        if (isConnectExpected) {
            verify(client, times(1)).connect();
        } else {
            verify(client, never()).connect();
        }
    }
}
