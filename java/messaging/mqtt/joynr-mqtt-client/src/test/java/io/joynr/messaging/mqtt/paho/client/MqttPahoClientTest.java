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

import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.rules.ExpectedException;
import org.junit.Test;
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
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessagingStub;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.routing.MessageRouter;
import joynr.system.RoutingTypes.MqttAddress;

import static org.mockito.Mockito.*;

public class MqttPahoClientTest {

    private static int mqttBrokerPort;
    private static Process mosquittoProcess;
    private Injector injector;
    private MqttClientFactory mqttClientFactory;
    private MqttAddress ownTopic;
    @Mock
    private IMqttMessagingSkeleton mockReceiver;
    @Mock
    private MessageRouter mockMessageRouter;
    private JoynrMqttClient client;
    private Properties properties;

    @Rule
    public ExpectedException thrown = ExpectedException.none();

    @BeforeClass
    public static void startBroker() throws Exception {
        mqttBrokerPort = 1883;
        String path = System.getProperty("path") != null ? System.getProperty("path") : "";
        ProcessBuilder processBuilder = new ProcessBuilder(path + "mosquitto", "-p", Integer.toString(mqttBrokerPort));
        mosquittoProcess = processBuilder.start();
    }

    @AfterClass
    public static void stopBroker() throws Exception {
        mosquittoProcess.destroy();
    }

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        properties = new Properties();
    }

    private void createClient() {
        properties.put(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://localhost:1883");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS, "100");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC, "60");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC, "30");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS, "-1");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "false");
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_MULTICAST, "");
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_REPLYTO, "");
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_UNICAST, "");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MSGS_INFLIGHT, "100");
        properties.put(MessagingPropertyKeys.CHANNELID, "myChannelId");
        properties.put(ConfigurableMessagingSettings.PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS, "1000");
        properties.put(ConfigurableMessagingSettings.PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE, "20");

        injector = Guice.createInjector(new MqttPahoModule(),
                                        new JoynrPropertiesModule(properties),
                                        new AbstractModule() {

                                            @Override
                                            protected void configure() {
                                                bind(MessageRouter.class).toInstance(mockMessageRouter);
                                                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                                                    .toInstance(Executors.newScheduledThreadPool(10));
                                                bind(RawMessagingPreprocessor.class).to(NoOpRawMessagingPreprocessor.class);
                                                Multibinder.newSetBinder(binder(),
                                                                         new TypeLiteral<JoynrMessageProcessor>() {
                                                                         });
                                            }
                                        });
        mqttClientFactory = injector.getInstance(MqttClientFactory.class);
        ownTopic = injector.getInstance((Key.get(MqttAddress.class,
                                                 Names.named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS))));

        client = mqttClientFactory.create();
        client.start();
        client.subscribe(ownTopic.getTopic());
        client.setMessageListener(mockReceiver);
    }

    @After
    public void tearDown() {
        client.shutdown();
    }

    @Test
    public void mqttClientTestWithEnabledMessageSizeCheck() throws Exception {
        final int maxMessageSize = 100;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES, String.valueOf(maxMessageSize));
        createClient();

        byte[] shortSerializedMessage = new byte[maxMessageSize];
        client.publishMessage(ownTopic.getTopic(), shortSerializedMessage);
        verify(mockReceiver, timeout(100).times(1)).transmit(eq(shortSerializedMessage),
                                                             anyInt(),
                                                             eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                                             any(FailureAction.class));

        byte[] largeSerializedMessage = new byte[maxMessageSize + 1];
        thrown.expect(JoynrMessageNotSentException.class);
        thrown.expectMessage("MQTT Publish failed: maximum allowed message size of " + maxMessageSize
                + " bytes exceeded, actual size is " + largeSerializedMessage.length + " bytes");
        client.publishMessage(ownTopic.getTopic(), largeSerializedMessage);
    }

    @Test
    public void mqttClientTestWithDisabledMessageSizeCheck() throws Exception {
        final int initialMessageSize = 100;
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES, "0");
        createClient();

        byte[] shortSerializedMessage = new byte[initialMessageSize];
        client.publishMessage(ownTopic.getTopic(), shortSerializedMessage);
        verify(mockReceiver, timeout(100).times(1)).transmit(eq(shortSerializedMessage),
                                                             anyInt(),
                                                             eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                                             any(FailureAction.class));

        byte[] largeSerializedMessage = new byte[initialMessageSize + 1];
        client.publishMessage(ownTopic.getTopic(), largeSerializedMessage);
        verify(mockReceiver, timeout(100).times(1)).transmit(eq(largeSerializedMessage),
                                                             anyInt(),
                                                             eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                                             any(FailureAction.class));
    }
}
