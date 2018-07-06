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
package io.joynr.test.interlanguage;

import java.io.InputStream;
import java.io.FileInputStream;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
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

import org.junit.Rule;
import org.junit.rules.TestName;

public abstract class IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerTest.class);

    public static final String INTER_LANGUAGE_PROVIDER_DOMAIN = "inter-language-test.provider.domain";
    private static final String STATIC_PERSISTENCE_FILE = "java-consumer.persistence_file";

    protected static final String TEST_FAILED = " - FAILED";
    protected static final String TEST_FAILED_CALLBACK_ERROR = TEST_FAILED + " - callback reported error";
    protected static final String TEST_FAILED_CALLBACK_EXCEPTION = TEST_FAILED + " - callback - caught exception";
    protected static final String TEST_FAILED_CALLBACK_INVALID_RESULT = TEST_FAILED
            + " - got invalid result from callback";
    protected static final String TEST_FAILED_CALLBACK_TIMEOUT = TEST_FAILED + " - callback not received in time";
    protected static final String TEST_FAILED_EXCEPTION = TEST_FAILED + " - caught unexpected exception: ";
    protected static final String TEST_FAILED_INVALID_RESULT = TEST_FAILED + " - got invalid result";
    protected static final String TEST_FAILED_NO_RESULT = TEST_FAILED + " - got no result";
    protected static final String TEST_WAIT_FOR_CALLBACK = " - about to wait for callback";
    protected static final String TEST_WAIT_FOR_CALLBACK_DONE = " - wait for callback is over";
    protected static final String TEST_SUCCEEDED = " - OK";

    @Rule
    public TestName name = new TestName();

    private static String providerDomain;
    private static Semaphore proxyCreated = new Semaphore(0);
    protected static TestInterfaceProxy testInterfaceProxy;
    private static JoynrRuntime consumerRuntime;

    protected static ObjectMapper objectMapper;

    public static void generalTearDown() throws InterruptedException {
        LOG.info("generalTearDown: Entering");
        if (consumerRuntime != null) {
            consumerRuntime.shutdown(true);
        }
        LOG.info("generalTearDown: Leaving");
    }

    private static Module getRuntimeModule(Properties joynrConfig) {
        LOG.info("getRuntimeModule: Entering");
        Module runtimeModule;
        String transport = System.getProperty("transport");
        if (transport == null) {
            throw new IllegalArgumentException("property \"transport\" not set");
        }
        LOG.info("getRuntimeModule: transport = " + transport);
        if (transport.contains("websocket")) {
            LOG.info("getRuntimeModule: websocket host = "
                    + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST));
            LOG.info("getRuntimeModule: websocket port = "
                    + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT));
            LOG.info("getRuntimeModule: websocket protocol = "
                    + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL));
            LOG.info("getRuntimeModule: websocket path = "
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
            joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
            backendTransportModules = Modules.combine(backendTransportModules, new MqttPahoModule());
        }

        LOG.info("getRuntimeModule: Leaving");
        return Modules.override(runtimeModule).with(backendTransportModules);
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

    protected static void setupConsumerRuntime(boolean msgQosCompressed) throws DiscoveryException,
                                                                        JoynrIllegalStateException,
                                                                        InterruptedException {
        LOG.info("setupConsumerRuntime: Entering");
        final String configFileName = "ilt-consumer-test.settings";

        InputStream resourceStream;
        try {
            resourceStream = new FileInputStream("src/main/resources/" + configFileName);
        } catch (IOException e) {
            LOG.error("setupConsumerRuntime: Error", e);
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
        discoveryQos.setDiscoveryTimeoutMs(10000);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        ProxyBuilder<TestInterfaceProxy> proxyBuilder = consumerRuntime.getProxyBuilder(providerDomain,
                                                                                        TestInterfaceProxy.class);

        MessagingQos messagingQos = new MessagingQos(10000);
        messagingQos.setCompress(msgQosCompressed);
        LOG.info("setupConsumerRuntime: msgQosCompression = " + msgQosCompressed);

        testInterfaceProxy = proxyBuilder.setMessagingQos(messagingQos)
                                         .setDiscoveryQos(discoveryQos)
                                         .build(new ProxyCreatedCallback<TestInterfaceProxy>() {
                                             @Override
                                             public void onProxyCreationFinished(TestInterfaceProxy result) {
                                                 LOG.info("proxy created");
                                                 proxyCreated.release();

                                             }

                                             @Override
                                             public void onProxyCreationError(JoynrRuntimeException error) {
                                                 LOG.info("error creating proxy");
                                             }
                                         });
        if (testInterfaceProxy == null) {
            LOG.info("setupConsumerRuntime: proxy = null");
        } else {
            LOG.info("setupConsumerRuntime: proxy is set != null");
        }
        // wait until proxy creation is finished or discovery timeout +
        // 1 second grace period have passed
        proxyCreated.tryAcquire(11000, TimeUnit.MILLISECONDS);
    }
}
