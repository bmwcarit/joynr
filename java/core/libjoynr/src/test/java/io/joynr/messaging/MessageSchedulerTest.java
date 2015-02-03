package io.joynr.messaging;

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

import static org.junit.Assert.assertEquals;
import io.joynr.messaging.http.operation.FailureAction;
import io.joynr.runtime.JoynrBaseModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.io.IOException;
import java.util.LinkedList;
import java.util.Properties;
import java.util.UUID;

import joynr.types.ChannelUrlInformation;

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

import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.util.Modules;

@RunWith(MockitoJUnitRunner.class)
public class MessageSchedulerTest {

    private LocalTestServer server;

    private static final String BOUNCEPROXYPATH = "/bounceproxy/";
    private static final String CHANNELPATH = BOUNCEPROXYPATH + "channels/";

    private String bounceProxyUrl;
    private String channelId = "MessageSchedulerTest_" + UUID.randomUUID().toString();

    private String serviceAddress;

    private int sendMessageResponseCode;
    private String sendMessageId;

    private MessageScheduler messageScheduler;

    @Mock
    private MessageContainer mockMessageContainer;
    @Mock
    private FailureAction mockFailureAction;
    @Mock
    private MessageReceiver mockMessageReceiver;

    @Mock
    private LocalChannelUrlDirectoryClient mockChannelUrlDir;

    private boolean serverResponded = false;

    @Before
    public void setUp() throws Exception {

        String messagePath = CHANNELPATH + channelId + "/message/";

        server = new LocalTestServer(null, null);
        server.register(messagePath, new HttpRequestHandler() {

            @Override
            public void handle(HttpRequest request, HttpResponse response, HttpContext context) throws HttpException,
                                                                                               IOException {
                response.setStatusCode(sendMessageResponseCode);
                response.setHeader("msgId", sendMessageId);
                serverResponded = true;
            }
        });
        server.start();
        serviceAddress = "http://" + server.getServiceAddress().getHostName() + ":"
                + server.getServiceAddress().getPort();
        bounceProxyUrl = serviceAddress + BOUNCEPROXYPATH;

        Properties properties = new Properties();
        properties.put(MessagingPropertyKeys.CHANNELID, channelId);
        properties.put(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrl);

        AbstractModule mockModule = new AbstractModule() {

            @Override
            protected void configure() {
                bind(LocalChannelUrlDirectoryClient.class).toInstance(mockChannelUrlDir);
            }

        };

        Injector injector = new JoynrInjectorFactory(new JoynrBaseModule(properties,
                                                                         Modules.override(new MessagingTestModule())
                                                                                .with(mockModule))).getInjector();
        messageScheduler = injector.getInstance(MessageScheduler.class);
    }

    @Test
    public void testSendMessageWithResponseCodeCreated() {
        sendMessageResponseCode = HttpStatus.SC_CREATED;
        testSendMessage();
    }

    @Test
    public void testMapDomainName() throws Exception {
        String uri = "http://myhost.com:80/x/y/z/index.html?name=xyz";
        String mapHost = messageScheduler.mapHost(uri);
        assertEquals(mapHost, "http://localhost:9096/x/y/z/index.html?name=xyz");

        uri = "https://myhost.com:80/x/y/z/index.html?name=xyz#XYZ";
        mapHost = messageScheduler.mapHost(uri);
        assertEquals(mapHost, "https://localhost:9096/x/y/z/index.html?name=xyz");

        uri = "https://myhost2.com:80/x/y/z/index.html?name=xyz#XYZ";
        mapHost = messageScheduler.mapHost(uri);
        assertEquals(mapHost, "https://localhost:9096/z/index.html?name=xyz");

        uri = "https://myhost3.com:80/x/y/z/index.html?name=xyz#XYZ";
        mapHost = messageScheduler.mapHost(uri);
        assertEquals(mapHost, "https://localhost:9096/a/z/index.html?name=xyz");

    }

    @Test
    public void testSendMessageWithResponseCodeOk() {
        sendMessageResponseCode = HttpStatus.SC_OK;
        testSendMessage();
    }

    private void testSendMessage() {

        sendMessageId = "msgId-" + UUID.randomUUID().toString();

        ChannelUrlInformation channelUrlInfo = new ChannelUrlInformation();
        LinkedList<String> urls = new LinkedList<String>();
        urls.add(bounceProxyUrl + "channels/" + channelId + "/");
        channelUrlInfo.setUrls(urls);
        Mockito.when(mockChannelUrlDir.getUrlsForChannel(channelId)).thenReturn(channelUrlInfo);

        Mockito.when(mockMessageContainer.getMessageId()).thenReturn(sendMessageId);
        Mockito.when(mockMessageContainer.getChannelId()).thenReturn(channelId);
        Mockito.when(mockMessageContainer.isExpired()).thenReturn(false);
        Mockito.when(mockMessageContainer.getSerializedMessage())
               .thenReturn("any message, doesn't matter here if it's JSON or not");
        Mockito.when(mockMessageContainer.getExpiryDate()).thenReturn(0l);
        Mockito.when(mockMessageReceiver.isChannelCreated()).thenReturn(true);

        messageScheduler.scheduleMessage(mockMessageContainer, 0, mockFailureAction, mockMessageReceiver);

        // There's no way to hook into some MessageScheduler method and to wait
        // until the response has been processed. Only if a failure is expected,
        // one could wait until failureAction is called.
        // wait a bit to make sure the request is handled. Thus we just wait a
        // little bit and check then if the request reached the server at all
        // and check that no failureAction was called.
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        Assert.assertTrue(serverResponded);
        Mockito.verifyZeroInteractions(mockFailureAction);
    }

    @After
    public void tearDown() throws Exception {
        server.stop();
    }
}
