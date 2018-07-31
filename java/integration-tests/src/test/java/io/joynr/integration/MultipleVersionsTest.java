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

import java.util.Properties;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.tests.DefaultMultipleVersionsInterface1Provider;
import joynr.tests.DefaultMultipleVersionsInterface2Provider;
import joynr.tests.DefaultMultipleVersionsInterfaceProvider;
import joynr.tests.MultipleVersionsInterface1Proxy;
import joynr.tests.MultipleVersionsInterface2Proxy;
import joynr.tests.v2.MultipleVersionsInterfaceProxy;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class MultipleVersionsTest {
    private static final String DOMAIN = "MultipleVersionsTestDomain";
    private static final String PROXYBUILD_FAILED_MESSAGE = "Building of proxy failed: ";
    private static final String REGISTERING_FAILED_MESSAGE = "Registering of provider failed: ";
    private static final String UNREGISTERING_FAILED_MESSAGE = "Unregistering of provider failed: ";

    private Semaphore proxyBuilt;

    private DiscoveryQos discoveryQos;
    private ProviderQos providerQos;

    private JoynrRuntime runtime;

    @Before
    public void setUp() {
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(5000);
        discoveryQos.setRetryIntervalMs(100);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        proxyBuilt = new Semaphore(0);

        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");

        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new TestGlobalAddressModule());
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             runtimeModule).createApplication(DummyJoynrApplication.class);

        runtime = application.getRuntime();
    }

    @After
    public void tearDown() {
        runtime.shutdown(true);
    }

    /**
     * Registers given provider in runtime.
     * @param provider
     */
    private void asyncRegisterProvider(AbstractJoynrProvider provider) {
        Future<Void> future = runtime.registerProvider(DOMAIN, provider, providerQos);

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
    private <T> T asyncBuildProxyForInterface(final Class<T> interfaceClass) {
        ProxyBuilder<T> proxyBuilder = runtime.getProxyBuilder(DOMAIN, interfaceClass);
        T proxy = null;
        try {
            proxy = proxyBuilder.setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<T>() {
                @Override
                public void onProxyCreationFinished(T result) {
                    proxyBuilt.release();
                }

                @Override
                public void onProxyCreationError(JoynrRuntimeException error) {
                    throw error;
                }
            });
            assertTrue(proxyBuilt.tryAcquire(1, TimeUnit.SECONDS));
        } catch (DiscoveryException | InterruptedException e) {
            fail(PROXYBUILD_FAILED_MESSAGE + e);
        }
        return proxy;
    }

    /*
    * This test tests if 2 proxies of same interface (name versioned and package versioned) can connect to
    * one provider (unversioned) and communicate with this without mutual interference.
    */
    @Test
    public void twoProxiesOfDifferentVersioningTypesVsUnversionedProvider() {
        // register provider
        DefaultMultipleVersionsInterfaceProvider provider = new DefaultMultipleVersionsInterfaceProvider();

        asyncRegisterProvider(provider);

        // build fitting proxies
        MultipleVersionsInterface2Proxy proxy1 = asyncBuildProxyForInterface(MultipleVersionsInterface2Proxy.class);
        MultipleVersionsInterfaceProxy proxy2 = asyncBuildProxyForInterface(MultipleVersionsInterfaceProxy.class);

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
            runtime.unregisterProvider(DOMAIN, provider);
        } catch (JoynrRuntimeException e) {
            fail(UNREGISTERING_FAILED_MESSAGE + e);
        }
    }

    /*
     * This test tests if 2 providers of same interface and different versions can be registered in a single runtime
     * and 2 proxies can communicate with those without mutual interference.
     */
    @Test
    @Ignore
    public void twoNameVersionedProvidersInSingleRuntime() {
        // register providers
        DefaultMultipleVersionsInterface1Provider provider1 = new DefaultMultipleVersionsInterface1Provider();
        DefaultMultipleVersionsInterface2Provider provider2 = new DefaultMultipleVersionsInterface2Provider();

        asyncRegisterProvider(provider1);
        asyncRegisterProvider(provider2);

        // build fitting proxies
        MultipleVersionsInterface1Proxy proxy1 = asyncBuildProxyForInterface(MultipleVersionsInterface1Proxy.class);
        MultipleVersionsInterface2Proxy proxy2 = asyncBuildProxyForInterface(MultipleVersionsInterface2Proxy.class);

        //set UInt8Attribute1 and check if it can be retrieved correctly
        proxy1.setUInt8Attribute1((byte) 100);
        proxy2.setUInt8Attribute1((byte) 50);
        Byte value1 = proxy1.getUInt8Attribute1();
        Byte value2 = proxy2.getUInt8Attribute1();
        assertEquals((byte) value1, 100);
        assertEquals((byte) value2, 50);

        // unregister providers
        try {
            runtime.unregisterProvider(DOMAIN, provider1);
            runtime.unregisterProvider(DOMAIN, provider2);
        } catch (JoynrRuntimeException e) {
            fail(UNREGISTERING_FAILED_MESSAGE + e);
        }
    }

}
