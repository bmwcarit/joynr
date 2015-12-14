package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.util.Properties;

import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import com.google.inject.AbstractModule;
import com.google.inject.util.Modules;

public class ShutdownTest {

    private DummyJoynrApplication dummyApplication;
    private JoynrProvider provider;

    @Mock
    private MessageReceiver messageReceiverMock;
    @Mock
    private MessageSender messageSenderMock;

    @Before
    public void setup() {
        Properties factoryPropertiesProvider = new Properties();
        factoryPropertiesProvider.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "localdomain");
        factoryPropertiesProvider.put(MessagingPropertyKeys.CHANNELID, "ShutdownTestChannelId");

        MockitoAnnotations.initMocks(this);
        dummyApplication = (DummyJoynrApplication) new JoynrInjectorFactory(factoryPropertiesProvider,
                                                                            Modules.override(new CCInProcessRuntimeModule())
                                                                                   .with(new AbstractModule() {
                                                                                       @Override
                                                                                       protected void configure() {
                                                                                           bind(MessageReceiver.class).toInstance(messageReceiverMock);
                                                                                           bind(MessageSender.class).toInstance(messageSenderMock);
                                                                                       }
                                                                                   })).createApplication(DummyJoynrApplication.class);

        provider = new DefaulttestProvider();
    }

    @Test(expected = JoynrShutdownException.class)
    public void testRegisterAfterShutdown() {
        dummyApplication.shutdown();
        dummyApplication.getRuntime().registerProvider("ShutdownTestdomain", provider);
    }

    @Test(expected = JoynrShutdownException.class)
    @Ignore
    // test is taking too long because it is attempting to send deregister requests that are not implemented in the mocks
    public void testProxyCallAfterShutdown() throws DiscoveryException, JoynrIllegalStateException,
                                            InterruptedException {
        Mockito.when(messageReceiverMock.getChannelId()).thenReturn("ShutdownTestChannelId");
        dummyApplication.getRuntime().registerProvider("ShutdownTestdomain", provider);
        ProxyBuilder<testProxy> proxyBuilder = dummyApplication.getRuntime().getProxyBuilder("ShutdownTestdomain",
                                                                                             testProxy.class);
        testProxy proxy = proxyBuilder.setDiscoveryQos(new DiscoveryQos(30000, ArbitrationStrategy.HighestPriority, 0))
                                      .build();
        dummyApplication.shutdown();

        proxy.getFirstPrime();
    }

    @Ignore
    @Test(expected = JoynrShutdownException.class)
    public void testProxyCreationAfterShutdown() throws DiscoveryException, JoynrIllegalStateException,
                                                InterruptedException {
        // TODO
        // Arbitration does not check if the runtime is already shutting down. A test like this would fail.
        ProxyBuilder<testProxy> proxyBuilder = dummyApplication.getRuntime().getProxyBuilder("ShutdownTestdomain",
                                                                                             testProxy.class);
        testProxy proxy = proxyBuilder.setDiscoveryQos(new DiscoveryQos(30000, ArbitrationStrategy.HighestPriority, 0))
                                      .build();
        dummyApplication.shutdown();

        proxy.getFirstPrime();
    }
}
