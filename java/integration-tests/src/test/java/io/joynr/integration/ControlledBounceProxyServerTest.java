package io.joynr.integration;

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

import static com.jayway.restassured.RestAssured.given;
import static io.joynr.integration.matchers.ChannelServiceResponseMatchers.isChannelUrlwithJsessionId;
import static io.joynr.integration.matchers.MessagingServiceResponseMatchers.containsMessage;
import static io.joynr.integration.matchers.MessagingServiceResponseMatchers.isMessageUrlwithJsessionId;
import static io.joynr.integration.util.BounceProxyTestConstants.HEADER_BOUNCEPROXY_ID;
import static io.joynr.integration.util.BounceProxyTestConstants.HEADER_LOCATION;
import static io.joynr.integration.util.BounceProxyTestConstants.HEADER_MSG_ID;
import static io.joynr.integration.util.BounceProxyTestConstants.SESSIONID_APPENDIX;
import static io.joynr.integration.util.BounceProxyTestConstants.SESSIONID_NAME;
import static io.joynr.integration.util.BounceProxyTestConstants.X_ATMOSPHERE_TRACKING_ID;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThat;
import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.messaging.datatypes.JoynrMessagingError;
import io.joynr.messaging.datatypes.JoynrMessagingErrorCode;
import io.joynr.messaging.util.Utilities;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;

import org.junit.Assert;
import org.junit.Ignore;
import org.junit.Test;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;

//@RunWith(MultipleBounceProxySetupsTestRunner.class)
//@BounceProxyServerSetups(value = { ControlledBounceProxyCluster.class, SingleControlledBounceProxy.class })
public class ControlledBounceProxyServerTest extends AbstractBounceProxyServerTest {

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    @Override
    protected String getBounceProxyBaseUri() {
        return configuration.getAnyBounceProxyUrl();
    }

    @Test(timeout = 20000)
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testDeleteNonExistingChannel() throws Exception {
        RestAssured.baseURI = configuration.getAnyBounceProxyUrl();
        assertEquals(204 /* No Content */, given().delete("channels/non-existing-channel").thenReturn().statusCode());
    }

    // timeout has to be longer than the long poll duration!!!
    @Test(timeout = 40000)
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testNormalMessagingWithoutMessagesPending() throws Exception {

        final String channelId = "channel_testNormalMessagingWithoutMessagesPending";
        final String trackingId = "trackingId_testNormalMessagingWithoutMessagesPending";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header(X_ATMOSPHERE_TRACKING_ID, trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertNotNull(responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID));
        String channelUrl = responseCreateChannel.getHeader(HEADER_LOCATION);

        String bpId = responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID);
        String bpUrl = configuration.getBounceProxyUrl(bpId);

        assertThat(channelUrl, isChannelUrlwithJsessionId(bpUrl, channelId, SESSIONID_NAME));

        RestAssured.baseURI = Utilities.getUrlWithoutSessionId(channelUrl, SESSIONID_NAME);
        String sessionId = Utilities.getSessionId(channelUrl, SESSIONID_NAME);

        // open long polling channel
        /* @formatter:off */
        Response responseOpenChannel = given().when()
                                              .contentType(ContentType.JSON)
                                              .header(X_ATMOSPHERE_TRACKING_ID, trackingId)
                                              .get(SESSIONID_APPENDIX + sessionId);
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
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testNormalMessagingWithOneMessagePendingBeforeChannelWasOpened() throws Exception {

        final String channelId = "channel-testNormalMessagingWithOneMessagePendingBeforeChannelWasOpened";
        final String trackingId = "trackingId-testNormalMessagingWithOneMessagePendingBeforeChannelWasOpened";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header(X_ATMOSPHERE_TRACKING_ID, trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertNotNull(responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID));

        String channelUrl = responseCreateChannel.getHeader(HEADER_LOCATION);

        String bpId = responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID);
        String bpUrl = configuration.getBounceProxyUrl(bpId);

        assertThat(channelUrl, isChannelUrlwithJsessionId(bpUrl, channelId, SESSIONID_NAME));

        String sessionId = Utilities.getSessionId(channelUrl, SESSIONID_NAME);
        RestAssured.baseURI = Utilities.getUrlWithoutSessionId(channelUrl, SESSIONID_NAME);

        // post messages to long polling channel before opening channel
        String serializedMessage = bpMock.createSerializedJoynrMessage(100000l, "message-123", "message-123");

