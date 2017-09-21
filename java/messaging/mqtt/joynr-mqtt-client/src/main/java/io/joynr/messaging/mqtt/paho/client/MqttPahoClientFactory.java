package io.joynr.messaging.mqtt.paho.client;

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

import java.util.concurrent.ScheduledExecutorService;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.MqttAddress;

@Singleton
public class MqttPahoClientFactory implements MqttClientFactory {

    private static final Logger logger = LoggerFactory.getLogger(MqttPahoClientFactory.class);
    private MqttAddress ownAddress;
    private JoynrMqttClient mqttClient = null;
    private int reconnectSleepMs;
    private int keepAliveTimerSec;
    private int connectionTimeoutSec;
    private int timeToWaitMs;
    private int maxMsgsInflight;
    private int maxMsgSizeBytes;
    private ScheduledExecutorService scheduledExecutorService;
    private MqttClientIdProvider clientIdProvider;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttPahoClientFactory(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS) int reconnectSleepMs,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC) int keepAliveTimerSec,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC) int connectionTimeoutSec,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS) int timeToWaitMs,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_MAX_MSGS_INFLIGHT) int maxMsgsInflight,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES) int maxMsgSizeBytes,
                                 @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduledExecutorService,
                                 MqttClientIdProvider mqttClientIdProvider) {
        this.ownAddress = ownAddress;
        this.reconnectSleepMs = reconnectSleepMs;
        this.scheduledExecutorService = scheduledExecutorService;
        this.clientIdProvider = mqttClientIdProvider;
        this.keepAliveTimerSec = keepAliveTimerSec;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.timeToWaitMs = timeToWaitMs;
        this.maxMsgsInflight = maxMsgsInflight;
        this.maxMsgSizeBytes = maxMsgSizeBytes;
    }

    @Override
    public synchronized JoynrMqttClient create() {
        if (mqttClient == null) {
            mqttClient = createInternal();
        }
        return mqttClient;
    }

    private JoynrMqttClient createInternal() {

        MqttPahoClient pahoClient = null;
        try {
            logger.debug("Create Mqtt Client. Address: {}", ownAddress);

            String clientId = clientIdProvider.getClientId();
            MqttClient mqttClient = new MqttClient(ownAddress.getBrokerUri(),
                                                   clientId,
                                                   new MemoryPersistence(),
                                                   scheduledExecutorService);
            logger.info("Creating MQTT Paho client using MQTT client ID: {}", clientId);
            pahoClient = new MqttPahoClient(mqttClient,
                                            reconnectSleepMs,
                                            keepAliveTimerSec,
                                            connectionTimeoutSec,
                                            timeToWaitMs,
                                            maxMsgsInflight,
                                            maxMsgSizeBytes);
        } catch (MqttException e) {
            logger.error("Create MqttClient failed", e);
        }

        return pahoClient;
    }

}
