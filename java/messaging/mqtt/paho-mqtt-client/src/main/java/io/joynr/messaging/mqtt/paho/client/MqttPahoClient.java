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
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.StampedLock;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.internal.security.SSLSocketFactoryFactory;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;

public class MqttPahoClient implements JoynrMqttClient, MqttCallback {

    private static final Logger logger = LoggerFactory.getLogger(MqttPahoClient.class);

    private MqttClient mqttClient;
    private StampedLock mqttClientLock;
    private boolean isReceiver;
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
    private String keyStoreType;
    private String trustStoreType;
    private String keyStorePWD;
    private String trustStorePWD;
    private String username;
    private String password;
    private MqttStatusReceiver mqttStatusReceiver;
    private boolean separateConnections;
    private boolean isSecureConnection;

    private Set<String> subscribedTopics = new HashSet<>();

    private AtomicBoolean shutdown = new AtomicBoolean(false);

    private String brokerUri;
    private String clientId;
    private ScheduledExecutorService scheduledExecutorService;

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttPahoClient(String brokerUri,
                          String clientId,
                          ScheduledExecutorService scheduledExecutorService,
                          int reconnectSleepMS,
                          int keepAliveTimerSec,
                          int connectionTimeoutSec,
                          int timeToWaitMs,
                          int maxMsgsInflight,
                          int maxMsgSizeBytes,
                          boolean cleanSession,
                          boolean isReceiver,
                          boolean separateConnections,
                          String keyStorePath,
                          String trustStorePath,
                          String keyStoreType,
                          String trustStoreType,
                          String keyStorePWD,
                          String trustStorePWD,
                          String username,
                          String password,
                          MqttStatusReceiver mqttStatusReceiver) throws MqttException {

        this.brokerUri = brokerUri;
        this.clientId = clientId;
        this.scheduledExecutorService = scheduledExecutorService;
        this.reconnectSleepMs = reconnectSleepMS;
        this.keepAliveTimerSec = keepAliveTimerSec;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.timeToWaitMs = timeToWaitMs;
        this.maxMsgsInflight = maxMsgsInflight;
        this.maxMsgSizeBytes = maxMsgSizeBytes;
        this.cleanSession = cleanSession;
        this.isReceiver = isReceiver;
        this.keyStorePath = keyStorePath;
        this.trustStorePath = trustStorePath;
        this.keyStoreType = keyStoreType;
        this.trustStoreType = trustStoreType;
        this.keyStorePWD = keyStorePWD;
        this.trustStorePWD = trustStorePWD;
        this.username = username;
        this.password = password;
        this.mqttStatusReceiver = mqttStatusReceiver;
        this.separateConnections = separateConnections;

        mqttClientLock = new StampedLock();
        mqttClient = createMqttClient();
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

    private boolean clientExistsAndIsNotConnected() {
        boolean clientExistsAndIsNotConnected = false;
        long stamp = mqttClientLock.readLock();
        try {
            clientExistsAndIsNotConnected = (mqttClient != null && !mqttClient.isConnected());
        } finally {
            mqttClientLock.unlockRead(stamp);
        }
        return clientExistsAndIsNotConnected;
    }

    @Override
    public void start() {
        while (!shutdown.get() && clientExistsAndIsNotConnected()) {
            final String unableToCreateClientErrorMessage = "Unable to create MqttClient: ";
            long stamp = mqttClientLock.readLock();
            try {
                logger.info("Attempting to connect client");
                if (mqttClient.isConnected()) {
                    logger.trace("Client connected while waiting for lock. Returning.");
                    return;
                }
                logger.debug("Started MqttPahoClient");
                mqttClient.setCallback(this);
                mqttClient.setTimeToWait(timeToWaitMs);
                mqttClient.connect(getConnectOptions());
                logger.info("Connected client");
                mqttStatusReceiver.notifyConnectionStatusChanged(MqttStatusReceiver.ConnectionStatus.CONNECTED);
                reestablishSubscriptions();
            } catch (MqttException mqttError) {
                logger.error("Connect failed. Error code {}", mqttError.getReasonCode(), mqttError);
                switch (mqttError.getReasonCode()) {
                case MqttException.REASON_CODE_NOT_AUTHORIZED:
                    logger.error("Failed to establish connection because of missing authorization , error: ",
                                 mqttError);
                    throw new JoynrIllegalStateException("Unable to create MqttPahoClient: " + mqttError);
                case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                    if (isSecureConnection) {
                        logger.error("Failed to establish TLS connection, error: ", mqttError);
                    }
                    // fall through
                case MqttException.REASON_CODE_BROKER_UNAVAILABLE:
                    // fall through
                case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
                    // fall through
                case MqttException.REASON_CODE_CLIENT_NOT_CONNECTED:
                    // fall through
                case MqttException.REASON_CODE_CLIENT_TIMEOUT:
                    // fall through
                case MqttException.REASON_CODE_CONNECT_IN_PROGRESS:
                    // fall through
                case MqttException.REASON_CODE_CONNECTION_LOST:
                    // fall through
                case MqttException.REASON_CODE_MAX_INFLIGHT:
                    // fall through
                case MqttException.REASON_CODE_NO_MESSAGE_IDS_AVAILABLE:
                    // fall through
                case MqttException.REASON_CODE_SERVER_CONNECT_ERROR:
                    // fall through
                case MqttException.REASON_CODE_SUBSCRIBE_FAILED:
                    // fall through
                case MqttException.REASON_CODE_UNEXPECTED_ERROR:
                    // fall through
                case MqttException.REASON_CODE_WRITE_TIMEOUT:
                    mqttClientLock.unlockRead(stamp);
                    try {
                        reconnect(reconnectSleepMs);
                    } catch (Exception e) {
                        stamp = mqttClientLock.readLock();
                        logger.error(unableToCreateClientErrorMessage, e);
                        throw new JoynrIllegalStateException(unableToCreateClientErrorMessage + e.getMessage(), e);
                    }
                    stamp = mqttClientLock.readLock();
                    continue;
                case MqttException.REASON_CODE_CLIENT_CONNECTED:
                    mqttClientLock.unlockRead(stamp);
                    try {
                        reconnect();
                    } catch (Exception e) {
                        stamp = mqttClientLock.readLock();
                        logger.error(unableToCreateClientErrorMessage, e);
                        throw new JoynrIllegalStateException(unableToCreateClientErrorMessage + e.getMessage(), e);
                    }
                    stamp = mqttClientLock.readLock();
                    continue;
                default:
                    logger.error("Unable to connect for unknown/unlisted reason.");
                    throw new JoynrIllegalStateException("Unable connect for unknown/unlisted reason.");
                }
            } catch (JoynrIllegalStateException e) {
                Throwable cause = e.getCause();
                if (cause instanceof MqttException) {
                    // this might have been thrown from subscribe() via reestablishSubscriptions()
                    mqttClientLock.unlockRead(stamp);
                    try {
                        reconnect(reconnectSleepMs);
                    } catch (MqttException mqttException) {
                        stamp = mqttClientLock.readLock();
                        logger.error(unableToCreateClientErrorMessage, mqttException);
                        throw new JoynrIllegalStateException(unableToCreateClientErrorMessage
                                + mqttException.getMessage(), mqttException);
                    }
                    stamp = mqttClientLock.readLock();
                    continue;
                }
                throw new JoynrIllegalStateException("Unable to start MqttPahoClient: " + e.getMessage(), e);
            } catch (Exception e) {
                throw new JoynrIllegalStateException("Unable to start MqttPahoClient: " + e.getMessage(), e);
            } finally {
                mqttClientLock.unlockRead(stamp);
            }
        }
        logger.info("Leaving start");
    }

    private MqttClient createMqttClient() throws MqttException {
        logger.info("Create Mqtt Client. brokerUri: {}", brokerUri);
        return new MqttClient(brokerUri, clientId, new MemoryPersistence(), scheduledExecutorService);
    }

    private void reestablishSubscriptions() {
        logger.debug("Reestablishing {} subscriptions after restart", subscribedTopics.size());
        Set<String> oldSubscribedTopics = subscribedTopics;
        subscribedTopics = new HashSet<>();
        for (String topic : oldSubscribedTopics) {
            try {
                subscribe(topic);
            } catch (Exception e) {
                // if a subscribe fails, restore original subscription list for next reconnect cycle
                subscribedTopics = oldSubscribedTopics;
                throw e;
            }
        }
    }

    private MqttConnectOptions getConnectOptions() {
        MqttConnectOptions options = new MqttConnectOptions();
        if (username != null && !username.isEmpty()) {
            if (password == null || password.isEmpty()) {
                throw new JoynrIllegalStateException("MQTT password not configured or empty");
            }
            options.setUserName(username);
            options.setPassword(password.toCharArray());
        }
        options.setAutomaticReconnect(false);
        options.setConnectionTimeout(connectionTimeoutSec);
        options.setKeepAliveInterval(keepAliveTimerSec);
        options.setMaxInflight(maxMsgsInflight);
        options.setCleanSession(cleanSession);

        if (isSecureConnection) {
            // Set global SSL properties for all Joynr SSL clients
            Properties sslClientProperties = new Properties();
            sslClientProperties.setProperty(SSLSocketFactoryFactory.KEYSTORETYPE, keyStoreType);
            sslClientProperties.setProperty(SSLSocketFactoryFactory.KEYSTORE, keyStorePath);
            sslClientProperties.setProperty(SSLSocketFactoryFactory.KEYSTOREPWD, keyStorePWD);
            sslClientProperties.setProperty(SSLSocketFactoryFactory.TRUSTSTORETYPE, trustStoreType);
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
            logger.debug("Checking for subscription to: {}", topic);
            long stamp = mqttClientLock.readLock();
            try {
                synchronized (subscribedTopics) {
                    if (!subscribedTopics.contains(topic)) {
                        logger.info("Attempting to subscribe to: {}", topic);
                        if (mqttClient == null) {
                            throw new MqttException(MqttException.REASON_CODE_CLIENT_NOT_CONNECTED);
                        }
                        mqttClient.subscribe(topic);
                        subscribedTopics.add(topic);
                        logger.info("Subscribed to: {}", topic);
                    }
                    subscribed = true;
                }
            } catch (MqttException mqttError) {
                logger.debug("Subscribe to {} failed: {}. Error code {}",
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
                    throw new JoynrIllegalStateException("Unexpected exception while subscribing to " + topic
                            + ", error: " + mqttError, mqttError);
                default:
                    throw new JoynrIllegalStateException("Unexpected exception while subscribing to " + topic
                            + ", error: " + mqttError);
                }

            } catch (Exception e) {
                throw new JoynrRuntimeException("Unable to start MqttPahoClient", e);
            } finally {
                mqttClientLock.unlockRead(stamp);
            }
        }
    }

    @Override
    public void unsubscribe(String topic) {
        try {
            synchronized (subscribedTopics) {
                if (subscribedTopics.remove(topic)) {
                    long stamp = mqttClientLock.readLock();
                    try {
                        if (mqttClient == null) {
                            throw new MqttException(MqttException.REASON_CODE_CLIENT_NOT_CONNECTED);
                        }
                        mqttClient.unsubscribe(topic);
                    } finally {
                        mqttClientLock.unlockRead(stamp);
                    }
                }
            }
        } catch (MqttException e) {
            throw new JoynrRuntimeException("Unable to unsubscribe from " + topic, e);
        }
    }

    @Override
    public synchronized void shutdown() {
        if (shutdown.get()) {
            return;
        }
        shutdown.set(true);
        logger.info("Attempting to shutdown connection.");
        long stamp = mqttClientLock.writeLock();
        try {
            if (mqttClient != null) {
                mqttStatusReceiver.notifyConnectionStatusChanged(MqttStatusReceiver.ConnectionStatus.NOT_CONNECTED);
                final boolean forcibly = false;
                MqttClient clientToDisconnect = mqttClient;
                mqttClient = null;
                disconnect(clientToDisconnect, forcibly);
            }
        } finally {
            mqttClientLock.unlockWrite(stamp);
        }
    }

    @Override
    public void publishMessage(String topic, byte[] serializedMessage, int qosLevel, long messageExpiryIntervalSec) {
        assert !separateConnections || (separateConnections && !isReceiver);

        if (!separateConnections && messagingSkeleton == null) {
            throw new JoynrDelayMessageException("MQTT Publish failed: messagingSkeleton has not been set yet");
        }
        if (maxMsgSizeBytes != 0 && serializedMessage.length > maxMsgSizeBytes) {
            throw new JoynrMessageNotSentException("MQTT Publish failed: maximum allowed message size of "
                    + maxMsgSizeBytes + " bytes exceeded, actual size is " + serializedMessage.length + " bytes");
        }

        MqttMessage message = new MqttMessage();
        try {
            message.setPayload(serializedMessage);
            message.setQos(qosLevel);
            message.setRetained(false);
        } catch (Exception e) {
            throw new JoynrMessageNotSentException(e.getMessage(), e);
        }

        long stamp = mqttClientLock.tryReadLock();
        if (stamp == 0l) {
            throw new JoynrDelayMessageException("Mqtt client not available (not connected)");
        }
        try {
            logger.debug("Publish to: {}", topic);
            if (mqttClient == null) {
                throw new MqttException(MqttException.REASON_CODE_CLIENT_NOT_CONNECTED);
            }
            mqttClient.publish(topic, message);
        } catch (MqttException e) {
            logger.debug("Publish failed: {}. Error code {}", e.getMessage(), e.getReasonCode(), e);
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
        } finally {
            mqttClientLock.unlockRead(stamp);
        }

        logger.debug("Published message: " + new String(serializedMessage, StandardCharsets.UTF_8));
    }

    @Override
    public void connectionLost(Throwable error) {
        logger.info("connectionLost: {}", error.getMessage());

        if (error instanceof MqttException) {
            mqttStatusReceiver.notifyConnectionStatusChanged(MqttStatusReceiver.ConnectionStatus.NOT_CONNECTED);
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
                logger.debug("Connection lost, trying to reconnect. Error code {}", reason);
                attemptDisconnectAndRestart();
                break;
            case MqttException.REASON_CODE_CLIENT_EXCEPTION:
                String causeMessage;
                Throwable cause = mqttError.getCause();
                if (cause != null) {
                    causeMessage = cause.getMessage();
                } else {
                    causeMessage = "<not available>";
                }
                logger.error("Connection lost due to client exception. Cause {}", causeMessage);
                attemptDisconnectAndRestart();
                break;
            // the following error codes indicate a configuration problem that is not recoverable through reconnecting
            case MqttException.REASON_CODE_INVALID_PROTOCOL_VERSION:
            case MqttException.REASON_CODE_INVALID_CLIENT_ID:
            case MqttException.REASON_CODE_FAILED_AUTHENTICATION:
            case MqttException.REASON_CODE_NOT_AUTHORIZED:
            case MqttException.REASON_CODE_SOCKET_FACTORY_MISMATCH:
            case MqttException.REASON_CODE_SSL_CONFIG_ERROR:
                logger.error("Connection is incorrectly configured. Connection not possible: {}. Error code {}",
                             mqttError.getMessage(),
                             reason);
                shutdown();
                break;
            // the following error codes can occur if the client is closing / already closed
            case MqttException.REASON_CODE_CLIENT_CLOSED:
            case MqttException.REASON_CODE_CLIENT_DISCONNECTING:
                logger.trace("Connection lost due to client shutting down. Error code {}", reason);
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
                logger.error("Received error reason that should not have been thrown for connection loss: {}. Error code {}",
                             mqttError.getMessage(),
                             reason);
                shutdown();
            }

        } else {
            logger.error("Connection lost due to unknown error ", error);
            shutdown();
        }
    }