        /* @formatter:off */
        Response responsePostMessage = given().when()
                                              .contentType(ContentType.JSON)
                                              .body(serializedMessage)
                                              .post("message" + SESSIONID_APPENDIX + sessionId);
        /* @formatter:on */
        assertEquals(201 /* Created */, responsePostMessage.getStatusCode());
        assertThat(responsePostMessage.getHeader(HEADER_LOCATION),
                   isMessageUrlwithJsessionId(bpUrl, "message-123", sessionId, SESSIONID_NAME));
        assertEquals("message-123", responsePostMessage.getHeader(HEADER_MSG_ID));

        // open long polling channel
        /* @formatter:off */
        Response responseOpenChannel = given().when()
                                              .contentType(ContentType.JSON)
                                              .header("X-Atmosphere-tracking-id", trackingId)
                                              .get(SESSIONID_APPENDIX + sessionId);
        /* @formatter:on */
        assertEquals(200 /* OK */, responseOpenChannel.getStatusCode());

        // body doesn't actually contain proper json, but each message
        // serialized as json attached. We have to split them first.
        List<JoynrMessage> messages = bpMock.getJoynrMessagesFromResponse(responseOpenChannel);
        assertEquals(1, messages.size());

        // we cut the brackets of some of the messages when we split the
        // messages
        assertThat(messages, containsMessage("message-123"));
    }

    @Test(timeout = 30000)
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testNormalMessagingWithMultipleMessagesPendingBeforeChannelWasOpened() throws Exception {

        final String channelId = "channel-testNormalMessagingWithMultipleMessagesPendingBeforeChannelWasOpened";
        final String trackingId = "trackingId-testNormalMessagingWithMultipleMessagesPendingBeforeChannelWasOpened";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header(X_ATMOSPHERE_TRACKING_ID, trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertNotNull(responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID));
        String channelUrl = responseCreateChannel.getHeader(HEADER_LOCATION);

        String bpId = responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID);
        String bpUrl = configuration.getBounceProxyUrl(bpId);
        assertThat(channelUrl, isChannelUrlwithJsessionId(bpUrl, channelId, SESSIONID_NAME));

        String sessionId = Utilities.getSessionId(channelUrl, SESSIONID_NAME);
        RestAssured.baseURI = Utilities.getUrlWithoutSessionId(channelUrl, SESSIONID_NAME);

        // post messages to long polling channel before opening channel
        String msgIds[] = { "message-123", "message-456", "message-789" };
        for (String msgId : msgIds) {
            String serializedMessage = bpMock.createSerializedJoynrMessage(100000l, msgId, msgId);

            /* @formatter:off */
            Response responsePostMessage = given().when()
                                                  .log()
                                                  .all()
                                                  .contentType(ContentType.JSON)
                                                  .body(serializedMessage)
                                                  .post("message/" + SESSIONID_APPENDIX + sessionId);
            /* @formatter:on */
            assertEquals(201 /* Created */, responsePostMessage.getStatusCode());
            assertThat(responsePostMessage.getHeader(HEADER_LOCATION),
                       isMessageUrlwithJsessionId(bpUrl, msgId, sessionId, SESSIONID_NAME));
            assertEquals(msgId, responsePostMessage.getHeader(HEADER_MSG_ID));
        }

        // open long polling channel
        /* @formatter:off */
        Response responseOpenChannel = given().when()
                                              .contentType(ContentType.JSON)
                                              .header(X_ATMOSPHERE_TRACKING_ID, trackingId)
                                              .get(SESSIONID_APPENDIX + sessionId);
        /* @formatter:on */
        assertEquals(200 /* OK */, responseOpenChannel.getStatusCode());

        // body doesn't actually contain proper json, but each message
        // serialized as json attached. We have to split them first.
        List<JoynrMessage> messages = bpMock.getJoynrMessagesFromResponse(responseOpenChannel);
        assertEquals(3, messages.size());

        assertThat(messages, containsMessage("message-123"));
        assertThat(messages, containsMessage("message-456"));
        assertThat(messages, containsMessage("message-789"));
    }

    @Test
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testNormalMessagingWithMultipleMessagePostsAfterChannelWasOpened() throws Exception {

        final String channelId = "channel-testNormalMessagingWithMultipleMessagePostsAfterChannelWasOpened";
        final String trackingId = "trackingId-testNormalMessagingWithMultipleMessagePostsAfterChannelWasOpened";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().header(X_ATMOSPHERE_TRACKING_ID, trackingId).post("channels?ccid="
                + channelId);
        /* @formatter:on */

        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertNotNull(responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID));
        final String channelUrl = responseCreateChannel.getHeader(HEADER_LOCATION);

        String bpId = responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID);
        String bpUrl = configuration.getBounceProxyUrl(bpId);
        assertThat(channelUrl, isChannelUrlwithJsessionId(bpUrl, channelId, SESSIONID_NAME));

        String sessionId = Utilities.getSessionId(channelUrl, SESSIONID_NAME);
        RestAssured.baseURI = Utilities.getUrlWithoutSessionId(channelUrl, SESSIONID_NAME);

        // open long polling channel first in separate thread
        Future<?> longPollingChannelFuture = Executors.newSingleThreadExecutor().submit(new Callable<Response>() {

            @Override
            public Response call() throws Exception {

                /* @formatter:off */
                return given().when()
                              .contentType(ContentType.JSON)
                              .header(X_ATMOSPHERE_TRACKING_ID, trackingId)
                              .get("");
                /* @formatter:on */
            }
        });

        // post messages to long polling channel after opening channel
        String msgIds[] = { "message-123", "message-456", "message-789" };
        for (String msgId : msgIds) {
            String serializedMessage = bpMock.createSerializedJoynrMessage(100000l, msgId, msgId);

            /* @formatter:off */
            Response responsePostMessage = given().when()
                                                  .contentType(ContentType.JSON)
                                                  .body(serializedMessage)
                                                  .post("message/" + SESSIONID_APPENDIX + sessionId);
            /* @formatter:on */
            assertEquals(201 /* Created */, responsePostMessage.getStatusCode());
            String messageUrl = responsePostMessage.getHeader(HEADER_LOCATION);
            assertThat(messageUrl, isMessageUrlwithJsessionId(bpUrl, msgId, sessionId, SESSIONID_NAME));
            assertEquals(msgId, responsePostMessage.getHeader(HEADER_MSG_ID));
        }

        Response responseOpenChannel = (Response) longPollingChannelFuture.get(10, TimeUnit.SECONDS);
        assertEquals(200 /* OK */, responseOpenChannel.getStatusCode());

        List<JoynrMessage> messages = bpMock.getJoynrMessagesFromResponse(responseOpenChannel);
        assertEquals(1, messages.size());

        // we cut the brackets of some of the messages when we split the
        // messages
        assertThat(messages, containsMessage("message-123"));
    }

    @Test
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testPostMessageToNonExistingChannel() throws Exception {
        RestAssured.baseURI = configuration.getAnyBounceProxyUrl();
        super.testPostMessageToNonExistingChannel();
    }

    @Ignore("ignore until duplicate messages bug is fixed")
    @Test(timeout = 20000)
    // This is a test to see if the atmos bug still exists. If the bug exists,
    // the server will hang 20 secs
    public void testSendAndReceiveMessagesOnAtmosphereServer() throws Exception {
        super.testSendAndReceiveMessagesOnAtmosphereServer();
    }

    @Ignore("ignore until duplicate messages bug is fixed")
    @Test(timeout = 1000000)
    // This is a test to see if sending and receiving messages at the same time
    // results in duplicate messages in the long poll.
    public void testSendAndReceiveMessagesConcurrently() throws Exception {
        super.testSendAndReceiveMessagesConcurrently();
    }

    @Test
    @Ignore("Feature not implemented yet")
    public void testCreateChannelWithSessionIdOnBounceProxy() throws Exception {

        RestAssured.baseURI = configuration.getAnyBounceProxyUrl();

        final String channelId = "channel-testCreateChannelWithSessionId";
        final String trackingId = "trackingId-testCreateChannelWithSessionId";

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = given().log()
                                                .all()
                                                .header(X_ATMOSPHERE_TRACKING_ID, trackingId)
                                                .post("channels" + SESSIONID_APPENDIX + "sessionId12345?ccid="
                                                        + channelId);
        /* @formatter:on */

        assertEquals(400 /* Bad Request */, responseCreateChannel.getStatusCode());

        String body = responseCreateChannel.getBody().asString();
        JoynrMessagingError error = objectMapper.readValue(body, JoynrMessagingError.class);
        assertNotNull(error);

        JoynrMessagingErrorCode joynrMessagingErrorCode = JoynrMessagingErrorCode.getJoynrMessagingErrorCode(error.getCode());
        assertNotNull(joynrMessagingErrorCode);
        assertEquals(JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_SESSIONIDSET, joynrMessagingErrorCode);
    }

    @Test
    @Ignore("Feature not yet implemented")
    public void testRejectPostMessageWithoutSessionId() {
        Assert.fail("Not yet implemented");
    }

    @Test
    @Ignore("Feature not yet implemented")
    public void testRejectOpenChannelWithoutSessionId() {
        Assert.fail("Not yet implemented");
    }

}
