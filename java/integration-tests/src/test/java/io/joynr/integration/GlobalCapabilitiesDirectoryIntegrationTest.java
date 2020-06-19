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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GBIDS;
import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Arrays;
import java.util.Properties;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.SystemServicesSettings;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.system.DiscoveryProxy;
import joynr.tests.DefaulttestProvider;
import joynr.tests.test;
import joynr.tests.testProxy;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.Key;

import java.lang.Thread;

public class GlobalCapabilitiesDirectoryIntegrationTest {
    private static final Logger log = LoggerFactory.getLogger(GlobalCapabilitiesDirectoryIntegrationTest.class);

    private static final String TEST_DOMAIN = "test";
    private static final long DISCOVERY_TIMEOUT = 10000;
    private static final long FRESHNESS_UPDATE_INTERVAL_MS = 500;

    private JoynrRuntime runtimeFirst;
    private JoynrRuntime runtimeSecond;
    private DefaulttestProvider testProvider;

    @Test
    public void testTouchOfProvidersWithGlobalScope() throws Exception {
        JoynrRuntime runtime;

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryTimeoutMs(DISCOVERY_TIMEOUT);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);

        Injector injector = createInjector();
        runtime = injector.getInstance(JoynrRuntime.class);
        registerProvider(runtime);
        Thread.sleep(1000);

        // Build globalCapabilitiesDirectoryProxy
        GlobalCapabilitiesDirectoryProxy gcdProxy;

