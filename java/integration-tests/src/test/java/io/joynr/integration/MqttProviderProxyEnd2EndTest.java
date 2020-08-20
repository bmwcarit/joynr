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
package io.joynr.integration;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.MulticastSubscriptionQos;
import joynr.tests.testBroadcastInterface;
import joynr.tests.testProxy;

public class MqttProviderProxyEnd2EndTest extends AbstractProviderProxyEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(MqttProviderProxyEnd2EndTest.class);
    private static final String MQTT_BROKER_URL = "tcp://localhost:1883";

    private Properties mqttConfig;

    @Override
    protected JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        mqttConfig = new Properties();
        mqttConfig.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, MQTT_BROKER_URL);
        // test is using 2 global address types, so need to set one of them as primary
        mqttConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        mqttConfig.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_MULTICAST, "");
        mqttConfig.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_REPLYTO, "replyto/");
        mqttConfig.put(MessagingPropertyKeys.MQTT_TOPIC_PREFIX_UNICAST, "");
        joynrConfig.putAll(mqttConfig);
        joynrConfig.putAll(baseTestConfig);
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(modules);
        Module modulesWithRuntime = Modules.override(runtimeModule).with(new HivemqMqttClientModule());

        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);

        return application.getRuntime();
    }

    private testProxy buildTestProxy() throws InterruptedException {
        Semaphore proxyCreatedSemaphore = new Semaphore(0);
        testProxy proxy = consumerRuntime.getProxyBuilder(domain, testProxy.class)
                                         .setMessagingQos(messagingQos)
                                         .setDiscoveryQos(discoveryQos)
                                         .build(new ProxyCreatedCallback<testProxy>() {

                                             @Override
                                             public void onProxyCreationFinished(testProxy result) {
                                                 logger.debug("Proxy created successfully for domain: {}", domain);
                                                 proxyCreatedSemaphore.release();
                                             }

                                             @Override
                                             public void onProxyCreationError(JoynrRuntimeException error) {
                                                 logger.error("Proxy creation failed: {}", error);
                                             }
                                         });
        assertTrue(proxyCreatedSemaphore.tryAcquire(60, TimeUnit.SECONDS));
        return proxy;
    }

    private Future<String> subscribeForBroadcastOnTestProxy(testProxy proxy,
                                                            Runnable onReceiveFunction,
                                                            String... partitions) {
        return proxy.subscribeToEmptyBroadcastBroadcast(new testBroadcastInterface.EmptyBroadcastBroadcastAdapter() {
            @Override
            public void onReceive() {
                onReceiveFunction.run();
            }
        }, new MulticastSubscriptionQos(), partitions);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT * 1000)
    public void testLargeByteArray() throws Exception {
        testProxy proxy = buildTestProxy();

        Byte[] largeByteArray = new Byte[1024 * 100];

        for (int i = 0; i < largeByteArray.length; i++) {
            largeByteArray[i] = (byte) (i % 256);
        }

        Byte[] returnValue = proxy.methodWithByteArray(largeByteArray);

        assertArrayEquals(returnValue, largeByteArray);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT * 1000)
    public void testSimpleMulticast() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        testProxy proxy = buildTestProxy();

        subscribeForBroadcastOnTestProxy(proxy, () -> semaphore.release());

        // wait to allow the subscription request to arrive at the provider
        Thread.sleep(500);

        provider.fireEmptyBroadcast();
        assertTrue(semaphore.tryAcquire(1, 60, TimeUnit.SECONDS));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMulticastWithPartitions() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        testProxy testProxy = buildTestProxy();

        final List<String> errors = new ArrayList<>();

        subscribeForBroadcastOnTestProxy(testProxy,
                                         () -> errors.add("On receive called on listener with no partitions."));
        subscribeForBroadcastOnTestProxy(testProxy, () -> semaphore.release(), "one", "two", "three");

        // wait to allow the subscription request to arrive at the provider
        Thread.sleep(500);

        provider.fireEmptyBroadcast("one", "two", "three");
        assertTrue(semaphore.tryAcquire(1, 60, TimeUnit.SECONDS));
        if (errors.size() > 0) {
            fail("Got errors. " + errors);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMulticastWithPartitionsAndMultiLevelWildcard() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        testProxy testProxy = buildTestProxy();

        final List<String> errors = new ArrayList<>();
        Future<String> subscriptionIdOfWildCard = subscribeForBroadcastOnTestProxy(testProxy,
                                                                                   () -> semaphore.release(),
                                                                                   "one",
                                                                                   "*");
        Future<String> subscriptionIdOfOtherTopics = subscribeForBroadcastOnTestProxy(testProxy,
                                                                                      () -> errors.add("Received multicast on partition which wasn't published to: four/five/six"),
                                                                                      "four",
                                                                                      "five",
                                                                                      "six");

        // wait to allow the subscription request to arrive at the provider
        Thread.sleep(500);

        provider.fireEmptyBroadcast("anotherOne");
        provider.fireEmptyBroadcast("one"); // match
        provider.fireEmptyBroadcast("one", "two"); // match
        provider.fireEmptyBroadcast("one", "two", "three"); // match
        provider.fireEmptyBroadcast("one", "two", "three", "four", "five", "six"); // match
        assertTrue(semaphore.tryAcquire(4, 60, TimeUnit.SECONDS));
        if (errors.size() > 0) {
            fail("Got errors. " + errors);
        }

        testProxy.unsubscribeFromEmptyBroadcastBroadcast(subscriptionIdOfWildCard.get());
        testProxy.unsubscribeFromEmptyBroadcastBroadcast(subscriptionIdOfOtherTopics.get());
    }

    @Test
    public void testMulticastWithPartitionsAndSingleLevelWildcard() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        testProxy testProxy = buildTestProxy();

        Future<String> futureOfWildCard = subscribeForBroadcastOnTestProxy(testProxy,
                                                                           () -> semaphore.release(),
                                                                           "one",
                                                                           "+",
                                                                           "three");

        // wait to allow the subscription request to arrive at the provider
        Thread.sleep(500);

        provider.fireEmptyBroadcast("anotherOne");
        provider.fireEmptyBroadcast("one");
        provider.fireEmptyBroadcast("one", "two");
        provider.fireEmptyBroadcast("one", "two", "three"); // match
        provider.fireEmptyBroadcast("one", "two", "three", "four", "five", "six");
        assertTrue(semaphore.tryAcquire(1, 60, TimeUnit.SECONDS));

        testProxy.unsubscribeFromEmptyBroadcastBroadcast(futureOfWildCard.get());
    }

    @Test
    public void testMulticastWithPartitionsAndSingleLevelWildcardAsLastPartition() throws Exception {
        final Semaphore semaphore = new Semaphore(0);
        testProxy testProxy = buildTestProxy();

        Future<String> subscriptionIdOfWildCard = subscribeForBroadcastOnTestProxy(testProxy,
                                                                                   () -> semaphore.release(),
                                                                                   "one",
                                                                                   "+");

        // wait to allow the subscription request to arrive at the provider
        Thread.sleep(500);

        provider.fireEmptyBroadcast("anotherOne");
        provider.fireEmptyBroadcast("one");
        provider.fireEmptyBroadcast("one", "two"); // match
        provider.fireEmptyBroadcast("one", "two", "three");
        provider.fireEmptyBroadcast("one", "two", "three", "four", "five", "six");
        assertTrue(semaphore.tryAcquire(1, 60, TimeUnit.SECONDS));

        testProxy.unsubscribeFromEmptyBroadcastBroadcast(subscriptionIdOfWildCard.get());
    }
}
