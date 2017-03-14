package io.joynr.messaging.mqtt.paho.client;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttClientFactory;
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
    private IMessaging mockReceiver;
    @Mock
    private MessageRouter mockMessageRouter;
    private JoynrMqttClient client;

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
        Properties properties = new Properties();
        properties.put(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://localhost:1883");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS, "100");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC, "60");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC, "30");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS, "-1");
        properties.put(MessagingPropertyKeys.CHANNELID, "myChannelId");

        injector = Guice.createInjector(new MqttPahoModule(),
                                        new JoynrPropertiesModule(properties),
                                        new AbstractModule() {

                                            @Override
                                            protected void configure() {
                                                bind(MessageRouter.class).toInstance(mockMessageRouter);
                                                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                                                    .toInstance(Executors.newScheduledThreadPool(10));
                                            }
                                        });
        mqttClientFactory = injector.getInstance(MqttClientFactory.class);
        ownTopic = injector.getInstance((Key.get(MqttAddress.class, Names.named(MqttModule.PROPERTY_MQTT_ADDRESS))));

        client = mqttClientFactory.create();
        client.start();
        client.subscribe(ownTopic.getTopic());
    }

    @After
    public void tearDown() {
        client.shutdown();
    }

    @Test
    public void mqqtClientTest() throws Exception {
        client.setMessageListener(mockReceiver);
        String serializedMessage = "test";
        client.publishMessage(ownTopic.getTopic(), serializedMessage);
        verify(mockReceiver, timeout(100).times(1)).transmit(eq(serializedMessage), any(FailureAction.class));
        client.shutdown();
    }

}
