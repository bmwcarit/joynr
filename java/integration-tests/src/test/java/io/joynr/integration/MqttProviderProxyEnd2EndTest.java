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
import java.util.Properties;

import org.junit.AfterClass;
import org.junit.BeforeClass;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.servlet.ServletUtil;

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
        mqttConfig.put(GlobalAddressProvider.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        joynrConfig.putAll(mqttConfig);
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(modules);
        Module modulesWithRuntime = Modules.override(runtimeModule).with(new AtmosphereMessagingModule(),
                                                                         new MqttPahoModule());
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);

        return application.getRuntime();
    }
}
