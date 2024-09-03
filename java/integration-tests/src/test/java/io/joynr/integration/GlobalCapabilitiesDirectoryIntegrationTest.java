/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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

import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS;
import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CC_REMOVE_STALE_DELAY_MS;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Properties;

import org.junit.Rule;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.SystemServicesSettings;
import io.joynr.statusmetrics.ConnectionStatusMetrics;
import io.joynr.statusmetrics.JoynrStatusMetrics;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.system.DiscoveryProxy;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.tests.test;
import joynr.tests.testProxy;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class GlobalCapabilitiesDirectoryIntegrationTest {
    private static final Logger logger = LoggerFactory.getLogger(GlobalCapabilitiesDirectoryIntegrationTest.class);

    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private static final String TEST_DOMAIN = "test";
    private static final long DISCOVERY_TIMEOUT = 10000;
    private static final long FRESHNESS_UPDATE_INTERVAL_MS = 1000;

    private DefaulttestProvider testProvider;

    private void waitForGlobalConnection(Injector injector) throws Exception {
        // wait some time for MQTT connection to be established
        Thread.sleep(FRESHNESS_UPDATE_INTERVAL_MS);
        JoynrStatusMetrics statusMetrics = injector.getInstance(JoynrStatusMetrics.class);
        while (!(statusMetrics.getAllConnectionStatusMetrics().size() > 0)
                && statusMetrics.getAllConnectionStatusMetrics()
                                .toArray(new ConnectionStatusMetrics[0])[0].isConnected()) {
            Thread.sleep(10);
        }
    }

    @Test
    public void testTouchOfProvidersWithGlobalScope() throws Exception {
        JoynrRuntime runtime;

        String domain = TEST_DOMAIN + System.currentTimeMillis();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryTimeoutMs(DISCOVERY_TIMEOUT);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);

        Injector injector = createInjector();
        runtime = injector.getInstance(JoynrRuntime.class);
        try {
            registerProvider(runtime, domain);
        } catch (Exception e) {
            runtime.shutdown(true);
            fail("Provider registration failed: " + e.toString());
        }
        waitForGlobalConnection(injector);

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
        long timeBeforeFirstLookup = System.currentTimeMillis();
        String[] providerDomains = new String[]{ domain };
        String interfaceName = test.INTERFACE_NAME;
        String[] lookupGbids = injector.getInstance(Key.get(String[].class,
                                                            Names.named(MessagingPropertyKeys.GBID_ARRAY)));

        DiscoveryEntry[] oldGlobalDiscoveryEntires = gcdProxy.lookup(providerDomains, interfaceName, lookupGbids);
        assertEquals(1, oldGlobalDiscoveryEntires.length);

        // Wait some time before the performing of second lookup
        Thread.sleep(FRESHNESS_UPDATE_INTERVAL_MS + FRESHNESS_UPDATE_INTERVAL_MS / 2);

        // Perform second lookup
        // Check entry in GCD
        DiscoveryEntry[] newGlobalDiscoveryEntires = gcdProxy.lookup(providerDomains, interfaceName, lookupGbids);
        assertEquals(1, newGlobalDiscoveryEntires.length);
        long timeAfterSecondLookup = System.currentTimeMillis();
        long deltaMax = timeAfterSecondLookup - timeBeforeFirstLookup + FRESHNESS_UPDATE_INTERVAL_MS;

        DiscoveryEntry oldGlobalEntry = oldGlobalDiscoveryEntires[0];
        DiscoveryEntry newGlobalEntry = newGlobalDiscoveryEntires[0];

        logger.info("oldGlobalEntry.getExpiryDateMs() = {}", oldGlobalEntry.getExpiryDateMs());
        logger.info("newGlobalEntry.getExpiryDateMs() = {}", newGlobalEntry.getExpiryDateMs());
        logger.info("newGlobalEntry - oldGlobalEntry = {}",
                    newGlobalEntry.getExpiryDateMs() - oldGlobalEntry.getExpiryDateMs());

        // real interval between 2 touch calls might be less than FRESHNESS_UPDATE_INTERVAL_MS because of messaging delays
        final long minNewLastSeenDate = oldGlobalEntry.getLastSeenDateMs() + FRESHNESS_UPDATE_INTERVAL_MS / 2;
        final long minNewExpiryDate = oldGlobalEntry.getExpiryDateMs() + FRESHNESS_UPDATE_INTERVAL_MS / 2;
        assertTrue(minNewLastSeenDate <= newGlobalEntry.getLastSeenDateMs());
        assertTrue(newGlobalEntry.getLastSeenDateMs() < (oldGlobalEntry.getLastSeenDateMs() + deltaMax));
        assertTrue(minNewExpiryDate <= newGlobalEntry.getExpiryDateMs());
        assertTrue(newGlobalEntry.getExpiryDateMs() < (oldGlobalEntry.getExpiryDateMs() + deltaMax));

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
        timeAfterSecondLookup = System.currentTimeMillis();
        deltaMax = timeAfterSecondLookup - timeBeforeFirstLookup + FRESHNESS_UPDATE_INTERVAL_MS;

        DiscoveryEntryWithMetaInfo newGlobalCacheLocalEntry = newGlobalCacheLocalEntries[0];

        assertTrue(minNewLastSeenDate <= newGlobalCacheLocalEntry.getLastSeenDateMs());
        assertTrue(newGlobalCacheLocalEntry.getLastSeenDateMs() < (oldGlobalEntry.getLastSeenDateMs() + deltaMax));
        assertTrue(minNewExpiryDate <= newGlobalCacheLocalEntry.getExpiryDateMs());
        assertTrue(newGlobalCacheLocalEntry.getExpiryDateMs() < (oldGlobalEntry.getExpiryDateMs() + deltaMax));

        // Check local store in LCD
        internalDiscoveryQos.setDiscoveryScope(joynr.types.DiscoveryScope.LOCAL_ONLY);

        DiscoveryEntryWithMetaInfo[] newLocalCacheLocalEntries = lcdProxy.lookup(providerDomains,
                                                                                 interfaceName,
                                                                                 internalDiscoveryQos,
                                                                                 lookupGbids);
        assertEquals(1, newLocalCacheLocalEntries.length);
        timeAfterSecondLookup = System.currentTimeMillis();
        deltaMax = timeAfterSecondLookup - timeBeforeFirstLookup + FRESHNESS_UPDATE_INTERVAL_MS;

        DiscoveryEntryWithMetaInfo newLocalCacheLocalEntry = newLocalCacheLocalEntries[0];

        assertTrue(minNewLastSeenDate <= newLocalCacheLocalEntry.getLastSeenDateMs());
        assertTrue("deltaMax: " + deltaMax,
                   newLocalCacheLocalEntry.getLastSeenDateMs() < (oldGlobalEntry.getLastSeenDateMs() + deltaMax));
        assertTrue(minNewExpiryDate <= newLocalCacheLocalEntry.getExpiryDateMs());
        assertTrue("deltaMax: " + deltaMax,
                   newLocalCacheLocalEntry.getExpiryDateMs() < (oldGlobalEntry.getExpiryDateMs() + deltaMax));

        // Unregister providers to not clutter the JDS
        runtime.unregisterProvider(TEST_DOMAIN, testProvider);
        runtime.shutdown(true);
    }

    @Test
    public void testRemoveStaleProvidersOfClusterController() throws Exception {
        JoynrRuntime runtimeFirst;
        JoynrRuntime runtimeSecond;
        String providerDomain = TEST_DOMAIN + "_removeStaleProvidersOfClusterController";

        int numberOfProxyBuildTries = 5;
        int iterator = 1;

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryTimeoutMs(DISCOVERY_TIMEOUT);
        discoveryQos.setRetryIntervalMs(DISCOVERY_TIMEOUT + 1L);
        discoveryQos.setCacheMaxAgeMs(0L);

        // create first joynr cluster controller runtime
        runtimeFirst = createRuntime();
        // register provider
        try {
            registerProvider(runtimeFirst, providerDomain);
        } catch (Exception e) {
            runtimeFirst.shutdown(true);
            fail("Provider registration failed: " + e.toString());
        }

        // shutdown first cluster controller
        runtimeFirst.shutdown(false);

        // create cluster controller second time
        final int removeStaleDelay = 2000;
        runtimeSecond = createRuntime(removeStaleDelay);

        // wait some time to make sure that cluster controller runtime has been started
        Thread.sleep(500);

        final Future<Void> futureFirstProxy = new Future<Void>();
        final Future<Void> futureSecondProxy = new Future<Void>();

        // Check if proxy is built before removeStale is called
        Properties additionalProperties = new Properties();
        additionalProperties.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, "persistence_file");
        Injector injector = createInjector(0, additionalProperties);
        JoynrRuntime proxyRuntime = injector.getInstance(JoynrRuntime.class);
        proxyRuntime.getProxyBuilder(providerDomain, testProxy.class)
                    .setDiscoveryQos(discoveryQos)
                    .build(new ProxyCreatedCallback<testProxy>() {
                        @Override
                        public void onProxyCreationFinished(testProxy result) {
                            futureFirstProxy.onSuccess(null);
                        }

                        @Override
                        public void onProxyCreationError(JoynrRuntimeException error) {
                            futureFirstProxy.onFailure(error);
                        }

                        @Override
                        public void onProxyCreationError(DiscoveryException error) {
                            futureFirstProxy.onFailure(error);
                        }
                    });
        try {
            futureFirstProxy.get();
        } catch (Exception e) {
            proxyRuntime.shutdown(true);
            fail("proxyRuntime.getProxyBuilder().build() should have been successful!");
        }

        Thread.sleep(1000);

        while (iterator <= numberOfProxyBuildTries) {
            // wait some time to make sure that removeStale has been published and processed
            Thread.sleep(iterator * 1000);
            // build proxy when provider are unregistered
            runtimeSecond.getProxyBuilder(providerDomain, testProxy.class)
                         .setDiscoveryQos(discoveryQos)
                         .build(new ProxyCreatedCallback<testProxy>() {
                             @Override
                             public void onProxyCreationFinished(testProxy result) {
                                 futureSecondProxy.onSuccess(null);
                             }

                             @Override
                             public void onProxyCreationError(JoynrRuntimeException error) {
                                 futureSecondProxy.onFailure(error);
                             }

                             @Override
                             public void onProxyCreationError(DiscoveryException error) {
                                 futureSecondProxy.onFailure(error);
                             }
                         });
            iterator++;
            try {
                futureSecondProxy.get(DISCOVERY_TIMEOUT + 1000);
                if (iterator == numberOfProxyBuildTries) {
                    runtimeSecond.shutdown(true);
                    fail("runtimeSecond.getProxyBuilder().build() should throw Exception!");
                }
            } catch (Exception e) {
                boolean isFound = e.getMessage().indexOf("Unable to find provider") != -1 ? true : false;
                assertTrue("Unexpected error: " + e, isFound);
                break;
            }
        }

        runtimeSecond.shutdown(true);
    }

    private Injector createInjector() {
        return createInjector(0, null);
    }

    private Injector createInjector(long removeStaleDelayMs, Properties additionalProperties) {
        Properties properties = new Properties();
        properties.put(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS,
                       String.valueOf(FRESHNESS_UPDATE_INTERVAL_MS));
        properties.put(PROPERTY_CC_REMOVE_STALE_DELAY_MS, String.valueOf(removeStaleDelayMs));

        if (additionalProperties != null) {
            properties.putAll(additionalProperties);
        }
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule())
                                      .with(new HivemqMqttClientModule(), new JoynrPropertiesModule(properties));
        return Guice.createInjector(runtimeModule);
    }

    private JoynrRuntime createRuntime() {
        return createRuntime(0);
    }

    private JoynrRuntime createRuntime(int removeStaleDelay) {
        Injector injector = createInjector(removeStaleDelay, null);
        return injector.getInstance(JoynrRuntime.class);
    }

    private void registerProvider(JoynrRuntime runtime, String domain) throws Exception {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
        providerQos.setPriority(System.currentTimeMillis());

        testProvider = new DefaulttestProvider();

        runtime.getProviderRegistrar(domain, testProvider)
               .withProviderQos(providerQos)
               .awaitGlobalRegistration()
               .register()
               .get(); // wait for successful registration
    }
}
