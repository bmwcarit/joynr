package io.joynr.messaging.mqtt;

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

import static org.junit.Assert.assertEquals;

import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.routing.MessageRouter;

/**
 * Unit tests for {@link DefaultMqttTopicPrefixProvider}.
 */
public class DefaultMqttTopicPrefixProviderTest {

    private MqttTopicPrefixProvider mqttTopicProviderInstance;

    private static final String expectedMulticastPrefix = "multicastPrefix/";
    private static final String expectedReplyToPrefix = "clusterReplyToPrefix/";
    private static final String expectedUnicastPrefix = "unicast/";

    @Mock
    private MessageRouter mockMessageRouter;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        Properties properties = new Properties();
        properties.put(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS, "100");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://localhost:1883");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC, "60");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC, "30");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS, "-1");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "false");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MSGS_INFLIGHT, "10");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES, "0");
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_MULTICAST, expectedMulticastPrefix);
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_REPLYTO, expectedReplyToPrefix);
        properties.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_UNICAST, expectedUnicastPrefix);
        properties.put(ConfigurableMessagingSettings.PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS, "1000");
        properties.put(ConfigurableMessagingSettings.PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE, "20");
        Module testModule = Modules.override(new MqttPahoModule()).with(new AbstractModule() {
            @Override
            protected void configure() {
                Multibinder.newSetBinder(binder(), JoynrMessageProcessor.class);
                bind(RawMessagingPreprocessor.class).to(NoOpRawMessagingPreprocessor.class);
                bind(MessageRouter.class).toInstance(mockMessageRouter);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                    .toInstance(Executors.newScheduledThreadPool(10));
            }
        });
        Injector injector = Guice.createInjector(testModule, new JoynrPropertiesModule(properties));
        mqttTopicProviderInstance = injector.getInstance(MqttTopicPrefixProvider.class);
    }

    @Test
    public void testGetMulticastTopicPrefix() {
        String topicPrefix = mqttTopicProviderInstance.getMulticastTopicPrefix();
        assertEquals(expectedMulticastPrefix, topicPrefix);
    }

    @Test
    public void testGetUnicastTopicPrefix() {
        String topicPrefix = mqttTopicProviderInstance.getUnicastTopicPrefix();
        assertEquals(expectedUnicastPrefix, topicPrefix);
    }

    @Test
    public void testGetReplyToTopicPrefix() {
        String topicPrefix = mqttTopicProviderInstance.getSharedSubscriptionsReplyToTopicPrefix();
        assertEquals(expectedReplyToPrefix, topicPrefix);
    }

}
