/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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
package io.joynr.messaging.bounceproxy.controller;

import static org.mockito.Matchers.any;

import java.io.IOException;
import java.net.URI;
import java.util.Properties;

import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
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
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.messaging.bounceproxy.IsCreateChannelHttpRequest;
import io.joynr.messaging.bounceproxy.controller.exception.JoynrProtocolException;
import io.joynr.messaging.info.ControlledBounceProxyInformation;

@RunWith(MockitoJUnitRunner.class)
public class RemoteBounceProxyFacadeTest {

    private RemoteBounceProxyFacade bpFacade;

    @Mock
    HttpRequestHandler handler;

    private LocalTestServer server;

    private String serverUrl;

    @Before
    public void setUp() throws Exception {

        // use local test server to intercept http requests sent by the reporter
        server = new LocalTestServer(null, null);
        server.register("*", handler);
        server.start();

        serverUrl = "http://localhost:" + server.getServiceAddress().getPort() + "/";

        Properties properties = new Properties();
        properties.put(BounceProxyControllerPropertyKeys.PROPERTY_BPC_SEND_CREATE_CHANNEL_MAX_RETRY_COUNT, "3");
        properties.put(BounceProxyControllerPropertyKeys.PROPERTY_BPC_SEND_CREATE_CHANNEL_RETRY_INTERVAL_MS, "100");

        Injector injector = Guice.createInjector(new PropertyLoadingModule(properties), new AbstractModule() {

            @Override
            protected void configure() {
                bind(CloseableHttpClient.class).toInstance(HttpClients.createDefault());
            }
        });

        bpFacade = injector.getInstance(RemoteBounceProxyFacade.class);
    }

    @After
    public void tearDown() {
        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
    }

    @Test
    public void testSuccessfulChannelCreation() throws Exception {

        setMockedHttpRequestHandlerResponse(HttpStatus.SC_CREATED, "http://www.joynX.de/channels", "X.Y");

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("X.Y", URI.create(serverUrl));
        URI channelUri = bpFacade.createChannel(bpInfo, "channel-123", "trackingId-123");

        Mockito.verify(handler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest("channel-123", "trackingId-123")),
                       Mockito.any(HttpResponse.class),
                       Mockito.any(HttpContext.class));
        Assert.assertEquals("http://www.joynX.de/channels", channelUri.toString());
    }

    @Test
    public void testChannelCreationWhenServerIsUnreachable() throws Exception {

        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
        server.awaitTermination(1000);

        try {
            ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("X.Y",
                                                                                           URI.create(serverUrl));
            bpFacade.createChannel(bpInfo, "channel-123", "trackingId-123");
            Assert.fail();
        } catch (Exception e) {
            System.err.println(e);
        }

    }

    @Test
    public void testBounceProxyRejectsChannelCreation() throws Exception {

        setMockedHttpRequestHandlerResponse(HttpStatus.SC_NO_CONTENT, "http://www.joynX.de/channels", "X.Y");

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("X.Y", URI.create(serverUrl));
        try {
            bpFacade.createChannel(bpInfo, "channel-123", "trackingId-123");
            Assert.fail();
        } catch (JoynrProtocolException e) {

        }
        Mockito.verify(handler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest("channel-123", "trackingId-123")),
                       Mockito.any(HttpResponse.class),
                       Mockito.any(HttpContext.class));
    }

    @Test
    public void testBounceProxyReturnsUnexpectedBounceProxyId() throws Exception {

        setMockedHttpRequestHandlerResponse(HttpStatus.SC_CREATED, "http://www.joynX.de/channels", "A.B");

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("X.Y", URI.create(serverUrl));
        try {
            bpFacade.createChannel(bpInfo, "channel-123", "trackingId-123");
            Assert.fail();
        } catch (JoynrProtocolException e) {

        }
        Mockito.verify(handler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest("channel-123", "trackingId-123")),
                       Mockito.any(HttpResponse.class),
                       Mockito.any(HttpContext.class));
    }

    @Test
    public void testBounceProxyReturnsNoBounceProxyId() throws Exception {

        setMockedHttpRequestHandlerResponse(HttpStatus.SC_CREATED, "http://www.joynX.de/channels", null);

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("X.Y", URI.create(serverUrl));
        try {
            bpFacade.createChannel(bpInfo, "channel-123", "trackingId-123");
            Assert.fail();
        } catch (JoynrProtocolException e) {

        }
        Mockito.verify(handler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest("channel-123", "trackingId-123")),
                       Mockito.any(HttpResponse.class),
                       Mockito.any(HttpContext.class));
    }

    @Test
    public void testBounceProxyReturnsNoLocation() throws Exception {

        setMockedHttpRequestHandlerResponse(HttpStatus.SC_CREATED, null, "X.Y");

        ControlledBounceProxyInformation bpInfo = new ControlledBounceProxyInformation("X.Y", URI.create(serverUrl));
        try {
            bpFacade.createChannel(bpInfo, "channel-123", "trackingId-123");
            Assert.fail();
        } catch (JoynrProtocolException e) {

        }
        Mockito.verify(handler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest("channel-123", "trackingId-123")),
                       Mockito.any(HttpResponse.class),
                       Mockito.any(HttpContext.class));
    }

    /**
     * Sets the HTTP response returned by the
     * {@link HttpRequestHandler#handle(HttpRequest, HttpResponse, HttpContext)}
     * method.
     *
     * @param httpStatus
     *            the desired HTTP status to be returned as HTTP response
     * @throws HttpException
     * @throws IOException
     */
    private void setMockedHttpRequestHandlerResponse(final int httpStatus,
                                                     final String location,
                                                     final String bpId) throws HttpException, IOException {

        // HttpResponse is set as out parameter of the handle method. The way to
        // set out parameters with Mockito is to use doAnswer
        Answer<Void> answerForHttpResponse = new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                HttpResponse httpResponse = (HttpResponse) invocation.getArguments()[1];
                httpResponse.setStatusCode(httpStatus);
                httpResponse.setHeader("Location", location);
                httpResponse.setHeader("bp", bpId);
                return null;
            }
        };
        Mockito.doAnswer(answerForHttpResponse)
               .when(handler)
               .handle(any(HttpRequest.class), any(HttpResponse.class), any(HttpContext.class));
    }
}