    private void reconnect() throws MqttException {
        reconnect(0);
    }

    private void reconnect(int reconnectSleepMs) throws MqttException {
        long stamp = mqttClientLock.writeLock();
        MqttClient clientToDisconnect = mqttClient;
        mqttClient = null;
        mqttClientLock.unlockWrite(stamp);

        final boolean forcibly = true;
        if (clientToDisconnect != null) {
            disconnect(clientToDisconnect, forcibly);
        }
        if (shutdown.get()) {
            logger.debug("joynr is shutting down. Will not attempt a reconnect.");
            return;
        }
        if (reconnectSleepMs > 0) {
            logger.info("Waiting {}ms before attempting reconnect.", reconnectSleepMs);
            try {
                Thread.sleep(reconnectSleepMs);
            } catch (InterruptedException e) {
                logger.error("Interrupted while waiting before reconnecting mqtt client.", e);
            }
        }
        stamp = mqttClientLock.writeLock();
        try {
            mqttClient = createMqttClient();
        } finally {
            mqttClientLock.unlockWrite(stamp);
        }
    }

    private void disconnect(MqttClient clientToDisconnect, final boolean forcibly) {
        logger.info("Attempting to remove callbacks from client.");
        clientToDisconnect.setCallback(null);
        try {
            if (forcibly) {
                logger.info("Attempting to disconnect client forcibly.");
                clientToDisconnect.disconnectForcibly(1L, 1L);
            } else {
                logger.info("Attempting to disconnect client.");
                clientToDisconnect.disconnect();
            }
            logger.info("Client disconnected.");
        } catch (Exception e) {
            if (!forcibly) {
                logger.trace("Failed to disconnect client. Error ", e);
                try {
                    if (clientToDisconnect.isConnected()) {
                        logger.info("Attempting to disconnect client forcibly.");
                        clientToDisconnect.disconnectForcibly(1L, 1L);
                        logger.info("Client forcibly disconnected.");
                    }
                } catch (Exception disconnectForciblyException) {
                    logger.trace("Failed to disconnect client forcibly. Error ", disconnectForciblyException);
                }
            } else {
                logger.trace("Failed to disconnect client forcibly. Error ", e);
            }
        } finally {
            logger.trace("Attempting to close client");
            try {
                clientToDisconnect.close();
                logger.info("Client closed.");
            } catch (Exception e) {
                logger.error("Failed to close client. Error ", e);
            }
        }
    }

    private synchronized void attemptDisconnectAndRestart() {
        try {
            reconnect();
            start();
        } catch (Exception e) {
            logger.error("Problem while attempting to close and restart. Error ", e);
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken mqttDeliveryToken) {
        logger.debug("Message delivered. id: {}", mqttDeliveryToken.getMessageId());
    }

    @Override
    public void messageArrived(String topic, MqttMessage mqttMessage) throws Exception {
        logger.debug("Message received: id {}, topic {}, payload\n{}",
                     mqttMessage.getId(),
                     topic,
                     new String(mqttMessage.getPayload(), StandardCharsets.UTF_8));
        if (messagingSkeleton == null) {
            logger.error("Message not processed: messagingSkeleton has not been set yet");
            return;
        }
        messagingSkeleton.transmit(mqttMessage.getPayload(), new FailureAction() {

            @Override
            public void execute(Throwable error) {
                logger.error("Message not processed.", error);
            }
        });
    }

    @Override
    public void setMessageListener(IMqttMessagingSkeleton messaging) {
        this.messagingSkeleton = messaging;
    }

    public boolean isShutdown() {
        return shutdown.get();
    }

    // for testing
    MqttClient getMqttClient() {
        return mqttClient;
    }
}
