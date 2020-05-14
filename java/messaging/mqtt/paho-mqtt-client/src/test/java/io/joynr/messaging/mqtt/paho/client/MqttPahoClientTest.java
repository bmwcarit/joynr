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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.io.File;
import java.io.PrintWriter;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import org.eclipse.paho.client.mqttv3.MqttException;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.statusmetrics.JoynrStatusMetricsAggregator;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import joynr.system.RoutingTypes.MqttAddress;

public class MqttPahoClientTest {

    private static final String[] gbids = new String[]{ "testGbid" };
    private static final int DEFAULT_QOS_LEVEL = 1;
    private static int mqttBrokerPort;
    private static int mqttSecureBrokerPort;
    private static final String joynrUser = "joynr";
    private static final String joynrPassword = "password";
    private static final String KEYSTORE_PASSWORD = "password";
    private static final boolean NON_SECURE_CONNECTION = false;
    private static Process mosquittoProcess;
    private Injector injector;
    private MqttClientFactory mqttClientFactory;
    private String ownTopic;
    @Mock
    private IMqttMessagingSkeleton mockReceiver;
    @Mock
    private MessageRouter mockMessageRouter;
    @Mock
    private RoutingTable mockRoutingTable;
    @Mock
    private SuccessAction mockSuccessAction;
    @Mock
    private FailureAction mockFailureAction;
    private JoynrMqttClient joynrMqttClient;
    private Properties properties;
    private byte[] serializedMessage;
    private static Path passwordFilePath;
    private static Path configFilePath;

    @Rule
    public ExpectedException thrown = ExpectedException.none();

    @BeforeClass
    public static void startBroker() throws Exception {
        mqttBrokerPort = 2883;
        mqttSecureBrokerPort = 9883;
        String path = System.getProperty("path") != null ? System.getProperty("path") : "";
        passwordFilePath = Files.createTempFile("mosquitto_passwd_", null);
        configFilePath = Files.createTempFile("mosquitto_conf_", null);

        // create mosquitto configuration with referenced password file
        Path cafilePath = Paths.get("/", "data", "ssl-data", "certs", "ca.cert.pem");
        Path certfilePath = Paths.get("/", "data", "ssl-data", "certs", "server.cert.pem");
        Path keyfilePath = Paths.get("/", "data", "ssl-data", "private", "server.key.pem");
        PrintWriter printWriter = new PrintWriter(configFilePath.toFile());
        printWriter.println("max_queued_messages 0");
        printWriter.println("persistence false");
        printWriter.println("listener " + Integer.toString(mqttBrokerPort) + " 127.0.0.1");
        printWriter.println("password_file " + passwordFilePath.toAbsolutePath().toString());
        printWriter.println("listener " + Integer.toString(mqttSecureBrokerPort) + " 127.0.0.1");
        printWriter.println("cafile " + cafilePath.toAbsolutePath().toString());
        printWriter.println("certfile " + certfilePath.toAbsolutePath().toString());
        printWriter.println("keyfile " + keyfilePath.toAbsolutePath().toString());
        printWriter.println("require_certificate true");
        printWriter.println("allow_anonymous false");
        printWriter.close();

        // create mosquitto password file with an entry for user 'joynr'
        File file = passwordFilePath.toFile();
        file.createNewFile();
        ProcessBuilder processBuilder = new ProcessBuilder(path
                + "mosquitto_passwd", "-b", passwordFilePath.toAbsolutePath().toString(), joynrUser, joynrPassword);
        int exitValue = processBuilder.start().waitFor();
        assertEquals(exitValue, 0);

        // start mosquitto with the above config file
        processBuilder = new ProcessBuilder(path + "mosquitto", "-c", configFilePath.toAbsolutePath().toString());
        mosquittoProcess = processBuilder.start();
    }

    @AfterClass
    public static void stopBroker() throws Exception {
        mosquittoProcess.destroy();
        Files.deleteIfExists(configFilePath);
        Files.deleteIfExists(passwordFilePath);
    }

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        properties = new Properties();

