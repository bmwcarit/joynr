package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.guice.LowerCaseProperties;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.PropertyLoader;

import java.util.Properties;
import java.util.UUID;

import joynr.infrastructure.ChannelUrlDirectorySync;
import joynr.types.ChannelUrlInformation;

import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;

public class ChannelUrlDirectoryTest {
    private static final Logger logger = LoggerFactory.getLogger(ChannelUrlDirectoryTest.class);

    @Rule
    public TestName name = new TestName();

    private Injector injectorConsumer;

    private static Server jettyServer;

    @BeforeClass
    public static void startServer() throws Exception {
        jettyServer = ServersUtil.startServers();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        jettyServer.stop();
    }

    @Before
    public void setup() {
        // prints the tests name in the log so we know what we are testing
        String methodName = name.getMethodName();
        logger.info(methodName + " setup beginning...");
        injectorConsumer = new JoynrInjectorFactory().getInjector();
    }

    @After
    public void tearDown() throws InterruptedException {
        // Get the messageReceiver and delete the channel
        MessageReceiver messageReceiver = injectorConsumer.getInstance(MessageReceiver.class);
        messageReceiver.shutdown(true);
        Thread.sleep(200);
    }

    @Test
    public void testRegisterChannelUrl() throws Exception {
        ChannelUrlDirectorySync proxy = createChannelUrlDirectoryProxy();

        String testChannelId = name.getMethodName() + UUID.randomUUID().toString();
        String[] urls = { "http://testurl.com/" + testChannelId + "/" };
        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        channelUrlInformation.setUrls(urls);

        proxy.registerChannelUrls(testChannelId, channelUrlInformation);

        ChannelUrlInformation urlsForChannelId = proxy.getUrlsForChannel(testChannelId);

        proxy.unregisterChannelUrls(testChannelId);

        String[] urlsFromServer = urlsForChannelId.getUrls();
        Assert.assertArrayEquals(urls, urlsFromServer);
        proxy.unregisterChannelUrls(testChannelId);

    }

    private ChannelUrlDirectorySync createChannelUrlDirectoryProxy() throws InterruptedException {
        return createChannelUrlDirectoryProxy(MessagingQos.DEFAULT_TTL);
    }

    @Test
    public void testMissingChannelUrl() throws Exception {
        int ttl_ms = 500;
        ChannelUrlDirectorySync proxy = createChannelUrlDirectoryProxy(ttl_ms);
        String testChannelId = name.getMethodName() + UUID.randomUUID().toString();

        ChannelUrlInformation urlInformation = null;
        try {
            urlInformation = proxy.getUrlsForChannel(testChannelId);
            assertTrue("synchronized proxy call \"getUrlsForChannel\" shall cause a timeout exception, because the provider only replies in case a url is available for the mentioned channel",
                       false);
        } catch (Exception e) {
            //exception expected
        }

        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        proxy.registerChannelUrls(testChannelId, channelUrlInformation);

        urlInformation = proxy.getUrlsForChannel(testChannelId);

        assertEquals(channelUrlInformation, urlInformation);
    }

    private ChannelUrlDirectorySync createChannelUrlDirectoryProxy(long ttl_ms) throws InterruptedException {
        JoynrRuntime runtime = injectorConsumer.getInstance(JoynrRuntime.class);

        MessagingQos messagingQos = new MessagingQos(ttl_ms);
        DiscoveryQos discoveryQos = new DiscoveryQos(50000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        Properties properties = PropertyLoader.loadProperties(new LowerCaseProperties(),
                                                              MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);

        String channelUrlDomain = properties.getProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN);
        ProxyBuilder<ChannelUrlDirectorySync> proxyBuilder = runtime.getProxyBuilder(channelUrlDomain,
                                                                                     ChannelUrlDirectorySync.class);

        ChannelUrlDirectorySync proxy = proxyBuilder.setMessagingQos(messagingQos)
                                                    .setDiscoveryQos(discoveryQos)
                                                    .build();
        return proxy;
    }
}
