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

import java.net.URI;
import java.net.URISyntaxException;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.net.ssl.SSLHandshakeException;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import org.eclipse.paho.client.mqttv3.internal.security.SSLSocketFactoryFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Charsets;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttMessagingStub;
import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;

public class MqttPahoClient implements JoynrMqttClient, MqttCallback {

    private static final Logger logger = LoggerFactory.getLogger(MqttPahoClient.class);

    private MqttClient mqttClient;
    private IMqttMessagingSkeleton messagingSkeleton;
    private int reconnectSleepMs;
    private int keepAliveTimerSec;
    private int connectionTimeoutSec;
    private int timeToWaitMs;
    private int maxMsgsInflight;
    private int maxMsgSizeBytes;
    private boolean cleanSession;
    private String keyStorePath;
    private String trustStorePath;
    private String keyStorePWD;
    private String trustStorePWD;
    private MqttStatusReceiver mqttStatusReceiver;
    private boolean isSecureConnection;
    private boolean disconnecting = false;

    private Set<String> subscribedTopics = new HashSet<>();

    private AtomicBoolean shutdown = new AtomicBoolean(false);

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttPahoClient(MqttClient mqttClient,
                          int reconnectSleepMS,
                          int keepAliveTimerSec,
                          int connectionTimeoutSec,
                          int timeToWaitMs,
                          int maxMsgsInflight,
                          int maxMsgSizeBytes,
                          boolean cleanSession,
                          String keyStorePath,
                          String trustStorePath,
                          String keyStorePWD,
                          String trustStorePWD,
                          MqttStatusReceiver mqttStatusReceiver) throws MqttException {
        this.mqttClient = mqttClient;
        this.reconnectSleepMs = reconnectSleepMS;
        this.keepAliveTimerSec = keepAliveTimerSec;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.timeToWaitMs = timeToWaitMs;
        this.maxMsgsInflight = maxMsgsInflight;
        this.maxMsgSizeBytes = maxMsgSizeBytes;
        this.cleanSession = cleanSession;
        this.keyStorePath = keyStorePath;
        this.trustStorePath = trustStorePath;
        this.keyStorePWD = keyStorePWD;
        this.trustStorePWD = trustStorePWD;
        this.mqttStatusReceiver = mqttStatusReceiver;

        String srvURI = mqttClient.getServerURI();
        URI vURI;
        try {
            vURI = new URI(srvURI);
            this.isSecureConnection = vURI.getScheme().equals("ssl");
        } catch (URISyntaxException e) {
            logger.error("Failed to read srvURI, error: ", e);
            throw new JoynrIllegalStateException("Fail to parse URI server: " + srvURI + " " + ", error: " + e);
        }
    }

