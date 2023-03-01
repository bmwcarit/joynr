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
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLSession;

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
import com.hivemq.client.mqtt.mqtt5.Mqtt5ClientBuilder;
import com.hivemq.client.mqtt.mqtt5.Mqtt5RxClient;
import com.hivemq.client.mqtt.mqtt5.exceptions.Mqtt5ConnAckException;
import com.hivemq.client.mqtt.mqtt5.exceptions.Mqtt5DisconnectException;
import com.hivemq.client.mqtt.mqtt5.message.connect.connack.Mqtt5ConnAck;
import com.hivemq.client.mqtt.mqtt5.message.disconnect.Mqtt5Disconnect;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.statusmetrics.ConnectionStatusMetricsImpl;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.reactivex.schedulers.Schedulers;
import io.joynr.messaging.MessagingPropertyKeys;

/**
 * This factory class is responsible for producing joynr MQTT clients using the HiveMQ MQTT Client library.
 * <p>
 * TODO
 * - When persistent session configuration exists, then enable configuration thereof
 */
@Singleton
public class HivemqMqttClientFactory implements MqttClientFactory, ShutdownListener {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClientFactory.class);

    private HashMap<String, JoynrMqttClient> receivingRequestClients; // gbid to client
    private HashMap<String, JoynrMqttClient> receivingReplyClients; // gbid to client
    private HashMap<String, JoynrMqttClient> sendingMqttClients; // gbid to client
    private final boolean separateConnections;
    private final MqttClientIdProvider mqttClientIdProvider;
    private final ScheduledExecutorService scheduledExecutorService;
    private HashMap<String, String> mqttGbidToBrokerUriMap;
    private HashMap<String, Integer> mqttGbidToKeepAliveTimerSecMap;
    private HashMap<String, Integer> mqttGbidToConnectionTimeoutSecMap;
    private final boolean cleanSession;
    private final JoynrStatusMetricsReceiver joynrStatusMetricsReceiver;
    private final IHivemqMqttClientTrustManagerFactory trustManagerFactory;
    private boolean separateReplyReceiver;

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

    @Inject(optional = true)
    @Named(MqttModule.PROPERTY_KEY_MQTT_DISABLE_HOSTNAME_VERIFICATION)
    private Boolean disableHostnameVerification = false;

    @Inject
    @Named(MqttModule.MQTT_CIPHERSUITE_LIST)
    private List<String> cipherSuiteList;

    @Inject
    @Named(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS)
    private int reconnectDelayMs;

    @Inject
    @Named(MqttModule.PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM)
    private int receiveMaximum;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public HivemqMqttClientFactory(@Named(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS) boolean separateConnections,
                                   @Named(MqttModule.MQTT_GBID_TO_BROKERURI_MAP) HashMap<String, String> mqttGbidToBrokerUriMap,
                                   @Named(MqttModule.MQTT_TO_KEEP_ALIVE_TIMER_SEC_MAP) HashMap<String, Integer> mqttGbidToKeepAliveTimerSecMap,
                                   @Named(MqttModule.MQTT_GBID_TO_CONNECTION_TIMEOUT_SEC_MAP) HashMap<String, Integer> mqttGbidToConnectionTimeoutSecMap,
                                   @Named(MqttModule.PROPERTY_MQTT_CLEAN_SESSION) boolean cleanSession,
                                   @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduledExecutorService,
                                   @Named(MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER) boolean separateReplyReceiver,
                                   MqttClientIdProvider mqttClientIdProvider,
                                   JoynrStatusMetricsReceiver joynrStatusMetricsReceiver,
                                   ShutdownNotifier shutdownNotifier,
                                   IHivemqMqttClientTrustManagerFactory trustManagerFactory) {
        this.mqttGbidToBrokerUriMap = mqttGbidToBrokerUriMap;
        this.mqttGbidToKeepAliveTimerSecMap = mqttGbidToKeepAliveTimerSecMap;
        this.mqttGbidToConnectionTimeoutSecMap = mqttGbidToConnectionTimeoutSecMap;
        this.separateConnections = separateConnections;
        this.scheduledExecutorService = scheduledExecutorService;
        this.mqttClientIdProvider = mqttClientIdProvider;
        sendingMqttClients = new HashMap<>(); // gbid to client
        receivingRequestClients = new HashMap<>(); // gbid to client
        receivingReplyClients = new HashMap<>(); // gbid to client
        this.cleanSession = cleanSession;
        this.joynrStatusMetricsReceiver = joynrStatusMetricsReceiver;
        this.trustManagerFactory = trustManagerFactory;
        this.separateReplyReceiver = separateReplyReceiver;
        shutdownNotifier.registerForShutdown(this);
    }

    @Override
    public synchronized JoynrMqttClient createSender(String gbid) {
        if (!sendingMqttClients.containsKey(gbid)) {
            if (separateConnections) {
                logger.info("Creating sender MQTT client for gbid {}", gbid);
                sendingMqttClients.put(gbid,
                                       createClient(gbid,
                                                    mqttClientIdProvider.getClientId() + "Pub",
                                                    false,
                                                    true,
                                                    false));
                logger.debug("Sender MQTT client for gbid {} now: {}", gbid, sendingMqttClients.get(gbid));
            } else {
                createCombinedClient(gbid);
            }
        }
        return sendingMqttClients.get(gbid);
    }

    @Override
    public synchronized JoynrMqttClient createReceiver(String gbid) {
        if (!receivingRequestClients.containsKey(gbid)) {
            logger.info("Creating receiver MQTT client for gbid {}", gbid);
            if (separateConnections) {
                receivingRequestClients.put(gbid,
                                            createClient(gbid,
                                                         mqttClientIdProvider.getClientId() + "Sub",
                                                         true,
                                                         false,
                                                         separateReplyReceiver ? false : true));
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
        if (!receivingReplyClients.containsKey(gbid)) {
            logger.info("Creating reply receiver MQTT client for gbid {}", gbid);
            receivingReplyClients.put(gbid,
                                      createClient(gbid,
                                                   mqttClientIdProvider.getClientId() + "SubReply",
                                                   true,
                                                   false,
                                                   true));
            logger.debug("Reply Receiver MQTT client for gbid {} now: {}", gbid, receivingReplyClients.get(gbid));
        }
        return receivingReplyClients.get(gbid);
    }

    @Override
    public synchronized void prepareForShutdown() {
        if (separateConnections) {
            for (JoynrMqttClient client : receivingRequestClients.values()) {
                client.shutdown();
            }
        }
    }

    @Override
    public synchronized void shutdown() {
        logger.debug("shutdown invoked");
        if (separateConnections) {
            for (JoynrMqttClient client : receivingRequestClients.values()) {
                if (!client.isShutdown()) {
                    client.shutdown();
                }
            }
        }
        if (separateReplyReceiver) {
            for (JoynrMqttClient client : receivingReplyClients.values()) {
                client.shutdown();
            }
        }
        for (JoynrMqttClient client : sendingMqttClients.values()) {
            client.shutdown();
        }
        Schedulers.shutdown();
        logger.debug("shutdown finished");
    }

    private void createCombinedClient(String gbid) {
        sendingMqttClients.put(gbid,
                               createClient(gbid,
                                            mqttClientIdProvider.getClientId(),
                                            true,
                                            true,
                                            separateReplyReceiver ? false : true));
        receivingRequestClients.put(gbid, sendingMqttClients.get(gbid));
    }

    private JoynrMqttClient createClient(String gbid,
                                         String clientId,
                                         boolean isReceiver,
                                         boolean isSender,
                                         boolean isReplyReceiver) {
        URI serverUri;
        try {
            serverUri = new URI(mqttGbidToBrokerUriMap.get(gbid));
        } catch (URISyntaxException e) {
            throw new JoynrIllegalStateException("Invalid MQTT broker URI: " + mqttGbidToBrokerUriMap.get(gbid), e);
        }
        logger.info("Creating MQTT client for gbid \"{}\", uri {}, clientId {}", gbid, serverUri, clientId);
        MqttClientExecutorConfig executorConfig = MqttClientExecutorConfig.builder()
                                                                          .nettyExecutor(scheduledExecutorService)
                                                                          .applicationScheduler(Schedulers.from(scheduledExecutorService))
                                                                          .build();
        assert (!isReplyReceiver || isReceiver);
        ConnectionStatusMetricsImpl connectionStatusMetrics = new ConnectionStatusMetricsImpl();
        connectionStatusMetrics.setGbid(gbid);
        connectionStatusMetrics.setSender(isSender);
        connectionStatusMetrics.setReceiver(isReceiver);
        connectionStatusMetrics.setReplyReceiver(isReplyReceiver);
        connectionStatusMetrics.setUrl(mqttGbidToBrokerUriMap.get(gbid));
        joynrStatusMetricsReceiver.addConnectionStatusMetrics(connectionStatusMetrics);
        ResubscribeHandler resubscribeHandler = new ResubscribeHandler(connectionStatusMetrics);
        DisconnectedListener disconnectedListener = new DisconnectedListener(connectionStatusMetrics);
        Mqtt5ClientBuilder clientBuilder = MqttClient.builder()
                                                     .useMqttVersion5()
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
        if (serverUri.getScheme().equals("ssl") || serverUri.getScheme().equals("tls")
                || serverUri.getScheme().equals("mqtts")) {
            clientBuilder.sslWithDefaultConfig();
            setupSslConfig(clientBuilder);
        }
        if (username != null && !username.isEmpty() && password != null && !password.isEmpty()) {
            clientBuilder.simpleAuth()
                         .username(username)
                         .password(password.getBytes(StandardCharsets.UTF_8))
                         .applySimpleAuth();
        }
        Mqtt5RxClient client = clientBuilder.buildRx();
        HivemqMqttClient result = new HivemqMqttClient(client,
                                                       mqttGbidToKeepAliveTimerSecMap.get(gbid),
                                                       cleanSession,
                                                       mqttGbidToConnectionTimeoutSecMap.get(gbid),
                                                       reconnectDelayMs,
                                                       receiveMaximum,
                                                       isReceiver,
                                                       isSender,
                                                       gbid,
                                                       connectionStatusMetrics);
        logger.info("Created MQTT client for gbid {}, uri {}, clientId {}: {}",
                    gbid,
                    serverUri,
                    clientId,
                    result.getClientInformationString());
        resubscribeHandler.setClient(result);
        disconnectedListener.setClientInformationString(result.getClientInformationString());
        return result;
    }

    private void setupSslConfig(Mqtt5ClientBuilder clientBuilder) {
        MqttClientSslConfigBuilder.Nested<? extends Mqtt5ClientBuilder> sslConfig = clientBuilder.sslConfig();
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
                    sslConfig.trustManagerFactory(trustManagerFactory.getTrustManagerFactory(trustStore));
                    if (disableHostnameVerification) {
                        sslConfig.hostnameVerifier(new HostnameVerifier() {
                            public boolean verify(String hostname, SSLSession session) {
                                logger.info("Skipping regular hostname verification");
                                return true;
                            }
                        });
                        logger.info("Hostname verification disabled.");
                    }
                } catch (NoSuchAlgorithmException | KeyStoreException e) {
                    logger.error("Unable to create trust store factory:", e);
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
                    logger.error("Unable to create key manager factory:", e);
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
            logger.error("Unable to load keystore from {} / {} (password omitted):", storePath, storeType, e);
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

        private ConnectionStatusMetricsImpl connectionStatusMetrics;

        void setClient(HivemqMqttClient client) {
            this.client = client;
        }

        public ResubscribeHandler(ConnectionStatusMetricsImpl connectionStatusMetrics) {
            this.connectionStatusMetrics = connectionStatusMetrics;
        }

        @Override
        public void onConnected(MqttClientConnectedContext context) {
            client.resubscribe();
            connectionStatusMetrics.setConnected(true);
        }

    }

    static class DisconnectedListener implements MqttClientDisconnectedListener {

        private String clientInformation;

        private ConnectionStatusMetricsImpl connectionStatusMetrics;

        DisconnectedListener(ConnectionStatusMetricsImpl connectionStatusMetrics) {
            this.connectionStatusMetrics = connectionStatusMetrics;
        }

        void setClientInformationString(String clientInformation) {
            this.clientInformation = clientInformation;
        }

        @Override
        public void onDisconnected(MqttClientDisconnectedContext context) {
            Throwable cause = context.getCause();
            logger.info("{}: HiveMQ MQTT client disconnected: source: {}{}",
                        clientInformation,
                        context.getSource(),
                        getCauseMessage(cause),
                        cause);
            connectionStatusMetrics.setConnected(false);
            connectionStatusMetrics.increaseConnectionDrops();
        }

        private String getCauseMessage(Throwable cause) {
            String message = "";
            if (cause instanceof Mqtt5DisconnectException) {
                Mqtt5DisconnectException disconnectException = (Mqtt5DisconnectException) cause;
                Mqtt5Disconnect disconnect = disconnectException.getMqttMessage();
                message = ", " + disconnect;
            } else if (cause instanceof Mqtt5ConnAckException) {
                Mqtt5ConnAckException mqtt5ConnAckException = (Mqtt5ConnAckException) cause;
                Mqtt5ConnAck connAck = mqtt5ConnAckException.getMqttMessage();
                message = ", " + connAck;
            }
            return message;
        }

    }

}
