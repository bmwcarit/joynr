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
import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static io.joynr.integration.matchers.ChannelServiceResponseMatchers.containsChannel;
import static io.joynr.integration.matchers.ChannelServiceResponseMatchers.numberOfChannels;

import io.joynr.integration.util.ServersUtil;

import org.eclipse.jetty.server.Server;
import org.hamcrest.BaseMatcher;

import static org.hamcrest.CoreMatchers.anyOf;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

public class ControlledBounceProxyServerTest extends AbstractBounceProxyServerTest {

    private static final Logger logger = LoggerFactory.getLogger(ControlledBounceProxyServerTest.class);

    private static Server bounceProxyServerXY;
    private static Server bounceProxyControllerServer;

    @BeforeClass
    public static void startServer() throws Exception {

        // start different servers to make sure that handling of different URLs
        // works
        logger.info("Starting Bounceproxy Controller");
        bounceProxyControllerServer = ServersUtil.startBounceproxyController();

        logger.info("Starting Controlled Bounceproxy X.Y");
        bounceProxyServerXY = ServersUtil.startControlledBounceproxy("X.Y");

        logger.info("All servers started");
    }

    @AfterClass
    public static void stopServer() throws Exception {
        bounceProxyServerXY.stop();
        bounceProxyControllerServer.stop();
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

    @Test(timeout = 20000)
    public void testSimpleChannelSetupAndDeletionOnSingleBounceProxy() throws Exception {

        // get bounce proxies list
        JsonPath listBps = given().get("controller/bounceproxies").body().jsonPath();
        assertThat(listBps, anyOf(containsBounceProxy("X.Y", "ALIVE"), containsBounceProxy("X.Y", "ACTIVE")));

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = //
        given().header("X-Atmosphere-Tracking-Id", "test-trackingId").post("channels?ccid=test-channel");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        int bpPort = bounceProxyServerXY.getConnectors()[0].getPort();
        assertEquals("http://localhost:" + bpPort + "/bounceproxy/channels/test-channel/;jsessionid=.Y",
                     responseCreateChannel.getHeader("Location"));

        // list channels
        JsonPath listChannels = given().get("channels").getBody().jsonPath();
        assertThat(listChannels, is(numberOfChannels(1)));
        assertThat(listChannels, containsChannel("test-channel"));

        String bpUrl = "http://localhost:" + bpPort + "/bounceproxy/";
        RestAssured.baseURI = bpUrl;

        assertEquals(200 /* OK */, given().delete("channels/test-channel/").thenReturn().statusCode());
        // TODO include when listChannels is implemented for bounce proxies
        // JsonPath listBpChannels =
        // given().get("channels").getBody().jsonPath();
        // assertThat(listBpChannels, is(numberOfChannels(0)));
        // assertThat(listBpChannels, not(containsChannel("test-channel")));
    }

    @Test(timeout = 20000)
    public void testDeleteNonExistingChannel() throws Exception {

        int bpPort = bounceProxyServerXY.getConnectors()[0].getPort();
        RestAssured.baseURI = "http://localhost:" + bpPort + "/bounceproxy/";

        assertEquals(204 /* No Content */, given().delete("channels/non-existing-channel").thenReturn().statusCode());
    }

    @Test
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testSimpleChannelSetupOnTwoBounceProxies() throws Exception {

        // get bounce proxies list
        /* @formatter:off */
        JsonPath listBps = given().get("controller/bounceproxies").getBody().jsonPath();
        /* @formatter:on */
        assertThat(listBps, allOf( //
                                  anyOf(containsBounceProxy("X.Y", "ALIVE"), containsBounceProxy("X.Y", "ACTIVE")), //
                                  anyOf(containsBounceProxy("A.B", "ALIVE"), containsBounceProxy("A.B", "ACTIVE"))));

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
        // TODO remove until delete channel is implemented
        // assertThat(listChannels, is(numberOfChannels(2)));
        assertThat(listChannels, containsChannel("channel-123"));
        assertThat(listChannels, containsChannel("channel-abc"));
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
