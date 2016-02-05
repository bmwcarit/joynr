package io.joynr.messaging.mqtt.paho.client;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.nio.charset.Charset;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttPahoClient implements JoynrMqttClient, MqttCallback {

    public static final int MQTT_QOS = 1;
    public static final String MQTT_PRIO = "low";

    private static final Logger logger = LoggerFactory.getLogger(MqttPahoClient.class);
    private MqttClient mqttClient;
    private MqttAddress ownTopic;
    private IMessaging messagingSkeleton;
    private int reconnectSleepMs;

    public MqttPahoClient(MqttClient mqttClient, MqttAddress ownTopic, int reconnectSleepMs) throws MqttException {
        this.mqttClient = mqttClient;
        this.ownTopic = ownTopic;
        this.reconnectSleepMs = reconnectSleepMs;
    }

    @Override
    public void start() {
        while (!mqttClient.isConnected()) {
            try {
                mqttClient.connect();
                mqttClient.setCallback(this);
                mqttClient.subscribe(ownTopic.getTopic());
                logger.debug("MQTT Connected client");
            } catch (Exception e) {
                // TODO which exceptions are recoverable?
                logger.error("MQTT Connect failed: {}", e.getMessage());
                try {
                    Thread.sleep(reconnectSleepMs);
                } catch (InterruptedException e1) {
                }
            }
        }
    }

    @Override
    public void shutdown() {
        try {
            mqttClient.disconnect();
            mqttClient.close();
        } catch (Exception e) {
            logger.error("MQTT Close failed", e);
        }
    }

    @Override
    public void publishMessage(String topic, String serializedMessage) {
        try {
            MqttMessage message = new MqttMessage();
            message.setPayload(serializedMessage.getBytes(Charset.forName("UTF-8")));
            message.setQos(MqttPahoClient.MQTT_QOS);
            message.setRetained(false);

            logger.debug("MQTT Publish to: {}", topic);
            mqttClient.publish(topic, message);
        } catch (MqttException e) {
            logger.error("MQTT Publish failed: {}", e.getMessage());
            throw new JoynrMessageNotSentException(e.getMessage());
        }

        logger.debug("Published message: " + serializedMessage);
    }

    @Override
    public void connectionLost(Throwable error) {
        logger.error("MQTT connection lost: {}", error.getMessage());
        //TODO which exceptions are recoverable?
        if (error instanceof MqttException) {
            start();
        } else {
            shutdown();
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken arg0) {
        // TODO Auto-generated method stub

    }

    @Override
    public void messageArrived(String topic, MqttMessage mqttMessage) throws Exception {
        String serializedMessage = new String(mqttMessage.getPayload(), Charset.forName("UTF-8"));
        messagingSkeleton.transmit(serializedMessage, new FailureAction() {

            @Override
            public void execute(Throwable error) {
                logger.error("MQTT message not processed");
            }
        });
    }

    @Override
    public void addMessageListener(IMessaging messaging) {
        this.messagingSkeleton = messaging;

    }
}
