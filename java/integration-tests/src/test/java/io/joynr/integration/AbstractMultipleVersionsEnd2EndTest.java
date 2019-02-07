/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import java.util.Properties;
import java.util.Random;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.Promise;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.tests.AnonymousVersionedStruct;
import joynr.tests.AnonymousVersionedStruct2;
import joynr.tests.DefaultMultipleVersionsInterface2Provider;
import joynr.tests.DefaultMultipleVersionsInterfaceProvider;
import joynr.tests.InterfaceVersionedStruct;
import joynr.tests.InterfaceVersionedStruct2;
import joynr.tests.MultipleVersionsTypeCollection.VersionedStruct;
import joynr.tests.MultipleVersionsTypeCollection.VersionedStruct2;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class AbstractMultipleVersionsEnd2EndTest {

    static final long DISCOVERY_TIMEOUT_MS = 1000;
    static final long CONST_DEFAULT_TEST_TIMEOUT_MS = DISCOVERY_TIMEOUT_MS * 3;
    static final String DOMAIN_PREFIX = "MultipleVersionsTestDomain-";
    private static final String PROXYBUILD_FAILED_MESSAGE = "Building of proxy failed: ";
    private static final String REGISTERING_FAILED_MESSAGE = "Registering of provider failed: ";

    Random random;
    DiscoveryQos discoveryQos;
    ProviderQos providerQos;
    JoynrRuntime runtime;
    private Semaphore proxyBuiltSemaphore;
    Semaphore noCompatibleProviderFoundCallbackSemaphore;
    String domain;
    private DefaultMultipleVersionsInterface2Provider nameVersionedProvider;
    private joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider packageVersionedProvider;
    private DefaultMultipleVersionsInterfaceProvider unversionedProvider;

    JoynrRuntime getCcRuntime() {
        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");

        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new TestGlobalAddressModule());
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             runtimeModule).createApplication(DummyJoynrApplication.class);

        return application.getRuntime();
    }

    @Before
    public void setUp() {
        random = new Random();

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(DISCOVERY_TIMEOUT_MS);
        discoveryQos.setRetryIntervalMs(100);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        // provider and proxy using same runtime to allow local-only communications
        runtime = getCcRuntime();

        domain = DOMAIN_PREFIX + createUuidString();
        proxyBuiltSemaphore = new Semaphore(0);
        noCompatibleProviderFoundCallbackSemaphore = new Semaphore(0, true);
    }

    @After
    public void tearDown() {
        if (nameVersionedProvider != null) {
            runtime.unregisterProvider(domain, nameVersionedProvider);
            nameVersionedProvider = null;
        }
        if (packageVersionedProvider != null) {
            runtime.unregisterProvider(domain, packageVersionedProvider);
            packageVersionedProvider = null;
        }
        if (unversionedProvider != null) {
            runtime.unregisterProvider(domain, unversionedProvider);
            unversionedProvider = null;
        }
        if (runtime != null) {
            runtime.shutdown(true);
        }
    }

    void registerProvider(AbstractJoynrProvider provider, String domain) {
        Future<Void> future = runtime.registerProvider(domain, provider, providerQos);

        try {
            future.get(100);
        } catch (Exception e) {
            fail(REGISTERING_FAILED_MESSAGE + e);
        }
    }

    <T> T buildProxy(final Class<T> interfaceClass,
                     final Set<String> domains,
                     final boolean waitForProxyCreation) throws Exception {
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

    void registerNameVersionedProvider() {
        nameVersionedProvider = new DefaultMultipleVersionsInterface2Provider() {

            @Override
            public Promise<GetVersionedStructDeferred> getVersionedStruct(VersionedStruct2 input) {
                GetVersionedStructDeferred deferred = new GetVersionedStructDeferred();
                final Promise<GetVersionedStructDeferred> promise = new Promise<GetVersionedStructDeferred>(deferred);
                deferred.resolve(new VersionedStruct2(input.getFlag2()));
                return promise;
            }

            @Override
            public Promise<GetAnonymousVersionedStructDeferred> getAnonymousVersionedStruct(AnonymousVersionedStruct2 input) {
                GetAnonymousVersionedStructDeferred deferred = new GetAnonymousVersionedStructDeferred();
                final Promise<GetAnonymousVersionedStructDeferred> promise = new Promise<GetAnonymousVersionedStructDeferred>(deferred);
                deferred.resolve(new AnonymousVersionedStruct2(input.getFlag2()));
                return promise;
            }

            @Override
            public Promise<GetInterfaceVersionedStructDeferred> getInterfaceVersionedStruct(InterfaceVersionedStruct2 input) {
                GetInterfaceVersionedStructDeferred deferred = new GetInterfaceVersionedStructDeferred();
                final Promise<GetInterfaceVersionedStructDeferred> promise = new Promise<GetInterfaceVersionedStructDeferred>(deferred);
                deferred.resolve(new InterfaceVersionedStruct2(input.getFlag1(), input.getFlag2()));
                return promise;
            }

        };
        registerProvider(nameVersionedProvider, domain);
    }

    void registerPackageVersionedProvider() {
        packageVersionedProvider = new joynr.tests.v2.DefaultMultipleVersionsInterfaceProvider() {

            @Override
            public Promise<GetVersionedStructDeferred> getVersionedStruct(joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct input) {
                GetVersionedStructDeferred deferred = new GetVersionedStructDeferred();
                final Promise<GetVersionedStructDeferred> promise = new Promise<GetVersionedStructDeferred>(deferred);
                deferred.resolve(new joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct(input.getFlag2()));
                return promise;
            }

            @Override
            public Promise<GetAnonymousVersionedStructDeferred> getAnonymousVersionedStruct(joynr.tests.v2.AnonymousVersionedStruct input) {
                GetAnonymousVersionedStructDeferred deferred = new GetAnonymousVersionedStructDeferred();
                final Promise<GetAnonymousVersionedStructDeferred> promise = new Promise<GetAnonymousVersionedStructDeferred>(deferred);
                deferred.resolve(new joynr.tests.v2.AnonymousVersionedStruct(input.getFlag2()));
                return promise;
            }

            @Override
            public Promise<GetInterfaceVersionedStructDeferred> getInterfaceVersionedStruct(joynr.tests.v2.InterfaceVersionedStruct input) {
                GetInterfaceVersionedStructDeferred deferred = new GetInterfaceVersionedStructDeferred();
                final Promise<GetInterfaceVersionedStructDeferred> promise = new Promise<GetInterfaceVersionedStructDeferred>(deferred);
                deferred.resolve(new joynr.tests.v2.InterfaceVersionedStruct(input.getFlag1(), input.getFlag2()));
                return promise;
            }

        };
        registerProvider(packageVersionedProvider, domain);
    }

    void registerUnversionedProvider() {
        unversionedProvider = new DefaultMultipleVersionsInterfaceProvider() {

            @Override
            public Promise<GetVersionedStructDeferred> getVersionedStruct(VersionedStruct input) {
                GetVersionedStructDeferred deferred = new GetVersionedStructDeferred();
                final Promise<GetVersionedStructDeferred> promise = new Promise<GetVersionedStructDeferred>(deferred);
                deferred.resolve(new VersionedStruct(input.getFlag2()));
                return promise;
            }

            @Override
            public Promise<GetAnonymousVersionedStructDeferred> getAnonymousVersionedStruct(AnonymousVersionedStruct input) {
                GetAnonymousVersionedStructDeferred deferred = new GetAnonymousVersionedStructDeferred();
                final Promise<GetAnonymousVersionedStructDeferred> promise = new Promise<GetAnonymousVersionedStructDeferred>(deferred);
                deferred.resolve(new AnonymousVersionedStruct(input.getFlag2()));
                return promise;
            }

            @Override
            public Promise<GetInterfaceVersionedStructDeferred> getInterfaceVersionedStruct(InterfaceVersionedStruct input) {
                GetInterfaceVersionedStructDeferred deferred = new GetInterfaceVersionedStructDeferred();
                final Promise<GetInterfaceVersionedStructDeferred> promise = new Promise<GetInterfaceVersionedStructDeferred>(deferred);
                deferred.resolve(new InterfaceVersionedStruct(input.getFlag1(), input.getFlag2()));
                return promise;
            }

        };
        registerProvider(unversionedProvider, domain);
    }

}