        GlobalDiscoveryEntry gcdDiscoveryEntry = injector.getInstance(Key.get(GlobalDiscoveryEntry.class,
                                                                              Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY)));

        ProxyBuilder<GlobalCapabilitiesDirectoryProxy> gcdProxyBuilder = runtime.getProxyBuilder(gcdDiscoveryEntry.getDomain(),
                                                                                                 GlobalCapabilitiesDirectoryProxy.class);

        gcdProxy = gcdProxyBuilder.setDiscoveryQos(discoveryQos).build();

        // Build localCapabiltiesProxy
        DiscoveryProxy lcdProxy;
        String systemServicesDomain = injector.getInstance(Key.get(String.class,
                                                                   Names.named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN)));

        ProxyBuilder<DiscoveryProxy> lcdProxyBuilder = runtime.getProxyBuilder(systemServicesDomain,
                                                                               DiscoveryProxy.class);

        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        lcdProxy = lcdProxyBuilder.setDiscoveryQos(discoveryQos).build();

        // Perform first lookup
        String[] providerDomains = new String[]{ TEST_DOMAIN };
        String interfaceName = test.INTERFACE_NAME;
        String lookupGbidsStr = injector.getInstance(Key.get(String.class,
                                                             Names.named(ConfigurableMessagingSettings.PROPERTY_GBIDS)));
        String[] lookupGbids = Arrays.stream(lookupGbidsStr.split(",")).map(a -> a.trim()).toArray(String[]::new);

        DiscoveryEntry[] oldGlobalDiscoveryEntires = gcdProxy.lookup(providerDomains, interfaceName, lookupGbids);
        assertEquals(1, oldGlobalDiscoveryEntires.length);

        // Wait some time before the performing of second lookup
        Thread.sleep(FRESHNESS_UPDATE_INTERVAL_MS + FRESHNESS_UPDATE_INTERVAL_MS / 2);

        // Perform second lookup
        // Check entry in GCD
        DiscoveryEntry[] newGlobalDiscoveryEntires = gcdProxy.lookup(providerDomains, interfaceName, lookupGbids);
        assertEquals(1, newGlobalDiscoveryEntires.length);

        DiscoveryEntry oldGlobalEntry = oldGlobalDiscoveryEntires[0];
        DiscoveryEntry newGlobalEntry = newGlobalDiscoveryEntires[0];

        log.info("oldGlobalEntry.getExpiryDateMs() = {}", oldGlobalEntry.getExpiryDateMs());
        log.info("newGlobalEntry.getExpiryDateMs() = {}", newGlobalEntry.getExpiryDateMs());
        log.info("newGlobalEntry - oldGlobalEntry = {}",
                 newGlobalEntry.getExpiryDateMs() - oldGlobalEntry.getExpiryDateMs());

        assertTrue(oldGlobalEntry.getLastSeenDateMs() < newGlobalEntry.getLastSeenDateMs());
        assertTrue(newGlobalEntry.getLastSeenDateMs() < (oldGlobalEntry.getLastSeenDateMs()
                + 2 * FRESHNESS_UPDATE_INTERVAL_MS));
        assertTrue(oldGlobalEntry.getExpiryDateMs() < newGlobalEntry.getExpiryDateMs());
        assertTrue(newGlobalEntry.getExpiryDateMs() < (oldGlobalEntry.getExpiryDateMs()
                + 2 * FRESHNESS_UPDATE_INTERVAL_MS));

        // Check global cache in LCD
        joynr.types.DiscoveryQos internalDiscoveryQos = new joynr.types.DiscoveryQos();
        internalDiscoveryQos.setCacheMaxAge(Long.MAX_VALUE);
        internalDiscoveryQos.setDiscoveryScope(joynr.types.DiscoveryScope.GLOBAL_ONLY);
        internalDiscoveryQos.setDiscoveryTimeout(DISCOVERY_TIMEOUT);

        DiscoveryEntryWithMetaInfo[] newGlobalCacheLocalEntries = lcdProxy.lookup(providerDomains,
                                                                                  interfaceName,
                                                                                  internalDiscoveryQos,
                                                                                  lookupGbids);
        assertEquals(1, newGlobalCacheLocalEntries.length);

        DiscoveryEntryWithMetaInfo newGlobalCacheLocalEntry = newGlobalCacheLocalEntries[0];

        assertTrue(oldGlobalEntry.getLastSeenDateMs() < newGlobalCacheLocalEntry.getLastSeenDateMs());
        assertTrue(newGlobalCacheLocalEntry.getLastSeenDateMs() < (oldGlobalEntry.getLastSeenDateMs()
                + 2 * FRESHNESS_UPDATE_INTERVAL_MS));
        assertTrue(oldGlobalEntry.getExpiryDateMs() < newGlobalCacheLocalEntry.getExpiryDateMs());
        assertTrue(newGlobalCacheLocalEntry.getExpiryDateMs() < (oldGlobalEntry.getExpiryDateMs()
                + 2 * FRESHNESS_UPDATE_INTERVAL_MS));

        // Check local store in LCD
        internalDiscoveryQos.setDiscoveryScope(joynr.types.DiscoveryScope.LOCAL_ONLY);

        DiscoveryEntryWithMetaInfo[] newLocalCacheLocalEntries = lcdProxy.lookup(providerDomains,
                                                                                 interfaceName,
                                                                                 internalDiscoveryQos,
                                                                                 lookupGbids);
        assertEquals(1, newLocalCacheLocalEntries.length);

        DiscoveryEntryWithMetaInfo newLocalCacheLocalEntry = newLocalCacheLocalEntries[0];

        assertTrue(oldGlobalEntry.getLastSeenDateMs() < newLocalCacheLocalEntry.getLastSeenDateMs());
        assertTrue(newLocalCacheLocalEntry.getLastSeenDateMs() < (oldGlobalEntry.getLastSeenDateMs()
                + 2 * FRESHNESS_UPDATE_INTERVAL_MS));
        assertTrue(oldGlobalEntry.getExpiryDateMs() < newLocalCacheLocalEntry.getExpiryDateMs());
        assertTrue(newLocalCacheLocalEntry.getExpiryDateMs() < (oldGlobalEntry.getExpiryDateMs()
                + 2 * FRESHNESS_UPDATE_INTERVAL_MS));

        // Unregister providers to not clutter the JDS
        runtime.unregisterProvider(TEST_DOMAIN, testProvider);
        runtime.shutdown(true);
    }

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

    private Injector createInjector() {
        Module mqttTransportModule = new MqttPahoModule();
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(mqttTransportModule,
                                                                                     new AbstractModule() {
                                                                                         @Override
                                                                                         protected void configure() {
                                                                                             bind(String.class).annotatedWith(Names.named(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS))
                                                                                                               .toInstance(String.valueOf(FRESHNESS_UPDATE_INTERVAL_MS));
                                                                                         }
                                                                                     });

        return new JoynrInjectorFactory(new Properties(), runtimeModule).getInjector();
    }

    private JoynrRuntime createRuntime() {
        Injector injector = createInjector();
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
