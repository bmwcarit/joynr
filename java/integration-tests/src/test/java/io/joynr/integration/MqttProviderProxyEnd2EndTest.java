package io.joynr.integration;

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

import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.Semaphore;

import com.google.inject.Module;
import com.google.inject.util.Modules;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.servlet.ServletUtil;
import joynr.OnChangeSubscriptionQos;
import joynr.tests.testBroadcastInterface;
import joynr.tests.testProxy;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

public class MqttProviderProxyEnd2EndTest extends ProviderProxyEnd2EndTest {

    private Properties mqttConfig;
    private static Process mosquittoProcess;
    private static int mqttBrokerPort;

    @BeforeClass
    public static void startBroker() throws Exception {
        mqttBrokerPort = ServletUtil.findFreePort();
        String path = System.getProperty("path") != null ? System.getProperty("path") : "";
        ProcessBuilder processBuilder = new ProcessBuilder(path + "mosquitto", "-p", Integer.toString(mqttBrokerPort));
        mosquittoProcess = processBuilder.start();
    }

    @AfterClass
    public static void stopBroker() throws Exception {
        mosquittoProcess.destroy();
    }

    @Override
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        mqttConfig = new Properties();
        mqttConfig.put(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://localhost:" + mqttBrokerPort);
        // test is using 2 global address typs, so need to set one of them as primary
        mqttConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        joynrConfig.putAll(mqttConfig);
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(modules);
        Module modulesWithRuntime = Modules.override(runtimeModule).with(new AtmosphereMessagingModule(),
                                                                         new MqttPahoModule());
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);

        return application.getRuntime();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testSimpleMulticast() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        testProxy proxy = consumerRuntime.getProxyBuilder(domain, testProxy.class)
                                         .setMessagingQos(messagingQos)
                                         .setDiscoveryQos(discoveryQos)
                                         .build();
        proxy.subscribeToEmptyBroadcastBroadcast(new testBroadcastInterface.EmptyBroadcastBroadcastAdapter() {
            @Override
            public void onReceive() {
                semaphore.release();
            }
        }, new OnChangeSubscriptionQos());

        // wait to allow the subscription request to arrive at the provider
        Thread.sleep(500);

        provider.fireEmptyBroadcast();
        semaphore.acquire();
    }

    @Ignore
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMulticastWithPartitions() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        testProxy testProxy = consumerRuntime.getProxyBuilder(domain, testProxy.class).setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        final List<String> errors = new ArrayList<>();
        testProxy.subscribeToEmptyBroadcastBroadcast(new testBroadcastInterface.EmptyBroadcastBroadcastAdapter() {
            @Override
            public void onReceive() {
                errors.add("On receive called on listener with no partitions.");
            }
        }, new OnChangeSubscriptionQos());
        testProxy.subscribeToEmptyBroadcastBroadcast(new testBroadcastInterface.EmptyBroadcastBroadcastAdapter() {
            @Override
            public void onReceive() {
                semaphore.release();
            }
        }, new OnChangeSubscriptionQos(), "one", "two", "three");

        // wait to allow the subscription request to arrive at the provider
        Thread.sleep(500);

        provider.fireEmptyBroadcast();
        semaphore.acquire();
        if (errors.size() > 0) {
            fail("Got errors. " + errors);
        }
    }
}
