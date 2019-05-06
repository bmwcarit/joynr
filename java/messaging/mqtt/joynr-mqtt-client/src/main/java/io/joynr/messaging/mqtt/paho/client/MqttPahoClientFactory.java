/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import java.util.concurrent.ScheduledExecutorService;

import org.eclipse.paho.client.mqttv3.MqttException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.system.RoutingTypes.MqttAddress;

@Singleton
public class MqttPahoClientFactory implements MqttClientFactory, ShutdownListener {

    private static final Logger logger = LoggerFactory.getLogger(MqttPahoClientFactory.class);
    private MqttAddress ownAddress;
    private JoynrMqttClient receivingMqttClient;
    private JoynrMqttClient sendingMqttClient;
    private int reconnectSleepMs;
    private int[] keepAliveTimersSec;
    private int[] connectionTimeoutsSec;
    private int timeToWaitMs;
    private int maxMsgsInflight;
    private int maxMsgSizeBytes;
    private ScheduledExecutorService scheduledExecutorService;
    private MqttClientIdProvider clientIdProvider;
    private MqttStatusReceiver mqttStatusReceiver;
    private boolean cleanSession;
    private boolean separateConnections;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH)
    private String keyStorePath = "";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH)
    private String trustStorePath = "";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_TYPE)
    private String keyStoreType = "JKS";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE)
    private String trustStoreType = "JKS";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD)
    private String keyStorePWD = "";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD)
    private String trustStorePWD = "";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_USERNAME)
    private String username = "";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_PASSWORD)
    private String password = "";

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttPahoClientFactory(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS) int reconnectSleepMs,
                                 @Named(MqttModule.MQTT_KEEP_ALIVE_TIMER_SEC_ARRAY) int[] keepAliveTimersSec,
                                 @Named(MqttModule.MQTT_CONNECTION_TIMEOUT_SEC_ARRAY) int[] connectionTimeoutsSec,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS) int timeToWaitMs,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_MAX_MSGS_INFLIGHT) int maxMsgsInflight,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES) int maxMsgSizeBytes,
                                 @Named(MqttModule.PROPERTY_MQTT_CLEAN_SESSION) boolean cleanSession,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS) boolean separateConnections,
                                 @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduledExecutorService,
                                 MqttClientIdProvider mqttClientIdProvider,
                                 MqttStatusReceiver mqttStatusReceiver,
                                 ShutdownNotifier shutdownNotifier) {
        this.ownAddress = ownAddress;
        this.reconnectSleepMs = reconnectSleepMs;
        this.scheduledExecutorService = scheduledExecutorService;
        this.clientIdProvider = mqttClientIdProvider;
        this.mqttStatusReceiver = mqttStatusReceiver;
        this.keepAliveTimersSec = keepAliveTimersSec.clone();
        this.connectionTimeoutsSec = connectionTimeoutsSec.clone();
        this.timeToWaitMs = timeToWaitMs;
        this.maxMsgsInflight = maxMsgsInflight;
        this.maxMsgSizeBytes = maxMsgSizeBytes;
        this.cleanSession = cleanSession;
        this.separateConnections = separateConnections;
        shutdownNotifier.registerForShutdown(this);
    }

    @Override
    public synchronized JoynrMqttClient createReceiver() {
        if (receivingMqttClient == null) {
            if (separateConnections) {
                receivingMqttClient = createInternal(true, "Sub");
            } else {
                createCombinedClient();
            }
        }
        return receivingMqttClient;
    }

    @Override
    public synchronized JoynrMqttClient createSender() {
        if (sendingMqttClient == null) {
            if (separateConnections) {
                sendingMqttClient = createInternal(false, "Pub");
            } else {
                createCombinedClient();
            }
        }
        return sendingMqttClient;
    }

    @Override
    public synchronized void prepareForShutdown() {
        if (separateConnections) {
            receivingMqttClient.shutdown();
        }
    }

    @Override
    public synchronized void shutdown() {
        sendingMqttClient.shutdown();
        if (separateConnections && !receivingMqttClient.isShutdown()) {
            receivingMqttClient.shutdown();
        }
    }

    private void createCombinedClient() {
        sendingMqttClient = createInternal(true, "");
        receivingMqttClient = sendingMqttClient;
    }

    private JoynrMqttClient createInternal(boolean isReceiver, String clientIdSuffix) {

        MqttPahoClient pahoClient = null;
        try {
            String clientId = clientIdProvider.getClientId() + clientIdSuffix;
            logger.info("Creating MQTT Paho client using MQTT client ID: {}", clientId);
            pahoClient = new MqttPahoClient(ownAddress,
                                            clientId,
                                            scheduledExecutorService,
                                            reconnectSleepMs,
                                            keepAliveTimersSec[0],
                                            connectionTimeoutsSec[0],
                                            timeToWaitMs,
                                            maxMsgsInflight,
                                            maxMsgSizeBytes,
                                            cleanSession,
                                            isReceiver,
                                            separateConnections,
                                            keyStorePath,
                                            trustStorePath,
                                            keyStoreType,
                                            trustStoreType,
                                            keyStorePWD,
                                            trustStorePWD,
                                            username,
                                            password,
                                            mqttStatusReceiver);
        } catch (MqttException e) {
            logger.error("Create MqttClient failed", e);
        }

        return pahoClient;
    }

}