        properties.put(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS, "100");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "60");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "30");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS, "-1");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "false");
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_MULTICAST, "");
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_REPLYTO, "");
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_UNICAST, "");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MSGS_INFLIGHT, "100");
        properties.put(MessagingPropertyKeys.CHANNELID, "myChannelId");
        properties.put(LimitAndBackpressureSettings.PROPERTY_MAX_INCOMING_MQTT_REQUESTS, "0");
        properties.put(LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_ENABLED, "false");
        properties.put(LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD, "80");
        properties.put(LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD, "20");
        properties.put(MqttModule.PROPERTY_MQTT_CLEAN_SESSION, "false");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES, "0");
        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:" + mqttBrokerPort);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_USERNAME, joynrUser);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_PASSWORD, joynrPassword);
        properties.put(ConfigurableMessagingSettings.PROPERTY_GBIDS,
                       Arrays.stream(gbids).collect(Collectors.joining(",")));
        serializedMessage = new byte[10];
    }

    @After
    public void tearDown() {
        if (joynrMqttClient != null) {
            joynrMqttClient.shutdown();
        }
    }

    private void createJoynrMqttClient() {
        try {
            createJoynrMqttClient(NON_SECURE_CONNECTION);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Get the path of the test resources
    private static String getResourcePath(String filename) throws URISyntaxException {
        URL resource = ClassLoader.getSystemClassLoader().getResource(filename);
        return resource.getPath();
    }

    private void createJoynrMqttClient(boolean isSecureConnection) {
        joynrMqttClient = createMqttClientWithoutSubscription(isSecureConnection);

        ownTopic = injector.getInstance((Key.get(MqttAddress.class,
                                                 Names.named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS))))
                           .getTopic();
        joynrMqttClient.subscribe(ownTopic);
    }

    private JoynrMqttClient createMqttClientWithoutSubscription() {
        return createMqttClientWithoutSubscription(NON_SECURE_CONNECTION);
    }

    private JoynrMqttClient createMqttClientWithoutSubscription(boolean isSecureConnection) {
        if (isSecureConnection) {
            properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "ssl://localhost:" + mqttSecureBrokerPort);
        } else {
            properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:" + mqttBrokerPort);
        }
        JoynrMqttClient client = createMqttClientInternal();
        final Semaphore startSemaphore = new Semaphore(0);

        Thread thread = new Thread(new Runnable() {
            public void run() {
                try {
                    client.start();
                    startSemaphore.release();
                } catch (Exception e) {
                    // ignore
                }
            }
        });
        thread.start();
        try {
            boolean started = startSemaphore.tryAcquire(2000, TimeUnit.MILLISECONDS);
            if (started) {
                thread.join();
                return client;
            }
        } catch (Exception e) {
            // ignore
        }
        try {
            client.shutdown();
            thread.join();
        } catch (Exception e) {
            // ignore
        }
        throw new JoynrIllegalStateException("failed to start client");
    }

    private JoynrMqttClient createMqttClientInternal() {
        // always create a new Factory because the factory caches its client.
        createMqttClientFactory();

        JoynrMqttClient client = mqttClientFactory.createSender(gbids[0]);
        client.setMessageListener(mockReceiver);
        return client;
    }

    private void createMqttClientFactory() {
        injector = Guice.createInjector(new MqttPahoModule(),
                                        new JoynrPropertiesModule(properties),
                                        new AbstractModule() {
                                            @Override
                                            protected void configure() {
                                                bind(MessageRouter.class).toInstance(mockMessageRouter);
                                                bind(RoutingTable.class).toInstance(mockRoutingTable);
                                                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                                                    .toInstance(Executors.newScheduledThreadPool(10));
                                                bind(RawMessagingPreprocessor.class).to(NoOpRawMessagingPreprocessor.class);
                                                Multibinder.newSetBinder(binder(),
                                                                         new TypeLiteral<JoynrMessageProcessor>() {
                                                                         });
                                                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                                                    .toInstance(gbids);
                                                bind(JoynrStatusMetricsReceiver.class).to(JoynrStatusMetricsAggregator.class);
                                            }
                                        });

        mqttClientFactory = injector.getInstance(MqttClientFactory.class);
    }

    @Test
    public void mqttClientTestWithTwoConnections() throws Exception {
        final boolean separateConnections = true;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, String.valueOf(separateConnections));
        properties.put(MqttModule.PROPERTY_MQTT_CLEAN_SESSION, "true");
        createMqttClientFactory();
        ownTopic = injector.getInstance((Key.get(MqttAddress.class,
                                                 Names.named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS))))
                           .getTopic();
        JoynrMqttClient clientSender = mqttClientFactory.createSender(gbids[0]);
        JoynrMqttClient clientReceiver = mqttClientFactory.createReceiver(gbids[0]);
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);

        clientSender.start();
        clientReceiver.start();

        clientReceiver.subscribe(ownTopic);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    DEFAULT_QOS_LEVEL,
                                    60,
                                    mockSuccessAction,
                                    mockFailureAction);
        verify(mockSuccessAction).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
        verify(mockReceiver, timeout(500).times(1)).transmit(eq(serializedMessage), any(FailureAction.class));

        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    @Test
    public void mqttClientTestWithOneConnection() throws Exception {
        createMqttClientFactory();

        JoynrMqttClient clientSender = mqttClientFactory.createSender(gbids[0]);
        JoynrMqttClient clientReceiver = mqttClientFactory.createReceiver(gbids[0]);

        assertEquals(clientSender, clientReceiver);

        clientSender.start();
        clientReceiver.start();

        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    private void joynrMqttClientPublishAndVerifyReceivedMessage(byte[] serializedMessage) {
        reset(mockSuccessAction);
        joynrMqttClient.publishMessage(ownTopic,
                                       serializedMessage,
                                       DEFAULT_QOS_LEVEL,
                                       60,
                                       mockSuccessAction,
                                       mockFailureAction);
        verify(mockSuccessAction).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
        verify(mockReceiver, timeout(100).times(1)).transmit(eq(serializedMessage), any(FailureAction.class));
    }

    @Test
    public void mqttClientTestWithEnabledMessageSizeCheck() throws Exception {
        final int maxMessageSize = 100;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES, String.valueOf(maxMessageSize));
        createJoynrMqttClient();

        byte[] shortSerializedMessage = new byte[maxMessageSize];
        joynrMqttClientPublishAndVerifyReceivedMessage(shortSerializedMessage);

        byte[] largeSerializedMessage = new byte[maxMessageSize + 1];
        thrown.expect(JoynrMessageNotSentException.class);
        thrown.expectMessage("MQTT Publish failed: maximum allowed message size of " + maxMessageSize
                + " bytes exceeded, actual size is " + largeSerializedMessage.length + " bytes");

        joynrMqttClient.publishMessage(ownTopic,
                                       largeSerializedMessage,
                                       DEFAULT_QOS_LEVEL,
                                       60,
                                       mockSuccessAction,
                                       mockFailureAction);
        verify(mockSuccessAction, times(0)).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
    }

    private void mqttClientTestWithDisabledMessageSizeCheck(boolean isSecureConnection) throws Exception {
        final int initialMessageSize = 100;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES, "0");
        createJoynrMqttClient(isSecureConnection);

        byte[] shortSerializedMessage = new byte[initialMessageSize];
        joynrMqttClientPublishAndVerifyReceivedMessage(shortSerializedMessage);

        byte[] largeSerializedMessage = new byte[initialMessageSize + 1];
        joynrMqttClientPublishAndVerifyReceivedMessage(largeSerializedMessage);
    }

    @Test
    public void mqttClientTestWithDisabledMessageSizeCheckWithoutTls() throws Exception {
        final boolean isSecureConnection = false;
        mqttClientTestWithDisabledMessageSizeCheck(isSecureConnection);
    }

    private void mqttClientTestWithCredentials(boolean expectException) throws Exception {
        final boolean isSecureConnection = false;
        if (expectException) {
            thrown.expect(JoynrIllegalStateException.class);
        }
        mqttClientTestWithDisabledMessageSizeCheck(isSecureConnection);
    }

    @Test
    public void mqttClientTestWithWrongUserAndSomePassword() throws Exception {
        boolean expectException = true;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_USERNAME, "wronguser");
        mqttClientTestWithCredentials(expectException);
    }

    @Test
    public void mqttClientTestWithCorrectUserButWrongPassword() throws Exception {
        boolean expectException = true;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_PASSWORD, "wrongpassword");
        mqttClientTestWithCredentials(expectException);
    }

    @Test
    public void mqttClientTestWithCorrectUserAndCorrectPassword() throws Exception {
        boolean expectException = false;
        mqttClientTestWithCredentials(expectException);
    }

    @Test
    public void mqttClientTestWithEmptyUser() throws Exception {
        final boolean isSecureConnection = false;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_USERNAME, "");
        thrown.expect(JoynrIllegalStateException.class);
        mqttClientTestWithDisabledMessageSizeCheck(isSecureConnection);
    }

    @Test
    public void mqttClientTestWithCorrectUserButEmptyPassword() throws Exception {
        final boolean isSecureConnection = false;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_PASSWORD, "");
        thrown.expect(JoynrIllegalStateException.class);
        mqttClientTestWithDisabledMessageSizeCheck(isSecureConnection);
    }

    @Test
    public void mqttClientTestWithDisabledMessageSizeCheckWithTlsAndDefaultJksStore() throws Exception {
        final String keyStorePath = getResourcePath("clientkeystore.jks");
        final String trustStorePath = getResourcePath("catruststore.jks");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, keyStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, trustStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, KEYSTORE_PASSWORD);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, KEYSTORE_PASSWORD);

        final boolean isSecureConnection = true;
        mqttClientTestWithDisabledMessageSizeCheck(isSecureConnection);
    }

    @Test
    public void mqttClientTestWithDisabledMessageSizeCheckWithTlsAndP12Store() throws Exception {
        final String keyStorePath = getResourcePath("clientkeystore.p12");
        final String trustStorePath = getResourcePath("catruststore.p12");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, keyStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, trustStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_TYPE, "PKCS12");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE, "PKCS12");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, KEYSTORE_PASSWORD);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, KEYSTORE_PASSWORD);

        final boolean isSecureConnection = true;
        mqttClientTestWithDisabledMessageSizeCheck(isSecureConnection);
    }

    private void testCreateMqttClientFailsWithJoynrIllegalArgumentException() {
        final boolean isSecureConnection = true;
        try {
            createJoynrMqttClient(isSecureConnection);
            fail("Expected JoynrIllegalStateException");
        } catch (JoynrIllegalStateException e) {
            // expected behaviour
        }
    }

    @Test
    public void mqttClientTLSCreationFailsIfKeystorePasswordIsWrongOrMissing() throws URISyntaxException {
        final String wrongPassword = "wrongPassword";

        final String keyStorePath = getResourcePath("clientkeystore.jks");
        final String trustStorePath = getResourcePath("catruststore.jks");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, keyStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, trustStorePath);

        // test missing keystore password
        properties.remove(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, KEYSTORE_PASSWORD);

        testCreateMqttClientFailsWithJoynrIllegalArgumentException();

        // test wrong keystore password
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, wrongPassword);
        testCreateMqttClientFailsWithJoynrIllegalArgumentException();
    }

    @Test
    public void mqttClientTLSCreationFailsIfTrustorePasswordIsWrongOrMissing() throws URISyntaxException {
        final String wrongPassword = "wrongPassword";

        final String keyStorePath = getResourcePath("clientkeystore.jks");
        final String trustStorePath = getResourcePath("catruststore.jks");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, keyStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, trustStorePath);

        // test missing truststore password
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, KEYSTORE_PASSWORD);
        properties.remove(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD);

        testCreateMqttClientFailsWithJoynrIllegalArgumentException();

        // test wrong truststore password
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, wrongPassword);
        testCreateMqttClientFailsWithJoynrIllegalArgumentException();
    }

    @Test
    public void mqttClientTLSCreationFailsIfKeystorePathIsWrongOrMissing() throws URISyntaxException {
        final String wrongKeyStorePath = getResourcePath("clientkeystore.jks") + "42";

        final String trustStorePath = getResourcePath("catruststore.jks");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, trustStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, KEYSTORE_PASSWORD);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, KEYSTORE_PASSWORD);

        // test missing keystore path
        properties.remove(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH);

        testCreateMqttClientFailsWithJoynrIllegalArgumentException();

        // test wrong keystore path
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, wrongKeyStorePath);
        testCreateMqttClientFailsWithJoynrIllegalArgumentException();
    }

    @Test
    public void mqttClientTLSCreationFailsIfTrustorePathIsWrongOrMissing() throws URISyntaxException {
        final String wrongTrustStorePath = getResourcePath("catruststore.jks") + "42";

        final String keyStorePath = getResourcePath("clientkeystore.jks");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, keyStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, KEYSTORE_PASSWORD);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, KEYSTORE_PASSWORD);

        // test missing truststore path
        properties.remove(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH);

        testCreateMqttClientFailsWithJoynrIllegalArgumentException();

        // test wrong truststore path
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, wrongTrustStorePath);
        testCreateMqttClientFailsWithJoynrIllegalArgumentException();
    }

    @Test
    public void mqttClientTestWithDisabledCleanSession() throws Exception {
        properties.put(MqttModule.PROPERTY_MQTT_CLEAN_SESSION, "false");
        String topic = "otherTopic";

        // create a MqttClient which was subscribed on the topic and shut it down.
        joynrMqttClient = createMqttClientWithoutSubscription();
        joynrMqttClient.subscribe(topic);
        joynrMqttClient.shutdown();

        // use another MqttClient to publish a message for the first topic
        joynrMqttClient = createMqttClientWithoutSubscription();
        joynrMqttClient.publishMessage(topic,
                                       serializedMessage,
                                       DEFAULT_QOS_LEVEL,
                                       60,
                                       mockSuccessAction,
                                       mockFailureAction);
        verify(mockSuccessAction).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
        Thread.sleep(100);
        joynrMqttClient.shutdown();

        // create a MqttClient and subscribe to the same topic as the first one
        // MqttClient will receive message if cleanSession is disabled
        joynrMqttClient = createMqttClientWithoutSubscription();
        joynrMqttClient.subscribe(topic);

        Thread.sleep(100);
        verify(mockReceiver, atLeast(1)).transmit(eq(serializedMessage), any(FailureAction.class));
    }

    @Test
    public void mqttClientTestWithEnabledCleanSession() throws Exception {
        properties.put(MqttModule.PROPERTY_MQTT_CLEAN_SESSION, "true");
        String topic = "otherTopic1";

        // create a MqttClient which was subscribed on the topic and shut it down.
        joynrMqttClient = createMqttClientWithoutSubscription();
        joynrMqttClient.subscribe(topic);
        joynrMqttClient.shutdown();

        // use another MqttClient to publish a message for the first topic
        joynrMqttClient = createMqttClientWithoutSubscription();
        joynrMqttClient.publishMessage(topic,
                                       serializedMessage,
                                       DEFAULT_QOS_LEVEL,
                                       60,
                                       mockSuccessAction,
                                       mockFailureAction);
        verify(mockSuccessAction).execute();
        verify(mockFailureAction, times(0)).execute(any(Throwable.class));
        Thread.sleep(100);
        joynrMqttClient.shutdown();

        // create a MqttClient and subscribe to the same topic as the first one
        // MqttClient will receive message if cleanSession is disabled
        joynrMqttClient = createMqttClientWithoutSubscription();
        joynrMqttClient.subscribe(topic);

        Thread.sleep(100);
        verify(mockReceiver, times(0)).transmit(eq(serializedMessage), any(FailureAction.class));
    }

    @Test
    public void mqttClientTestResubscriptionWithCleanRestartEnabled() throws Exception {
        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:" + mqttBrokerPort);
        injector = Guice.createInjector(new MqttPahoModule(),
                                        new JoynrPropertiesModule(properties),
                                        new AbstractModule() {

                                            @Override
                                            protected void configure() {
                                                bind(MessageRouter.class).toInstance(mockMessageRouter);
                                                bind(RoutingTable.class).toInstance(mockRoutingTable);
                                                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                                                    .toInstance(Executors.newScheduledThreadPool(10));
                                                bind(RawMessagingPreprocessor.class).to(NoOpRawMessagingPreprocessor.class);
                                                Multibinder.newSetBinder(binder(),
                                                                         new TypeLiteral<JoynrMessageProcessor>() {
                                                                         });
                                                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                                                    .toInstance(gbids);
                                                bind(JoynrStatusMetricsReceiver.class).to(JoynrStatusMetricsAggregator.class);
                                            }
                                        });

        ownTopic = injector.getInstance((Key.get(MqttAddress.class,
                                                 Names.named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS))))
                           .getTopic();

        ScheduledExecutorService scheduledExecutorService = Executors.newScheduledThreadPool(10);
        MqttClientIdProvider mqttClientIdProvider = injector.getInstance(MqttClientIdProvider.class);

        String clientId = mqttClientIdProvider.getClientId();
        String brokerUri = "tcp://localhost:" + mqttBrokerPort;
        int reconnectSleepMs = 100;
        int keepAliveTimerSec = 60;
        int connectionTimeoutSec = 60;
        int timeToWaitMs = -1;
        int maxMsgsInflight = 100;
        int maxMsgSizeBytes = 0;
        boolean cleanSession = true;
        final boolean isReceiver = true;
        final boolean separateConnections = false;
        String username = joynrUser;
        String password = joynrPassword;

        joynrMqttClient = new MqttPahoClient(brokerUri,
                                             clientId,
                                             scheduledExecutorService,
                                             reconnectSleepMs,
                                             keepAliveTimerSec,
                                             connectionTimeoutSec,
                                             timeToWaitMs,
                                             maxMsgsInflight,
                                             maxMsgSizeBytes,
                                             cleanSession,
                                             isReceiver,
                                             separateConnections,
                                             "",
                                             "",
                                             "",
                                             "",
                                             "",
                                             "",
                                             username,
                                             password,
                                             "gbid");

        joynrMqttClient.start();
        joynrMqttClient.setMessageListener(mockReceiver);
        joynrMqttClient.subscribe(ownTopic);

        // manually call disconnect and connectionLost

        MqttPahoClient mqttPahoClient = (MqttPahoClient) joynrMqttClient;
        mqttPahoClient.getMqttClient().disconnect(500);
        MqttException exception = new MqttException(MqttException.REASON_CODE_CLIENT_TIMEOUT);
        mqttPahoClient.connectionLost(exception);

        joynrMqttClientPublishAndVerifyReceivedMessage(serializedMessage);
    }

    @Test
    public void mqttClientTestShutdownIfDisconnectFromMQTT() throws Exception {
        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:1111");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS, "100");
        // create and start client
        final JoynrMqttClient client = createMqttClientInternal();
        final Semaphore semaphoreBeforeStartMethod = new Semaphore(0);
        final Semaphore semaphoreAfterStartMethod = new Semaphore(0);
        final int timeout = 500;
        Runnable myRunnable = new Runnable() {
            @Override
            public void run() {
                semaphoreBeforeStartMethod.release();
                client.start();
                semaphoreAfterStartMethod.release();
            }
        };
        new Thread(myRunnable).start();
        assertTrue(semaphoreBeforeStartMethod.tryAcquire(timeout, TimeUnit.MILLISECONDS));
        // sleep in order to increase the probability of the runnable
        // to be in the sleep part of the start method
        Thread.sleep(timeout);
        // At this point the semaphoreAfterStartMethod is supposed to be not released
        // because we expect to be still in start()
        assertFalse(semaphoreAfterStartMethod.tryAcquire());

        client.shutdown();
        assertTrue(semaphoreAfterStartMethod.tryAcquire(timeout, TimeUnit.MILLISECONDS));
    }

}
