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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.Properties;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.Before;
import org.junit.Test;
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

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;

public class HivemqMqttClientIntegrationTest {
    private static final Logger logger = LoggerFactory.getLogger(HivemqMqttClientIntegrationTest.class);

    private static final int DEFAULT_QOS_LEVEL = 1; // AT_LEAST_ONCE
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
    private RoutingTable mockRoutingTable;
    @Mock
    private MqttClientIdProvider mockMqttClientIdProvider;
    @Mock
    private SuccessAction mockSuccessAction;
    @Mock
    private FailureAction mockFailureAction;
    private Properties properties;
    private byte[] serializedMessage;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        properties = new Properties();
        properties.put(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://localhost:1883");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC, "60");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC, "30");
        serializedMessage = new byte[10];

        doAnswer(new Answer<String>() {
            private AtomicInteger counter = new AtomicInteger();

            @Override
            public String answer(InvocationOnMock invocation) throws Throwable {
                return "HivemqMqttClientTest-" + counter.getAndIncrement() + "_" + System.currentTimeMillis();
            }
        }).when(mockMqttClientIdProvider).getClientId();
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
                bind(RoutingTable.class).toInstance(mockRoutingTable);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                    .toInstance(Executors.newScheduledThreadPool(10));
                bind(RawMessagingPreprocessor.class).to(NoOpRawMessagingPreprocessor.class);
                Multibinder.newSetBinder(binder(), new TypeLiteral<JoynrMessageProcessor>() {
                });
            }
        });

        hivemqMqttClientFactory = injector.getInstance(HivemqMqttClientFactory.class);
    }

    @Test
    public void publishAndReceiveWithTwoClients() throws Exception {
        properties.put(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, "true");
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver();
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);

        clientSender.start();
        clientReceiver.start();

        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    DEFAULT_QOS_LEVEL,
                                    mockSuccessAction,
                                    mockFailureAction);
        verify(mockReceiver, timeout(500).times(1)).transmit(eq(serializedMessage), any(FailureAction.class));

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    @Test
    public void publishMultiThreadedInALoop() throws Exception {
        properties.put(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, "true");
        final int count = 10;
        final AtomicInteger payloadCounter = new AtomicInteger();
        final String topicPrefix = "testTopicMt-" + System.currentTimeMillis() + "-";
        createHivemqMqttClientFactory();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver();
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);

        clientSender.start();
        clientReceiver.start();

        try {
            final int runs = 16;
            for (int run = 1; run <= runs; run++) {
                logger.info("# run {}", run);
                final String topic = topicPrefix + run;

                clientReceiver.subscribe(topic);
                // wait for subscription to be established
                Thread.sleep(128);

                final CountDownLatch threadsLatch = new CountDownLatch(count);
                final CountDownLatch publishedLatch = new CountDownLatch(count);
                final CountDownLatch receivedLatch = new CountDownLatch(count);
                doAnswer(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        receivedLatch.countDown();
                        return null;
                    }
                }).when(mockReceiver).transmit(any(byte[].class), any(FailureAction.class));

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
                                                        DEFAULT_QOS_LEVEL,
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
                if (!publishedLatch.await(5, TimeUnit.SECONDS)) {
                    fail("PublishLatch.await failed in run " + run);
                }
                if (!receivedLatch.await(5, TimeUnit.SECONDS)) {
                    fail("ReceivedLatch failed in run " + run);
                }

                clientReceiver.unsubscribe(topic);
                Thread.sleep(512);

                verify(mockReceiver, times(count)).transmit(any(byte[].class), any(FailureAction.class));
                reset(mockReceiver);
            }
        } catch (Exception e) {
            throw e;
        } finally {
            clientSender.shutdown();
            clientReceiver.shutdown();
        }
    }

    @Test
    public void subscribeMultipleTimes_receivesOnlyOnce() throws Exception {
        properties.put(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, "true");
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver();
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);

        clientSender.start();
        clientReceiver.start();

        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    DEFAULT_QOS_LEVEL,
                                    mockSuccessAction,
                                    mockFailureAction);
        Thread.sleep(512);
        verify(mockReceiver, times(1)).transmit(eq(serializedMessage), any(FailureAction.class));

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    @Test
    public void shutdownTwiceDoesNotThrow() {
        createHivemqMqttClientFactory();
        HivemqMqttClient client = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        client.setMessageListener(mockReceiver2);
        assertFalse(client.isShutdown());
        client.start();

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
        HivemqMqttClient client = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        client.setMessageListener(mockReceiver2);
        client.start();

        client.subscribe(testTopic);
        Thread.sleep(128);
        client.unsubscribe(testTopic);
        Thread.sleep(128);
        client.unsubscribe(testTopic);
        Thread.sleep(128);
        client.shutdown();
        assertTrue(client.isShutdown());
    }

    @Test
    public void subscribeBeforeConnected() throws Exception {
        properties.put(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, "true");
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver();
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);

        clientSender.start();

        clientReceiver.subscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.start();
        // wait for subscription to be established
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    DEFAULT_QOS_LEVEL,
                                    mockSuccessAction,
                                    mockFailureAction);
        verify(mockReceiver, timeout(500).times(1)).transmit(eq(serializedMessage), any(FailureAction.class));

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    @Test
    public void subscribeWhenNotConnected() throws Exception {
        properties.put(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, "true");
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver();
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);

        clientSender.start();
        clientReceiver.start();

        clientReceiver.shutdown();
        clientReceiver.subscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.start();
        // wait for subscription to be established
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    DEFAULT_QOS_LEVEL,
                                    mockSuccessAction,
                                    mockFailureAction);
        verify(mockReceiver, timeout(500).times(1)).transmit(eq(serializedMessage), any(FailureAction.class));

        clientReceiver.unsubscribe(ownTopic);
        clientReceiver.shutdown();
        clientSender.shutdown();
    }

    @Test
    public void receivePublicationFromPreviousSessionWithoutSubscribe() throws Exception {
        properties.put(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, "true");
        createHivemqMqttClientFactory();
        ownTopic = "testTopic-" + System.currentTimeMillis();
        HivemqMqttClient clientSender = (HivemqMqttClient) hivemqMqttClientFactory.createSender();
        HivemqMqttClient clientReceiver = (HivemqMqttClient) hivemqMqttClientFactory.createReceiver();
        assertNotEquals(clientSender, clientReceiver);

        clientReceiver.setMessageListener(mockReceiver);

        clientSender.start();

        clientReceiver.start();
        clientReceiver.subscribe(ownTopic);
        // wait for subscription to be established
        Thread.sleep(128);

        clientReceiver.shutdown();
        // unsubscribe when disconnected to prevent resubscribe on connect
        clientReceiver.unsubscribe(ownTopic);
        Thread.sleep(128);

        clientSender.publishMessage(ownTopic,
                                    serializedMessage,
                                    DEFAULT_QOS_LEVEL,
                                    mockSuccessAction,
                                    mockFailureAction);
        Thread.sleep(128);
        clientReceiver.start();

        verify(mockReceiver, timeout(500).times(1)).transmit(eq(serializedMessage), any(FailureAction.class));

        clientReceiver.unsubscribe(ownTopic);
        Thread.sleep(128);
        clientReceiver.shutdown();
        clientSender.shutdown();
    }

}
