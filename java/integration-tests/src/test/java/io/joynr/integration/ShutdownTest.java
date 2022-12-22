/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import java.util.Properties;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import joynr.exceptions.ApplicationException;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class ShutdownTest {
    private static final Logger logger = LoggerFactory.getLogger(ShutdownTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private DummyJoynrApplication dummyApplication;
    private JoynrProvider provider;

    @Mock
    private MessageReceiver messageReceiverMock;

    private ProviderQos providerQos;

    @Before
    public void setup() {
        Properties factoryPropertiesProvider = new Properties();
        factoryPropertiesProvider.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "localdomain");
        factoryPropertiesProvider.put(MessagingPropertyKeys.CHANNELID, "ShutdownTestChannelId");

        MockitoAnnotations.initMocks(this);
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new TestGlobalAddressModule());
        dummyApplication = (DummyJoynrApplication) new JoynrInjectorFactory(factoryPropertiesProvider,
                                                                            runtimeModule).createApplication(DummyJoynrApplication.class);

        provider = new DefaulttestProvider();
        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        providerQos.setPriority(System.currentTimeMillis());
    }

    @Test(expected = JoynrShutdownException.class)
    public void testRegisterAfterShutdown() {
        dummyApplication.shutdown();
        dummyApplication.getRuntime()
                        .getProviderRegistrar("ShutdownTestdomain", provider)
                        .withProviderQos(providerQos)
                        .register();
    }

    @Test(expected = JoynrShutdownException.class)
    public void testUnregisterProviderAfterShutdown() {
        dummyApplication.getRuntime()
                        .getProviderRegistrar("ShutdownTestdomain", provider)
                        .withProviderQos(providerQos)
                        .register();
        dummyApplication.shutdown();
        dummyApplication.getRuntime().unregisterProvider("ShutdownTestdomain", provider);
    }

    @Test
    public void unregisterMultibleProvidersBeforeShutdown() throws JoynrWaitExpiredException, JoynrRuntimeException,
                                                            InterruptedException, ApplicationException {
        int providercount = 10;
        JoynrProvider[] providers = new JoynrProvider[providercount];
        for (int i = 0; i < providers.length; i++) {
            providerQos = new ProviderQos();
            providerQos.setScope(ProviderScope.LOCAL);
            providerQos.setPriority(System.currentTimeMillis());
            providers[i] = new DefaulttestProvider();
            Future<Void> registerFinished = dummyApplication.getRuntime()
                                                            .getProviderRegistrar("ShutdownTestdomain" + i,
                                                                                  providers[i])
                                                            .withProviderQos(providerQos)
                                                            .register();
            registerFinished.get();
        }
        for (int i = 0; i < providers.length; i++) {
            dummyApplication.getRuntime().unregisterProvider("ShutdownTestdomain" + i, providers[i]);
        }
        dummyApplication.shutdown();
    }

    @Test(expected = JoynrShutdownException.class)
    @Ignore
    // test is taking too long because it is attempting to send deregister requests that are not implemented in the mocks
    public void testProxyCallAfterShutdown() throws DiscoveryException, JoynrIllegalStateException,
                                             InterruptedException {
        Mockito.when(messageReceiverMock.getChannelId()).thenReturn("ShutdownTestChannelId");
        dummyApplication.getRuntime()
                        .getProviderRegistrar("ShutdownTestdomain", provider)
                        .withProviderQos(providerQos)
                        .register();
        ProxyBuilder<testProxy> proxyBuilder = dummyApplication.getRuntime().getProxyBuilder("ShutdownTestdomain",
                                                                                             testProxy.class);
        testProxy proxy = proxyBuilder.setDiscoveryQos(new DiscoveryQos(30000, ArbitrationStrategy.HighestPriority, 0))
                                      .build();
        dummyApplication.shutdown();

        proxy.getFirstPrime();
    }
}
