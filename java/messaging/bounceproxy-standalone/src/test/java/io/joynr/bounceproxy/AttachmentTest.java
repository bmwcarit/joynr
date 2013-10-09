package io.joynr.bounceproxy;

/*
 * #%L
 * joynr::java::messaging::bounceproxy-standalone
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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

import joynr.JoynrMessage;

import org.junit.Assert;
import org.junit.Ignore;
import org.junit.Test;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.jayway.restassured.response.Response;

public class AttachmentTest extends BounceProxyTest {

    @Test
    @Ignore
    public void testSendAttachedByteArray() {
        String channelId = "AttachmentTest_" + UUID.randomUUID().toString();
        try {
            createChannel(channelId);
        } catch (Exception e) {
            logger.error("Exception :", e);
        }

        sendAttachmentMessage(channelId);
    }

    private Response sendAttachmentMessage(String channelId) {
        long ttl_ms = 1000000;
        String payload = "attachmentTest";
        JoynrMessage message = createJoynrMessage(payload, ttl_ms);

        String serializedMessageWrapper = "";
        try {
            serializedMessageWrapper = objectMapper.writeValueAsString(message);
        } catch (Exception e) {
            e.printStackTrace();
        }

        String content = "This is a binary content";
        Response response = null;
        try {
            response = given().port(server.getPort())
                              .multiPart("wrapper", serializedMessageWrapper, "application/json")
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
    public void testSendAndDownload() throws JsonParseException, JsonMappingException, IOException,
                                     InterruptedException, ExecutionException {
        String channelId = "AttachmentTest_" + UUID.randomUUID().toString();

        createChannel(channelId);

        Response senMsgResponse = sendAttachmentMessage(channelId);

        String msgId = senMsgResponse.getHeader("msgId");

        Future<Response> response = longPoll(channelId, 1000000, 200);
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
