package io.joynr.messaging.http.operation;

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

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Singleton;
import io.joynr.capabilities.DummyDiscoveryModule;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingSettings;
import io.joynr.messaging.ReceiverStatusListener;

import java.io.IOException;
import java.util.Properties;
import java.util.UUID;

import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.localserver.LocalTestServer;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestHandler;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.Injector;

@RunWith(MockitoJUnitRunner.class)
public class LongPollingChannelLifecycleTest {

    private LocalTestServer server;

    private static final String BOUNCEPROXYPATH = "/bounceproxy/";
    private static final String CHANNELPATH = BOUNCEPROXYPATH + "channels/";

    private String bounceProxyUrl;
    private String channelId = "LongPollingCallableTest_" + UUID.randomUUID().toString();

    private String serviceAddress;

    private LongPollingChannelLifecycle longpollingChannelLifecycle;

    @Mock
    private MessageArrivedListener mockMessageArrivedListener;
    @Mock
    private ReceiverStatusListener mockReceiverStatusListener;

    private int createChannelResponseCode;

    @Before
    public void setUp() throws Exception {
        server = new LocalTestServer(null, null);
        server.register(CHANNELPATH, new HttpRequestHandler() {

            @Override
            public void handle(HttpRequest request, HttpResponse response, HttpContext context) throws HttpException,
                                                                                               IOException {
                response.setStatusCode(createChannelResponseCode);
                response.setHeader("Location", bounceProxyUrl + "channels/" + channelId);
            }
        });
        server.start();
        serviceAddress = "http://" + server.getServiceAddress().getHostName() + ":"
                + server.getServiceAddress().getPort();
        bounceProxyUrl = serviceAddress + BOUNCEPROXYPATH;

        Properties properties = new Properties();
        properties.put(MessagingPropertyKeys.CHANNELID, channelId);
        properties.put(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);

        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            public void configure() {
                bind(HttpRequestFactory.class).to(ApacheHttpRequestFactory.class);
                bind(CloseableHttpClient.class).toProvider(HttpClientProvider.class).in(Singleton.class);
                bind(RequestConfig.class).toProvider(HttpDefaultRequestConfigProvider.class).in(Singleton.class);
                bind(MessagingSettings.class).to(ConfigurableMessagingSettings.class);
            }
        }, new JoynrPropertiesModule(properties), new JsonMessageSerializerModule(), new DummyDiscoveryModule());
        longpollingChannelLifecycle = injector.getInstance(LongPollingChannelLifecycle.class);
    }

    @Test
    public void testLongPollingLifecycleWithResponseCodeCreated() {
        createChannelResponseCode = HttpStatus.SC_CREATED;
        testCreateChannel();
    }

    @Test
    public void testLongPollingLifecycleWithResponseCodeOk() {
        createChannelResponseCode = HttpStatus.SC_OK;
        testCreateChannel();
    }

    private void testCreateChannel() {
        longpollingChannelLifecycle.startLongPolling(mockMessageArrivedListener, mockReceiverStatusListener);

        Mockito.verify(mockReceiverStatusListener, Mockito.timeout(5000)).receiverStarted();
        Assert.assertTrue(longpollingChannelLifecycle.isChannelCreated());

        Mockito.verifyNoMoreInteractions(mockReceiverStatusListener);
        Mockito.verifyZeroInteractions(mockMessageArrivedListener);
    }

    @After
    public void tearDown() throws Exception {
        longpollingChannelLifecycle.shutdown();
        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
        ;
    }
}
