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
package io.joynr.integration;

import static io.joynr.integration.matchers.MessagingServiceResponseMatchers.containsPayload;
import static io.joynr.util.JoynrUtil.createUuidString;
import static org.hamcrest.CoreMatchers.allOf;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.List;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.response.Response;

import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.integration.util.BounceProxyCommunicationMock;
import io.joynr.messaging.util.Utilities;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;

//@RunWith(MultipleBounceProxySetupsTestRunner.class)
//@BounceProxyServerSetups(value = { /*ControlledBounceProxyCluster.class, */ClusteredBounceProxyWithDispatcher.class })
public class MessagingLoadDistributionTest {

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    private BounceProxyCommunicationMock bpMock1;
    private BounceProxyCommunicationMock bpMock2;

    @Before
    public void setUp() throws Exception {

        String trackingId1 = "trackingId-" + createUuidString();
        bpMock1 = new BounceProxyCommunicationMock(configuration.getBounceProxyControllerUrl(), trackingId1);

        String trackingId2 = "trackingId-" + createUuidString();
        bpMock2 = new BounceProxyCommunicationMock(configuration.getBounceProxyControllerUrl(), trackingId2);
    }

    @Test
    @Ignore("Ignore until servers are started in a separate JVM. Guice static problem")
    public void testMessagePostsToCorrectInstances() throws Exception {

        String uuid1 = createUuidString();
        String channelId1 = "channel-" + uuid1;

        String uuid2 = createUuidString();
        String channelId2 = "channel-" + uuid2;

        // create channels; use extra method as we don't know which location to expect
        Response responseCreateChannel1 = createChannel(bpMock1, channelId1);
        Response responseCreateChannel2 = createChannel(bpMock2, channelId2);

        Assert.assertNotSame("Channels created on two different Bounce Proxies",
                             responseCreateChannel1.getHeader("bp"),
                             responseCreateChannel2.getHeader("bp"));

        String channelUrl1 = responseCreateChannel1.getHeader("Location");
        String channelUrl2 = responseCreateChannel2.getHeader("Location");

        // post messages to long polling channel before opening channel
        String payloads1[] = { "message-1_1", "message-1_2", "message-1_3" };
        for (String payload : payloads1) {
            postMessageToBounceProxy(bpMock1,
                                     channelUrl1,
                                     channelId1,
                                     30000l,
                                     payload.getBytes(StandardCharsets.UTF_8));
        }

        String payloads2[] = { "message-2_1", "message-2_2", "message-2_3" };
        for (String payload : payloads2) {
            postMessageToBounceProxy(bpMock2,
                                     channelUrl2,
                                     channelId2,
                                     30000l,
                                     payload.getBytes(StandardCharsets.UTF_8));
        }

        // open long polling channel
        List<ImmutableMessage> messages1 = getMessagesFromBounceProxy(bpMock1, channelUrl1, channelId1);

        assertEquals(3, messages1.size());
        assertThat(messages1,
                   allOf(containsPayload("message-1_1"),
                         containsPayload("message-1_2"),
                         containsPayload("message-1_3")));

        List<ImmutableMessage> messages2 = getMessagesFromBounceProxy(bpMock2, channelUrl2, channelId2);
        assertEquals(3, messages2.size());
        assertThat(messages2,
                   allOf(containsPayload("message-2_1"),
                         containsPayload("message-2_2"),
                         containsPayload("message-2_3")));
    }

    @Test
    @Ignore("Ignore until failover scenarios are implemented")
    public void testMessagePostsToWrongInstance() {
        // right now it will fail as messaging failover is not implemented yet
        Assert.fail("Not yet implemented");
    }

    @Test
    @Ignore("Ignore until failover scenarios are implemented")
    public void testMessagePostsWithServerUnavailable() {
        Assert.fail("Not yet implemented");
    }

    private Response createChannel(BounceProxyCommunicationMock bpMock, String myChannelId) {
        return bpMock.onrequest()
                     .with()
                     .headers("X-Atmosphere-Tracking-Id", bpMock.getReceiverId())
                     .queryParam("ccid", myChannelId)
                     .expect()
                     .statusCode(201)
                     .when()
                     .post("/channels/");
    }

    private void postMessageToBounceProxy(BounceProxyCommunicationMock bpMock,
                                          String channelUrl,
                                          String channelId,
                                          long relativeTtlMs,
                                          byte[] payload) throws Exception {

        String previousBaseUri = RestAssured.baseURI;
        RestAssured.baseURI = Utilities.getUrlWithoutSessionId(channelUrl, "jsessionid");
        String sessionId = Utilities.getSessionId(channelUrl, "jsessionid");

        byte[] serializedMessage = bpMock.createImmutableMessage(relativeTtlMs, payload).getSerializedMessage();
        /* @formatter:off */
        bpMock.onrequest()
              .with()
              .body(serializedMessage)
              .expect()
              .statusCode(201)
              .when()
              .post("message;jsessionid=" + sessionId);
        /* @formatter:on */
        RestAssured.baseURI = previousBaseUri;
    }

    private List<ImmutableMessage> getMessagesFromBounceProxy(BounceProxyCommunicationMock bpMock,
                                                              String channelUrl,
                                                              String channelId) throws IOException, EncodingException,
                                                                                UnsuppportedVersionException {

        String previousBaseUri = RestAssured.baseURI;
        RestAssured.baseURI = Utilities.getUrlWithoutSessionId(channelUrl, "jsessionid");
        String sessionId = Utilities.getSessionId(channelUrl, "jsessionid");

        /* @formatter:off */
        Response response = bpMock.onrequest()
                                  .with()
                                  .header("X-Atmosphere-tracking-id", bpMock.getReceiverId())
                                  .expect()
                                  .statusCode(200)
                                  .when()
                                  .get(";jsessionid=" + sessionId);
        /* @formatter:on */

        List<ImmutableMessage> messagesFromResponse = bpMock.getJoynrMessagesFromResponse(response);

        RestAssured.baseURI = previousBaseUri;

        return messagesFromResponse;
    }

}
