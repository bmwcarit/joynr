package io.joynr.messaging.bounceproxy.controller.integration;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import static com.jayway.restassured.RestAssured.given;
import static org.hamcrest.CoreMatchers.allOf;
import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static org.mockito.Matchers.any;
import io.joynr.messaging.bounceproxy.controller.IsCreateChannelHttpRequest;
import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.service.ChannelServiceConstants;

import java.io.IOException;
import java.util.List;

import org.apache.http.HttpException;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.localserver.LocalTestServer;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestHandler;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.webapp.WebAppContext;
import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

@RunWith(MockitoJUnitRunner.class)
public class NormalOperationTest {

    private String serverUrl;

    private Server jettyServer;

    @Mock
    HttpRequestHandler mockBounceProxyRequestHandler;

    private LocalTestServer mockBounceProxy;
    private String mockBounceProxyUrl;

    @Before
    public void setUp() throws Exception {

        // starts the server with a random port
        jettyServer = new Server(0);

        WebAppContext bpCtrlWebapp = new WebAppContext();
        bpCtrlWebapp.setResourceBase("./src/main/java");
        bpCtrlWebapp.setDescriptor("./src/main/resources/WEB-INF/web.xml");

        jettyServer.setHandler(bpCtrlWebapp);

        jettyServer.start();

        int port = jettyServer.getConnectors()[0].getLocalPort();
        serverUrl = String.format("http://localhost:%d", port);

        // use local test server to intercept http requests sent by the reporter
        mockBounceProxy = new LocalTestServer(null, null);
        mockBounceProxy.register("*", mockBounceProxyRequestHandler);
        mockBounceProxy.start();

        mockBounceProxyUrl = "http://localhost:" + mockBounceProxy.getServiceAddress().getPort() + "/";
    }

    @After
    public void tearDown() throws Exception {
        jettyServer.stop();
        mockBounceProxy.stop();
    }

