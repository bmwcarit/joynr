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
import static io.joynr.integration.util.BounceProxyTestUtils.onrequest;
import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.SingleBounceProxy;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.integration.setup.testrunner.BounceProxyServerSetups;
import io.joynr.integration.setup.testrunner.MultipleBounceProxySetupsTestRunner;

import java.io.ByteArrayInputStream;
import java.util.UUID;
import java.util.concurrent.Future;

import javax.annotation.Nullable;

import joynr.JoynrMessage;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.response.Response;

@RunWith(MultipleBounceProxySetupsTestRunner.class)
@BounceProxyServerSetups(value = { SingleBounceProxy.class })
public class AttachmentTest {

    private static final Logger logger = LoggerFactory.getLogger(AttachmentTest.class);

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    @Before
    public void setUp() {
        RestAssured.baseURI = configuration.getAnyBounceProxyUrl();
    }

    @Test
    @Ignore
    public void testSendAttachedByteArray() throws Exception {
        String channelId = "AttachmentTest_" + UUID.randomUUID().toString();
        try {
            createChannel(channelId);
        } catch (Exception e) {
            logger.error("Exception :", e);
        }

        sendAttachmentMessage(channelId);
    }

    @Nullable
    private Response sendAttachmentMessage(String channelId) throws Exception {
        long ttl_ms = 1000000;
        String payload = "attachmentTest";
        String serializedMessageWrapper = createSerializedJoynrMessage(ttl_ms, payload);

        String content = "This is a binary content";
        Response response = null;
        try {
            response = given().multiPart("wrapper", serializedMessageWrapper, "application/json")
                              .multiPart("attachment", "attachment.txt", new ByteArrayInputStream(content.getBytes()))
                              .expect()
                              .response()
                              .statusCode(201)
                              .log()
                              .all()
                              .when()
                              .post("/bounceproxy/channels/" + channelId + "/message/");

        } catch (Exception e) {
            logger.error("Exception :", e);
        }
        logger.debug("Response : " + response);
        return response;
    }

    @Test
    @Ignore
    public void testSendAndDownload() throws Exception {
        String channelId = "AttachmentTest_" + UUID.randomUUID().toString();

        createChannel(channelId);

        Response senMsgResponse = sendAttachmentMessage(channelId);

        String msgId = senMsgResponse.getHeader("msgId");

        Future<Response> response = longPollInOwnThread(channelId, 1000000, 200);
        Response longPoll = response.get();

        String json = longPoll.getBody().asString();

        JoynrMessage message = objectMapper.readValue(json, JoynrMessage.class);

        Assert.assertTrue(message.getPayload() != null);

        Response attachment = getAttachment(channelId, msgId);
        logger.debug("received attachment: " + convertStreamToString(attachment.getBody().asInputStream()));
    }

    public static String convertStreamToString(java.io.InputStream is) {
        java.util.Scanner s = new java.util.Scanner(is);
        s.useDelimiter("\\A");

        String string = s.hasNext() ? s.next() : "";
        s.close();
        return string;
    }

    private Response getAttachment(String channelId, String messageId) {
        Response response = onrequest(100000).with()
                                             .queryParam("attachmentId", String.valueOf(messageId))
                                             .expect()
                                             .statusCode(200)
                                             .log()
                                             .all()
                                             .when()
                                             .get("/bounceproxy/channels/" + channelId + "/attachment/");

        return response;

    }

}
