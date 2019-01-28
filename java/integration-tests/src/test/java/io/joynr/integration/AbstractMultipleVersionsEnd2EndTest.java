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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Properties;
import java.util.Set;
import java.util.UUID;
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
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class AbstractMultipleVersionsEnd2EndTest {

    static final long DISCOVERY_TIMEOUT_MS = 1000;
    static final long CONST_DEFAULT_TEST_TIMEOUT_MS = DISCOVERY_TIMEOUT_MS * 3;
    static final String DOMAIN_PREFIX = "MultipleVersionsTestDomain-";
    private static final String PROXYBUILD_FAILED_MESSAGE = "Building of proxy failed: ";
    private static final String REGISTERING_FAILED_MESSAGE = "Registering of provider failed: ";

    DiscoveryQos discoveryQos;
    ProviderQos providerQos;
    JoynrRuntime runtime;
    private Semaphore proxyBuiltSemaphore;
    Semaphore noCompatibleProviderFoundCallbackSemaphore;
    String domain;

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
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(DISCOVERY_TIMEOUT_MS);
        discoveryQos.setRetryIntervalMs(100);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        // provider and proxy using same runtime to allow local-only communications
        runtime = getCcRuntime();

        domain = DOMAIN_PREFIX + UUID.randomUUID().toString();
        proxyBuiltSemaphore = new Semaphore(0);
        noCompatibleProviderFoundCallbackSemaphore = new Semaphore(0, true);
    }

    @After
    public void tearDown() {
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

}