    @Override
    public void start() {
        while (!shutdown.get() && !mqttClient.isConnected()) {
            try {
                synchronized (this) {
                    logger.debug("Started MqttPahoClient");
                    mqttClient.setCallback(this);
                    mqttClient.setTimeToWait(timeToWaitMs);
                    mqttClient.connect(getConnectOptions());
                    logger.debug("MQTT Connected client");
                    mqttStatusReceiver.notifyConnectionStatusChanged(MqttStatusReceiver.ConnectionStatus.CONNECTED);
                    reestablishSubscriptions();
                }
            } catch (MqttException mqttError) {
                logger.error("MQTT Connect failed. Error code {}", mqttError.getReasonCode(), mqttError);
                switch (mqttError.getReasonCode()) {
                case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                    if (isSecureConnection) {
                        logger.error("Failed to establish TLS connection, error: " + mqttError);
                        if (mqttError instanceof MqttSecurityException
                                || (mqttError.getCause() != null && mqttError.getCause() instanceof SSLHandshakeException)) {
                            throw new JoynrIllegalStateException("Unable to create TLS MqttPahoClient: " + mqttError);
                        }
                    }
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
                    if (shutdown.get()) {
                        return;
                    }
                    synchronized (this) {
                        try {
                            this.wait(reconnectSleepMs);
                        } catch (InterruptedException e) {
                            Thread.currentThread().interrupt();
                            return;
                        }
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

    private void reestablishSubscriptions() {
        logger.debug("reestablishing {} Subscriptions for MQTT after restart", subscribedTopics.size());
        Set<String> oldSubscribedTopics = subscribedTopics;
        subscribedTopics = new HashSet<>();
        for (String topic : oldSubscribedTopics) {
            subscribe(topic);
        }
    }

    private MqttConnectOptions getConnectOptions() {
        MqttConnectOptions options = new MqttConnectOptions();
        options.setAutomaticReconnect(false);
        options.setConnectionTimeout(connectionTimeoutSec);
        options.setKeepAliveInterval(keepAliveTimerSec);
        options.setMaxInflight(maxMsgsInflight);
        options.setCleanSession(cleanSession);

        if (isSecureConnection) {
            // Set global SSL properties for all Joynr SSL clients
            Properties sslClientProperties = new Properties();
            sslClientProperties.setProperty(SSLSocketFactoryFactory.KEYSTORETYPE, "JKS");
            sslClientProperties.setProperty(SSLSocketFactoryFactory.KEYSTORE, keyStorePath);
            sslClientProperties.setProperty(SSLSocketFactoryFactory.KEYSTOREPWD, keyStorePWD);
            sslClientProperties.setProperty(SSLSocketFactoryFactory.TRUSTSTORETYPE, "JKS");
            sslClientProperties.setProperty(SSLSocketFactoryFactory.TRUSTSTORE, trustStorePath);
            sslClientProperties.setProperty(SSLSocketFactoryFactory.TRUSTSTOREPWD, trustStorePWD);
            options.setSSLProperties(sslClientProperties);
        }

        return options;
    }

    @Override
    public void subscribe(String topic) {
        boolean subscribed = false;
        while (!subscribed && !shutdown.get()) {
            logger.debug("MQTT subscribing to: {}", topic);
            try {
                synchronized (subscribedTopics) {
                    if (!subscribedTopics.contains(topic)) {
                        mqttClient.subscribe(topic);
                        subscribedTopics.add(topic);
                    }
                    subscribed = true;
                }
            } catch (MqttException mqttError) {
                logger.debug("MQTT subscribe to {} failed: {}. Error code {}",
                             topic,
                             mqttError.getMessage(),
                             mqttError.getReasonCode(),
                             mqttError);
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
                case MqttException.REASON_CODE_CONNECTION_LOST:
                case MqttException.REASON_CODE_CLIENT_NOT_CONNECTED:
                case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
                    try {
                        Thread.sleep(reconnectSleepMs);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                    continue;
                default:
                    throw new JoynrIllegalStateException("Unexpected exception while subscribing to " + topic
                            + ", error: " + mqttError);
                }

            } catch (Exception e) {
                throw new JoynrRuntimeException("Unable to start MqttPahoClient", e);
            }
        }
    }

    @Override
    public void unsubscribe(String topic) {
        try {
            synchronized (subscribedTopics) {
                if (subscribedTopics.remove(topic)) {
                    mqttClient.unsubscribe(topic);
                }
            }
        } catch (MqttException e) {
            throw new JoynrRuntimeException("Unable to unsubscribe from " + topic, e);
        }
    }

    @Override
    public void shutdown() {
        shutdown.set(true);
        synchronized (this) {
            this.notifyAll();
        }
        logger.info("Attempting shutdown of MQTT connection.");
        try {
            synchronized (this) {
                mqttClient.disconnectForcibly(10000, 10000);
                mqttClient.close();
            }
        } catch (Exception e) {
            logger.error("MQTT Close failed", e);
        }
    }

    @Override
    public void publishMessage(String topic, byte[] serializedMessage) {
        publishMessage(topic, serializedMessage, MqttMessagingStub.DEFAULT_QOS_LEVEL);
    }

    @Override
    public void publishMessage(String topic, byte[] serializedMessage, int qosLevel) {
        if (messagingSkeleton == null) {
            throw new JoynrDelayMessageException("MQTT Publish failed: messagingSkeleton has not been set yet");
        }
        if (maxMsgSizeBytes != 0 && serializedMessage.length > maxMsgSizeBytes) {
            throw new JoynrMessageNotSentException("MQTT Publish failed: maximum allowed message size of "
                    + maxMsgSizeBytes + " bytes exceeded, actual size is " + serializedMessage.length + " bytes");
        }
        try {
            MqttMessage message = new MqttMessage();
            message.setPayload(serializedMessage);
            message.setQos(qosLevel);
            message.setRetained(false);

            logger.debug("MQTT Publish to: {}", topic);
            mqttClient.publish(topic, message);
        } catch (MqttException e) {
            logger.debug("MQTT Publish failed: {}. Error code {}", e.getMessage(), e.getReasonCode(), e);
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

        logger.debug("Published message: " + new String(serializedMessage, Charsets.UTF_8));
    }

    @Override
    public void connectionLost(Throwable error) {
        logger.debug("connectionLost: {}", error.getMessage());
        mqttStatusReceiver.notifyConnectionStatusChanged(MqttStatusReceiver.ConnectionStatus.NOT_CONNECTED);

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
                logger.debug("MQTT connection lost, trying to reconnect. Error code {}", reason);
                attemptDisconnectAndRestart();
                break;
            case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                logger.error("MQTT connection lost due to client exception");
                Throwable cause = mqttError.getCause();
                if (cause != null) {
                    logger.error(cause.getMessage());
                }
                attemptDisconnectAndRestart();
                break;
            // the following error codes indicate a configuration problem that is not recoverable through reconnecting
            case MqttException.REASON_CODE_INVALID_PROTOCOL_VERSION:
            case MqttException.REASON_CODE_INVALID_CLIENT_ID:
            case MqttException.REASON_CODE_FAILED_AUTHENTICATION:
            case MqttException.REASON_CODE_NOT_AUTHORIZED:
            case MqttException.REASON_CODE_SOCKET_FACTORY_MISMATCH:
            case MqttException.REASON_CODE_SSL_CONFIG_ERROR:
                logger.error("MQTT Connection is incorrectly configured. Connection not possible: {}. Error code {}",
                             mqttError.getMessage(),
                             reason);
                shutdown();
                break;
            // the following error codes can occur if the client is closing / already closed
            case MqttException.REASON_CODE_CLIENT_CLOSED:
            case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
                logger.trace("MQTT connection lost due to client shutting down. Error code {}", reason);
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
                logger.error("received error reason that should not have been thrown for connection loss: {}. Error code {}",
                             mqttError.getMessage(),
                             reason);
                shutdown();
            }

        } else {
            logger.error("MQTT connection lost due to unknown error " + error);
            shutdown();
        }
    }

    private void attemptDisconnectAndRestart() {
        if (!disconnecting) {
            disconnecting = true;
            try {
                mqttClient.disconnect();
            } catch (Exception e) {
                logger.trace("Problem while attempting disconnect.", e);
                try {
                    mqttClient.disconnectForcibly();
                } catch (Exception e2) {
                    logger.trace("Problem while attempting to disconnect forcibly.", e2);
                }
            } finally {
                disconnecting = false;
                start();
            }
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken mqttDeliveryToken) {
        logger.debug("MQTT message delivered. id: {}", mqttDeliveryToken.getMessageId());
    }

    @Override
    public void messageArrived(String topic, MqttMessage mqttMessage) throws Exception {
        logger.debug("MQTT message received: id {}, topic {}, payload\n{}",
                     mqttMessage.getId(),
                     topic,
                     new String(mqttMessage.getPayload(), Charsets.UTF_8));
        if (messagingSkeleton == null) {
            logger.error("MQTT message not processed: messagingSkeleton has not been set yet");
            return;
        }
        messagingSkeleton.transmit(mqttMessage.getPayload(), new FailureAction() {

            @Override
            public void execute(Throwable error) {
                logger.error("MQTT message not processed");
            }
        });
    }

    @Override
    public void setMessageListener(IMqttMessagingSkeleton messaging) {
        this.messagingSkeleton = messaging;
    }
}
