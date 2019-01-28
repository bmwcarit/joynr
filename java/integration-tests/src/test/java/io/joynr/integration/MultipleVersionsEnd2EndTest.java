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

import static org.junit.Assert.assertEquals;
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
import joynr.tests.DefaultMultipleVersionsInterface1Provider;
import joynr.tests.DefaultMultipleVersionsInterface2Provider;
import joynr.tests.DefaultMultipleVersionsInterfaceProvider;
import joynr.tests.MultipleVersionsInterface1Proxy;
import joynr.tests.MultipleVersionsInterface2Proxy;
import joynr.tests.v2.MultipleVersionsInterfaceProxy;

public class MultipleVersionsEnd2EndTest extends AbstractMultipleVersionsEnd2EndTest {
    private static final String DOMAIN_PREFIX = "MultipleVersionsTestDomain-";
    private static final String PROXYBUILD_FAILED_MESSAGE = "Building of proxy failed: ";
    private static final String REGISTERING_FAILED_MESSAGE = "Registering of provider failed: ";
    private static final String UNREGISTERING_FAILED_MESSAGE = "Unregistering of provider failed: ";

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

    /**
     * Registers given provider in runtime.
     * @param provider
     * @param domain
     */
    private void registerProvider(AbstractJoynrProvider provider, String domain) {
        Future<Void> future = runtime.registerProvider(domain, provider, providerQos);

        try {
            future.get(100);
        } catch (Exception e) {
            fail(REGISTERING_FAILED_MESSAGE + e);
        }
    }

    /**
     * Builds a proxy of the requested interface in runtime.
     * @param interfaceClass Interface of the proxy to build.
     * @return The built proxy.
     */
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
            fail(PROXYBUILD_FAILED_MESSAGE + e);
        }
        return proxy;
    }

    /*
    * This test tests if 2 proxies of same interface (name versioned and package versioned) can connect to
    * one provider (unversioned) and communicate with this without mutual interference.
    */
    @Test
    public void twoProxiesOfDifferentVersioningTypesVsUnversionedProvider() throws Exception {
        // register provider
        DefaultMultipleVersionsInterfaceProvider provider = new DefaultMultipleVersionsInterfaceProvider();

        registerProvider(provider, domain);

        // build fitting proxies
        MultipleVersionsInterface2Proxy proxy1 = buildProxy(MultipleVersionsInterface2Proxy.class, new HashSet<String>(Arrays.asList(domain)), true);
        MultipleVersionsInterfaceProxy proxy2 = buildProxy(MultipleVersionsInterfaceProxy.class, new HashSet<String>(Arrays.asList(domain)), true);

        try {
            //set UInt8Attribute1 and check if it can be retrieved correctly
            proxy1.setUInt8Attribute1((byte) 100);
            Byte value1 = proxy1.getUInt8Attribute1();
            Byte value2 = proxy2.getUInt8Attribute1();
            assertEquals((byte) value1, 100);
            assertEquals((byte) value2, 100);

            proxy2.setUInt8Attribute1((byte) 50);
            value1 = proxy1.getUInt8Attribute1();
            value2 = proxy2.getUInt8Attribute1();
            assertEquals((byte) value1, 50);
            assertEquals((byte) value2, 50);

            // unregister provider
            runtime.unregisterProvider(domain, provider);
        } catch (JoynrRuntimeException e) {
            fail(UNREGISTERING_FAILED_MESSAGE + e);
        }
    }

    /*
     * This test tests if 2 providers of same interface and different versions can be registered in a single runtime
     * and 2 proxies can communicate with those without mutual interference.
     */
    @Test
    public void twoNameVersionedProvidersInSingleRuntime() throws Exception {
        // register providers
        DefaultMultipleVersionsInterface1Provider provider1 = new DefaultMultipleVersionsInterface1Provider();
        DefaultMultipleVersionsInterface2Provider provider2 = new DefaultMultipleVersionsInterface2Provider();

        registerProvider(provider1, domain);
        registerProvider(provider2, domain);

        // build fitting proxies
        MultipleVersionsInterface1Proxy proxy1 = buildProxy(MultipleVersionsInterface1Proxy.class, new HashSet<String>(Arrays.asList(domain)), true);
        MultipleVersionsInterface2Proxy proxy2 = buildProxy(MultipleVersionsInterface2Proxy.class, new HashSet<String>(Arrays.asList(domain)), true);

        //set UInt8Attribute1 and check if it can be retrieved correctly
        proxy1.setUInt8Attribute1((byte) 100);
        proxy2.setUInt8Attribute1((byte) 50);
        Byte value1 = proxy1.getUInt8Attribute1();
        Byte value2 = proxy2.getUInt8Attribute1();
        assertEquals((byte) value1, 100);
        assertEquals((byte) value2, 50);

        // unregister providers
        try {
            runtime.unregisterProvider(domain, provider1);
            runtime.unregisterProvider(domain, provider2);
        } catch (JoynrRuntimeException e) {
            fail(UNREGISTERING_FAILED_MESSAGE + e);
        }
    }

}
