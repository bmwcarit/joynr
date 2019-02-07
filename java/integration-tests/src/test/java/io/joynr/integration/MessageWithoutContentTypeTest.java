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

import static com.jayway.restassured.RestAssured.given;
import static io.joynr.util.JoynrUtil.createUuidString;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.ScheduledFuture;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;

import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.SingleBounceProxy;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.integration.setup.testrunner.BounceProxyServerSetups;
import io.joynr.integration.setup.testrunner.MultipleBounceProxySetupsTestRunner;
import io.joynr.integration.util.BounceProxyCommunicationMock;
import joynr.ImmutableMessage;

@Ignore("Bounceproxy not supported at the moment")
@RunWith(MultipleBounceProxySetupsTestRunner.class)
@BounceProxyServerSetups(value = { SingleBounceProxy.class })
public class MessageWithoutContentTypeTest {

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    private String receiverId = "channelservicestest-" + createUuidString();

    private BounceProxyCommunicationMock bpMock;

    @Before
    public void setUp() {
        bpMock = new BounceProxyCommunicationMock(configuration.getAnyBounceProxyUrl(), receiverId);
    }

    @Test
    public void testSendAndReceiveMessage() throws Exception {

        String channelId = "MessageWithoutContentTypeTest_" + createUuidString();

        bpMock.createChannel(channelId);

        byte[] postPayload = ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
        ImmutableMessage messageToSend = bpMock.createImmutableMessage(100000l, postPayload);

        given().contentType(ContentType.BINARY)
               .content(messageToSend.getSerializedMessage())
               .log()
               .all()
               .expect()
               .response()
               .statusCode(201)
               .header("Location", RestAssured.baseURI + "messages/" + messageToSend.getId())
               .header("msgId", messageToSend.getId())
               .when()
               .post("/channels/" + channelId + "/messageWithoutContentType");

        ScheduledFuture<Response> longPollConsumer = bpMock.longPollInOwnThread(channelId, 30000);
        Response responseLongPoll = longPollConsumer.get();

        List<ImmutableMessage> messagesFromResponse = bpMock.getJoynrMessagesFromResponse(responseLongPoll);
        Assert.assertEquals(1, messagesFromResponse.size());

        ImmutableMessage message = messagesFromResponse.get(0);
        Assert.assertEquals(postPayload, message.getUnencryptedBody());
    }

    @Test
    public void testMixMessagesWithAndWithoutContentType() throws Exception {

        String channelId = "MessageWithoutContentTypeTest_" + createUuidString();

        bpMock.createChannel(channelId);

        byte[] postPayload1 = ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
        ImmutableMessage serializedMessage1 = bpMock.createImmutableMessage(100000l, postPayload1);

        given().contentType(ContentType.BINARY)
               .content(serializedMessage1)
               .log()
               .all()
               .expect()
               .response()
               .statusCode(201)
               .header("Location", RestAssured.baseURI + "messages/" + serializedMessage1.getId())
               .header("msgId", serializedMessage1.getId())
               .when()
               .post("/channels/" + channelId + "/messageWithoutContentType");

        byte[] postPayload2 = ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
        ImmutableMessage immutableMessage2 = bpMock.createImmutableMessage(100000l, postPayload2);

        given().contentType(ContentType.BINARY)
               .content(immutableMessage2.getSerializedMessage())
               .contentType("application/json")
               .log()
               .all()
               .expect()
               .response()
               .statusCode(201)
               .header("Location", RestAssured.baseURI + "messages/" + immutableMessage2.getId())
               .header("msgId", immutableMessage2.getId())
               .when()
               .post("/channels/" + channelId + "/message");

        ScheduledFuture<Response> longPollConsumer = bpMock.longPollInOwnThread(channelId, 30000);
        Response responseLongPoll = longPollConsumer.get();

        List<ImmutableMessage> messagesFromResponse = bpMock.getJoynrMessagesFromResponse(responseLongPoll);

        Assert.assertEquals(2, messagesFromResponse.size());

        ImmutableMessage message = messagesFromResponse.get(0);
        Assert.assertEquals(postPayload1, message.getUnencryptedBody());
    }

}
