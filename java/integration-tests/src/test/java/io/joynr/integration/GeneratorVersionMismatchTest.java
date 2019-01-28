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
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider;

public class GeneratorVersionMismatchTest extends AbstractMultipleVersionsEnd2EndTest {
    private static final long CONST_DEFAULT_TEST_TIMEOUT_MS = 3000;
    private static final String DOMAIN_PREFIX = "MultipleVersionsTestDomain-";
    private static final String PROXYBUILD_FAILED_MESSAGE = "Building of proxy failed: ";
    private static final String REGISTERING_FAILED_MESSAGE = "Registering of provider failed: ";

    private Semaphore proxyBuiltSemaphore;
    private Semaphore noCompatibleProviderFoundCallbackSemaphore;
    private String domain;
    private String domain2;
    private joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider provider;
    private joynr.tests.v2.MultipleVersionsInterfaceProxy proxy;

    @Before
    public void setUp() {
        super.setUp();
        domain = DOMAIN_PREFIX + UUID.randomUUID().toString();
        proxyBuiltSemaphore = new Semaphore(0);
        noCompatibleProviderFoundCallbackSemaphore = new Semaphore(0, true);

        domain2 = "domain2-" + UUID.randomUUID().toString();
        provider = new DefaultMultipleVersionsInterfaceProvider();

        registerProvider(provider, domain);
        registerProvider(provider, domain2);
    }

    @After
    public void tearDown() {
        runtime.unregisterProvider(domain, provider);
        runtime.unregisterProvider(domain2, provider);

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
        ProxyBuilder<T> proxyBuilder = runtime.getProxyBuilder(domains, interfaceClass);
        T proxy = null;
        try {
            proxy = proxyBuilder.setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<T>() {
                @Override
                public void onProxyCreationFinished(T result) {
                    proxyBuiltSemaphore.release();
                }
    
                @Override
                public void onProxyCreationError(JoynrRuntimeException error) {
                    if (error instanceof NoCompatibleProviderFoundException
                            || error instanceof MultiDomainNoCompatibleProviderFoundException) {
                        noCompatibleProviderFoundCallbackSemaphore.release();
                    }
                }
            });
            if (waitForProxyCreation) {
                assertTrue(proxyBuiltSemaphore.tryAcquire(1, TimeUnit.SECONDS));
            }
        } catch (DiscoveryException | InterruptedException e) {
            if (!waitForProxyCreation) {
                throw e;
            }
            fail(PROXYBUILD_FAILED_MESSAGE + e);
        }
        return proxy;
    }

    private void checkProxy() {
        try {
            proxy.getTrue();
            fail("Proxy call didn't cause a discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a discovery exception, but got: " + e);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound() throws Exception {

        proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class, new HashSet<String>(Arrays.asList(domain)), false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(3, TimeUnit.SECONDS));

        checkProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testMultiDomainNoCompatibleProviderFound() throws Exception {

        proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class, new HashSet<String>(Arrays.asList(domain, domain2)), false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(3, TimeUnit.SECONDS));

        checkProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testProxyIsInvalidatedOnceArbitrationExceptionThrown() throws Exception {

        proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class, new HashSet<String>(Arrays.asList(domain)), false);

        checkProxy();
        checkProxy();
    }
}
