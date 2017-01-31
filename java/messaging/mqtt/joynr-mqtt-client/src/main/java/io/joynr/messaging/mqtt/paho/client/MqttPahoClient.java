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
import java.util.HashSet;
import java.util.Set;

import io.joynr.messaging.mqtt.MqttMessagingStub;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IRawMessaging;
import io.joynr.messaging.mqtt.JoynrMqttClient;

public class MqttPahoClient implements JoynrMqttClient, MqttCallback {

    public static final String MQTT_PRIO = "low";

    private static final Logger logger = LoggerFactory.getLogger(MqttPahoClient.class);
    private MqttClient mqttClient;
    private IRawMessaging messagingSkeleton;
    private int reconnectSleepMs;

    private Set<String> subscribedTopics = new HashSet<>();

    public MqttPahoClient(MqttClient mqttClient, int reconnectSleepMs) throws MqttException {
        this.mqttClient = mqttClient;
        this.reconnectSleepMs = reconnectSleepMs;
    }

    @Override
    public void start() {
        while (!mqttClient.isConnected()) {
            try {
                mqttClient.connect(getConnectOptions());
                mqttClient.setCallback(this);
                logger.debug("MQTT Connected client");
                for (String topic : subscribedTopics) {
                    subscribe(topic);
                }
            } catch (MqttException mqttError) {
                logger.error("MQTT Connect failed.", mqttError);
                switch (mqttError.getReasonCode()) {
                case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                case MqttException.REASON_CODE_BROKER_UNAVAILABLE:
                case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
                case MqttException.REASON_CODE_CLIENT_NOT_CONNECTED:
                case MqttException.REASON_CODE_CLIENT_TIMEOUT:
                case MqttException.REASON_CODE_CONNECT_IN_PROGRESS:
                case MqttException.REASON_CODE_CONNECTION_LOST:
                case MqttException.REASON_CODE_MAX_INFLIGHT:
                case MqttException.REASON_CODE_NO_MESSAGE_IDS_AVAILABLE:
                case MqttException.REASON_CODE_SERVER_CONNECT_ERROR:
                case MqttException.REASON_CODE_SUBSCRIBE_FAILED:
                case MqttException.REASON_CODE_UNEXPECTED_ERROR:
                case MqttException.REASON_CODE_WRITE_TIMEOUT:
                    try {
                        Thread.sleep(reconnectSleepMs);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                    continue;
                case MqttException.REASON_CODE_CLIENT_CONNECTED:
                    continue;
                }
            } catch (Exception e) {
                throw new JoynrIllegalStateException("Unable to start MqttPahoClient: " + e.getMessage(), e);
            }
        }
    }

    private MqttConnectOptions getConnectOptions() {
        MqttConnectOptions options = new MqttConnectOptions();
        options.setAutomaticReconnect(false);
        options.setConnectionTimeout(120);
        options.setKeepAliveInterval(60);
        options.setCleanSession(false);
        return options;
    }

    @Override
    public void subscribe(String topic) {
        boolean subscribed = false;
        while (!subscribed) {
            logger.debug("MQTT subscribed to: {}", topic);
            try {
                mqttClient.subscribe(topic);
                subscribed = true;
                subscribedTopics.add(topic);
            } catch (MqttException mqttError) {
                logger.debug("MQTT subscribe to: " + topic + " failed: " + mqttError.getMessage(), mqttError);
                switch (mqttError.getReasonCode()) {
                case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                case MqttException.REASON_CODE_BROKER_UNAVAILABLE:
                case MqttException.REASON_CODE_CLIENT_TIMEOUT:
                case MqttException.REASON_CODE_CONNECT_IN_PROGRESS:
                case MqttException.REASON_CODE_MAX_INFLIGHT:
                case MqttException.REASON_CODE_NO_MESSAGE_IDS_AVAILABLE:
                case MqttException.REASON_CODE_SERVER_CONNECT_ERROR:
                case MqttException.REASON_CODE_SUBSCRIBE_FAILED:
                case MqttException.REASON_CODE_UNEXPECTED_ERROR:
                case MqttException.REASON_CODE_WRITE_TIMEOUT:
                    try {
                        Thread.sleep(reconnectSleepMs);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                    continue;
                case MqttException.REASON_CODE_CONNECTION_LOST:
                case MqttException.REASON_CODE_CLIENT_NOT_CONNECTED:
                case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
                    throw new JoynrIllegalStateException("client is not connected");
                }

            } catch (Exception e) {
                throw new JoynrRuntimeException("Unable to start MqttPahoClient", e);
            }
        }
    }

    @Override
    public void unsubscribe(String topic) {
        try {
            mqttClient.unsubscribe(topic);
        } catch (MqttException e) {
            throw new JoynrRuntimeException("Unable to unsubscribe from " + topic, e);
        }
    }

    @Override
    public void shutdown() {
        logger.info("Attempting shutdown of MQTT connection.");
        try {
            mqttClient.disconnect();
            mqttClient.close();
        } catch (Exception e) {
            logger.error("MQTT Close failed", e);
        }
    }

    @Override
    public void publishMessage(String topic, String serializedMessage) {
        publishMessage(topic, serializedMessage, MqttMessagingStub.DEFAULT_QOS_LEVEL);
    }

    @Override
    public void publishMessage(String topic, String serializedMessage, int qosLevel) {
        if (messagingSkeleton == null) {
            throw new JoynrDelayMessageException("MQTT Publish failed: messagingSkeleton has not been set yet");
        }
        try {
            MqttMessage message = new MqttMessage();
            message.setPayload(serializedMessage.getBytes(Charset.forName("UTF-8")));
            message.setQos(qosLevel);
            message.setRetained(false);

            logger.debug("MQTT Publish to: {}", topic);
            mqttClient.publish(topic, message);
        } catch (MqttException e) {
            logger.debug("MQTT Publish failed: {}", e.getMessage(), e);
            switch (e.getReasonCode()) {
            case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                Throwable cause = e.getCause();
                if (cause != null) {
                    throw new JoynrDelayMessageException("MqttException: " + cause.getMessage());
                } else {
                    throw new JoynrDelayMessageException("MqttException: " + e.getMessage());
                }
            case MqttException.REASON_CODE_BROKER_UNAVAILABLE:
            case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
            case MqttException.REASON_CODE_CLIENT_NOT_CONNECTED:
            case MqttException.REASON_CODE_CLIENT_TIMEOUT:
            case MqttException.REASON_CODE_CONNECTION_LOST:
            case MqttException.REASON_CODE_MAX_INFLIGHT:
            case MqttException.REASON_CODE_NO_MESSAGE_IDS_AVAILABLE:
            case MqttException.REASON_CODE_SERVER_CONNECT_ERROR:
            case MqttException.REASON_CODE_UNEXPECTED_ERROR:
            case MqttException.REASON_CODE_WRITE_TIMEOUT:
            case MqttException.REASON_CODE_CONNECT_IN_PROGRESS:
                throw new JoynrDelayMessageException("MqttException: " + e.getMessage());
            default:
                throw new JoynrMessageNotSentException(e.getMessage());
            }
        } catch (Exception e) {
            throw new JoynrMessageNotSentException(e.getMessage(), e);
        }

        logger.debug("Published message: " + serializedMessage);
    }

    @Override
    public void connectionLost(Throwable error) {
        logger.error("MQTT connection lost: {}", error.getMessage());
        logger.trace("Connection lost because of error", error);
        if (error instanceof MqttException) {
            MqttException mqttError = (MqttException) error;
            int reason = mqttError.getReasonCode();
            switch (reason) {
            case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                connectionLost(mqttError.getCause());
                break;
            case MqttException.REASON_CODE_BROKER_UNAVAILABLE:
            case MqttException.REASON_CODE_CLIENT_NOT_CONNECTED:
            case MqttException.REASON_CODE_CLIENT_TIMEOUT:
            case MqttException.REASON_CODE_CONNECTION_LOST:
            case MqttException.REASON_CODE_SERVER_CONNECT_ERROR:
            case MqttException.REASON_CODE_UNEXPECTED_ERROR:
            case MqttException.REASON_CODE_WRITE_TIMEOUT:
                start();
                break;
            default:
                shutdown();
                break;
            }
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken arg0) {
        // nothing to do here
    }

    @Override
    public void messageArrived(String topic, MqttMessage mqttMessage) throws Exception {
        String serializedMessage = new String(mqttMessage.getPayload(), Charset.forName("UTF-8"));
        logger.debug("Received message via MQTT from topic {}:\n{}", topic, serializedMessage);
        if (messagingSkeleton == null) {
            logger.error("MQTT message not processed: messagingSkeleton has not been set yet");
            return;
        }
        messagingSkeleton.transmit(serializedMessage, new FailureAction() {

            @Override
            public void execute(Throwable error) {
                logger.error("MQTT message not processed");
            }
        });
    }

    @Override
    public void setMessageListener(IRawMessaging messaging) {
        this.messagingSkeleton = messaging;

    }
}
