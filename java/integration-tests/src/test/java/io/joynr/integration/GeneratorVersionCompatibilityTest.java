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

import java.util.UUID;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import io.joynr.exceptions.DiscoveryException;
import io.joynr.proxy.ProxyBuilder;
import joynr.tests.DefaultMultipleVersionsInterface2Provider;
import joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider;

public class GeneratorVersionCompatibilityTest extends AbstractMultipleVersionsEnd2EndTest {
    private static final long CONST_DEFAULT_TEST_TIMEOUT_MS = 3000;

    private String domain;

    @Override
    @Before
    public void setUp() {
        super.setUp();
        domain = "domain-" + UUID.randomUUID().toString();
    }

    @After
    public void tearDown() {
        if (runtime != null) {
            runtime.shutdown(true);
        }
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
