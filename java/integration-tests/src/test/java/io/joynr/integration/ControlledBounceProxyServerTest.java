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
import static io.joynr.integration.matchers.ChannelServiceResponseMatchers.containsChannel;
import static io.joynr.integration.matchers.MessagingServiceResponseMatchers.containsMessage;
import static org.hamcrest.CoreMatchers.allOf;
import static org.hamcrest.CoreMatchers.anyOf;
import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.util.Utilities;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import org.eclipse.jetty.server.Server;
import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

public class ControlledBounceProxyServerTest extends AbstractBounceProxyServerTest {

    private static final Logger logger = LoggerFactory.getLogger(ControlledBounceProxyServerTest.class);

    private static Server bounceProxyServerXY;
    private static Server bounceProxyControllerServer;

    private static int bounceProxyServerXyPort;
    private static String bounceProxyServerXyUrl;

    @BeforeClass
    public static void startServer() throws Exception {

        // start different servers to make sure that handling of different URLs
        // works
        logger.info("Starting Bounceproxy Controller");
        bounceProxyControllerServer = ServersUtil.startBounceproxyController();

        logger.info("Starting Controlled Bounceproxy X.Y");
        bounceProxyServerXY = ServersUtil.startControlledBounceproxy("X.Y");

        logger.info("All servers started");

        bounceProxyServerXyPort = bounceProxyServerXY.getConnectors()[0].getPort();
        bounceProxyServerXyUrl = "http://localhost:" + bounceProxyServerXyPort + "/bounceproxy/";
    }

    @AfterClass
    public static void stopServer() throws Exception {
        bounceProxyServerXY.stop();
        bounceProxyControllerServer.stop();
    }

    @Override
    protected String getBounceProxyBaseUri() {
        return bounceProxyServerXyUrl;
    }

