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
import static org.junit.Assert.fail;

import java.util.Arrays;
import java.util.HashSet;

import org.junit.Test;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.tests.AnonymousVersionedStruct;
import joynr.tests.DefaultMultipleVersionsInterfaceProvider;
import joynr.tests.InterfaceVersionedStruct;
import joynr.tests.MultipleVersionsInterfaceProxy;
import joynr.tests.MultipleVersionsTypeCollection.VersionedStruct;

public class MultipleVersionsEnd2EndTest extends AbstractMultipleVersionsEnd2EndTest {

    private static final String UNREGISTERING_FAILED_MESSAGE = "Unregistering of provider failed: ";

    /*
    * This test tests if 2 proxies of same interface (package versioned, unversioned) can connect to
    * one provider (unversioned) and communicate with this without mutual interference.
    */
    @Test
    public void proxiesOfDifferentVersioningTypesVsUnversionedProviderInSingleRuntime() throws Exception {
        // register provider
        DefaultMultipleVersionsInterfaceProvider unversionedProvider = new DefaultMultipleVersionsInterfaceProvider();

        registerProvider(unversionedProvider, domain);

        // build fitting proxies
        joynr.tests.v2.MultipleVersionsInterfaceProxy packageVersionedProxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                                         new HashSet<String>(Arrays.asList(domain)),
                                                                                         true);
        MultipleVersionsInterfaceProxy unversionedProxy = buildProxy(MultipleVersionsInterfaceProxy.class,
                                                                     new HashSet<String>(Arrays.asList(domain)),
                                                                     true);

        try {
            //set UInt8Attribute1 and check if it can be retrieved correctly
            unversionedProxy.setUInt8Attribute1((byte) 100);
            Byte value1 = packageVersionedProxy.getUInt8Attribute1();
            Byte value2 = unversionedProxy.getUInt8Attribute1();
            assertEquals((byte) value1, 100);
            assertEquals((byte) value2, 100);

            packageVersionedProxy.setUInt8Attribute1((byte) 50);
            value1 = packageVersionedProxy.getUInt8Attribute1();
            value2 = unversionedProxy.getUInt8Attribute1();
            assertEquals((byte) value1, 50);
            assertEquals((byte) value2, 50);

            // unregister provider
            providerRuntime.unregisterProvider(domain, unversionedProvider);
        } catch (JoynrRuntimeException e) {
            fail(UNREGISTERING_FAILED_MESSAGE + e);
        }
    }

    /*
     * This test tests if 2 providers of same interface and different versions can be registered in a single runtime
     * and 2 proxies can communicate with those without mutual interference.
     */
    @Test
    public void twoPackageVersionedProvidersInSingleRuntime() throws Exception {
        // register providers
        joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider provider1 = new joynr.tests.v1.DefaultMultipleVersionsInterfaceProvider();
        joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider provider2 = new joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider();

        registerProvider(provider1, domain);
        registerProvider(provider2, domain);

        // build fitting proxies
        joynr.tests.v1.MultipleVersionsInterfaceProxy proxy1 = buildProxy(joynr.tests.v1.MultipleVersionsInterfaceProxy.class,
                                                                          new HashSet<String>(Arrays.asList(domain)),
                                                                          true);
        MultipleVersionsInterfaceProxy proxy2 = buildProxy(MultipleVersionsInterfaceProxy.class,
                                                           new HashSet<String>(Arrays.asList(domain)),
                                                           true);

        //set UInt8Attribute1 and check if it can be retrieved correctly
        proxy1.setUInt8Attribute1((byte) 100);
        proxy2.setUInt8Attribute1((byte) 50);
        Byte value1 = proxy1.getUInt8Attribute1();
        Byte value2 = proxy2.getUInt8Attribute1();
        assertEquals((byte) value1, 100);
        assertEquals((byte) value2, 50);

        // unregister providers
        try {
            providerRuntime.unregisterProvider(domain, provider1);
            providerRuntime.unregisterProvider(domain, provider2);
        } catch (JoynrRuntimeException e) {
            fail(UNREGISTERING_FAILED_MESSAGE + e);
        }
    }

    private void testPackageVersionedTypes() throws Exception {
        final joynr.tests.v2.MultipleVersionsInterfaceProxy packageVersionedProxy = buildProxy(joynr.tests.v2.MultipleVersionsInterfaceProxy.class,
                                                                                               new HashSet<String>(Arrays.asList(domain)),
                                                                                               true);

        final joynr.tests.v2.AnonymousVersionedStruct input1 = new joynr.tests.v2.AnonymousVersionedStruct(random.nextBoolean());
        final joynr.tests.v2.AnonymousVersionedStruct output1 = packageVersionedProxy.getAnonymousVersionedStruct(input1);
        assertEquals(input1.getFlag2(), output1.getFlag2());

        final joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct input2 = new joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct(random.nextBoolean());
        final joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct output2 = packageVersionedProxy.getVersionedStruct(input2);
        assertEquals(input2.getFlag2(), output2.getFlag2());

        final joynr.tests.v2.InterfaceVersionedStruct input3 = new joynr.tests.v2.InterfaceVersionedStruct(random.nextBoolean(),
                                                                                                           random.nextBoolean());
        final joynr.tests.v2.InterfaceVersionedStruct output3 = packageVersionedProxy.getInterfaceVersionedStruct(input3);
        assertEquals(input3.getFlag1(), output3.getFlag1());
        assertEquals(input3.getFlag2(), output3.getFlag2());
    }

    private void testUnversionedTypes() throws Exception {
        final MultipleVersionsInterfaceProxy unversionedProxy = buildProxy(MultipleVersionsInterfaceProxy.class,
                                                                           new HashSet<String>(Arrays.asList(domain)),
                                                                           true);

        final AnonymousVersionedStruct input1 = new AnonymousVersionedStruct(random.nextBoolean());
        final AnonymousVersionedStruct output1 = unversionedProxy.getAnonymousVersionedStruct(input1);
        assertEquals(input1.getFlag2(), output1.getFlag2());

        final VersionedStruct input2 = new VersionedStruct(random.nextBoolean());
        final VersionedStruct output2 = unversionedProxy.getVersionedStruct(input2);
        assertEquals(input2.getFlag2(), output2.getFlag2());

        final InterfaceVersionedStruct input3 = new InterfaceVersionedStruct(random.nextBoolean(),
                                                                             random.nextBoolean());
        final InterfaceVersionedStruct output3 = unversionedProxy.getInterfaceVersionedStruct(input3);
        assertEquals(input3.getFlag1(), output3.getFlag1());
        assertEquals(input3.getFlag2(), output3.getFlag2());
    }

    @Test
    public void packageVersionedProxy_packageVersionedProvider_singleRuntime() throws Exception {
        registerPackageVersionedProvider();
        testPackageVersionedTypes();
    }

    @Test(timeout = CONST_GLOBAL_TEST_TIMEOUT_MS)
    public void packageVersionedProxy_packageVersionedProvider_separateRuntime() throws Exception {
        useGlobalCommunication();
        registerPackageVersionedProvider();
        testPackageVersionedTypes();
    }

    @Test
    public void unversionedProxy_unversionedProvider_singleRuntime() throws Exception {
        registerUnversionedProvider();
        testUnversionedTypes();
    }

    @Test(timeout = CONST_GLOBAL_TEST_TIMEOUT_MS)
    public void unversionedProxy_UnversionedProvider_separateRuntime() throws Exception {
        useGlobalCommunication();
        registerUnversionedProvider();
        testUnversionedTypes();
    }

}
