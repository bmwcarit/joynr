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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import joynr.tests.DefaultMultipleVersionsInterface2Provider;
import joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider;

public class GeneratorVersionCompatibilityTest extends AbstractMultipleVersionsEnd2EndTest {
    private static final long CONST_DEFAULT_TEST_TIMEOUT_MS = 3000;
    private static final String DOMAIN_PREFIX = "MultipleVersionsTestDomain-";
    private static final String REGISTERING_FAILED_MESSAGE = "Registering of provider failed: ";

    private Semaphore proxyBuiltSemaphore;
    private String domain;

    @Override
    @Before
    public void setUp() {
        super.setUp();
        domain = DOMAIN_PREFIX + UUID.randomUUID().toString();
        proxyBuiltSemaphore = new Semaphore(0);
    }

    @After
    public void tearDown() {
        if (runtime != null) {
            runtime.shutdown(true);
        }
    }

    private void registerProvider(AbstractJoynrProvider provider, String domain) {
        Future<Void> future = runtime.registerProvider(domain, provider, providerQos);

        try {
            future.get(100);
        } catch (Exception e) {
            fail(REGISTERING_FAILED_MESSAGE + e);
        }
    }

    private <T> T buildProxy(final Class<T> interfaceClass, final Set<String> domains, final boolean waitForProxyCreation) throws Exception {
        ProxyBuilder<T> proxyBuilder = runtime.getProxyBuilder(domain, interfaceClass);
        T proxy = null;
        try {
            proxy = proxyBuilder.setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<T>() {
                @Override
                public void onProxyCreationFinished(T result) {
                    proxyBuiltSemaphore.release();
                }

                @Override
                public void onProxyCreationError(JoynrRuntimeException error) {
                    throw error;
                }
            });
            if (waitForProxyCreation) {
                assertTrue(proxyBuiltSemaphore.tryAcquire(1, TimeUnit.SECONDS));
            }
        } catch (DiscoveryException | InterruptedException e) {
            if (!waitForProxyCreation) {
                throw e;
            }
            fail("did not expect discovery exception " + e);
        }
        return proxy;
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void nonVersionedProxyCreationAgainstPackageVersionedProviderSucceeds() throws Exception {
        joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider provider_packageVersion = new DefaultMultipleVersionsInterfaceProvider();
        registerProvider(provider_packageVersion, domain);

        buildProxy(joynr.tests.MultipleVersionsInterfaceProxy.class, new HashSet<String>(Arrays.asList(domain)), true);
        
        runtime.unregisterProvider(domain, provider_packageVersion);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void nonVersionedProxyCreationAgainstNameVersionedProviderSucceeds() throws Exception {
        joynr.tests.DefaultMultipleVersionsInterface2Provider provider_nameVersion = new DefaultMultipleVersionsInterface2Provider();
        registerProvider(provider_nameVersion, domain);

        buildProxy(joynr.tests.MultipleVersionsInterfaceProxy.class, new HashSet<String>(Arrays.asList(domain)), true);

        runtime.unregisterProvider(domain, provider_nameVersion);
    }
}
