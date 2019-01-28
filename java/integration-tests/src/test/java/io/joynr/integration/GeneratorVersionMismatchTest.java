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
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Test;

import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import joynr.tests.DefaultMultipleVersionsInterface1Provider;
import joynr.tests.DefaultMultipleVersionsInterfaceProvider;
import joynr.tests.MultipleVersionsInterface1Proxy;
import joynr.tests.MultipleVersionsInterface2Proxy;
import joynr.tests.MultipleVersionsInterfaceProxy;

public class GeneratorVersionMismatchTest extends AbstractMultipleVersionsEnd2EndTest {

    private joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider packageVersionedProvider;
    private DefaultMultipleVersionsInterface1Provider nameVersionedProvider;
    private DefaultMultipleVersionsInterfaceProvider unversionedProvider;

    @Override
    @After
    public void tearDown() {
        if (packageVersionedProvider != null) {
            runtime.unregisterProvider(domain, packageVersionedProvider);
        }
        if (nameVersionedProvider != null) {
            runtime.unregisterProvider(domain, nameVersionedProvider);
        }
        if (unversionedProvider != null) {
            runtime.unregisterProvider(domain, unversionedProvider);
        }

        super.tearDown();
    }

    private void checkPackageVersionedProxy() throws Exception {
        final joynr.tests.v2.MultipleVersionsInterfaceProxy proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                               new HashSet<String>(Arrays.asList(domain)),
                                                                               false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(DISCOVERY_TIMEOUT_MS * 2,
                                                                         TimeUnit.MILLISECONDS));

        try {
            proxy.getTrue();
            fail("Proxy call didn't cause a discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a *NoCompatibleProviderFoundException, but got: " + e);
        }
    }

    private void checkNameVersionedProxy() throws Exception {
        final MultipleVersionsInterface2Proxy proxy = buildProxy(MultipleVersionsInterface2Proxy.class,
                                                                 new HashSet<String>(Arrays.asList(domain)),
                                                                 false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(DISCOVERY_TIMEOUT_MS * 2,
                                                                         TimeUnit.MILLISECONDS));

        try {
            proxy.getTrue();
            fail("Proxy call didn't cause a discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a *NoCompatibleProviderFoundException, but got: " + e);
        }
    }

    private void checkUnversionedProxy() throws Exception {
        final MultipleVersionsInterfaceProxy proxy = buildProxy(MultipleVersionsInterfaceProxy.class,
                                                                new HashSet<String>(Arrays.asList(domain)),
                                                                false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(DISCOVERY_TIMEOUT_MS * 2,
                                                                         TimeUnit.MILLISECONDS));

        try {
            proxy.getTrue();
            fail("Proxy call didn't cause a discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a *NoCompatibleProviderFoundException, but got: " + e);
        }
    }

    private void registerPackageVersionedProvider() throws Exception {
        registerPackageVersionedProvider(domain);
    }

    private void registerPackageVersionedProvider(final String domain) throws Exception {
        packageVersionedProvider = new joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider();
        registerProvider(packageVersionedProvider, domain);
    }

    private void registerNameVersionedProvider() throws Exception {
        nameVersionedProvider = new DefaultMultipleVersionsInterface1Provider();
        registerProvider(nameVersionedProvider, domain);
    }

    private void registerUnversionedProvider() throws Exception {
        unversionedProvider = new DefaultMultipleVersionsInterfaceProvider();
        registerProvider(unversionedProvider, domain);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_packageVersionedProvider_packageVersionedProxy() throws Exception {
        registerPackageVersionedProvider();
        checkPackageVersionedProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_packageVersionedProvider_nameVersionedProxy() throws Exception {
        registerPackageVersionedProvider();
        checkNameVersionedProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_packageVersionedProvider_unversionedProxy() throws Exception {
        registerPackageVersionedProvider();
        checkUnversionedProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_nameVersionedProvider_packageVersionedProxy() throws Exception {
        registerNameVersionedProvider();
        checkPackageVersionedProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_nameVersionedProvider_nameVersionedProxy() throws Exception {
        registerNameVersionedProvider();
        checkNameVersionedProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_nameVersionedProvider_unversionedProxy() throws Exception {
        registerNameVersionedProvider();
        checkUnversionedProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_unversionedProvider_packageVersionedProxy() throws Exception {
        registerUnversionedProvider();

        final joynr.tests.v1.MultipleVersionsInterfaceProxy proxy = buildProxy(joynr.tests.v1.MultipleVersionsInterfaceProxy.class,
                                                                               new HashSet<String>(Arrays.asList(domain)),
                                                                               false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(DISCOVERY_TIMEOUT_MS * 2,
                                                                         TimeUnit.MILLISECONDS));

        try {
            proxy.getTrue();
            fail("Proxy call didn't cause a discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a *NoCompatibleProviderFoundException, but got: " + e);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_unversionedProvider_nameVersionedProxy() throws Exception {
        registerUnversionedProvider();

        final MultipleVersionsInterface1Proxy proxy = buildProxy(MultipleVersionsInterface1Proxy.class,
                                                                 new HashSet<String>(Arrays.asList(domain)),
                                                                 false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(DISCOVERY_TIMEOUT_MS * 2,
                                                                         TimeUnit.MILLISECONDS));

        try {
            proxy.getTrue();
            fail("Proxy call didn't cause a discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a *NoCompatibleProviderFoundException, but got: " + e);
        }
    }

    private void checkProxy(final joynr.tests.v2.MultipleVersionsInterfaceProxy proxy) {
        try {
            proxy.getTrue();
            fail("Proxy call didn't cause a discovery exception");
        } catch (NoCompatibleProviderFoundException | MultiDomainNoCompatibleProviderFoundException e) {
            // These exceptions are expected, so no need to fail here.
        } catch (Exception e) {
            fail("Expected a *NoCompatibleProviderFoundException, but got: " + e);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testMultiDomainNoCompatibleProviderFound() throws Exception {
        registerPackageVersionedProvider();
        final String domain2 = "domain2-" + UUID.randomUUID().toString();
        registerPackageVersionedProvider(domain2);

        final joynr.tests.v2.MultipleVersionsInterfaceProxy proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                               new HashSet<String>(Arrays.asList(domain,
                                                                                                                 domain2)),
                                                                               false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(DISCOVERY_TIMEOUT_MS * 2,
                                                                         TimeUnit.MILLISECONDS));

        checkProxy(proxy);

        runtime.unregisterProvider(domain2, packageVersionedProvider);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testProxyIsInvalidatedOnceArbitrationExceptionThrown() throws Exception {
        registerPackageVersionedProvider();

        final joynr.tests.v2.MultipleVersionsInterfaceProxy proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                               new HashSet<String>(Arrays.asList(domain)),
                                                                               false);

        checkProxy(proxy);
        checkProxy(proxy);
    }

}
