package io.joynr.channel;

/*
 * #%L
 * joynr::java::backend-services::channelurldirectory
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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.bounceproxy.LocalGrizzlyBounceProxy;
import io.joynr.discovery.DiscoveryDirectoriesLauncher;
import io.joynr.guice.LowerCaseProperties;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.PropertyLoader;

import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.UUID;

import joynr.infrastructure.ChannelUrlDirectorySync;
import joynr.types.ChannelUrlInformation;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.google.inject.Injector;

public class ChannelUrlDirectoryTest {
    private Injector injectorConsumer;

    private static int port;

    private static LocalGrizzlyBounceProxy server = new LocalGrizzlyBounceProxy();

    private static DiscoveryDirectoriesLauncher directories;

    @BeforeClass
    public static void startServer() throws IOException {
        server = new LocalGrizzlyBounceProxy();
        port = server.start();
        String serverUrl = "http://localhost:" + port;
        String bounceProxyUrl = serverUrl + "/bounceproxy/";
        String directoriesUrl = bounceProxyUrl + "channels/discoverydirectory_channelid/";
        System.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);
        System.setProperty(MessagingPropertyKeys.CAPABILITIESDIRECTORYURL, directoriesUrl);
        System.setProperty(MessagingPropertyKeys.CHANNELURLDIRECTORYURL, directoriesUrl);

        directories = DiscoveryDirectoriesLauncher.start();
    }

    @AfterClass
    public static void stopServer() {
        directories.shutdown();
        server.stop();
    }

    @Before
    public void setup() {

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
    public void testChannelUrl() throws Exception {
        JoynrRuntime runtime = injectorConsumer.getInstance(JoynrRuntime.class);

        MessagingQos messagingQos = new MessagingQos();
        DiscoveryQos discoveryQos = new DiscoveryQos(50000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        Properties properties = PropertyLoader.loadProperties(new LowerCaseProperties(),
                                                              MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE);

        String channelUrlDomain = properties.getProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN);
        ProxyBuilder<ChannelUrlDirectorySync> proxyBuilder = runtime.getProxyBuilder(channelUrlDomain,
                                                                                     ChannelUrlDirectorySync.class);

        ChannelUrlDirectorySync proxy = proxyBuilder.setMessagingQos(messagingQos)
                                                    .setDiscoveryQos(discoveryQos)
                                                    .build();

        String[] urls = { "http://hello.com" };
        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        channelUrlInformation.setUrls(Arrays.asList(urls));
        String testChannelId = "testChannelId-ChannelUrlDirectoryTest.testChannelUrl";

        proxy.registerChannelUrls(testChannelId, channelUrlInformation);
        // proxy.registerChannelUrls(consumerChannelId, channelUrlInformation);

        String channelId = "testChannelUrl" + UUID.randomUUID().toString();
        proxy.registerChannelUrls(channelId, channelUrlInformation);
        ChannelUrlInformation urlInformation = proxy.getUrlsForChannel(channelId);

        List<String> urls2 = urlInformation.getUrls();
        proxy.unregisterChannelUrls(testChannelId);

        Assert.assertArrayEquals(urls, urls2.toArray(new String[urls2.size()]));
        proxy.unregisterChannelUrls(channelId);

    }
}
