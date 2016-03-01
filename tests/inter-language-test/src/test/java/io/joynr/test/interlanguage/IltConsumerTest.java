package io.joynr.test.interlanguage;

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

import java.io.InputStream;
import java.io.FileInputStream;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.runtime.AbstractJoynrApplication;

import java.io.IOException;
import java.util.Properties;

import joynr.interlanguagetest.TestInterfaceProxy;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.rules.TestName;

public abstract class IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerTest.class);
    public static final String INTER_LANGUAGE_PROVIDER_DOMAIN = "inter-language-test.provider.domain";
    private static final String STATIC_PERSISTENCE_FILE = "java-consumer.persistence_file";

    @Rule
    public TestName name = new TestName();

    private static String providerDomain;
    protected static TestInterfaceProxy testInterfaceProxy;
    private static JoynrRuntime consumerRuntime;

    protected static ObjectMapper objectMapper;

    @BeforeClass
    public static void generalSetUp() throws Exception {
        LOG.info("generalSetUp: Entering");
        setupConsumerRuntime();
        LOG.info("generalSetUp: Leaving");
    }

    @Before
    public void setUp() {
    }

    @AfterClass
    public static void generalTearDown() throws InterruptedException {
        LOG.info("generalTearDown: Entering");
        consumerRuntime.shutdown(true);
        LOG.info("generalTearDown: Leaving");
    }

    @After
    public void tearDown() {
    }

    private static Module getRuntimeModule(Properties joynrConfig) {
        LOG.info("getRuntimeModule: Entering");
        Module runtimeModule;
        String transport = joynrConfig.getProperty("transport");
        if (transport != null) {
            LOG.info("getRuntimeModule: transport = " + transport);
            if (transport.contains("websocket")) {
                LOG.info("getRuntimeModule = "
                        + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST));
                LOG.info("getRuntimeModule = "
                        + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT));
                LOG.info("getRuntimeModule = "
                        + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL));
                LOG.info("getRuntimeModule = "
                        + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH));
                LOG.info("getRuntimeModule: selecting LibjoynrWebSocketRuntimeModule");
                runtimeModule = new LibjoynrWebSocketRuntimeModule();
            } else {
                LOG.info("getRuntimeModule: selecting CCInProcessRuntimeModule");
                runtimeModule = new CCInProcessRuntimeModule();
            }

            Module backendTransportModules = Modules.EMPTY_MODULE;
            if (transport.contains("http")) {
                LOG.info("getRuntimeModule: using AtmosphereMessagingModule");
                backendTransportModules = Modules.combine(backendTransportModules, new AtmosphereMessagingModule());
            }

            if (transport.contains("mqtt")) {
                LOG.info("getRuntimeModule: using MqttPahoModule");
                backendTransportModules = Modules.combine(backendTransportModules, new MqttPahoModule());
            }

            LOG.info("getRuntimeModule: Leaving");
            return Modules.override(runtimeModule).with(backendTransportModules);
        } else {
            LOG.info("getRuntimeModule: transport = null");
        }

        LOG.info("getRuntimeModule: Leaving");
        return Modules.override(new CCInProcessRuntimeModule()).with(new AtmosphereMessagingModule());
    }

    protected static JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        LOG.info("getRuntime: Entering");
        providerDomain = joynrConfig.getProperty("provider.domain");
        LOG.info("getRuntime: providerDomain = " + providerDomain);
        Properties appConfig = new Properties();
        appConfig.setProperty(INTER_LANGUAGE_PROVIDER_DOMAIN, providerDomain);

        Module modulesWithRuntime = getRuntimeModule(joynrConfig);
        IltDummyApplication application = (IltDummyApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                         modulesWithRuntime).createApplication(IltDummyApplication.class,
                                                                                                                               appConfig);

        objectMapper = application.getObjectMapper();
        LOG.info("getRuntime: Leaving");
        return application.getRuntime();
    }

    private static void setupConsumerRuntime() throws DiscoveryException, JoynrIllegalStateException,
                                              InterruptedException {
        LOG.info("setupConsumerRuntime: Entering");
        final String configFileName = "ilt-consumer-test.settings";

        InputStream resourceStream;
        try {
            resourceStream = new FileInputStream("src/main/resources/" + configFileName);
        } catch (IOException e) {
            e.printStackTrace();
            resourceStream = null;
        }

        Properties joynrConfig = new Properties();
        try {
            if (resourceStream != null) {
                LOG.info("setupConsumerRuntime: resources from " + configFileName);
                joynrConfig.load(resourceStream);
            }
        } catch (IOException ex) {
            LOG.info("setupConsumerRuntime: not load the configuration file: " + configFileName + ": " + ex);
        }

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "inter_language_test_consumer_local_domain");

        consumerRuntime = getRuntime(joynrConfig);

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeout(10000);
        discoveryQos.setCacheMaxAge(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        ProxyBuilder<TestInterfaceProxy> proxyBuilder = consumerRuntime.getProxyBuilder(providerDomain,
                                                                                        TestInterfaceProxy.class);
        testInterfaceProxy = proxyBuilder.setMessagingQos(new MessagingQos(10000))
                                         .setDiscoveryQos(discoveryQos)
                                         .build();
        if (testInterfaceProxy == null) {
            LOG.info("setupConsumerRuntime: proxy = null");
        } else {
            LOG.info("setupConsumerRuntime: proxy is set != null");
        }
    }
}
