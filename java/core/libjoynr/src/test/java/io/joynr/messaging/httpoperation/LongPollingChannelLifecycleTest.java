package io.joynr.messaging.httpoperation;

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

import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingTestModule;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.runtime.JoynrBaseModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.io.IOException;
import java.util.Properties;
import java.util.UUID;

import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
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
    private MessageReceiver mockMessageReceiver;
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

        Injector injector = new JoynrInjectorFactory(new JoynrBaseModule(properties, new MessagingTestModule())).getInjector();
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
        longpollingChannelLifecycle.startLongPolling(mockMessageReceiver, mockReceiverStatusListener);

        Mockito.verify(mockReceiverStatusListener, Mockito.timeout(5000)).receiverStarted();
        Assert.assertTrue(longpollingChannelLifecycle.isChannelCreated());

        Mockito.verifyNoMoreInteractions(mockReceiverStatusListener);
        Mockito.verifyZeroInteractions(mockMessageReceiver);
    }

    @After
    public void tearDown() throws Exception {
        server.stop();
    }
}
