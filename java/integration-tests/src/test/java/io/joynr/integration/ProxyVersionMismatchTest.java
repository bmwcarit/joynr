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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Properties;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.google.common.collect.Sets;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class ProxyVersionMismatchTest {
    private static final long CONST_DEFAULT_TEST_TIMEOUT_MS = 3000;

    private Semaphore errorCallbackSemaphore;
    private String domain;
    private String domain2;
    private JoynrRuntime runtime;
    private DiscoveryQos discoveryQos;
    private joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider provider;
    private ProxyCreatedCallback<joynr.tests.v2.MultipleVersionsInterfaceProxy> callback;
    private joynr.tests.v2.MultipleVersionsInterfaceProxy proxy;

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
        // the error callback will make a permit available.
        // The test will wait until a permit is available, or fail.
        errorCallbackSemaphore = new Semaphore(0, true);
        domain = "domain-" + UUID.randomUUID().toString();
        domain2 = "domain2-" + UUID.randomUUID().toString();
        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");

        // provider and proxy using same runtime to allow local-only communications
        runtime = getRuntime(joynrConfig);
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        provider = new DefaultMultipleVersionsInterfaceProvider();
        runtime.registerProvider(domain, provider, providerQos);
        runtime.registerProvider(domain2, provider, providerQos);

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        discoveryQos.setDiscoveryTimeoutMs(1000);
        discoveryQos.setRetryIntervalMs(100);

        callback = new ProxyCreatedCallback<joynr.tests.v2.MultipleVersionsInterfaceProxy>() {

            @Override
            public void onProxyCreationFinished(joynr.tests.v2.MultipleVersionsInterfaceProxy result) {
                // Fail doesn't work here, so just do nothing
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                if (error instanceof NoCompatibleProviderFoundException
                        || error instanceof MultiDomainNoCompatibleProviderFoundException) {
                    errorCallbackSemaphore.release();
                }
            }
        };
    }

    @After
    public void tearDown() {
        runtime.shutdown(true);
    }

    private void createProxy(final Set<String> domains, boolean async) {
        ProxyBuilder<joynr.tests.v2.MultipleVersionsInterfaceProxy> proxyBuilder = runtime.getProxyBuilder(domains,
                                                                                                           joynr.tests.v2.MultipleVersionsInterfaceProxy.class);
        if (async) {
            proxy = proxyBuilder.setDiscoveryQos(discoveryQos).build(callback);
        } else {
            proxy = proxyBuilder.setDiscoveryQos(discoveryQos).build();
        }
    }

    private void checkProxy() {
        try {
            proxy.getTrue();
            fail("Proxy call didn't cause an discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a discovery exception, but got: " + e);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound() throws Exception {

        createProxy(Sets.newHashSet(domain), true);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   errorCallbackSemaphore.tryAcquire(3, TimeUnit.SECONDS));

        checkProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testMultiDomainNoCompatibleProviderFound() throws Exception {

        createProxy(Sets.newHashSet(domain, domain2), true);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   errorCallbackSemaphore.tryAcquire(3, TimeUnit.SECONDS));

        checkProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testProxyIsInvalidatedOnceArbitrationExceptionThrown() throws Exception {

        createProxy(Sets.newHashSet(domain), false);

        checkProxy();
        checkProxy();
    }
}
