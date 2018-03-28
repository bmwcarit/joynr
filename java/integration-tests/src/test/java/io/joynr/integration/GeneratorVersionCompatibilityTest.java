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

import static org.junit.Assert.fail;

import java.util.Properties;
import java.util.UUID;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider;
import joynr.tests.DefaultMultipleVersionsInterface2Provider;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class GeneratorVersionCompatibilityTest {
    private static final long CONST_DEFAULT_TEST_TIMEOUT_MS = 3000;

    private String domain;
    private JoynrRuntime runtime;
    private ProviderQos providerQos;
    private DiscoveryQos discoveryQos;

    private JoynrRuntime getRuntime(Properties joynrConfig, Module... modules) {
        Module runtimeModule = new CCInProcessRuntimeModule();
        Module modulesWithRuntime = Modules.override(runtimeModule).with(modules);
        modulesWithRuntime = Modules.override(modulesWithRuntime).with(new TestGlobalAddressModule());

        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);

        return application.getRuntime();
    }

    @Before
    public void setUp() {
        domain = "domain-" + UUID.randomUUID().toString();
        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");

        // provider and proxy using same runtime to allow local-only communications
        runtime = getRuntime(joynrConfig);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        discoveryQos.setDiscoveryTimeoutMs(1000);
        discoveryQos.setRetryIntervalMs(100);
    }

    @After
    public void tearDown() {
        runtime.shutdown(true);
    }

    private void createAndCheckProxy() {
        ProxyBuilder<joynr.tests.MultipleVersionsInterfaceProxy> proxyBuilder = runtime.getProxyBuilder(domain,
                                                                                                        joynr.tests.MultipleVersionsInterfaceProxy.class);

        try {
            proxyBuilder.setDiscoveryQos(discoveryQos).build();
        } catch (DiscoveryException e) {
            fail("did not expect discovery exception " + e);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void proxyCreationAgainstPackageVersionedProviderSucceeds() throws Exception {
        joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider provider_packageVersion = new DefaultMultipleVersionsInterfaceProvider();
        runtime.registerProvider(domain, provider_packageVersion, providerQos);

        createAndCheckProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void proxyCreationAgainstNameVersionedProviderSucceeds() throws Exception {
        joynr.tests.DefaultMultipleVersionsInterface2Provider provider_nameVersion = new DefaultMultipleVersionsInterface2Provider();
        runtime.registerProvider(domain, provider_nameVersion, providerQos);

        createAndCheckProxy();
    }
}
