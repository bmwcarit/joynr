package io.joynr.messaging.mqtt.hivemq.client;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.HashMap;

import com.hivemq.client.mqtt.MqttClientSslConfigBuilder;
import com.hivemq.client.mqtt.lifecycle.MqttClientDisconnectedContext;
import com.hivemq.client.mqtt.lifecycle.MqttClientDisconnectedListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import com.hivemq.client.mqtt.MqttClient;
import com.hivemq.client.mqtt.MqttClientExecutorConfig;
import com.hivemq.client.mqtt.lifecycle.MqttClientConnectedContext;
import com.hivemq.client.mqtt.lifecycle.MqttClientConnectedListener;
import com.hivemq.client.mqtt.mqtt3.Mqtt3ClientBuilder;
import com.hivemq.client.mqtt.mqtt3.Mqtt3RxClient;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.routing.MessageRouter;
import io.reactivex.schedulers.Schedulers;
import joynr.system.RoutingTypes.MqttAddress;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.TrustManagerFactory;

/**
 * This factory class is responsible for producing joynr MQTT clients using the HiveMQ MQTT Client library.
 * The HiveMQ MQTT Client library is explicitly intended for use in backend applications, so using it on
 * constrained devices might not work well. Consider using the Paho client in those circumstances.
 *
 * Here are a list of things that need doing once the relevant missing
 * feature have been completed in HiveMQ MQTT Client and we want to make this integration production ready:
 *
 * TODO
 * - When persistent session configuration exists, then enable configuration thereof
 * - Document usage and configuration of HiveMQ MQTT Client variant
 */
