/*-
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
package io.joynr.messaging.mqtt.hivemq.client;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.JoynrMqttClientCreator;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttClientSignalService;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.reactivex.schedulers.Schedulers;
import io.joynr.messaging.MessagingPropertyKeys;

/**
 * This factory class is responsible for producing joynr MQTT clients using the HiveMQ MQTT Client library.
 * <p>
 * TODO
 * - When persistent session configuration exists, then enable configuration thereof
 */
@Singleton
public class HivemqMqttClientFactory implements MqttClientFactory, ShutdownListener, MqttClientSignalService {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClientFactory.class);

    public static final String SENDER_PREFIX = "Pub";
    public static final String RECEIVER_PREFIX = "Sub";
    public static final String REPLY_RECEIVER_PREFIX = "SubReply";
    private final HashMap<String, JoynrMqttClient> receivingRequestClients; // gbid to client
    private final HashMap<String, JoynrMqttClient> receivingReplyClients; // gbid to client
    private final HashMap<String, JoynrMqttClient> sendingMqttClients; // gbid to client
    private final boolean separateConnections;
    private final MqttClientIdProvider mqttClientIdProvider;
    private final boolean separateReplyReceiver;
    private final JoynrMqttClientCreator clientCreator;

    // Boolean signifying whether the clients SHOULD connect to the broker or not
    private boolean canConnect;
    private final boolean sharedSubscriptions;

    @Inject
    public HivemqMqttClientFactory(@Named(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS) boolean separateConnections,
                                   MqttClientIdProvider mqttClientIdProvider,
                                   ShutdownNotifier shutdownNotifier,
                                   JoynrMqttClientCreator clientCreator,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_CONNECT_ON_START) boolean canConnect,
                                   @Named(MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER) boolean separateReplyReceiver,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) boolean sharedSubscriptions) {
        this.separateConnections = separateConnections;
        this.mqttClientIdProvider = mqttClientIdProvider;
        sendingMqttClients = new HashMap<>(); // gbid to client
        receivingRequestClients = new HashMap<>(); // gbid to client
        receivingReplyClients = new HashMap<>(); // gbid to client
        this.clientCreator = clientCreator;
        this.separateReplyReceiver = separateReplyReceiver;
        this.canConnect = canConnect;
        this.sharedSubscriptions = sharedSubscriptions;
        shutdownNotifier.registerForShutdown(this);
    }

    @Override
    public synchronized JoynrMqttClient createSender(String gbid) {
        JoynrMqttClient client = sendingMqttClients.get(gbid);
        if (client == null) {
            logger.info("Creating sender MQTT client for gbid {}", gbid);
            if (separateConnections) {
                sendingMqttClients.put(gbid, clientCreator.createClient(gbid, getSenderClientId(), false, true, false));
            } else {
                createCombinedClient(gbid);
            }
            logger.debug("Sender MQTT client for gbid {} now: {}", gbid, sendingMqttClients.get(gbid));
        }
        return sendingMqttClients.get(gbid);
    }

    @Override
    public synchronized JoynrMqttClient createReceiver(String gbid) {
        JoynrMqttClient client = receivingRequestClients.get(gbid);
        if (client == null) {
            logger.info("Creating receiver MQTT client for gbid {}", gbid);
            if (separateConnections) {
                receivingRequestClients.put(gbid,
                                            clientCreator.createClient(gbid,
                                                                       getReceiverClientId(),
                                                                       true,
                                                                       false,
                                                                       !separateReplyReceiver));
            } else if (separateReplyReceiver && sharedSubscriptions) {
                logger.info("Receiver MQTT client for gbid {} will not handle incoming messages.", gbid);
                receivingRequestClients.put(gbid,
                                            clientCreator.createClient(gbid,
                                                                       getReceiverClientId(),
                                                                       true,
                                                                       false,
                                                                       false));
                logger.info("A separate client for receiving replies and sending messages will be created for gbid {}.",
                            gbid);
                final JoynrMqttClient replyReceiver = createReplyReceiver(gbid);
                sendingMqttClients.put(gbid, replyReceiver);
            } else {
                createCombinedClient(gbid);
            }
            logger.debug("Receiver MQTT client for gbid {} now: {}", gbid, receivingRequestClients.get(gbid));
        }
        return receivingRequestClients.get(gbid);
    }

    @Override
    public synchronized JoynrMqttClient createReplyReceiver(String gbid) {
        if (!separateReplyReceiver) {
            return createReceiver(gbid);
        }
        JoynrMqttClient client = receivingReplyClients.get(gbid);
        if (client == null) {
            logger.info("Creating reply receiver MQTT client for gbid {}", gbid);
            receivingReplyClients.put(gbid,
                                      clientCreator.createClient(gbid,
                                                                 getReplyReceiverClientId(),
                                                                 true,
                                                                 !separateConnections && sharedSubscriptions,
                                                                 true));
            logger.debug("Reply Receiver MQTT client for gbid {} now: {}", gbid, receivingReplyClients.get(gbid));
        }
        return receivingReplyClients.get(gbid);
    }

    @Override
    public synchronized void prepareForShutdown() {
        if (shouldShutdownReceivingRequestsClients()) {
            receivingRequestClients.values().forEach(JoynrMqttClient::shutdown);
        }
    }

    private boolean shouldShutdownReceivingRequestsClients() {
        return separateConnections || (sharedSubscriptions && separateReplyReceiver);
    }

    @Override
    public synchronized void shutdown() {
        logger.debug("shutdown invoked");
        stop();
        Stream.of(sendingMqttClients.values().stream(),
                  receivingRequestClients.values().stream(),
                  receivingReplyClients.values().stream())
              .flatMap(i -> i)
              .distinct()
              .forEach(JoynrMqttClient::shutdown);
        Schedulers.shutdown();
        logger.debug("shutdown finished");
    }

    @Override
    public synchronized void start() {
        canConnect = true;
        Stream.of(sendingMqttClients.values().stream(),
                  receivingRequestClients.values().stream(),
                  receivingReplyClients.values().stream())
              .flatMap(i -> i)
              .distinct()
              .forEach(this::connectIfNotShuttingDown);
    }

    private void connectIfNotShuttingDown(final JoynrMqttClient client) {
        if (!client.isShutdown()) {
            connect(client);
        }
    }

    @Override
    public synchronized void connect(JoynrMqttClient client) {
        if (canConnect) {
            client.connect();
        }
    }

    @Override
    public synchronized void stop() {
        if (!canConnect) {
            return;
        }
        canConnect = false;
        final Set<JoynrMqttClient> clientsToDisconnect = new HashSet<>(sendingMqttClients.values());
        if (separateConnections) {
            clientsToDisconnect.addAll(receivingRequestClients.values());
        }
        if (separateReplyReceiver) {
            clientsToDisconnect.addAll(receivingReplyClients.values());
        }
        clientsToDisconnect.forEach(JoynrMqttClient::disconnect);
    }

    private void createCombinedClient(String gbid) {
        sendingMqttClients.put(gbid,
                               clientCreator.createClient(gbid,
                                                          mqttClientIdProvider.getClientId(),
                                                          true,
                                                          true,
                                                          !separateReplyReceiver));
        receivingRequestClients.put(gbid, sendingMqttClients.get(gbid));
    }

    private String getClientId(final String suffix) {
        return mqttClientIdProvider.getClientId() + suffix;
    }

    private String getSenderClientId() {
        return getClientId(SENDER_PREFIX);
    }

    private String getReceiverClientId() {
        return getClientId(RECEIVER_PREFIX);
    }

    private String getReplyReceiverClientId() {
        return getClientId(REPLY_RECEIVER_PREFIX);
    }
}