    @Test
    public void testSimpleChannelSetupOnSingleBounceProxy() throws Exception {

        setMockedHttpResponseForChannel("channel-123",
                                        "trackingId-123",
                                        "X.Y",
                                        "http://www.joynX.de/bp/channels/channel-123");

        // register new bounce proxy
        Response responseCreateBp = //
        given(). //
               when()
               .queryParam("url4cc", "http://www.joynX.de/bp")
               .and()
               .queryParam("url4bpc", mockBounceProxyUrl)
               .put(serverUrl + "/controller/bounceproxies?bpid=X.Y");
        assertEquals(201 /* Created */, responseCreateBp.getStatusCode());

        // get bounce proxies list
        JsonPath listBps = given().get(serverUrl + "/controller/bounceproxies").body().jsonPath();
        assertThat(listBps, containsBounceProxy("X.Y", BounceProxyStatus.ALIVE));

        // create channel on bounce proxy
        Response responseCreateChannel = //
        given().header(ChannelServiceConstants.X_ATMOSPHERE_TRACKING_ID, "trackingId-123").post(serverUrl
                + "/channels?ccid=channel-123");
        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        assertEquals("http://www.joynX.de/bp/channels/channel-123;jsessionid=.Y",
                     responseCreateChannel.getHeader("Location"));

        // list channels
        JsonPath listChannels = given().get(serverUrl + "/channels").getBody().jsonPath();
        assertThat(listChannels, is(numberOfChannels(1)));
        assertThat(listChannels, containsChannel("channel-123"));

        // check if handler was called
        Mockito.verify(mockBounceProxyRequestHandler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest("channel-123", "trackingId-123")),
                       Mockito.any(HttpResponse.class),
                       Mockito.any(HttpContext.class));
    }

    @Test
    public void testSimpleChannelSetupOnTwoBounceProxies() throws Exception {

        setMockedHttpResponseForChannel("channel-123",
                                        "trackingId-123",
                                        "X.Y",
                                        "http://www.joynX.de/bp/channels/channel-123");
        setMockedHttpResponseForChannel("channel-abc",
                                        "trackingId-abc",
                                        "A.B",
                                        "http://www.joynA.de/bp/channels/channel-abc");

        // register two bounce proxies
        Response responseCreateFirstBp = //
        given(). //
               when()
               .queryParam("url4cc", "http://www.joynX.de/bp")
               .and()
               .queryParam("url4bpc", mockBounceProxyUrl)
               .put(serverUrl + "/controller/bounceproxies?bpid=X.Y");
        assertEquals(201 /* Created */, responseCreateFirstBp.getStatusCode());

        Response responseCreateSecondBp = //
        given(). //
               when()
               .queryParam("url4cc", "http://www.joynA.de/bp")
               .and()
               .queryParam("url4bpc", mockBounceProxyUrl)
               .put(serverUrl + "/controller/bounceproxies?bpid=A.B");
        assertEquals(201 /* Created */, responseCreateSecondBp.getStatusCode());

        // get bounce proxies list
        JsonPath listBps = given().get(serverUrl + "/controller/bounceproxies").getBody().jsonPath();
        assertThat(listBps, allOf( //
                                  containsBounceProxy("X.Y", BounceProxyStatus.ALIVE), //
                                  containsBounceProxy("A.B", BounceProxyStatus.ALIVE)));

        // create channel on bounce proxy
        Response responseCreateFirstChannel = //
        given().header(ChannelServiceConstants.X_ATMOSPHERE_TRACKING_ID, "trackingId-123").post(serverUrl
                + "/channels?ccid=channel-123");
        assertEquals(201 /* Created */, responseCreateFirstChannel.getStatusCode());
        assertEquals("X.Y", responseCreateFirstChannel.getHeader("bp"));
        assertEquals("http://www.joynX.de/bp/channels/channel-123;jsessionid=.Y",
                     responseCreateFirstChannel.getHeader("Location"));

        // create channel on different bounce proxy
        Response responseCreateSecondChannel = //
        given().header(ChannelServiceConstants.X_ATMOSPHERE_TRACKING_ID, "trackingId-abc").post(serverUrl
                + "/channels?ccid=channel-abc");
        assertEquals(201 /* Created */, responseCreateSecondChannel.getStatusCode());
        assertEquals("A.B", responseCreateSecondChannel.getHeader("bp"));
        assertEquals("http://www.joynA.de/bp/channels/channel-abc;jsessionid=.B",
                     responseCreateSecondChannel.getHeader("Location"));

        // list channels
        JsonPath listChannels = given().get(serverUrl + "/channels").getBody().jsonPath();
        assertThat(listChannels, is(numberOfChannels(2)));
        assertThat(listChannels, containsChannel("channel-123"));
        assertThat(listChannels, containsChannel("channel-abc"));
    }

    private void setMockedHttpResponseForChannel(final String ccid,
                                                 final String trackingId,
                                                 final String bpId,
                                                 final String location) throws HttpException, IOException {
        // HttpResponse is set as out parameter of the handle method. The way to
        // set out parameters with Mockito is to use doAnswer
        Answer<Void> answerForHttpResponse = new Answer<Void>() {
            public Void answer(InvocationOnMock invocation) throws Throwable {
                HttpResponse httpResponse = (HttpResponse) invocation.getArguments()[1];
                httpResponse.setStatusCode(HttpStatus.SC_CREATED);
                httpResponse.setHeader("Location", location);
                httpResponse.setHeader("bp", bpId);
                return null;
            }
        };
        Mockito.doAnswer(answerForHttpResponse)
               .when(mockBounceProxyRequestHandler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest(ccid, trackingId)),
                       any(HttpResponse.class),
                       any(HttpContext.class));
    }

    private Matcher<JsonPath> containsChannel(final String channelId) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                List<String> channelIds = jsonPath.getList("channelId");

                for (String c : channelIds) {
                    if (c.equals(channelId)) {
                        return true;
                    }
                }

                return false;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains channel ID '" + channelId + "'");
            }

        };
    }

    private Matcher<JsonPath> numberOfChannels(final int size) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                List<String> channelIds = jsonPath.getList("channelId");
                return channelIds.size() == size;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains " + size + " channels");
            }

        };
    }

    private Matcher<JsonPath> containsBounceProxy(final String id, final BounceProxyStatus status) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                for (int i = 0; i < jsonPath.getList("").size(); i++) {

                    if (jsonPath.get("[" + i + "].status").equals(status.name())
                            && jsonPath.get("[" + i + "].bounceProxyId").equals(id)) {
                        return true;
                    }
                }

                return false;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains entry with status=" + status.name() + " and bounceProxyId=" + id);
            }

            @Override
            public void describeMismatch(final Object item, final Description description) {
                description.appendText("was").appendValue(((JsonPath) item).get(""));
            }
        };
    }

}
