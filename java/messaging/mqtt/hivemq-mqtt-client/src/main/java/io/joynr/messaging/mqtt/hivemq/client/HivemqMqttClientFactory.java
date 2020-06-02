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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.TrustManagerFactory;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import com.hivemq.client.mqtt.MqttClient;
import com.hivemq.client.mqtt.MqttClientExecutorConfig;
import com.hivemq.client.mqtt.MqttClientSslConfigBuilder;
import com.hivemq.client.mqtt.lifecycle.MqttClientConnectedContext;
import com.hivemq.client.mqtt.lifecycle.MqttClientConnectedListener;
import com.hivemq.client.mqtt.lifecycle.MqttClientDisconnectedContext;
import com.hivemq.client.mqtt.lifecycle.MqttClientDisconnectedListener;
import com.hivemq.client.mqtt.mqtt3.Mqtt3ClientBuilder;
import com.hivemq.client.mqtt.mqtt3.Mqtt3RxClient;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.statusmetrics.MqttStatusReceiver;
import io.joynr.messaging.routing.MessageRouter;
import io.reactivex.schedulers.Schedulers;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * This factory class is responsible for producing joynr MQTT clients using the HiveMQ MQTT Client library.
 * The HiveMQ MQTT Client library is explicitly intended for use in backend applications, so using it on
 * constrained devices might not work well. Consider using the Paho client in those circumstances.
 *
 * TODO
 * - When persistent session configuration exists, then enable configuration thereof
 */
