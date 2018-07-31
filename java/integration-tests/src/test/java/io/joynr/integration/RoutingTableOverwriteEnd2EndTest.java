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

import java.util.Properties;

import org.junit.Rule;
import org.junit.Test;
import org.mockito.Mockito;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class RoutingTableOverwriteEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(RoutingTableOverwriteEnd2EndTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private static int mqttBrokerPort = 1883;

    protected JoynrRuntime createRuntime(String runtimeId, Properties additionalProperties) {
        Properties properties = new Properties();
        properties.put(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI, "tcp://localhost:" + mqttBrokerPort);
        properties.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        properties.put(MessagingPropertyKeys.DISCOVERYDIRECTORYURL, "tcp://localhost:" + mqttBrokerPort);
        properties.put(MessagingPropertyKeys.CHANNELID, runtimeId);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CLIENT_ID_PREFIX, runtimeId);

        if (additionalProperties != null) {
            properties.putAll(additionalProperties);
        }

        Module module = Modules.override(new CCInProcessRuntimeModule()).with(new MqttPahoModule());
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(properties,
                                                                                             module).createApplication(DummyJoynrApplication.class);

        return application.getRuntime();
    }

    protected ProviderQos createProviderQos() {
        final ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
        providerQos.setPriority(System.currentTimeMillis());
        return providerQos;
    }

    protected Properties createFixedParticipantIdProperties(String domain,
                                                            @SuppressWarnings("rawtypes") Class clazz,
                                                            String participantId) {
        Properties result = new Properties();
        String interfaceName = ProviderAnnotations.getInterfaceName(clazz);
        int majorVersion = ProviderAnnotations.getMajorVersion(clazz);
        result.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(domain, interfaceName, majorVersion),
                   participantId);
        return result;
    }

    @Test
    public void testProviderAddressCanBeOverwrittenAfterDiscovery() throws Exception {
        // Tests that if a provider's address is changed in the discovery directory and a new proxy is
        // created for the provider, the routing table of the proxy's runtime will also be updated as long as
        // a new arbitration (cache max age = 0) is performed.
        final String providerDomain = "testDomain";
        final Properties fixedParticipantIdProperty = createFixedParticipantIdProperties(providerDomain,
                                                                                         DefaulttestProvider.class,
                                                                                         "fixedParticipantId");

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setCacheMaxAgeMs(0);

        JoynrRuntime runtimeProxy = createRuntime("proxy", null);

        JoynrRuntime runtimeProvider1 = createRuntime("provider_initial", fixedParticipantIdProperty);
        DefaulttestProvider provider1 = Mockito.spy(DefaulttestProvider.class);
        boolean awaitGlobalRegistration = true;
        runtimeProvider1.registerProvider(providerDomain, provider1, createProviderQos(), awaitGlobalRegistration)
                        .get();

        testProxy proxy1 = runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                                       .setMessagingQos(new MessagingQos(2000))
                                       .setDiscoveryQos(discoveryQos)
                                       .build();

        proxy1.addNumbers(1, 2, 3);
        verify(provider1).addNumbers(1, 2, 3);
        reset(provider1);

        JoynrRuntime runtimeProvider2 = createRuntime("provider_override", fixedParticipantIdProperty);
        DefaulttestProvider provider2 = Mockito.spy(DefaulttestProvider.class);
        runtimeProvider2.registerProvider(providerDomain, provider2, createProviderQos(), awaitGlobalRegistration)
                        .get();

        testProxy proxy2 = runtimeProxy.getProxyBuilder(providerDomain, testProxy.class)
                                       .setMessagingQos(new MessagingQos(2000))
                                       .setDiscoveryQos(discoveryQos)
                                       .build();
        proxy2.addNumbers(1, 2, 3);
        verify(provider1, never()).addNumbers(1, 2, 3);
        verify(provider2).addNumbers(1, 2, 3);
    }
}
