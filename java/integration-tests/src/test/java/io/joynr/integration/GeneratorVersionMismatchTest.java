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
import org.junit.Before;
import org.junit.Test;

import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider;

public class GeneratorVersionMismatchTest extends AbstractMultipleVersionsEnd2EndTest {

    private String domain2;
    private joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider provider;
    private joynr.tests.v2.MultipleVersionsInterfaceProxy proxy;

    @Override
    @Before
    public void setUp() {
        super.setUp();

        domain2 = "domain2-" + UUID.randomUUID().toString();
        provider = new DefaultMultipleVersionsInterfaceProvider();

        registerProvider(provider, domain);
        registerProvider(provider, domain2);
    }

    @Override
    @After
    public void tearDown() {
        runtime.unregisterProvider(domain, provider);
        runtime.unregisterProvider(domain2, provider);

        super.tearDown();
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

        proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                           new HashSet<String>(Arrays.asList(domain)),
                           false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(3, TimeUnit.SECONDS));

        checkProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testMultiDomainNoCompatibleProviderFound() throws Exception {

        proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                           new HashSet<String>(Arrays.asList(domain, domain2)),
                           false);

        // wait for the proxy created error callback to be called
        assertTrue("Unexpected successful proxy creation or timeout",
                   noCompatibleProviderFoundCallbackSemaphore.tryAcquire(3, TimeUnit.SECONDS));

        checkProxy();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void testProxyIsInvalidatedOnceArbitrationExceptionThrown() throws Exception {

        proxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                           new HashSet<String>(Arrays.asList(domain)),
                           false);

        checkProxy();
        checkProxy();
    }

}
