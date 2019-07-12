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
package io.joynr.messaging.mqtt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

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
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.runtime.PropertyLoader;

/**
 * Unit tests for {@link DefaultMqttClientIdProvider}.
 */
public class DefaultMqttClientIdProviderTest {

    private String standardReceiverId = "testReceiverId12"; // 16 bytes length
    private String shortReceiverId = "testReceiver";
    private String longReceiverId = "testReceiverId12345678";
    private String clientIdPrefix = "testPrefix-";
    private MqttClientIdProvider clientIdProviderWithoutClientIdPrefix;
    private MqttClientIdProvider clientIdProviderWithClientIdPrefix;
    private MqttClientIdProvider clientIdProviderWithShortReceiverId;
    private MqttClientIdProvider clientIdProviderWithLongReceiverId;

    @Mock
    private MessageRouter mockMessageRouter;

    @Mock
    private RoutingTable mockRoutingTable;

    @Mock
    private MqttClientFactory mqttClientFactory;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        Properties properties = PropertyLoader.loadProperties(MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);

        Module testModule = Modules.override(new MqttModule()).with(new AbstractModule() {
            @Override
            protected void configure() {
                Multibinder.newSetBinder(binder(), JoynrMessageProcessor.class);
                bind(RawMessagingPreprocessor.class).to(NoOpRawMessagingPreprocessor.class);
                bind(MessageRouter.class).toInstance(mockMessageRouter);
                bind(RoutingTable.class).toInstance(mockRoutingTable);
                bind(MqttClientFactory.class).toInstance(mqttClientFactory);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(MessageRouter.SCHEDULEDTHREADPOOL))
                                                    .toInstance(Executors.newScheduledThreadPool(10));
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                    .toInstance(new String[]{ "testGbid" });
            }
        });

        properties.put(MessagingPropertyKeys.RECEIVERID, shortReceiverId);
        Injector injectorWithShortReceiverId = Guice.createInjector(testModule, new JoynrPropertiesModule(properties));
        clientIdProviderWithShortReceiverId = injectorWithShortReceiverId.getInstance(MqttClientIdProvider.class);

        properties.put(MessagingPropertyKeys.RECEIVERID, longReceiverId);
        Injector injectorWithLongReceiverId = Guice.createInjector(testModule, new JoynrPropertiesModule(properties));
        clientIdProviderWithLongReceiverId = injectorWithLongReceiverId.getInstance(MqttClientIdProvider.class);

        properties.put(MessagingPropertyKeys.RECEIVERID, standardReceiverId);
        Injector injectorWithoutClientIdPrefix = Guice.createInjector(testModule,
                                                                      new JoynrPropertiesModule(properties));
        clientIdProviderWithoutClientIdPrefix = injectorWithoutClientIdPrefix.getInstance(MqttClientIdProvider.class);

        properties.put(MqttModule.PROPERTY_KEY_MQTT_CLIENT_ID_PREFIX, clientIdPrefix);
        Injector injectorWithClientIdPrefix = Guice.createInjector(testModule, new JoynrPropertiesModule(properties));
        clientIdProviderWithClientIdPrefix = injectorWithClientIdPrefix.getInstance(MqttClientIdProvider.class);
    }

    @Test
    public void testGetClientIdWithShortReceiverId() {
        String clientId = clientIdProviderWithShortReceiverId.getClientId();
        String expectedClientId = "joynr:" + shortReceiverId;
        assertTrue("shortReceiverId is less than 16 characters long", shortReceiverId.length() < 16);
        assertEquals(expectedClientId, clientId);
    }

    @Test
    public void testGetClientIdWithLongReceiverId() {
        String clientId = clientIdProviderWithLongReceiverId.getClientId();
        String expectedClientId = "joynr:" + longReceiverId.substring(0, Math.min(16, longReceiverId.length()));
        assertTrue("longReceiverId is more than 16 characters long", longReceiverId.length() > 16);
        assertEquals(expectedClientId, clientId);
    }

    @Test
    public void testGetClientIdWithoutPrefix() {
        String clientId = clientIdProviderWithoutClientIdPrefix.getClientId();
        String expectedClientId = "joynr:" + standardReceiverId;
        assertTrue("standardReceiverId is exactly 16 characters long (expected uuid length)",
                   standardReceiverId.length() == 16);
        assertEquals(expectedClientId, clientId);
    }

    @Test
    public void testGetClientIdWithPrefix() {
        String clientId = clientIdProviderWithClientIdPrefix.getClientId();
        String expectedClientId = clientIdPrefix + "joynr:" + standardReceiverId;
        assertEquals(expectedClientId, clientId);
    }

}
