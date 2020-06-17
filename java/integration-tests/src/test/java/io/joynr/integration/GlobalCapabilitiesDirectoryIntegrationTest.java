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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Properties;

import org.junit.Test;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

import com.google.inject.Guice;
import com.google.inject.Injector;
import java.lang.Thread;

public class GlobalCapabilitiesDirectoryIntegrationTest {
    private static final String TEST_DOMAIN = "test";
    private static final long DISCOVERY_TIMEOUT = 10000;

    private JoynrRuntime runtimeFirst;
    private JoynrRuntime runtimeSecond;
    private DefaulttestProvider testProvider;

    @Test
    public void testRemoveStaleProvidersOfClusterController() throws Exception {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryTimeoutMs(DISCOVERY_TIMEOUT);
        discoveryQos.setRetryIntervalMs(DISCOVERY_TIMEOUT + 1L);
        discoveryQos.setCacheMaxAgeMs(0L);

        final Future<Void> future = new Future<Void>();

        // create first joynr cluster controller runtime
        runtimeFirst = createRuntime();
        // register provider
        registerProvider(runtimeFirst);

        // shutdown first cluster controller
        if (testProvider != null) {
            testProvider = null;
        }
        runtimeFirst.shutdown(false);

        // create cluster controller second time
        runtimeSecond = createRuntime();
        // wait some time to make sure that removeStale has been published and processed
        Thread.sleep(1000);

        // build proxy when provider are unregistered
        runtimeSecond.getProxyBuilder(TEST_DOMAIN, testProxy.class)
                     .setDiscoveryQos(discoveryQos)
                     .build(new ProxyCreatedCallback<testProxy>() {
                         @Override
                         public void onProxyCreationFinished(testProxy result) {
                             future.onSuccess(null);
                         }

                         @Override
                         public void onProxyCreationError(JoynrRuntimeException error) {
                             future.onFailure(error);
                         }
                     });
        try {
            future.get(DISCOVERY_TIMEOUT + 1000);
            fail("runtimeSecond.getProxyBuilder().build() should throw Exception!");
        } catch (Exception e) {
            boolean isFound = e.getMessage().indexOf("Unable to find provider") != -1 ? true : false;
            assertTrue("Unexpected error: " + e, isFound);
        }

        runtimeSecond.shutdown(true);
    }

    private JoynrRuntime createRuntime() {
        Module mqttTransportModule = new MqttPahoModule();
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule())
                                      .with(new JoynrPropertiesModule(new Properties()), mqttTransportModule);

        Injector injector = Guice.createInjector(runtimeModule);

        return injector.getInstance(JoynrRuntime.class);
    }

    private void registerProvider(JoynrRuntime runtime) {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
        providerQos.setPriority(System.currentTimeMillis());

        testProvider = new DefaulttestProvider();

        runtime.getProviderRegistrar(TEST_DOMAIN, testProvider)
               .withProviderQos(providerQos)
               .awaitGlobalRegistration()
               .register();
    }
}
