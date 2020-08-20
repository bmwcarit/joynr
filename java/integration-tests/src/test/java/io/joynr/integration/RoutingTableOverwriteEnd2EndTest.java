/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;

import java.io.IOException;
import java.util.Properties;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.mockito.Mockito;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.runtime.ProviderRegistrar;
import io.joynr.servlet.ServletUtil;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class RoutingTableOverwriteEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(RoutingTableOverwriteEnd2EndTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private static final long CONST_DEFAULT_TEST_TIMEOUT = 10000;
    private static int mqttBrokerPort = 1883;
    private Properties webSocketConfig;
    private Properties mqttConfig;
    String providerDomain;

    @Before
    public void setUp() throws IOException {
        webSocketConfig = new Properties();
        final int port = ServletUtil.findFreePort();
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        mqttConfig = new Properties();
        mqttConfig.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://localhost:" + mqttBrokerPort);
        mqttConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");

        providerDomain = "testDomain" + System.currentTimeMillis();
    }

    private JoynrRuntime createCcRuntimeInternal(String runtimeId,
                                                 Module module,
                                                 Properties properties,
                                                 Properties additionalProperties) {
        properties.put(MessagingPropertyKeys.CHANNELID, runtimeId);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CLIENT_ID_PREFIX, runtimeId);
        properties.putAll(mqttConfig);

        if (additionalProperties != null) {
            properties.putAll(additionalProperties);
        }

        return new JoynrInjectorFactory(properties, module).getInjector().getInstance(JoynrRuntime.class);
    }

    private JoynrRuntime createCcRuntime(String runtimeId, Properties additionalProperties) {
        Module module = Modules.override(new CCInProcessRuntimeModule()).with(new HivemqMqttClientModule());
        return createCcRuntimeInternal(runtimeId, module, new Properties(), additionalProperties);
    }

    private JoynrRuntime createCcWsRuntime(String runtimeId, Properties additionalProperties) {
        Properties properties = new Properties();
        properties.putAll(webSocketConfig);

        Module module = Modules.override(new CCWebSocketRuntimeModule()).with(new HivemqMqttClientModule());
        return createCcRuntimeInternal(runtimeId, module, new Properties(), additionalProperties);
    }

    private JoynrRuntime createWsRuntime(Properties additionalProperties) {
        Properties properties = new Properties();
        properties.putAll(webSocketConfig);

        if (additionalProperties != null) {
            properties.putAll(additionalProperties);
        }

        return new JoynrInjectorFactory(properties,
                                        new LibjoynrWebSocketRuntimeModule()).getInjector()
                                                                             .getInstance(JoynrRuntime.class);
    }

    private ProviderQos createProviderQos(ProviderScope scope) {
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(scope);
        providerQos.setPriority(System.currentTimeMillis());
        return providerQos;
    }

    private Properties createFixedParticipantIdProperties(String domain,
                                                          Class<? extends AbstractJoynrProvider> clazz,
                                                          String participantId) {
        Properties result = new Properties();
        String interfaceName = ProviderAnnotations.getInterfaceName(clazz);
        int majorVersion = ProviderAnnotations.getMajorVersion(clazz);
        result.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(domain, interfaceName, majorVersion),
                   participantId);
        return result;
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testGlobalAddressCanBeOverwrittenByGlobalAddress() throws Exception {
        // Tests that if a provider's address is changed in the global capabilities directory and a new proxy is
        // created for the provider, the routing table of the proxy's runtime will also be updated as long as
        // a new global lookup (cache max age = 0) is performed.
        final Properties fixedParticipantIdProperty = createFixedParticipantIdProperties(providerDomain,
                                                                                         DefaulttestProvider.class,
                                                                                         "fixedParticipantId");

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setCacheMaxAgeMs(0);

        JoynrRuntime runtimeProxy = createCcRuntime("proxy", null);

        JoynrRuntime runtimeProvider1 = createCcRuntime("provider_initial", fixedParticipantIdProperty);
        DefaulttestProvider provider1 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider1.getProviderRegistrar(providerDomain, provider1)
                        .withProviderQos(createProviderQos(ProviderScope.GLOBAL))
                        .awaitGlobalRegistration()
                        .register()
                        .get();

        Future<testProxy> proxy1Future = new Future<>();
        runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                    .setMessagingQos(new MessagingQos(2000))
                    .setDiscoveryQos(discoveryQos)
                    .build(new ProxyCreatedCallback<testProxy>() {

                        @Override
                        public void onProxyCreationFinished(testProxy result) {
                            proxy1Future.resolve(result);
                        }

                        @Override
                        public void onProxyCreationError(JoynrRuntimeException error) {
                            proxy1Future.onFailure(error);
                        }
                    });
        testProxy proxy1 = proxy1Future.get();

        proxy1.addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);
        reset(provider1);

        JoynrRuntime runtimeProvider2 = createCcRuntime("provider_override", fixedParticipantIdProperty);
        DefaulttestProvider provider2 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider2.getProviderRegistrar(providerDomain, provider2)
                        .withProviderQos(createProviderQos(ProviderScope.GLOBAL))
                        .awaitGlobalRegistration()
                        .register()
                        .get();

        Future<testProxy> proxy2Future = new Future<>();
        runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                    .setMessagingQos(new MessagingQos(2000))
                    .setDiscoveryQos(discoveryQos)
                    .build(new ProxyCreatedCallback<testProxy>() {

                        @Override
                        public void onProxyCreationFinished(testProxy result) {
                            proxy2Future.resolve(result);
                        }

                        @Override
                        public void onProxyCreationError(JoynrRuntimeException error) {
                            proxy2Future.onFailure(error);
                        }
                    });
        testProxy proxy2 = proxy2Future.get();

        proxy2.addNumbers(1, 2, 3);
        verify(provider1, never()).addNumbers(1, 2, 3);
        verify(provider2).addNumbers(1, 2, 3);

        // cleanup
        runtimeProvider1.unregisterProvider(providerDomain, provider1);
        runtimeProvider2.unregisterProvider(providerDomain, provider2);
        // wait grace period for the unregister (remove) message to get
        // sent to global discovery
        Thread.sleep(1000);
        runtimeProvider1.shutdown(true);
        runtimeProvider2.shutdown(true);
        runtimeProxy.shutdown(true);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testWebSocketClientAddressCanBeOverwrittenByWebSocketClientAddress() throws Exception {
        // Tests that after a WebSocketClient provider's address is changed in the local cluster controller,
        // the proxy communicates only with the new provider
        final Properties fixedParticipantIdProperty = createFixedParticipantIdProperties(providerDomain,
                                                                                         DefaulttestProvider.class,
                                                                                         "fixedParticipantId");

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setCacheMaxAgeMs(0);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        JoynrRuntime runtimeProxy = createCcWsRuntime("proxy", webSocketConfig);

        JoynrRuntime runtimeProvider1 = createWsRuntime(fixedParticipantIdProperty);
        DefaulttestProvider provider1 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider1.getProviderRegistrar(providerDomain, provider1)
                        .withProviderQos(createProviderQos(ProviderScope.LOCAL))
                        .register()
                        .get();

        Future<testProxy> proxy1Future = new Future<>();
        runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                    .setMessagingQos(new MessagingQos(2000))
                    .setDiscoveryQos(discoveryQos)
                    .build(new ProxyCreatedCallback<testProxy>() {

                        @Override
                        public void onProxyCreationFinished(testProxy result) {
                            proxy1Future.resolve(result);
                        }

                        @Override
                        public void onProxyCreationError(JoynrRuntimeException error) {
                            proxy1Future.onFailure(error);
                        }
                    });
        testProxy proxy1 = proxy1Future.get();

        proxy1.addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);
        reset(provider1);

        JoynrRuntime runtimeProvider2 = createWsRuntime(fixedParticipantIdProperty);
        DefaulttestProvider provider2 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider2.getProviderRegistrar(providerDomain, provider2)
                        .withProviderQos(createProviderQos(ProviderScope.LOCAL))
                        .register()
                        .get();

        proxy1.addNumbers(1, 2, 3);
        verify(provider1, never()).addNumbers(1, 2, 3);
        verify(provider2).addNumbers(1, 2, 3);
        reset(provider2);

        testProxy proxy2 = runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                                       .setMessagingQos(new MessagingQos(2000))
                                       .setDiscoveryQos(discoveryQos)
                                       .build();
        proxy2.addNumbers(1, 2, 3);
        verify(provider1, never()).addNumbers(1, 2, 3);
        verify(provider2).addNumbers(1, 2, 3);

        // cleanup
        runtimeProvider1.unregisterProvider(providerDomain, provider1);
        runtimeProvider2.unregisterProvider(providerDomain, provider2);
        // wait grace period for the unregister (remove) message to get
        // sent to global discovery
        Thread.sleep(1000);
        runtimeProvider1.shutdown(true);
        runtimeProvider2.shutdown(true);
        runtimeProxy.shutdown(true);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testWebSocketClientAddressCanNotBeOverwrittenByGlobalAddress() throws Exception {
        // Tests that after a WebSocketClient provider has registered globally
        // - its address in the cc is not overwritten by a global lookup for that provider which would result in a
        //   message loop between the cc and the broker
        // - its address in the cc is not overwritten by another globally registered provider with the same participantId
        final Properties fixedParticipantIdProperty = createFixedParticipantIdProperties(providerDomain,
                                                                                         DefaulttestProvider.class,
                                                                                         "fixedParticipantId");

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setCacheMaxAgeMs(0);
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        JoynrRuntime runtimeProxy = createCcWsRuntime("proxy", webSocketConfig);

        JoynrRuntime runtimeProvider1 = createWsRuntime(fixedParticipantIdProperty);
        DefaulttestProvider provider1 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider1.getProviderRegistrar(providerDomain, provider1)
                        .withProviderQos(createProviderQos(ProviderScope.GLOBAL))
                        .awaitGlobalRegistration()
                        .register()
                        .get();

        Future<testProxy> proxy1Future = new Future<>();
        runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                    .setMessagingQos(new MessagingQos(2000))
                    .setDiscoveryQos(discoveryQos)
                    .build(new ProxyCreatedCallback<testProxy>() {

                        @Override
                        public void onProxyCreationFinished(testProxy result) {
                            proxy1Future.resolve(result);
                        }

                        @Override
                        public void onProxyCreationError(JoynrRuntimeException error) {
                            proxy1Future.onFailure(error);
                        }
                    });
        testProxy proxy1 = proxy1Future.get();

        proxy1.addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);
        reset(provider1);

        JoynrRuntime runtimeProvider2 = createCcRuntime("provider2", fixedParticipantIdProperty);
        DefaulttestProvider provider2 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider2.getProviderRegistrar(providerDomain, provider2)
                        .withProviderQos(createProviderQos(ProviderScope.GLOBAL))
                        .awaitGlobalRegistration()
                        .register()
                        .get();

        proxy1.addNumbers(1, 2, 3);
        verify(provider2, never()).addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);
        reset(provider1);

        testProxy proxy2 = runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                                       .setMessagingQos(new MessagingQos(2000))
                                       .setDiscoveryQos(discoveryQos)
                                       .build();
        proxy2.addNumbers(1, 2, 3);
        verify(provider2, never()).addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);

        // cleanup
        runtimeProvider1.unregisterProvider(providerDomain, provider1);
        runtimeProvider2.unregisterProvider(providerDomain, provider2);
        // wait grace period for the unregister (remove) message to get
        // sent to global discovery
        Thread.sleep(1000);
        runtimeProvider1.shutdown(true);
        runtimeProvider2.shutdown(true);
        runtimeProxy.shutdown(true);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testInProcessAddressCanNotBeOverwrittenByGlobalAddress() throws Exception {
        // Tests that after a InProcess provider has registered globally
        // - its address in the cc is not overwritten by a global lookup for that provider which would result in a
        //   message loop between the cc and the broker
        // - its address in the cc is not overwritten by another globally registered provider with the same participantId
        final Properties fixedParticipantIdProperty = createFixedParticipantIdProperties(providerDomain,
                                                                                         DefaulttestProvider.class,
                                                                                         "fixedParticipantId");

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setCacheMaxAgeMs(0);
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        Properties propertiesProxyAndProvider1 = new Properties();
        propertiesProxyAndProvider1.putAll(webSocketConfig);
        propertiesProxyAndProvider1.putAll(fixedParticipantIdProperty);
        JoynrRuntime runtimeProxyAndProvider1 = createCcWsRuntime("proxyAndProvider1", propertiesProxyAndProvider1);

        DefaulttestProvider provider1 = Mockito.spy(DefaulttestProvider.class);
        runtimeProxyAndProvider1.getProviderRegistrar(providerDomain, provider1)
                                .withProviderQos(createProviderQos(ProviderScope.GLOBAL))
                                .awaitGlobalRegistration()
                                .register()
                                .get();

        Future<testProxy> proxy1Future = new Future<>();
        runtimeProxyAndProvider1.getProxyBuilder(providerDomain, testProxy.class)
                                .setMessagingQos(new MessagingQos(2000))
                                .setDiscoveryQos(discoveryQos)
                                .build(new ProxyCreatedCallback<testProxy>() {

                                    @Override
                                    public void onProxyCreationFinished(testProxy result) {
                                        proxy1Future.resolve(result);
                                    }

                                    @Override
                                    public void onProxyCreationError(JoynrRuntimeException error) {
                                        proxy1Future.onFailure(error);
                                    }
                                });
        testProxy proxy1 = proxy1Future.get();

        proxy1.addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);
        reset(provider1);

        JoynrRuntime runtimeProvider2 = createCcRuntime("provider2", fixedParticipantIdProperty);
        DefaulttestProvider provider2 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider2.getProviderRegistrar(providerDomain, provider2)
                        .withProviderQos(createProviderQos(ProviderScope.GLOBAL))
                        .awaitGlobalRegistration()
                        .register()
                        .get();

        proxy1.addNumbers(1, 2, 3);
        verify(provider2, never()).addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);
        reset(provider1);

        testProxy proxy2 = runtimeProxyAndProvider1.getProxyBuilder(providerDomain, testProxy.class)
                                                   .setMessagingQos(new MessagingQos(2000))
                                                   .setDiscoveryQos(discoveryQos)
                                                   .build();
        proxy2.addNumbers(1, 2, 3);
        verify(provider2, never()).addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);

        // cleanup
        runtimeProxyAndProvider1.unregisterProvider(providerDomain, provider1);
        runtimeProvider2.unregisterProvider(providerDomain, provider2);
        // wait grace period for the unregister (remove) message to get
        // sent to global discovery
        Thread.sleep(1000);
        runtimeProxyAndProvider1.shutdown(true);
        runtimeProvider2.shutdown(true);
    }
}
