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
import static io.joynr.integration.util.BounceProxyTestUtils.createChannel;
import static io.joynr.integration.util.BounceProxyTestUtils.createSerializedJoynrMessage;
import static io.joynr.integration.util.BounceProxyTestUtils.longPollInOwnThread;
import static io.joynr.integration.util.BounceProxyTestUtils.objectMapper;
import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.SingleBounceProxy;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.integration.setup.testrunner.BounceProxyServerSetups;
import io.joynr.integration.setup.testrunner.MultipleBounceProxySetupsTestRunner;
import io.joynr.integration.util.BounceProxyTestUtils;
import io.joynr.messaging.MessagingModule;
import io.joynr.messaging.util.Utilities;

import java.util.List;
import java.util.UUID;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import joynr.JoynrMessage;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.jayway.restassured.RestAssured;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;

@RunWith(MultipleBounceProxySetupsTestRunner.class)
@BounceProxyServerSetups(value = { SingleBounceProxy.class })
public class MessageWithoutContentTypeTest {

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    private ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(2);
    private String receiverId = "channelservicestest-" + UUID.randomUUID().toString();

    @Before
    public void setUp() {
        RestAssured.baseURI = configuration.getAnyBounceProxyUrl();

        BounceProxyTestUtils.receiverId = receiverId;
        BounceProxyTestUtils.scheduler = scheduler;

        Injector injector = Guice.createInjector(new MessagingModule());
        BounceProxyTestUtils.objectMapper = injector.getInstance(ObjectMapper.class);
    }

    @Test
    public void testSendAndReceiveMessage() throws Exception {

        String channelId = "MessageWithoutContentTypeTest_" + UUID.randomUUID().toString();

        createChannel(channelId);

        String postPayload = "payload-" + UUID.randomUUID().toString();
        String msgId = "msgId-" + UUID.randomUUID().toString();
        String serializedMessage = createSerializedJoynrMessage(100000l, postPayload, msgId);

        given().contentType(ContentType.TEXT)
               .content(serializedMessage)
               .log()
               .all()
               .expect()
               .response()
               .statusCode(201)
               .header("Location", RestAssured.baseURI + "messages/" + msgId)
               .header("msgId", msgId)
               .when()
               .post("/channels/" + channelId + "/messageWithoutContentType");

        ScheduledFuture<Response> longPollConsumer = longPollInOwnThread(channelId, 30000);
        Response responseLongPoll = longPollConsumer.get();

        String responseBody = responseLongPoll.getBody().asString();
        List<String> listOfJsonStrings = Utilities.splitJson(responseBody);

        Assert.assertEquals(1, listOfJsonStrings.size());

        String json = listOfJsonStrings.get(0);
        JoynrMessage message = objectMapper.readValue(json, JoynrMessage.class);
        Assert.assertEquals(postPayload, message.getPayload());
    }

    @Test
    public void testMixMessagesWithAndWithoutContentType() throws Exception {

        String channelId = "MessageWithoutContentTypeTest_" + UUID.randomUUID().toString();

        createChannel(channelId);

        String postPayload1 = "payload-" + UUID.randomUUID().toString();
        String msgId1 = "msgId-" + UUID.randomUUID().toString();
        String serializedMessage1 = createSerializedJoynrMessage(100000l, postPayload1, msgId1);

        given().contentType(ContentType.TEXT)
               .content(serializedMessage1)
               .log()
               .all()
               .expect()
               .response()
               .statusCode(201)
               .header("Location", RestAssured.baseURI + "messages/" + msgId1)
               .header("msgId", msgId1)
               .when()
               .post("/channels/" + channelId + "/messageWithoutContentType");

        String postPayload2 = "payload-" + UUID.randomUUID().toString();
        String msgId2 = "msgId-" + UUID.randomUUID().toString();
        String serializedMessage2 = createSerializedJoynrMessage(100000l, postPayload2, msgId2);

        given().contentType(ContentType.TEXT)
               .content(serializedMessage2)
               .contentType("application/json")
               .log()
               .all()
               .expect()
               .response()
               .statusCode(201)
               .header("Location", RestAssured.baseURI + "messages/" + msgId2)
               .header("msgId", msgId2)
               .when()
               .post("/channels/" + channelId + "/message");

        ScheduledFuture<Response> longPollConsumer = longPollInOwnThread(channelId, 30000);
        Response responseLongPoll = longPollConsumer.get();

        String responseBody = responseLongPoll.getBody().asString();
        List<String> listOfJsonStrings = Utilities.splitJson(responseBody);

        Assert.assertEquals(2, listOfJsonStrings.size());

        String json = listOfJsonStrings.get(0);
        JoynrMessage message = objectMapper.readValue(json, JoynrMessage.class);
        Assert.assertEquals(postPayload1, message.getPayload());
    }

}
