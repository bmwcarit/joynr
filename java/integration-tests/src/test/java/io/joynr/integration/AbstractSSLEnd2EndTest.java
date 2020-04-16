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

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;

import java.util.Properties;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.mockito.MockitoAnnotations;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;

import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.PropertyLoader;
import io.joynr.runtime.ProviderRegistrar;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;

public abstract class AbstractSSLEnd2EndTest extends JoynrEnd2EndTest {

    private static final Logger logger = LoggerFactory.getLogger(AbstractSSLEnd2EndTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    DefaulttestProvider provider;
    String domain;

    private MessagingQos messagingQos;
    private DiscoveryQos discoveryQos;

    @Rule
    public TestName name = new TestName();

    private JoynrRuntime providerRuntime;
    private JoynrRuntime consumerRuntime;

    // Overridden by test environment implementations
    protected abstract JoynrRuntime getRuntime(Properties joynrConfig, Module... modules);

    @Before
    public void setup() throws Exception {
        MockitoAnnotations.initMocks(this);

        String methodName = name.getMethodName();
        logger.info("{} setup beginning...", methodName);

        domain = "SSLEnd2EndTest." + methodName + System.currentTimeMillis();
        provisionPermissiveAccessControlEntry(domain, ProviderAnnotations.getInterfaceName(DefaulttestProvider.class));

        // use channelNames = test name
        String channelIdProvider = "JavaTest-" + methodName + createUuidString() + "-end2endTestProvider";
        String channelIdConsumer = "JavaTest-" + methodName + createUuidString() + "-end2endConsumer";

        Properties joynrConfigProvider = PropertyLoader.loadProperties("testMessaging.properties");
        joynrConfigProvider.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "localdomain." + createUuidString());
        joynrConfigProvider.put(MessagingPropertyKeys.CHANNELID, channelIdProvider);
        joynrConfigProvider.put(MessagingPropertyKeys.RECEIVERID, createUuidString());

        providerRuntime = getRuntime(joynrConfigProvider, new StaticDomainAccessControlProvisioningModule());

        Properties joynrConfigConsumer = PropertyLoader.loadProperties("testMessaging.properties");
        joynrConfigConsumer.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "localdomain." + createUuidString());
        joynrConfigConsumer.put(MessagingPropertyKeys.CHANNELID, channelIdConsumer);
        joynrConfigConsumer.put(MessagingPropertyKeys.RECEIVERID, createUuidString());

        consumerRuntime = getRuntime(joynrConfigConsumer);

        provider = new DefaulttestProvider();
        providerRuntime.getProviderRegistrar(domain, provider).register();

        messagingQos = new MessagingQos(5000);
        discoveryQos = new DiscoveryQos(5000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        // Make sure the channel is created before the first messages sent.
        Thread.sleep(200);
        logger.info("Setup finished");
    }

    @After
    public void tearDown() {
        providerRuntime.shutdown(true);
        consumerRuntime.shutdown(true);
    }

    @Ignore
    @Test
    public void getAndSetAttribute() {

        // Build a client proxy
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);

        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        // Set an attribute value
        int value = 1234;
        proxy.setReadWriteAttribute(value);

        // Get the attribute value
        int actual = proxy.getReadWriteAttribute();
        assertEquals(value, actual);
    }

}
