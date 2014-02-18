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

import static com.jayway.restassured.RestAssured.given;
import static org.hamcrest.CoreMatchers.allOf;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static org.mockito.Matchers.any;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.bounceproxy.IsCreateChannelHttpRequest;

import java.io.IOException;
import java.util.List;

import org.apache.http.HttpException;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.localserver.LocalTestServer;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestHandler;
import org.eclipse.jetty.server.Server;
import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
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
public class ControlledBounceProxyServerTest extends AbstractBounceProxyServerTest {

    private static Server bounceProxyServer;
    private static Server bounceProxyControllerServer;

    @BeforeClass
    public static void startServer() throws Exception {
        // start different servers to make sure that handling of different URLs
        // works
        bounceProxyServer = ServersUtil.startBounceproxy();
        bounceProxyControllerServer = ServersUtil.startBounceproxyController();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        bounceProxyServer.stop();
        bounceProxyControllerServer.stop();
    }

    @Mock
    HttpRequestHandler mockBounceProxyRequestHandler;

    private LocalTestServer mockBounceProxy;
    private String mockBounceProxyUrl;

    @Before
    public void setUp() throws Exception {
        super.setUp();

        // use local test server to intercept http requests sent by the reporter
        mockBounceProxy = new LocalTestServer(null, null);
        mockBounceProxy.register("*", mockBounceProxyRequestHandler);
        mockBounceProxy.start();

        mockBounceProxyUrl = "http://localhost:" + mockBounceProxy.getServiceAddress().getPort() + "/";
    }

    @After
    public void tearDown() throws Exception {
        mockBounceProxy.stop();
    }

    @Ignore("Messaging not yet implemented for controlled bounceproxy")
    @Test(timeout = 20000)
    // This is a test to see if the atmos bug still exists. If the bug exists,
    // the server will hang 20 secs
    public void testSendAndReceiveMessagesOnAtmosphereServer() throws Exception {
    }

    @Ignore("Messaging not yet implemented for controlled bounceproxy")
    @Test
    public void testPostMessageToNonExistingChannel() throws Exception {
    }

    @Test
    public void testSimpleChannelSetupOnSingleBounceProxy() throws Exception {

        setMockedHttpResponseForChannel("test-channel",
                                        "test-trackingId",
                                        "testX.testY",
                                        "http://www.joynX.de/bp/channels/test-channel");

        // register new bounce proxy
        /* @formatter:off */
        Response responseCreateBp = //
        given().when()
               .queryParam("url4cc", "http://www.joynX.de/bp")
               .and()
               .queryParam("url4bpc", mockBounceProxyUrl)
               .put("controller/bounceproxies?bpid=testX.testY");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateBp.getStatusCode());

        // get bounce proxies list
        JsonPath listBps = given().get("controller/bounceproxies").body().jsonPath();
        assertThat(listBps, containsBounceProxy("testX.testY", "ALIVE"));

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = //
        given().header("X-Atmosphere-Tracking-Id", "test-trackingId").post("channels?ccid=test-channel");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("testX.testY", responseCreateChannel.getHeader("bp"));
        assertEquals("http://www.joynX.de/bp/channels/test-channel;jsessionid=.testY",
                     responseCreateChannel.getHeader("Location"));

        // list channels
        JsonPath listChannels = given().get("channels").getBody().jsonPath();
        // TODO remove until delete channel is implemented assertThat(listChannels, is(numberOfChannels(1)));
        assertThat(listChannels, containsChannel("test-channel"));

        // check if handler was called
        Mockito.verify(mockBounceProxyRequestHandler)
               .handle(Mockito.argThat(new IsCreateChannelHttpRequest("test-channel", "test-trackingId")),
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
        /* @formatter:off */
        Response responseCreateFirstBp = //
        given(). //
               when()
               .queryParam("url4cc", "http://www.joynX.de/bp")
               .and()
               .queryParam("url4bpc", mockBounceProxyUrl)
               .put("controller/bounceproxies?bpid=X.Y");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateFirstBp.getStatusCode());

        /* @formatter:off */
        Response responseCreateSecondBp = //
        given(). //
               when()
               .queryParam("url4cc", "http://www.joynA.de/bp")
               .and()
               .queryParam("url4bpc", mockBounceProxyUrl)
               .put("controller/bounceproxies?bpid=A.B");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateSecondBp.getStatusCode());

        // get bounce proxies list
        /* @formatter:off */
        JsonPath listBps = given().get("controller/bounceproxies").getBody().jsonPath();
        /* @formatter:on */
        assertThat(listBps, allOf( //
                                  containsBounceProxy("X.Y", "ALIVE"), //
                                  containsBounceProxy("A.B", "ALIVE")));

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateFirstChannel = //
        given().header("X-Atmosphere-Tracking-Id", "trackingId-123").post("channels?ccid=channel-123");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateFirstChannel.getStatusCode());
        assertEquals("X.Y", responseCreateFirstChannel.getHeader("bp"));
        assertEquals("http://www.joynX.de/bp/channels/channel-123;jsessionid=.Y",
                     responseCreateFirstChannel.getHeader("Location"));

        // create channel on different bounce proxy
        /* @formatter:off */
        Response responseCreateSecondChannel = //
        given().header("X-Atmosphere-Tracking-Id", "trackingId-abc").post("channels?ccid=channel-abc");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateSecondChannel.getStatusCode());
        assertEquals("A.B", responseCreateSecondChannel.getHeader("bp"));
        assertEquals("http://www.joynA.de/bp/channels/channel-abc;jsessionid=.B",
                     responseCreateSecondChannel.getHeader("Location"));

        // list channels
        /* @formatter:off */
        JsonPath listChannels = given().get("channels").getBody().jsonPath();
        /* @formatter:on */
        // TODO remove until delete channel is implemented assertThat(listChannels, is(numberOfChannels(2)));
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

    private Matcher<JsonPath> containsBounceProxy(final String id, final String status) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                for (int i = 0; i < jsonPath.getList("").size(); i++) {

                    if (jsonPath.get("[" + i + "].status").equals(status)
                            && jsonPath.get("[" + i + "].bounceProxyId").equals(id)) {
                        return true;
                    }
                }

                return false;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains entry with status=" + status + " and bounceProxyId=" + id);
            }

            @Override
            public void describeMismatch(final Object item, final Description description) {
                description.appendText("was").appendValue(((JsonPath) item).get(""));
            }
        };
    }
}
