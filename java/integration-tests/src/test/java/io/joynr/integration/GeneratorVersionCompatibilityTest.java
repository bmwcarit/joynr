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

import java.util.Arrays;
import java.util.HashSet;

import org.junit.Test;

import joynr.tests.MultipleVersionsInterfaceProxy;
import joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider;

public class GeneratorVersionCompatibilityTest extends AbstractMultipleVersionsEnd2EndTest {

    private void testProxyCreationAgainstPackageVersionedProvider(Class<?> proxyClass) throws Exception {
        joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider provider_packageVersion = new DefaultMultipleVersionsInterfaceProvider();
        registerProvider(provider_packageVersion, domain);

        buildProxy(proxyClass, new HashSet<String>(Arrays.asList(domain)), true);

        providerRuntime.unregisterProvider(domain, provider_packageVersion);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void nonVersionedProxyCreationAgainstPackageVersionedProviderSucceeds() throws Exception {
        testProxyCreationAgainstPackageVersionedProvider(MultipleVersionsInterfaceProxy.class);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void packageVersionedProxyCreationAgainstPackageVersionedProviderSucceeds() throws Exception {
        testProxyCreationAgainstPackageVersionedProvider(joynr.tests.v2.MultipleVersionsInterfaceProxy.class);
    }

}