@Singleton
public class HivemqMqttClientFactory implements MqttClientFactory {

    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClientFactory.class);

    private HashMap<String, JoynrMqttClient> receivingMqttClients; // gbid to client
    private HashMap<String, JoynrMqttClient> sendingMqttClients; // gbid to client
    private final MqttAddress ownAddress;
    private final boolean separateConnections;
    private final MqttClientIdProvider mqttClientIdProvider;
    private final ScheduledExecutorService scheduledExecutorService;
    private HashMap<String, String> mqttGbidToBrokerUriMap;
    private HashMap<String, Integer> mqttGbidToKeepAliveTimerSecMap;
    private HashMap<String, Integer> mqttGbidToConnectionTimeoutSecMap;
    private final boolean cleanSession;

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

    @Inject
    public HivemqMqttClientFactory(@Named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                   @Named(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS) boolean separateConnections,
                                   @Named(MqttModule.MQTT_GBID_TO_BROKERURI_MAP) HashMap<String, String> mqttGbidToBrokerUriMap,
                                   @Named(MqttModule.MQTT_TO_KEEP_ALIVE_TIMER_SEC_MAP) HashMap<String, Integer> mqttGbidToKeepAliveTimerSecMap,
                                   @Named(MqttModule.MQTT_GBID_TO_CONNECTION_TIMEOUT_SEC_MAP) HashMap<String, Integer> mqttGbidToConnectionTimeoutSecMap,
                                   @Named(MqttModule.PROPERTY_MQTT_CLEAN_SESSION) boolean cleanSession,
                                   @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduledExecutorService,
                                   MqttClientIdProvider mqttClientIdProvider) {
        this.ownAddress = ownAddress;
        this.mqttGbidToBrokerUriMap = mqttGbidToBrokerUriMap;
        this.mqttGbidToKeepAliveTimerSecMap = mqttGbidToKeepAliveTimerSecMap;
        this.mqttGbidToConnectionTimeoutSecMap = mqttGbidToConnectionTimeoutSecMap;
        this.separateConnections = separateConnections;
        this.scheduledExecutorService = scheduledExecutorService;
        this.mqttClientIdProvider = mqttClientIdProvider;
        sendingMqttClients = new HashMap<>(); // gbid to client
        receivingMqttClients = new HashMap<>(); // gbid to client
        this.cleanSession = cleanSession;
    }

    @Override
    public synchronized JoynrMqttClient createSender(String gbid) {
        if (!sendingMqttClients.containsKey(gbid)) {
            if (separateConnections) {
                logger.info("Creating sender MQTT client for gbid {}", gbid);
                sendingMqttClients.put(gbid, createClient(gbid, mqttClientIdProvider.getClientId() + "Pub"));
                logger.debug("Sender MQTT client for gbid {} now: {}", gbid, sendingMqttClients.get(gbid));
            } else {
                createCombinedClient(gbid);
            }
        }
        return sendingMqttClients.get(gbid);
    }

    @Override
    public synchronized JoynrMqttClient createReceiver(String gbid) {
        if (!receivingMqttClients.containsKey(gbid)) {
            logger.info("Creating receiver MQTT client for gbid {}", gbid);
            if (separateConnections) {
                receivingMqttClients.put(gbid, createClient(gbid, mqttClientIdProvider.getClientId() + "Sub"));
            } else {
                createCombinedClient(gbid);
            }
            logger.debug("Receiver MQTT client for gbid {} now: {}", gbid, receivingMqttClients.get(gbid));
        }
        return receivingMqttClients.get(gbid);
    }

    private void createCombinedClient(String gbid) {
        sendingMqttClients.put(gbid, createClient(gbid, mqttClientIdProvider.getClientId()));
        receivingMqttClients.put(gbid, sendingMqttClients.get(gbid));
    }

    private JoynrMqttClient createClient(String gbid, String clientId) {
        try {
            URI serverUri = new URI(ownAddress.getBrokerUri());
            logger.info("Connecting to {}:{}", serverUri.getHost(), serverUri.getPort());
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
                                                         .automaticReconnectWithDefaultConfig()
                                                         .addConnectedListener(resubscribeHandler)
                                                         .addDisconnectedListener(disconnectedListener)
                                                         .executorConfig(executorConfig);
            if (serverUri.getScheme().equals("ssl") || serverUri.getScheme().equals("tls")) {
                clientBuilder.sslWithDefaultConfig();
                setupSslConfig(clientBuilder);
            }
            Mqtt3RxClient client = clientBuilder.buildRx();
            HivemqMqttClient result = new HivemqMqttClient(client,
                                                           mqttGbidToKeepAliveTimerSecMap.get(gbid),
                                                           cleanSession);
            resubscribeHandler.setClient(result);
            disconnectedListener.setClient(result);
            return result;
        } catch (URISyntaxException e) {
            throw new JoynrIllegalStateException("Invalid MQTT broker URI: " + ownAddress.getBrokerUri(), e);
        }
    }

    private void setupSslConfig(Mqtt3ClientBuilder clientBuilder) {
        MqttClientSslConfigBuilder.Nested<? extends Mqtt3ClientBuilder> sslConfig = clientBuilder.sslConfig();
        if (trustStorePath != null && trustStorePWD != null) {
            KeyStore trustStore = getKeystore(trustStorePath, trustStorePWD, trustStoreType);
            logger.debug("Setting up trust manager with {} / {} (password omitted)", trustStorePath, trustStoreType);
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
            logger.debug("Setting up key manager with {} / {} (password omitted)", keyStorePath, keyStoreType);
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
            ClassLoader.getSystemClassLoader().getResourceAsStream(path);
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

        void setClient(HivemqMqttClient client) {
            this.client = client;
        }

        @Override
        public void onConnected(MqttClientConnectedContext context) {
            client.resubscribe();
        }

    }

    static class DisconnectedListener implements MqttClientDisconnectedListener {

        private HivemqMqttClient client;

        void setClient(HivemqMqttClient client) {
            this.client = client;
        }

        @Override
        public void onDisconnected(MqttClientDisconnectedContext context) {
            logger.info("Hive MQTT Client {} disconnected: {}", client, context);
        }

    }

}
