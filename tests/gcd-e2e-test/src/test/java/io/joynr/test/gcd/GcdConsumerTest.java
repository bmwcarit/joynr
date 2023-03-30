/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.test.gcd;

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
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;

public abstract class GcdConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(GcdConsumerTest.class);

    //public static final String INTER_LANGUAGE_PROVIDER_DOMAIN = "inter-language-test.provider.domain";
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
    protected static GlobalCapabilitiesDirectoryProxy globalCapabilitiesDirectoryProxy;
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
        logger.info("getRuntimeModule: selecting CCInProcessRuntimeModule");
        runtimeModule = new CCInProcessRuntimeModule();

        Module backendTransportModules = Modules.EMPTY_MODULE;

        if (transport.contains("mqtt")) {
            logger.info("getRuntimeModule: using HivemqMqttClientModule");
            backendTransportModules = Modules.combine(backendTransportModules, new HivemqMqttClientModule());
        }

        logger.info("getRuntimeModule: Leaving");
        return Modules.override(runtimeModule).with(backendTransportModules);
    }

    protected static JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        logger.info("getRuntime: Entering");
        providerDomain = "io.joynr";
        logger.info("getRuntime: providerDomain = " + providerDomain);
        Properties appConfig = new Properties();
        //appConfig.setProperty(INTER_LANGUAGE_PROVIDER_DOMAIN, providerDomain);

        Module modulesWithRuntime = getRuntimeModule(joynrConfig);
        GcdDummyApplication application = (GcdDummyApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                         modulesWithRuntime).createApplication(GcdDummyApplication.class,
                                                                                                                               appConfig);

        objectMapper = application.getObjectMapper();
        logger.info("getRuntime: Leaving");
        return application.getRuntime();
    }

    protected static void setupConsumerRuntime(boolean msgQosCompressed) throws DiscoveryException,
                                                                         JoynrIllegalStateException,
                                                                         InterruptedException {
        logger.info("setupConsumerRuntime: Entering");
        final String configFileName = "gcd-consumer-test.settings";

        logger.info("setupConsumerRuntime: Setting up properties #0");
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

        // setup additional properties here
        try {
            joynrConfig.setProperty(MessagingPropertyKeys.CHANNELID, System.getenv("GCD_TEST_CHANNELID"));
            joynrConfig.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, System.getenv("GCD_TEST_BROKERURIS"));
            joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS, System.getenv("GCD_TEST_GBIDS"));
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC,
                                    System.getenv("GCD_TEST_MQTT_CONNECTION_TIMEOUTS_SEC"));
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC,
                                    System.getenv("GCD_TEST_MQTT_KEEP_ALIVE_TIMERS_SEC"));
        } catch (NullPointerException e) {
            throw new JoynrIllegalStateException("Incomplete MQTT base configuration", e);
        }

        // possible TLS settings
        boolean tls_complete = false;
        boolean tls_partially = false;
        try {
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH,
                                    System.getenv("GCD_TEST_MQTT_KEYSTORE_PATH"));
            tls_partially = true;
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH,
                                    System.getenv("GCD_TEST_MQTT_TRUSTSTORE_PATH"));
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_TYPE,
                                    System.getenv("GCD_TEST_MQTT_KEYSTORE_TYPE"));
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE,
                                    System.getenv("GCD_TEST_MQTT_TRUSTSTORE_TYPE"));
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD,
                                    System.getenv("GCD_TEST_MQTT_KEYSTORE_PWD"));
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD,
                                    System.getenv("GCD_TEST_MQTT_TRUSTSTORE_PWD"));
            tls_partially = false;
            tls_complete = true;
        } catch (NullPointerException e) {
            tls_complete = false;
            if (tls_partially) {
                throw new JoynrIllegalStateException("Incomplete TLS configuration", e);
            }
        }
        if (tls_complete) {
            logger.info("setupConsumerRuntime: TLS configuration provided.");
            try {
                joynrConfig.setProperty(MqttModule.MQTT_CIPHERSUITE_LIST, "GCD_TEST_MQTT_CIPHERSUITE_LIST");
            } catch (NullPointerException e) {
                logger.info("setupConsumerRuntime: GCD_TEST_MQTT_CIPHERSUITE_LIST not provided; using default.");
            }
        } else {
            logger.info("setupConsumerRuntime: Assuming Non-TLS configuration.");
        }

        // possible MQTT user/password settings
        boolean user = false;
        boolean password = false;
        try {
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_USERNAME, System.getenv("GCD_TEST_MQTT_USERNAME"));
            user = true;
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_PASSWORD, System.getenv("GCD_TEST_MQTT_PASSWORD"));
            password = true;
        } catch (NullPointerException e) {
        }
        if (user == true && password == false) {
            throw new JoynrIllegalStateException("Incomplete MQTT User/Password configuration");
        } else {
            logger.info("setupConsumerRuntime: no MQTT user/password provided; using plain login.");
        }

        try {
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_DISABLE_HOSTNAME_VERIFICATION,
                                    System.getenv("GCD_MQTT_DISABLE_HOSTNAME_VERIFICATION"));

        } catch (NullPointerException e) {
            logger.info("setupConsumerRuntime: MQTT_DISABLE_HOSTNAME_VERIFICATION not provided; full hostname verification enabled.");
        }

        try {
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS,
                                    System.getenv("GCD_MQTT_RECONNECT_SLEEP_MS"));
        } catch (NullPointerException e) {
            logger.info("setupConsumerRuntime: MQTT_RECONNECT_SLEEP_MS not provided; using default.");
        }

        try {
            joynrConfig.setProperty(MqttModule.PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM,
                                    System.getenv("GCD_MQTT_RECEIVE_MAXIMUM"));
        } catch (NullPointerException e) {
            logger.info("setupConsumerRuntime: MQTT_RECEIVE_MAXIMUM not provided; using default.");
        }

        consumerRuntime = getRuntime(joynrConfig);

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(60000);
        discoveryQos.setRetryIntervalMs(5000);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        ProxyBuilder<GlobalCapabilitiesDirectoryProxy> proxyBuilder = consumerRuntime.getProxyBuilder(providerDomain,
                                                                                                      GlobalCapabilitiesDirectoryProxy.class);

        MessagingQos messagingQos = new MessagingQos(10000);
        messagingQos.setCompress(msgQosCompressed);
        logger.info("setupConsumerRuntime: msgQosCompression = " + msgQosCompressed);

        globalCapabilitiesDirectoryProxy = proxyBuilder.setMessagingQos(messagingQos)
                                                       .setDiscoveryQos(discoveryQos)
                                                       .build(new ProxyCreatedCallback<GlobalCapabilitiesDirectoryProxy>() {
                                                           @Override
                                                           public void onProxyCreationFinished(GlobalCapabilitiesDirectoryProxy result) {
                                                               logger.info("proxy created");
                                                               proxyCreated.release();

                                                           }

                                                           @Override
                                                           public void onProxyCreationError(JoynrRuntimeException error) {
                                                               logger.info("error creating proxy");
                                                           }
                                                       });
        if (globalCapabilitiesDirectoryProxy == null) {
            logger.info("setupConsumerRuntime: proxy = null");
        } else {
            logger.info("setupConsumerRuntime: proxy is set != null");
        }
        // wait until proxy creation is finished or discovery timeout +
        // 1 second grace period have passed
        proxyCreated.tryAcquire(11000, TimeUnit.MILLISECONDS);
    }
}
