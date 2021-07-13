/*
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
package io.joynr.integration;

import static com.google.inject.util.Modules.override;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;

import java.util.Properties;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.SystemServicesSettings;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public abstract class AbstractMqttMultipleBackendTest {

    protected final String TESTGBID1 = "testgbid1";
    protected final String TESTGBID2 = "testgbid2";
    protected final String[] gbids = new String[]{ TESTGBID1, TESTGBID2 };
    protected final String TESTDOMAIN = "testDomain";
    protected final String TESTTOPIC = "testTopic";

    private Properties properties;
    protected Injector injector;
    protected JoynrRuntime joynrRuntime;

    protected DiscoveryQos discoveryQos;
    protected ProviderQos providerQos;

    @Mock
    protected GlobalCapabilitiesDirectoryClient gcdClient;

    @Mock
    private HivemqMqttClientFactory hiveMqMqttClientFactory;

    @Mock
    protected JoynrMqttClient joynrMqttClient1;

    @Mock
    protected JoynrMqttClient joynrMqttClient2;

    @Before
    public void setUp() throws InterruptedException {
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createReceiver(TESTGBID1);
        doReturn(joynrMqttClient1).when(hiveMqMqttClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createReceiver(TESTGBID2);
        doReturn(joynrMqttClient2).when(hiveMqMqttClientFactory).createSender(TESTGBID2);

        properties = createProperties(TESTGBID1 + ", " + TESTGBID2, "tcp://localhost:1883, tcp://otherhost:1883");

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(30000);
        discoveryQos.setRetryIntervalMs(discoveryQos.getDiscoveryTimeoutMs() + 1); // no retry
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
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
        properties.put(SystemServicesSettings.PROPERTY_CC_REMOVE_STALE_DELAY_MS, String.valueOf(0));
        return properties;
    }

    protected void createJoynrRuntimeWithMockedGcdClient() {
        shutdownRuntime();

        injector = Guice.createInjector(override(new CCInProcessRuntimeModule(),
                                                 new HivemqMqttClientModule()).with(new JoynrPropertiesModule(properties),
                                                                                    new AbstractModule() {
                                                                                        @Override
                                                                                        protected void configure() {
                                                                                            bind(MqttClientFactory.class).toInstance(hiveMqMqttClientFactory);
                                                                                            bind(GlobalCapabilitiesDirectoryClient.class).toInstance(gcdClient);
                                                                                            bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                                                                                                .toInstance(gbids);
                                                                                        }
                                                                                    }));
        joynrRuntime = injector.getInstance(JoynrRuntime.class);
    }

    private CountDownLatch waitForRemoveStale(JoynrMqttClient joynrMqttClient) throws InterruptedException {
        // removeStale is called once at startup because ClusterControllerRuntime calls removeStale
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl.countDown();
                return null;
            }
        }).when(joynrMqttClient).publishMessage(anyString(),
                                                any(byte[].class),
                                                anyInt(),
                                                anyLong(),
                                                any(SuccessAction.class),
                                                any(FailureAction.class));
        return cdl;
    }

    protected void createJoynrRuntime() throws InterruptedException {
        shutdownRuntime();

        injector = Guice.createInjector(override(new CCInProcessRuntimeModule(),
                                                 new HivemqMqttClientModule()).with(new JoynrPropertiesModule(properties),
                                                                                    new AbstractModule() {
                                                                                        @Override
                                                                                        protected void configure() {
                                                                                            bind(MqttClientFactory.class).toInstance(hiveMqMqttClientFactory);
                                                                                            bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                                                                                                .toInstance(gbids);
                                                                                        }
                                                                                    }));
        CountDownLatch cdl1 = waitForRemoveStale(joynrMqttClient1);
        CountDownLatch cdl2 = waitForRemoveStale(joynrMqttClient2);

        joynrRuntime = injector.getInstance(JoynrRuntime.class);

        assertTrue(cdl1.await(10, TimeUnit.SECONDS));
        reset(joynrMqttClient1);
        assertTrue(cdl2.await(10, TimeUnit.SECONDS));
        reset(joynrMqttClient2);
    }

    protected Answer<Void> createVoidCountDownAnswer(CountDownLatch countDownLatch) {
        return new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                countDownLatch.countDown();
                return null;
            }
        };
    }

}
