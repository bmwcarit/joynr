/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static com.google.inject.util.Modules.override;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyMap;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Arrays;
import java.util.Collections;
import java.util.Properties;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;
import com.hivemq.client.mqtt.MqttGlobalPublishFilter;
import com.hivemq.client.mqtt.mqtt5.Mqtt5RxClient;
import com.hivemq.client.mqtt.mqtt5.message.connect.connack.Mqtt5ConnAckRestrictions;
import com.hivemq.client.mqtt.mqtt5.message.publish.Mqtt5Publish;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.reactivex.functions.Consumer;

public class HivemqMqttClientIntegrationTest {
    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClientIntegrationTest.class);

    private static final String[] gbids = new String[]{ "testGbid1", "testGbid2", "testGbid3", "testGbid4",
            "testGbids5" };

    private static final int DEFAULT_QOS_LEVEL = 1; // AT_LEAST_ONCE
    private static final int DEFAULT_EXPIRY_INTERVAL_SEC = 60;

    private Injector injector;
    private HivemqMqttClientFactory hivemqMqttClientFactory;
    private String ownTopic;
    @Mock
    private IMqttMessagingSkeleton mockReceiver;
    @Mock
    private IMqttMessagingSkeleton mockReceiver2;
    @Mock
    private MessageRouter mockMessageRouter;
    @Mock
    private MessageProcessedHandler mockMessageProcessedHandler;
    @Mock
    private RoutingTable mockRoutingTable;
    @Mock
    private MqttClientIdProvider mockMqttClientIdProvider;
    @Mock
    private SuccessAction mockSuccessAction;
    @Mock
    private FailureAction mockFailureAction;
    @Mock
    private JoynrStatusMetricsReceiver mockJoynrStatusMetrics;
    private Properties properties;
    private byte[] serializedMessage;

    // Get the path of the test resources
    private static String getResourcePath(String filename) throws URISyntaxException {
        URL resource = ClassLoader.getSystemClassLoader().getResource(filename);
        if (resource == null) {
            return "";
        }
        return resource.getPath();
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        properties = new Properties();
        final String keyStorePath = getResourcePath("clientkeystore.jks");
        final String trustStorePath = getResourcePath("catruststore.jks");
        final String KEY_AND_TRUSTSTORE_PASSWORD = "password";
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH, keyStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH, trustStorePath);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD, KEY_AND_TRUSTSTORE_PASSWORD);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD, KEY_AND_TRUSTSTORE_PASSWORD);
        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS,
                       "tcp://localhost:1883,mqtt://localhost:1883,ssl://localhost:8883,tls://localhost:8883,mqtts://localhost:8883");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECT_ON_START, "true");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,60,60,60,60");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "30,30,30,30,30");
        properties.put(ConfigurableMessagingSettings.PROPERTY_GBIDS,
                       Arrays.stream(gbids).collect(Collectors.joining(",")));
        serializedMessage = new byte[10];

        doAnswer(new Answer<String>() {
            private AtomicInteger counter = new AtomicInteger();

            @Override
            public String answer(InvocationOnMock invocation) throws Throwable {
                return "HivemqMqttClientTest-" + counter.getAndIncrement() + "_" + System.currentTimeMillis();
            }
        }).when(mockMqttClientIdProvider).getClientId();
    }

    @After
    public void tearDown() throws Exception {
        if (hivemqMqttClientFactory != null) {
            hivemqMqttClientFactory.shutdown();
        }
    }

    private void createHivemqMqttClientFactory() {
        injector = Guice.createInjector(override(new HivemqMqttClientModule()).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(MqttClientIdProvider.class).toInstance(mockMqttClientIdProvider);
            }
        }), new JoynrPropertiesModule(properties), new AbstractModule() {
            @Override
            protected void configure() {
                bind(MessageRouter.class).toInstance(mockMessageRouter);
                bind(MessageProcessedHandler.class).toInstance(mockMessageProcessedHandler);
                bind(RoutingTable.class).toInstance(mockRoutingTable);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                    .toInstance(Executors.newScheduledThreadPool(10));
                bind(RawMessagingPreprocessor.class).to(NoOpRawMessagingPreprocessor.class);
                bind(JoynrStatusMetricsReceiver.class).toInstance(mockJoynrStatusMetrics);
                Multibinder.newSetBinder(binder(), new TypeLiteral<JoynrMessageProcessor>() {
                });
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY)).toInstance(gbids);
            }
        });

        hivemqMqttClientFactory = injector.getInstance(HivemqMqttClientFactory.class);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void publishAndReceiveWithTwoClients() throws Exception {
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);
        clientSender.setMessageListener(mockReceiver2);

        clientSender.start();
        clientSender.connect();

        clientReceiver.start();
        clientReceiver.connect();

        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(1000);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    Collections.<String, String> emptyMap(),
                                    DEFAULT_QOS_LEVEL,
                                    DEFAULT_EXPIRY_INTERVAL_SEC,
                                    mockSuccessAction,
                                    mockFailureAction);
        ArgumentCaptor<Mqtt5Publish> publishCaptor = ArgumentCaptor.forClass(Mqtt5Publish.class);
        verify(mockReceiver, timeout(500).times(1)).transmit(publishCaptor.capture(),
                                                             anyMap(),
                                                             any(FailureAction.class));
        assertArrayEquals(serializedMessage, publishCaptor.getValue().getPayloadAsBytes());

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
        verify(mockReceiver2, times(0)).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void publishMultiThreadedInALoop() throws Exception {
        final int count = 10;
        final AtomicInteger payloadCounter = new AtomicInteger();
        final String topicPrefix = "testTopicMt-" + System.currentTimeMillis() + "-";
        createHivemqMqttClientFactory();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);
        clientSender.setMessageListener(mockReceiver2);

        clientSender.start();
        clientSender.connect();

        clientReceiver.start();
        clientReceiver.connect();

        try {
            final int runs = 16;
            for (int run = 1; run <= runs; run++) {
                logger.info("# run {}", run);
                final String topic = topicPrefix + run;

                clientReceiver.subscribe(topic);
                // wait for subscription to be established
                Thread.sleep(1000);

                final CountDownLatch threadsLatch = new CountDownLatch(count);
                final CountDownLatch publishedLatch = new CountDownLatch(count);
                final CountDownLatch receivedLatch = new CountDownLatch(count);
                doAnswer(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        invocation.getArgument(0, Mqtt5Publish.class).acknowledge();
                        receivedLatch.countDown();
                        return null;
                    }
                }).when(mockReceiver).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));

                Object triggerObj = new Object();
                Thread[] threads = new Thread[count];
                for (int i = 0; i < count; i++) {
                    threads[i] = new Thread(new Runnable() {
                        @Override
                        public void run() {
                            final byte[] payload = ("test " + payloadCounter.incrementAndGet()).getBytes();
                            synchronized (triggerObj) {
                                threadsLatch.countDown();
                                try {
                                    triggerObj.wait();
                                } catch (InterruptedException e) {
                                    fail("Thread.wait() FAILED: " + e);
                                }
                            }
                            clientSender.publishMessage(topic,
                                                        payload,
                                                        Collections.<String, String> emptyMap(),
                                                        DEFAULT_QOS_LEVEL,
                                                        DEFAULT_EXPIRY_INTERVAL_SEC,
                                                        mockSuccessAction,
                                                        mockFailureAction);
                            publishedLatch.countDown();
                        }
                    });
                }
                for (Thread t : threads) {
                    t.start();
                }
                threadsLatch.await();
                verify(mockReceiver2, times(0)).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));
                synchronized (triggerObj) {
                    // trigger parallel publish
                    triggerObj.notifyAll();
                }

                for (Thread t : threads) {
                    try {
                        t.join();
                    } catch (InterruptedException e) {
                        fail("Thread.join() FAILED: " + e);
                    }
                }
                assertTrue("PublishLatch.await failed in run " + run, publishedLatch.await(5, TimeUnit.SECONDS));
                assertTrue("ReceivedLatch failed in run " + run, receivedLatch.await(5, TimeUnit.SECONDS));

                clientReceiver.unsubscribe(topic);
                Thread.sleep(1000);

                verify(mockReceiver, times(count)).transmit(any(Mqtt5Publish.class),
                                                            anyMap(),
                                                            any(FailureAction.class));
                verify(mockReceiver2, times(0)).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));
                reset(mockReceiver);
            }
        } catch (Exception e) {
            throw e;
        } finally {
            clientSender.shutdown();
            clientReceiver.shutdown();
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscribeMultipleTimes_receivesOnlyOnce() throws Exception {
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);
        clientSender.setMessageListener(mockReceiver2);

        clientSender.start();
        clientSender.connect();

        clientReceiver.start();
        clientReceiver.connect();

        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(1000);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    Collections.<String, String> emptyMap(),
                                    DEFAULT_QOS_LEVEL,
                                    DEFAULT_EXPIRY_INTERVAL_SEC,
                                    mockSuccessAction,
                                    mockFailureAction);
        Thread.sleep(1000);
        ArgumentCaptor<Mqtt5Publish> publishCaptor = ArgumentCaptor.forClass(Mqtt5Publish.class);
        verify(mockReceiver, times(1)).transmit(publishCaptor.capture(), anyMap(), any(FailureAction.class));
        assertArrayEquals(serializedMessage, publishCaptor.getValue().getPayloadAsBytes());

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
        verify(mockReceiver2, times(0)).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));
    }

    @Test
    public void shutdownTwiceDoesNotThrow() {
        createHivemqMqttClientFactory();
        HivemqMqttClient client = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        client.setMessageListener(mockReceiver2);
        client.start();
        assertFalse(client.isShutdown());
        client.connect();

        assertFalse(client.isShutdown());
        client.shutdown();
        assertTrue(client.isShutdown());
        client.shutdown();
        assertTrue(client.isShutdown());
    }

    @Test
    public void unsubscribeTwiceDoesNotThrow() throws Exception {
        final String testTopic = "HivemqMqttClientTest-topic";
        createHivemqMqttClientFactory();
        HivemqMqttClient client = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        client.setMessageListener(mockReceiver2);
        client.start();
        client.connect();

        client.subscribe(testTopic);
        Thread.sleep(1000);
        client.unsubscribe(testTopic);
        Thread.sleep(1000);
        client.unsubscribe(testTopic);
        Thread.sleep(1000);
        client.shutdown();
        assertTrue(client.isShutdown());
    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscribeBeforeConnected() throws Exception {
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);
        clientSender.setMessageListener(mockReceiver2);

        clientSender.start();
        clientSender.connect();

        clientReceiver.subscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.start();
        clientReceiver.connect();
        // wait for subscription to be established
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    Collections.<String, String> emptyMap(),
                                    DEFAULT_QOS_LEVEL,
                                    DEFAULT_EXPIRY_INTERVAL_SEC,
                                    mockSuccessAction,
                                    mockFailureAction);
        ArgumentCaptor<Mqtt5Publish> publishCaptor = ArgumentCaptor.forClass(Mqtt5Publish.class);
        verify(mockReceiver, timeout(500).times(1)).transmit(publishCaptor.capture(),
                                                             anyMap(),
                                                             any(FailureAction.class));

        assertArrayEquals(serializedMessage, publishCaptor.getValue().getPayloadAsBytes());

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
        verify(mockReceiver2, times(0)).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscribeWhenNotConnected() throws Exception {
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);
        clientSender.setMessageListener(mockReceiver2);

        clientSender.start();
        clientSender.connect();

        clientReceiver.start();
        clientReceiver.connect();

        clientReceiver.shutdown();
        clientReceiver.subscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.start();
        clientReceiver.connect();
        // wait for subscription to be established
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    Collections.<String, String> emptyMap(),
                                    DEFAULT_QOS_LEVEL,
                                    DEFAULT_EXPIRY_INTERVAL_SEC,
                                    mockSuccessAction,
                                    mockFailureAction);
        ArgumentCaptor<Mqtt5Publish> publishCaptor = ArgumentCaptor.forClass(Mqtt5Publish.class);
        verify(mockReceiver, timeout(500).times(1)).transmit(publishCaptor.capture(),
                                                             anyMap(),
                                                             any(FailureAction.class));
        assertArrayEquals(serializedMessage, publishCaptor.getValue().getPayloadAsBytes());

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
        verify(mockReceiver2, times(0)).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void receivePublicationFromPreviousSessionWithoutSubscribe() throws Exception {
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);
        clientSender.setMessageListener(mockReceiver2);

        clientSender.start();
        clientSender.connect();

        clientReceiver.start();
        clientReceiver.connect();
        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(128);

        clientReceiver.shutdown();
        // unsubscribe when disconnected to prevent resubscribe on connect
        clientReceiver.unsubscribe(ownTopic);
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    Collections.<String, String> emptyMap(),
                                    DEFAULT_QOS_LEVEL,
                                    DEFAULT_EXPIRY_INTERVAL_SEC,
                                    mockSuccessAction,
                                    mockFailureAction);
        Thread.sleep(128);

        clientReceiver.start();
        clientReceiver.connect();

        ArgumentCaptor<Mqtt5Publish> publishCaptor = ArgumentCaptor.forClass(Mqtt5Publish.class);
        verify(mockReceiver, timeout(500).times(1)).transmit(publishCaptor.capture(),
                                                             anyMap(),
                                                             any(FailureAction.class));
        assertArrayEquals(serializedMessage, publishCaptor.getValue().getPayloadAsBytes());

        clientReceiver.unsubscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.shutdown();
        clientSender.shutdown();
        verify(mockReceiver2, times(0)).transmit(any(Mqtt5Publish.class), anyMap(), any(FailureAction.class));
    }

    private void setIncomingMessageHandler(Mqtt5RxClient client, Consumer<? super Mqtt5Publish> handler) {
        client.publishes(MqttGlobalPublishFilter.ALL)
              .subscribe(handler, throwable -> fail("Error encountered for publish callback: " + throwable));
    }

    @Test
    public void publishAndReceiveWithMessageExpiryInterval() throws Exception {
        final int expectedExpiryInterval = DEFAULT_EXPIRY_INTERVAL_SEC;
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        CountDownLatch publicationCdl = new CountDownLatch(1);
        setIncomingMessageHandler(clientReceiver.getClient(), mqtt5Publish -> {
            logger.trace("Incoming message {}", mqtt5Publish);
            assertEquals(expectedExpiryInterval, mqtt5Publish.getMessageExpiryInterval().getAsLong());
            publicationCdl.countDown();
        });

        clientSender.setMessageListener(mockReceiver);

        clientSender.start();
        clientSender.connect();

        clientReceiver.setMessageListener(mockReceiver2);

        clientReceiver.start();
        clientReceiver.connect();

        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    Collections.<String, String> emptyMap(),
                                    DEFAULT_QOS_LEVEL,
                                    DEFAULT_EXPIRY_INTERVAL_SEC,
                                    mockSuccessAction,
                                    mockFailureAction);
        assertTrue(publicationCdl.await(10, TimeUnit.SECONDS));
        clientReceiver.unsubscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    @Test
    public void publishWithMessageExpiryInterval_receiveWithDelay() throws Exception {
        final int sleepTimeSec = 1;
        final int expectedExpiryInterval = DEFAULT_EXPIRY_INTERVAL_SEC - sleepTimeSec;
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver(gbids[1]);
        assertNotEquals(clientSender, clientReceiver);

        CountDownLatch cdl = new CountDownLatch(1);
        setIncomingMessageHandler(clientReceiver.getClient(), mqtt5Publish -> {
            logger.trace("Incoming message {}", mqtt5Publish);
            assertTrue(expectedExpiryInterval >= mqtt5Publish.getMessageExpiryInterval().getAsLong());
            assertTrue(expectedExpiryInterval - mqtt5Publish.getMessageExpiryInterval().getAsLong() <= 2); // tolerance: 2sec
            cdl.countDown();
        });

        clientSender.setMessageListener(mockReceiver);
        clientSender.start();
        clientSender.connect();

        clientReceiver.setMessageListener(mockReceiver2);
        clientReceiver.start();
        clientReceiver.connect();

        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(128);

        clientReceiver.shutdown();
        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    Collections.<String, String> emptyMap(),
                                    DEFAULT_QOS_LEVEL,
                                    DEFAULT_EXPIRY_INTERVAL_SEC,
                                    mockSuccessAction,
                                    mockFailureAction);
        // wait some time to let the expiry interval decrease at the broker before receiving the message
        Thread.sleep(sleepTimeSec * 1000);
        clientReceiver.start();
        clientReceiver.connect();

        assertTrue(cdl.await(10, TimeUnit.SECONDS));
        clientReceiver.unsubscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    // Note: In case another broker with configured non-default limit would be available,
    //       this test could be cloned and it could be tested whether the real limit
    //       set for that broker is returned.
    @Test
    public void maxMsgSizeBytesIsSetToNoLimit() throws Exception {
        createHivemqMqttClientFactory();
        HivemqMqttClient client = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[0]);
        client.setMessageListener(mockReceiver2);
        client.start();
        assertFalse(client.isShutdown());
        client.connect();

        Thread.sleep(1000);

        Field maxMsgSizeBytesField = HivemqMqttClient.class.getDeclaredField("maxMsgSizeBytes");
        maxMsgSizeBytesField.setAccessible(true);
        int maxMsgSizeBytes = (int) maxMsgSizeBytesField.get(client);
        assertEquals(Mqtt5ConnAckRestrictions.DEFAULT_MAXIMUM_PACKET_SIZE_NO_LIMIT, maxMsgSizeBytes);

        assertFalse(client.isShutdown());
        client.shutdown();
        assertTrue(client.isShutdown());
    }

    @SuppressWarnings("unchecked")
    @Test
    public void connectionsUseSslAccordingToProtocolPrefix() throws Exception {
        // the following relates to PROPERTY_MQTT_BROKER_URIS defined above
        // tcp://localhost:1883,mqtt://localhost:1883,ssl://localhost:8883,tls://localhost:8883,mqtts://localhost:8883
        final boolean[] expectedSslConfigPresent = new boolean[]{ false, false, true, true, true };

        createHivemqMqttClientFactory();

        assertEquals(gbids.length, expectedSslConfigPresent.length);
        for (int i = 0; i < gbids.length; i++) {
            HivemqMqttClient client = (HivemqMqttClient) hivemqMqttClientFactory.createSender(gbids[i]);
            assertEquals(expectedSslConfigPresent[i], client.getClient().getConfig().getSslConfig().isPresent());
            client.shutdown();
        }
    }
}