    @Test(timeout = 20000)
    public void testSimpleChannelSetupAndDeletionOnSingleBounceProxy() throws Exception {

        // get bounce proxies list
        JsonPath listBps = given().get("bounceproxies").body().jsonPath();
        assertThat(listBps, anyOf(containsBounceProxy("X.Y", "ALIVE"), containsBounceProxy("X.Y", "ACTIVE")));

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = //
        given().header("X-Atmosphere-Tracking-Id", "test-trackingId").post("channels?ccid=test-channel");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        int bpPort = bounceProxyServerXY.getConnectors()[0].getPort();
        assertEquals("http://localhost:" + bpPort + "/bounceproxy/channels/test-channel/",
                     responseCreateChannel.getHeader("Location"));

        // list channels
        JsonPath listChannels = given().get("channels").getBody().jsonPath();
        // TODO uncomment as soon as channel deletion is implemented
        // assertThat(listChannels, is(numberOfChannels(1)));
        assertThat(listChannels, containsChannel("test-channel"));

        String bpUrl = "http://localhost:" + bpPort + "/bounceproxy/";
        RestAssured.baseURI = bpUrl;

        JsonPath listBpChannels = given().get("channels").getBody().jsonPath();
        // TODO uncomment as soon as channel deletion is implemented
        // assertThat(listBpChannels, is(numberOfChannels(2)));
        assertThat(listBpChannels, containsChannel("test-channel"));
        assertThat(listBpChannels, containsChannel("/*"));

        assertEquals(200 /* OK */, given().delete("channels/test-channel/").thenReturn().statusCode());
        JsonPath listBpChannelsAfterDelete = given().get("channels").getBody().jsonPath();
        // TODO uncomment as soon as channel deletion is implemented
        // assertThat(listBpChannelsAfterDelete, is(numberOfChannels(1)));
        assertThat(listBpChannelsAfterDelete, not(containsChannel("test-channel")));
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
        JsonPath listBps = given().get("bounceproxies").getBody().jsonPath();
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

    // timeout has to be longer than the long poll duration!!!
    @Test(timeout = 40000)
    public void testNormalMessagingWithoutMessagesPending() throws Exception {

        final String channelId = "channel_testNormalMessagingWithoutMessagesPending";
        final String trackingId = "trackingId_testNormalMessagingWithoutMessagesPending";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header("X-Atmosphere-Tracking-Id", trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        String channelUrl = responseCreateChannel.getHeader("Location");
        assertEquals(bounceProxyServerXyUrl + "channels/" + channelId + "/", channelUrl);

        RestAssured.baseURI = channelUrl;

        // open long polling channel
        /* @formatter:off */
        Response responseOpenChannel = given().when().contentType(ContentType.JSON).header("X-Atmosphere-tracking-id",
                                                                                           trackingId).get("");
        /* @formatter:on */
        assertEquals(200 /* OK */, responseOpenChannel.getStatusCode());

        // long poll should return without messages after a while
        // body doesn't actually contain proper json, but each message
        // serialized as json attached. We have to split them first.
        String body = responseOpenChannel.getBody().asString();
        List<String> messages = Utilities.splitJson(body);
        assertEquals(0, messages.size());
    }

    @Test(timeout = 20000)
    public void testNormalMessagingWithOneMessagePendingBeforeChannelWasOpened() throws Exception {

        final String channelId = "channel-testNormalMessagingWithOneMessagePendingBeforeChannelWasOpened";
        final String trackingId = "trackingId-testNormalMessagingWithOneMessagePendingBeforeChannelWasOpened";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header("X-Atmosphere-Tracking-Id", trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        String channelUrl = responseCreateChannel.getHeader("Location");
        assertEquals(bounceProxyServerXyUrl + "channels/" + channelId + "/", channelUrl);

        RestAssured.baseURI = channelUrl;

        // post messages to long polling channel before opening channel
        String serializedMessage = createJoynrMessage(100000l, "message-123", "message-123");

        /* @formatter:off */
        Response responsePostMessage = given().when()
                                              .contentType(ContentType.JSON)
                                              .body(serializedMessage)
                                              .post("message");
        /* @formatter:on */
        assertEquals(201 /* Created */, responsePostMessage.getStatusCode());
        assertEquals(bounceProxyServerXyUrl + "messages/message-123", responsePostMessage.getHeader("Location"));
        assertEquals("message-123", responsePostMessage.getHeader("msgId"));

        // open long polling channel
        /* @formatter:off */
        Response responseOpenChannel = given().when().contentType(ContentType.JSON).header("X-Atmosphere-tracking-id",
                                                                                           trackingId).get("");
        /* @formatter:on */
        assertEquals(200 /* OK */, responseOpenChannel.getStatusCode());

        // body doesn't actually contain proper json, but each message
        // serialized as json attached. We have to split them first.
        String body = responseOpenChannel.getBody().asString();
        List<String> messages = Utilities.splitJson(body);
        assertEquals(1, messages.size());

        // we cut the brackets of some of the messages when we split the
        // messages
        assertThat(messages, containsMessage("message-123"));
    }

    @Test(timeout = 30000)
    public void testNormalMessagingWithMultipleMessagesPendingBeforeChannelWasOpened() throws Exception {

        final String channelId = "channel-testNormalMessagingWithMultipleMessagesPendingBeforeChannelWasOpened";
        final String trackingId = "trackingId-testNormalMessagingWithMultipleMessagesPendingBeforeChannelWasOpened";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header("X-Atmosphere-Tracking-Id", trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        String channelUrl = responseCreateChannel.getHeader("Location");
        assertEquals(bounceProxyServerXyUrl + "channels/" + channelId + "/", channelUrl);

        RestAssured.baseURI = channelUrl;

        // post messages to long polling channel before opening channel
        String msgIds[] = { "message-123", "message-456", "message-789" };
        for (String msgId : msgIds) {
            String serializedMessage = createJoynrMessage(100000l, msgId, msgId);

            /* @formatter:off */
            Response responsePostMessage = given().when()
                                                  .log()
                                                  .all()
                                                  .contentType(ContentType.JSON)
                                                  .body(serializedMessage)
                                                  .post("message");
            /* @formatter:on */
            assertEquals(201 /* Created */, responsePostMessage.getStatusCode());
            assertEquals(bounceProxyServerXyUrl + "messages/" + msgId, responsePostMessage.getHeader("Location"));
            assertEquals(msgId, responsePostMessage.getHeader("msgId"));
        }

        // open long polling channel
        /* @formatter:off */
        Response responseOpenChannel = given().when().contentType(ContentType.JSON).header("X-Atmosphere-tracking-id",
                                                                                           trackingId).get("");
        /* @formatter:on */
        assertEquals(200 /* OK */, responseOpenChannel.getStatusCode());

        // body doesn't actually contain proper json, but each message
        // serialized as json attached. We have to split them first.
        String body = responseOpenChannel.getBody().asString();
        List<String> messages = Utilities.splitJson(body);
        assertEquals(3, messages.size());

        assertThat(messages, containsMessage("message-123"));
        assertThat(messages, containsMessage("message-456"));
        assertThat(messages, containsMessage("message-789"));
    }

    @Test
    public void testNormalMessagingWithMultipleMessagePostsAfterChannelWasOpened() throws Exception {

        final String channelId = "channel-testNormalMessagingWithMultipleMessagePostsAfterChannelWasOpened";
        final String trackingId = "trackingId-testNormalMessagingWithMultipleMessagePostsAfterChannelWasOpened";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header("X-Atmosphere-Tracking-Id", trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        final String channelUrl = responseCreateChannel.getHeader("Location");
        assertEquals(bounceProxyServerXyUrl + "channels/" + channelId + "/", channelUrl);

        RestAssured.baseURI = channelUrl;

        // open long polling channel first in separate thread
        Future<?> longPollingChannelFuture = Executors.newSingleThreadExecutor().submit(new Callable<Response>() {

            @Override
            public Response call() throws Exception {

                /* @formatter:off */
                return given().when()
                              .contentType(ContentType.JSON)
                              .header("X-Atmosphere-tracking-id", trackingId)
                              .get("");
                /* @formatter:on */
            }
        });

        // post messages to long polling channel after opening channel
        String msgIds[] = { "message-123", "message-456", "message-789" };
        for (String msgId : msgIds) {
            String serializedMessage = createJoynrMessage(100000l, msgId, msgId);

            /* @formatter:off */
            Response responsePostMessage = given().when()
                                                  .contentType(ContentType.JSON)
                                                  .body(serializedMessage)
                                                  .post("message");
            /* @formatter:on */
            assertEquals(201 /* Created */, responsePostMessage.getStatusCode());
            assertEquals(bounceProxyServerXyUrl + "messages/" + msgId, responsePostMessage.getHeader("Location"));
            assertEquals(msgId, responsePostMessage.getHeader("msgId"));
        }

        Response responseOpenChannel = (Response) longPollingChannelFuture.get(10, TimeUnit.SECONDS);
        assertEquals(200 /* OK */, responseOpenChannel.getStatusCode());

        // body doesn't actually contain proper json, but each message
        // serialized as json attached. We have to split them first.
        // Long poll will return after first message sent.
        String body = responseOpenChannel.getBody().asString();
        List<String> messages = Utilities.splitJson(body);
        assertEquals(1, messages.size());

        // we cut the brackets of some of the messages when we split the
        // messages
        assertThat(messages, containsMessage("message-123"));
    }

    @Test
    public void testPostMessageToNonExistingChannel() throws Exception {
        RestAssured.baseURI = bounceProxyServerXyUrl;
        super.testPostMessageToNonExistingChannel();
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
