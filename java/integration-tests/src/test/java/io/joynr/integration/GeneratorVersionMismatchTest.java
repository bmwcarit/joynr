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

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Arrays;
import java.util.HashSet;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

import org.junit.After;
import org.junit.Test;

import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import joynr.exceptions.MethodInvocationException;
import joynr.tests.AnonymousVersionedStruct;
import joynr.tests.InterfaceVersionedStruct;
import joynr.tests.MultipleVersionsInterfaceProxy;

import joynr.tests.MultipleVersionsTypeCollection.VersionedStruct;

public class GeneratorVersionMismatchTest extends AbstractMultipleVersionsEnd2EndTest {

    private joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider packageVersionedProviderV1;

    @Override
    @After
    public void tearDown() {
        if (packageVersionedProviderV1 != null) {
            providerRuntime.unregisterProvider(domain, packageVersionedProviderV1);
        }

        super.tearDown();
    }

    private void checkPackageVersionedProxyV2() throws Exception {
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

    private void checkUnversionedProxyV2() throws Exception {
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

    private void registerPackageVersionedProviderV1() throws Exception {
        registerPackageVersionedProviderV1(domain);
    }

    private void registerPackageVersionedProviderV1(final String domain) throws Exception {
        packageVersionedProviderV1 = new joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider();
        registerProvider(packageVersionedProviderV1, domain);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_packageVersionedProviderV1_packageVersionedProxyV2() throws Exception {
        registerPackageVersionedProviderV1();
        checkPackageVersionedProxyV2();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_packageVersionedProviderV1_unversionedProxyV2() throws Exception {
        registerPackageVersionedProviderV1();
        checkUnversionedProxyV2();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testNoCompatibleProviderFound_unversionedProviderV2_packageVersionedProxyV1() throws Exception {
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
        registerPackageVersionedProviderV1();
        final String domain2 = "domain2-" + createUuidString();
        registerPackageVersionedProviderV1(domain2);

        final joynr.tests.v2.MultipleVersionsInterfaceProxy proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                               new HashSet<String>(Arrays.asList(domain,
                                                                                                                 domain2)),
                                                                               false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(DISCOVERY_TIMEOUT_MS * 2,
                                                                         TimeUnit.MILLISECONDS));

        checkProxy(proxy);

        providerRuntime.unregisterProvider(domain2, packageVersionedProviderV1);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testProxyIsInvalidatedOnceArbitrationExceptionThrown() throws Exception {
        registerPackageVersionedProviderV1();

        final joynr.tests.v2.MultipleVersionsInterfaceProxy proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                               new HashSet<String>(Arrays.asList(domain)),
                                                                               false);

        checkProxy(proxy);
        checkProxy(proxy);
    }

    private void expectException(final Consumer<Void> func, final Class<? extends Exception> expectedException) {
        try {
            func.accept(null);
            fail("Function did not throw. Expected " + expectedException);
        } catch (Exception e) {
            assertTrue("Exception \"" + e + "\" is not instanceof " + expectedException,
                       expectedException.isInstance(e));
        }
    }

    private void testPackageVersionedTypes() throws Exception {
        final joynr.tests.v2.MultipleVersionsInterfaceProxy packageVersionedProxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                                               new HashSet<String>(Arrays.asList(domain)),
                                                                                               true);
        final joynr.tests.v2.AnonymousVersionedStruct input1 = new joynr.tests.v2.AnonymousVersionedStruct(random.nextBoolean());
        expectException(x -> {
            packageVersionedProxy.getAnonymousVersionedStruct(input1);
        }, MethodInvocationException.class);

        final joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct input2 = new joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct(random.nextBoolean());
        expectException(x -> {
            packageVersionedProxy.getVersionedStruct(input2);
        }, MethodInvocationException.class);

        final joynr.tests.v2.InterfaceVersionedStruct input3 = new joynr.tests.v2.InterfaceVersionedStruct(random.nextBoolean(),
                                                                                                           random.nextBoolean());
        expectException(x -> {
            packageVersionedProxy.getInterfaceVersionedStruct(input3);
        }, MethodInvocationException.class);
    }

    private void testUnversionedTypes() throws Exception {
        final MultipleVersionsInterfaceProxy unversionedProxy = buildProxy(MultipleVersionsInterfaceProxy.class,
                                                                           new HashSet<String>(Arrays.asList(domain)),
                                                                           true);

        final AnonymousVersionedStruct input1 = new AnonymousVersionedStruct(random.nextBoolean());
        expectException(x -> {
            unversionedProxy.getAnonymousVersionedStruct(input1);
        }, MethodInvocationException.class);

        final VersionedStruct input2 = new VersionedStruct(random.nextBoolean());
        expectException(x -> {
            unversionedProxy.getVersionedStruct(input2);
        }, MethodInvocationException.class);

        final InterfaceVersionedStruct input3 = new InterfaceVersionedStruct(random.nextBoolean(),
                                                                             random.nextBoolean());
        expectException(x -> {
            unversionedProxy.getInterfaceVersionedStruct(input3);
        }, MethodInvocationException.class);
    }

    @Test
    public void packageVersionedProxy_unversionedProvider_singleRuntime() throws Exception {
        registerUnversionedProvider();
        testPackageVersionedTypes();
    }

    @Test(timeout = CONST_GLOBAL_TEST_TIMEOUT_MS)
    public void packageVersionedProxy_unversionedProvider_separateRuntime() throws Exception {
        useGlobalCommunication();
        registerUnversionedProvider();
        testPackageVersionedTypes();
    }

    @Test
    public void unversionedProxy_packageVersionedProvider_singleRuntime() throws Exception {
        registerPackageVersionedProvider();
        testUnversionedTypes();
    }

    @Test(timeout = CONST_GLOBAL_TEST_TIMEOUT_MS)
    public void unversionedProxy_packageVersionedProvider_separateRuntime() throws Exception {
        useGlobalCommunication();
        registerPackageVersionedProvider();
        testUnversionedTypes();
    }

}
