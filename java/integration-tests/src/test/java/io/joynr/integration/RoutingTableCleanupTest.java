/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.integration;

import static com.google.inject.util.Modules.override;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;

import java.util.Properties;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonProvider;
import io.joynr.messaging.mqtt.MqttMessagingStub;
import io.joynr.messaging.mqtt.MqttMessagingStubFactory;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.websocket.WebSocketClientMessagingStubFactory;
import io.joynr.messaging.websocket.WebSocketMessagingStub;
import io.joynr.messaging.websocket.WebSocketMessagingStubFactory;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@RunWith(MockitoJUnitRunner.class)
public class RoutingTableCleanupTest {

    private final String TESTGBID1 = "testgbid1";
    private final String TESTGBID2 = "testgbid2";
    private final String[] gbids = new String[]{ TESTGBID1, TESTGBID2 };
    private final String TESTDOMAIN = "testDomain";

    private Properties properties;
    private Injector injector;
    // JoynrRuntime to simulate user operations
    private JoynrRuntime joynrRuntime;
    // MessagingSkeletonFactory to simulate incoming messages through messaging skeletons
    private MqttMessagingSkeletonProvider mqttMessagingSkeletonProvider;

    // RoutingTable for verification, use the apply() method to count entries and get info about stored addresses
    private RoutingTable routingTable;

    private DiscoveryQos discoveryQos;
    private ProviderQos providerQos;

    @Mock
    private MqttMessagingStubFactory mqttMessagingStubFactoryMock;
    @Mock
    private WebSocketClientMessagingStubFactory websocketClientMessagingStubFactoryMock;
    @Mock
    private WebSocketMessagingStubFactory webSocketMessagingStubFactoryMock;

    @Mock
    private MqttMessagingStub mqttMessagingStubMock;
    @Mock
    private WebSocketMessagingStub webSocketClientMessagingStubMock;
    @Mock
    private WebSocketMessagingStub webSocketMessagingStubMock;

    @Mock
    private HivemqMqttClientFactory hiveMqMqttClientFactory;

    @Mock
    private JoynrMqttClient joynrMqttClient1;

    @Mock
    private JoynrMqttClient joynrMqttClient2;

    @Before
    public void setUp() throws InterruptedException {
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createReceiver(TESTGBID1);
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createReceiver(TESTGBID2);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createSender(TESTGBID2);

        doReturn(mqttMessagingStubMock).when(mqttMessagingStubFactoryMock).create(any());
        doReturn(webSocketClientMessagingStubMock).when(websocketClientMessagingStubFactoryMock).create(any());
        doReturn(webSocketMessagingStubMock).when(webSocketMessagingStubFactoryMock).create(any());

        properties = createProperties(TESTGBID1 + ", " + TESTGBID2, "tcp://localhost:1883, tcp://otherhost:1883");

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(30000);
        discoveryQos.setRetryIntervalMs(discoveryQos.getDiscoveryTimeoutMs() + 1); // no retry
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        createJoynrRuntime();
    }

    @After
    public void tearDown() {
        shutdownRuntime();
    }

    private void shutdownRuntime() {
        if (joynrRuntime != null) {
            joynrRuntime.shutdown(true);
        }
    }

    private Properties createProperties(String gbids, String brokerUris) {
        Properties properties = new Properties();
        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, brokerUris);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "60,30");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,30");
        properties.put(ConfigurableMessagingSettings.PROPERTY_GBIDS, gbids);
        return properties;
    }

    private CountDownLatch waitForRemoveStale(MqttMessagingStub mqttMessagingStubMock) throws InterruptedException {
        // removeStale is called once at startup because ClusterControllerRuntime calls removeStale
        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer((invocation) -> {
            cdl.countDown();
            return null;
        }).when(mqttMessagingStubMock).transmit(any(), any(), any());
        return cdl;
    }

    protected void createJoynrRuntime() throws InterruptedException {
        shutdownRuntime();

        AbstractModule testBindingsModule = new AbstractModule() {
            @Override
            protected void configure() {
                // Any mqtt clients shall be mocked
                bind(MqttClientFactory.class).toInstance(hiveMqMqttClientFactory);
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY)).toInstance(gbids);
                // We want to mock all MessagingStubs except the InProcess one
                bind(WebSocketClientMessagingStubFactory.class).toInstance(websocketClientMessagingStubFactoryMock);
                bind(WebSocketMessagingStubFactory.class).toInstance(webSocketMessagingStubFactoryMock);
                bind(MqttMessagingStubFactory.class).toInstance(mqttMessagingStubFactoryMock);
                //bind(InProcessMessagingStubFactory.class).toInstance(inProcessMessagingStubFactory);
            }
        };

        injector = Guice.createInjector(override(new CCInProcessRuntimeModule(),
                                                 new HivemqMqttClientModule()).with(new JoynrPropertiesModule(properties),
                                                                                    testBindingsModule));
        CountDownLatch cdl = waitForRemoveStale(mqttMessagingStubMock);

        joynrRuntime = injector.getInstance(JoynrRuntime.class);
        routingTable = injector.getInstance(RoutingTable.class);
        mqttMessagingSkeletonProvider = injector.getInstance(MqttMessagingSkeletonProvider.class);

        assertTrue(cdl.await(10, TimeUnit.SECONDS));
        reset(mqttMessagingStubMock);
    }

    @Test
    public void dummyTest() {
        assertEquals(2, 1 + 1);
        IMessagingSkeleton skeleton = mqttMessagingSkeletonProvider.get()
                                                                   .getSkeleton(new MqttAddress("testgbid1", "test"));
        assertNotNull(skeleton);
    }

}
