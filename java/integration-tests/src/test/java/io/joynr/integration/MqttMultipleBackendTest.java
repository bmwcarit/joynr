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
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.JoynrVersion;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.paho.client.MqttPahoClientFactory;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.tests.testProxy;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendTest {

    private final String TESTGBID1 = "testgbid1";
    private final String TESTGBID2 = "testgbid2";
    private final String TESTDOMAIN = "testDomain";
    private final String TESTTOPIC = "testTopic";

    private JoynrRuntime joynrRuntime;

    @Mock
    private GlobalCapabilitiesDirectoryClient gcdClient;

    @Mock
    private MqttPahoClientFactory mqttPahoClientFactory;

    @Mock
    private JoynrMqttClient joynrMqttClient1;

    @Mock
    private JoynrMqttClient joynrMqttClient2;

    private GlobalDiscoveryEntry globalDiscoveryEntry1;

    private GlobalDiscoveryEntry globalDiscoveryEntry2;

    @Before
    public void setUp() {
        JoynrVersion joynrVersion = testProxy.class.getAnnotation(JoynrVersion.class);

        doReturn(joynrMqttClient1).when(mqttPahoClientFactory).createReceiver(TESTGBID1);
        doReturn(joynrMqttClient1).when(mqttPahoClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(mqttPahoClientFactory).createReceiver(TESTGBID2);
        doReturn(joynrMqttClient2).when(mqttPahoClientFactory).createSender(TESTGBID2);

        Properties properties = createProperties(TESTGBID1 + ", " + TESTGBID2,
                                                 "tcp://localhost:1883, tcp://otherhost:1883");
        Injector injector = getInjector(properties);
        joynrRuntime = injector.getInstance(JoynrRuntime.class);

        globalDiscoveryEntry1 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry1.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry1.setParticipantId("participantId1");
        globalDiscoveryEntry1.setDomain(TESTDOMAIN);
        MqttAddress mqttAddress1 = new MqttAddress(TESTGBID1, TESTTOPIC);
        globalDiscoveryEntry1.setAddress(CapabilityUtils.serializeAddress(mqttAddress1));

        globalDiscoveryEntry2 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry2.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry2.setParticipantId("participantId2");
        globalDiscoveryEntry2.setDomain(TESTDOMAIN);
        MqttAddress mqttAddress2 = new MqttAddress(TESTGBID2, TESTTOPIC);
        globalDiscoveryEntry2.setAddress(CapabilityUtils.serializeAddress(mqttAddress2));
    }

    @After
    public void tearDown() {
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

    private Injector getInjector(Properties properties) {
        return Guice.createInjector(override(new CCInProcessRuntimeModule(),
                                             new MqttPahoModule()).with(new JoynrPropertiesModule(properties),
                                                                        new AbstractModule() {
                                                                            @Override
                                                                            protected void configure() {
                                                                                bind(MqttClientFactory.class).toInstance(mqttPahoClientFactory);
                                                                                bind(GlobalCapabilitiesDirectoryClient.class).toInstance(gcdClient);
                                                                            }
                                                                        }));
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProxyCall() throws InterruptedException {
        ArgumentCaptor<String> topicCaptor = ArgumentCaptor.forClass(String.class);
        testProxy proxy1 = buildProxyForGlobalDiscoveryEntry(globalDiscoveryEntry1);
        testProxy proxy2 = buildProxyForGlobalDiscoveryEntry(globalDiscoveryEntry2);
        verify(joynrMqttClient1, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());

        Semaphore publishSemaphore = new Semaphore(0);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                publishSemaphore.release();
                return null;
            }
        }).when(joynrMqttClient1).publishMessage(anyString(), any(byte[].class), anyInt());
        proxy1.methodFireAndForgetWithoutParams();
        assertTrue(publishSemaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
        verify(joynrMqttClient1).publishMessage(topicCaptor.capture(), any(byte[].class), anyInt());
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC));
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());

        reset(joynrMqttClient1);
        reset(joynrMqttClient2);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                publishSemaphore.release();
                return null;
            }
        }).when(joynrMqttClient2).publishMessage(anyString(), any(byte[].class), anyInt());
        proxy2.methodFireAndForgetWithoutParams();
        assertTrue(publishSemaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
        verify(joynrMqttClient1, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        verify(joynrMqttClient2).publishMessage(topicCaptor.capture(), any(byte[].class), anyInt());
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC));
    }

    private testProxy buildProxyForGlobalDiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) throws InterruptedException {
        Semaphore semaphore = new Semaphore(0);

        doAnswer(new Answer<Void>() {

            @SuppressWarnings("unchecked")
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Callback<List<GlobalDiscoveryEntry>> callback = (Callback<List<GlobalDiscoveryEntry>>) invocation.getArguments()[0];
                List<GlobalDiscoveryEntry> globalDiscoveryEntryList = new ArrayList<>();
                globalDiscoveryEntryList.add(globalDiscoveryEntry);
                callback.onSuccess(globalDiscoveryEntryList);
                return null;
            }
        }).when(gcdClient)
          .lookup(Matchers.<Callback<List<GlobalDiscoveryEntry>>> any(), any(String[].class), anyString(), anyLong());

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTDOMAIN, testProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        testProxy proxy = proxyBuilder.build(new ProxyCreatedCallback<testProxy>() {

            @Override
            public void onProxyCreationFinished(testProxy result) {
                semaphore.release();

            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("Proxy creation failed: " + error.toString());

            }
        });
        assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        return proxy;
    }

}
