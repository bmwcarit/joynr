/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Rule;
import org.junit.rules.TestName;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.util.ObjectMapper;
import joynr.interlanguagetest.TestInterfaceProxy;

public abstract class IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerTest.class);

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
        logger.info("generalTearDown: Entering");
        if (consumerRuntime != null) {
            consumerRuntime.shutdown(true);
        }
        logger.info("generalTearDown: Leaving");
    }

    private static Module getRuntimeModule(Properties joynrConfig) {
        logger.info("getRuntimeModule: Entering");
        Module runtimeModule;
        String transport = System.getProperty("transport");
        if (transport == null) {
            throw new IllegalArgumentException("property \"transport\" not set");
        }
        logger.info("getRuntimeModule: transport = " + transport);
        if (transport.contains("websocket")) {
            logger.info("getRuntimeModule: websocket host = "
                    + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST));
            logger.info("getRuntimeModule: websocket port = "
                    + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT));
            logger.info("getRuntimeModule: websocket protocol = "
                    + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL));
            logger.info("getRuntimeModule: websocket path = "
                    + joynrConfig.getProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH));
            logger.info("getRuntimeModule: selecting LibjoynrWebSocketRuntimeModule");
            runtimeModule = new LibjoynrWebSocketRuntimeModule();
        } else {
            logger.info("getRuntimeModule: selecting CCInProcessRuntimeModule");
            runtimeModule = new CCInProcessRuntimeModule();
        }

        Module backendTransportModules = Modules.EMPTY_MODULE;

        if (transport.contains("mqtt")) {
            logger.info("getRuntimeModule: using HivemqMqttClientModule");
            joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
            backendTransportModules = Modules.combine(backendTransportModules, new HivemqMqttClientModule());
        }

        logger.info("getRuntimeModule: Leaving");
        return Modules.override(runtimeModule).with(backendTransportModules);
    }

    protected static JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        logger.info("getRuntime: Entering");
        providerDomain = joynrConfig.getProperty("provider.domain");
        logger.info("getRuntime: providerDomain = " + providerDomain);
        Properties appConfig = new Properties();
        appConfig.setProperty(INTER_LANGUAGE_PROVIDER_DOMAIN, providerDomain);

        Module modulesWithRuntime = getRuntimeModule(joynrConfig);
        IltDummyApplication application = (IltDummyApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                         modulesWithRuntime).createApplication(IltDummyApplication.class,
                                                                                                                               appConfig);

        objectMapper = application.getObjectMapper();
        logger.info("getRuntime: Leaving");
        return application.getRuntime();
    }

    protected static void setupConsumerRuntime(boolean msgQosCompressed) throws DiscoveryException,
                                                                         JoynrIllegalStateException,
                                                                         InterruptedException {
        logger.info("setupConsumerRuntime: Entering");
        final String configFileName = "ilt-consumer-test.settings";

        InputStream resourceStream;
        try {
            resourceStream = new FileInputStream("src/main/resources/" + configFileName);
        } catch (IOException e) {
            logger.error("setupConsumerRuntime: Error", e);
            resourceStream = null;
        }

        Properties joynrConfig = new Properties();
        try {
            if (resourceStream != null) {
                logger.info("setupConsumerRuntime: resources from " + configFileName);
                joynrConfig.load(resourceStream);
            }
        } catch (IOException ex) {
            logger.info("setupConsumerRuntime: not load the configuration file: " + configFileName + ": " + ex);
        }

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "inter_language_test_consumer_local_domain");

        consumerRuntime = getRuntime(joynrConfig);

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(60000);
        discoveryQos.setRetryIntervalMs(5000);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        ProxyBuilder<TestInterfaceProxy> proxyBuilder = consumerRuntime.getProxyBuilder(providerDomain,
                                                                                        TestInterfaceProxy.class);

        MessagingQos messagingQos = new MessagingQos(10000);
        messagingQos.setCompress(msgQosCompressed);
        logger.info("setupConsumerRuntime: msgQosCompression = " + msgQosCompressed);

        testInterfaceProxy = proxyBuilder.setMessagingQos(messagingQos)
                                         .setDiscoveryQos(discoveryQos)
                                         .build(new ProxyCreatedCallback<TestInterfaceProxy>() {
                                             @Override
                                             public void onProxyCreationFinished(TestInterfaceProxy result) {
                                                 logger.info("proxy created");
                                                 proxyCreated.release();

                                             }

                                             @Override
                                             public void onProxyCreationError(JoynrRuntimeException error) {
                                                 logger.info("error creating proxy");
                                             }
                                         });
        if (testInterfaceProxy == null) {
            logger.info("setupConsumerRuntime: proxy = null");
        } else {
            logger.info("setupConsumerRuntime: proxy is set != null");
        }
        // wait until proxy creation is finished or discovery timeout +
        // 1 second grace period have passed
        proxyCreated.tryAcquire(11000, TimeUnit.MILLISECONDS);
    }
}