@Singleton
public class HivemqMqttClientFactory implements MqttClientFactory {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClientFactory.class);

    private final MqttAddress ownAddress;
    private final boolean separateConnections;
    private final MqttClientIdProvider mqttClientIdProvider;
    private final ScheduledExecutorService scheduledExecutorService;
    private final int keepAliveTimeSeconds;
    private final int maxMsgSizeBytes;
    private final boolean cleanSession;
    private final int connectionTimeoutSec;
    private final MqttStatusReceiver mqttStatusReceiver;

    private volatile JoynrMqttClient sender;
    private volatile JoynrMqttClient receiver;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH)
    private String keyStorePath;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH)
    private String trustStorePath;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_TYPE)
    private String keyStoreType;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE)
    private String trustStoreType;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD)
    private String keyStorePWD;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD)
    private String trustStorePWD;

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_USERNAME)
    private String username = "";

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_PASSWORD)
    private String password = "";

    @Inject
    @Named(MqttModule.MQTT_CIPHERSUITE_LIST)
    private List<String> cipherSuiteList;

    @Inject
    @Named(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS)
    private int reconnectDelayMs;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public HivemqMqttClientFactory(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS) boolean separateConnections,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC) int keepAliveTimeSeconds,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES) int maxMsgSizeBytes,
                                   @Named(MqttModule.PROPERTY_MQTT_CLEAN_SESSION) boolean cleanSession,
                                   @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduledExecutorService,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC) int connectionTimeoutSec,
                                   MqttClientIdProvider mqttClientIdProvider,
                                   MqttStatusReceiver mqttStatusReceiver) {
        this.ownAddress = ownAddress;
        this.separateConnections = separateConnections;
        this.scheduledExecutorService = scheduledExecutorService;
        this.mqttClientIdProvider = mqttClientIdProvider;
        this.keepAliveTimeSeconds = keepAliveTimeSeconds;
        this.maxMsgSizeBytes = maxMsgSizeBytes;
        this.cleanSession = cleanSession;
        this.connectionTimeoutSec = connectionTimeoutSec;
        this.mqttStatusReceiver = mqttStatusReceiver;
    }

    @Override
    public JoynrMqttClient createSender() {
        if (sender == null) {
            synchronized (this) {
                if (sender == null) {
                    logger.info("Creating sender MQTT client");
                    if (separateConnections) {
                        sender = createClient(mqttClientIdProvider.getClientId() + "Pub", false, true);
                    } else {
                        createCombinedClient();
                    }
                    logger.debug("Sender MQTT client now: {}", sender);
                }
            }
        }
        return sender;
    }

    @Override
    public JoynrMqttClient createReceiver() {
        if (receiver == null) {
            synchronized (this) {
                if (receiver == null) {
                    logger.info("Creating receiver MQTT client");
                    if (separateConnections) {
                        receiver = createClient(mqttClientIdProvider.getClientId() + "Sub", true, false);
                    } else {
                        createCombinedClient();
                    }
                    logger.debug("Receiver MQTT client now: {}", receiver);
                }
            }
        }
        return receiver;
    }

    private void createCombinedClient() {
        sender = createClient(mqttClientIdProvider.getClientId(), true, true);
        receiver = sender;
    }

    private JoynrMqttClient createClient(String clientId, boolean isReceiver, boolean isSender) {
        URI serverUri;
        try {
            serverUri = new URI(ownAddress.getBrokerUri());
        } catch (URISyntaxException e) {
            throw new JoynrIllegalStateException("Invalid MQTT broker URI: " + ownAddress.getBrokerUri(), e);
        }
        logger.info("Creating MQTT client for uri {}, clientId {}", serverUri, clientId);
        MqttClientExecutorConfig executorConfig = MqttClientExecutorConfig.builder()
                                                                          .nettyExecutor(scheduledExecutorService)
                                                                          .applicationScheduler(Schedulers.from(scheduledExecutorService))
                                                                          .build();
        ResubscribeHandler resubscribeHandler = new ResubscribeHandler();
        DisconnectedListener disconnectedListener = new DisconnectedListener();
        Mqtt3ClientBuilder clientBuilder = MqttClient.builder()
                                                     .useMqttVersion3()
                                                     .identifier(clientId)
                                                     .serverHost(serverUri.getHost())
                                                     .serverPort(serverUri.getPort())
                                                     // automaticReconnectWithDefaultConfig (see MqttClientAutoReconnectImpl)
                                                     // uses reconnectDelay = delay + randomDelay
                                                     // delay: startDelay: 1s, maxDelay: 120s
                                                     // randomDelay =(long) (delay / 4d / Integer.MAX_VALUE * ThreadLocalRandom.current().nextInt());
                                                     // => maxRandomDelay = 30s
                                                     // => maxReconnectDelay = 150s
                                                     .automaticReconnectWithDefaultConfig()
                                                     .addConnectedListener(resubscribeHandler)
                                                     .addDisconnectedListener(disconnectedListener)
                                                     .executorConfig(executorConfig);
        if (serverUri.getScheme().equals("ssl") || serverUri.getScheme().equals("tls")) {
            clientBuilder.sslWithDefaultConfig();
            setupSslConfig(clientBuilder);
        }
        if (username != null && !username.isEmpty() && password != null && !password.isEmpty()) {
            clientBuilder.simpleAuth()
                         .username(username)
                         .password(password.getBytes(StandardCharsets.UTF_8))
                         .applySimpleAuth();
        }
        Mqtt3RxClient client = clientBuilder.buildRx();
        HivemqMqttClient result = new HivemqMqttClient(client,
                                                       keepAliveTimeSeconds,
                                                       maxMsgSizeBytes,
                                                       cleanSession,
                                                       connectionTimeoutSec,
                                                       reconnectDelayMs,
                                                       isReceiver,
                                                       isSender);
        logger.info("Created MQTT client for uri {}, clientId {}: {}",
                    serverUri,
                    clientId,
                    result.getClientInformationString());
        resubscribeHandler.setClient(result);
        resubscribeHandler.setMqttStatusReceiver(mqttStatusReceiver);
        disconnectedListener.setClientInformationString(result.getClientInformationString());
        disconnectedListener.setMqttStatusReceiver(mqttStatusReceiver);
        return result;
    }

    private void setupSslConfig(Mqtt3ClientBuilder clientBuilder) {
        MqttClientSslConfigBuilder.Nested<? extends Mqtt3ClientBuilder> sslConfig = clientBuilder.sslConfig();
        if (cipherSuiteList != null && cipherSuiteList.size() > 0) {
            for (String cipherSuite : cipherSuiteList) {
                logger.debug("Using cipher suite {}.", cipherSuite);
            }
            sslConfig.cipherSuites(cipherSuiteList);
        } else {
            List<String> cipherSuites = getEnabledCipherSuites();
            for (String cipherSuite : cipherSuites) {
                logger.debug("Using cipher suite {}.", cipherSuite);
            }
            sslConfig.cipherSuites(cipherSuites);
        }

        if (trustStorePath != null && trustStorePWD != null) {
            KeyStore trustStore = getKeystore(trustStorePath, trustStorePWD, trustStoreType);
            logger.info("Setting up trust manager with {} / {} (password omitted)", trustStorePath, trustStoreType);
            if (trustStore != null) {
                try {
                    TrustManagerFactory trustManagerFactory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
                    trustManagerFactory.init(trustStore);
                    sslConfig.trustManagerFactory(trustManagerFactory);
                } catch (NoSuchAlgorithmException | KeyStoreException e) {
                    logger.error("Unable to create trust store factory.", e);
                }
            }
        }
        if (keyStorePath != null && keyStorePWD != null) {
            logger.info("Setting up key manager with {} / {} (password omitted)", keyStorePath, keyStoreType);
            KeyStore keyStore = getKeystore(keyStorePath, keyStorePWD, keyStoreType);
            if (keyStore != null) {
                try {
                    KeyManagerFactory keyManagerFactory = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
                    keyManagerFactory.init(keyStore, keyStorePWD.toCharArray());
                    sslConfig.keyManagerFactory(keyManagerFactory);
                } catch (NoSuchAlgorithmException | KeyStoreException | UnrecoverableKeyException e) {
                    logger.error("Unable to create key manager factory.", e);
                }
            }
        }
        sslConfig.applySslConfig();
    }

    private List<String> getEnabledCipherSuites() {
        try {
            final SSLContext context = SSLContext.getInstance("TLS");
            context.init(null, null, null);
            final SSLEngine sslEngine = context.createSSLEngine();
            return Arrays.asList(sslEngine.getEnabledCipherSuites());
        } catch (Exception e) {
            return Arrays.asList(new String[]{ "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384" });
        }
    }

    private KeyStore getKeystore(String storePath, String storePassword, String storeType) {
        try {
            KeyStore keyStore = KeyStore.getInstance(storeType == null ? KeyStore.getDefaultType() : storeType);
            try (InputStream inputStream = getInputStream(storePath)) {
                keyStore.load(inputStream, storePassword.toCharArray());
            }
            return keyStore;
        } catch (SecurityException | KeyStoreException | IOException | NoSuchAlgorithmException
                | CertificateException e) {
            logger.error("Unable to load keystore from {} / {} (password omitted)", storePath, storeType, e);
        }
        return null;
    }

    private InputStream getInputStream(String path) throws IOException {
        InputStream result = Thread.currentThread().getContextClassLoader().getResourceAsStream(path);
        if (result == null) {
            result = ClassLoader.getSystemClassLoader().getResourceAsStream(path);
        }
        if (result == null) {
            File file = new File(path);
            if (file.exists()) {
                result = new FileInputStream(file);
            }
        }
        if (result == null) {
            try {
                URL url = new URL(path);
                result = url.openStream();
            } catch (MalformedURLException e) {
                logger.debug("Attempt to interpret {} as URL failed. Ignoring.", path);
            }
        }
        return result;
    }

    static class ResubscribeHandler implements MqttClientConnectedListener {

        private HivemqMqttClient client;

        private MqttStatusReceiver mqttStatusReceiver;

        void setClient(HivemqMqttClient client) {
            this.client = client;
        }

        void setMqttStatusReceiver(MqttStatusReceiver mqttStatusReceiver) {
            this.mqttStatusReceiver = mqttStatusReceiver;
        }

        @Override
        public void onConnected(MqttClientConnectedContext context) {
            client.resubscribe();
            mqttStatusReceiver.notifyConnectionStatusChanged(MqttStatusReceiver.ConnectionStatus.CONNECTED);
        }

    }

    static class DisconnectedListener implements MqttClientDisconnectedListener {

        private String clientInformation;

        private MqttStatusReceiver mqttStatusReceiver;

        void setClientInformationString(String clientInformation) {
            this.clientInformation = clientInformation;
        }

        void setMqttStatusReceiver(MqttStatusReceiver mqttStatusReceiver) {
            this.mqttStatusReceiver = mqttStatusReceiver;
        }

        @Override
        public void onDisconnected(MqttClientDisconnectedContext context) {
            logger.info("{}: HiveMQ MQTT client disconnected: source: {}",
                        clientInformation,
                        context.getSource(),
                        context.getCause());
            mqttStatusReceiver.notifyConnectionStatusChanged(MqttStatusReceiver.ConnectionStatus.NOT_CONNECTED);
        }

    }

}
