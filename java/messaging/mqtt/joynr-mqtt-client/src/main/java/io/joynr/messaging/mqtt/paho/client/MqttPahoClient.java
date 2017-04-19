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
    private int keepAliveTimerSec;
    private int connectionTimeoutSec;
    private int timeToWaitMs;
    private int maxMsgsInflight;

    private Set<String> subscribedTopics = new HashSet<>();

    public MqttPahoClient(MqttClient mqttClient,
                          int reconnectSleepMS,
                          int keepAliveTimerSec,
                          int connectionTimeoutSec,
                          int timeToWaitMs,
                          int maxMsgsInflight) throws MqttException {
        this.mqttClient = mqttClient;
        this.reconnectSleepMs = reconnectSleepMS;
        this.keepAliveTimerSec = keepAliveTimerSec;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.timeToWaitMs = timeToWaitMs;
        this.maxMsgsInflight = maxMsgsInflight;
    }

    @Override
    public void start() {
        while (!mqttClient.isConnected()) {
            try {
                mqttClient.setCallback(this);
                mqttClient.setTimeToWait(timeToWaitMs);
                mqttClient.connect(getConnectOptions());
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
        options.setConnectionTimeout(connectionTimeoutSec);
        options.setKeepAliveInterval(keepAliveTimerSec);
        options.setMaxInflight(maxMsgsInflight);
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
        if (error instanceof MqttException) {
            MqttException mqttError = (MqttException) error;
            int reason = mqttError.getReasonCode();

            switch (reason) {
            // the following error codes indicate recoverable errors, hence a reconnect is initiated
            case MqttException.REASON_CODE_BROKER_UNAVAILABLE:
            case MqttException.REASON_CODE_CLIENT_TIMEOUT:
            case MqttException.REASON_CODE_WRITE_TIMEOUT:
            case MqttException.REASON_CODE_CLIENT_NOT_CONNECTED:
            case MqttException.REASON_CODE_INVALID_MESSAGE:
            case MqttException.REASON_CODE_CONNECTION_LOST:
            case MqttException.REASON_CODE_UNEXPECTED_ERROR:
                logger.debug("MQTT connection lost, trying to reconnect");
                start();
                break;
            case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                logger.error("MQTT connection lost due to client exception");
                Throwable cause = mqttError.getCause();
                if (cause != null) {
                    logger.error(cause.getMessage());
                }
                start();
                break;
            // the following error codes indicate a configuration problem that is not recoverable through reconnecting
            case MqttException.REASON_CODE_INVALID_PROTOCOL_VERSION:
            case MqttException.REASON_CODE_INVALID_CLIENT_ID:
            case MqttException.REASON_CODE_FAILED_AUTHENTICATION:
            case MqttException.REASON_CODE_NOT_AUTHORIZED:
            case MqttException.REASON_CODE_SOCKET_FACTORY_MISMATCH:
            case MqttException.REASON_CODE_SSL_CONFIG_ERROR:
                logger.error("MQTT Connection is incorrectly configured. Connection not possible: "
                        + mqttError.getMessage());
                shutdown();
                break;
            // the following error codes can occur if the client is closing / already closed
            case MqttException.REASON_CODE_CLIENT_CLOSED:
            case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
                logger.trace("MQTT connection lost due to client shutting down");
                break;
            // the following error codes should not be thrown when the connectionLost() callback is called
            // they are listed here for the sake of completeness
            case MqttException.REASON_CODE_CLIENT_CONNECTED:
            case MqttException.REASON_CODE_SUBSCRIBE_FAILED:
            case MqttException.REASON_CODE_CLIENT_ALREADY_DISCONNECTED:
            case MqttException.REASON_CODE_CLIENT_DISCONNECT_PROHIBITED:
            case MqttException.REASON_CODE_CONNECT_IN_PROGRESS:
            case MqttException.REASON_CODE_NO_MESSAGE_IDS_AVAILABLE:
            case MqttException.REASON_CODE_TOKEN_INUSE:
            case MqttException.REASON_CODE_MAX_INFLIGHT:
            case MqttException.REASON_CODE_DISCONNECTED_BUFFER_FULL:
            default:
                logger.error("received error reason that should not have been thrown for connection loss: "
                        + mqttError.getMessage());
                shutdown();
            }

        } else {
            logger.error("MQTT connection lost due to unknown error " + error);
            shutdown();
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
