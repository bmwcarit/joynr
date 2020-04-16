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
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.io.ByteArrayInputStream;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.Future;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.response.Response;

import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.SingleBounceProxy;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.integration.setup.testrunner.BounceProxyServerSetups;
import io.joynr.integration.setup.testrunner.MultipleBounceProxySetupsTestRunner;
import io.joynr.integration.util.BounceProxyCommunicationMock;
import joynr.ImmutableMessage;
import joynr.test.JoynrTestLoggingRule;

@Ignore("Bounceproxy not supported at the moment")
@RunWith(MultipleBounceProxySetupsTestRunner.class)
@BounceProxyServerSetups(value = { SingleBounceProxy.class })
public class AttachmentTest {

    private static final Logger logger = LoggerFactory.getLogger(AttachmentTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    private String receiverId = "channelservicestest-" + createUuidString();

    private BounceProxyCommunicationMock bpMock;

    @Before
    public void setUp() {
        RestAssured.baseURI = configuration.getAnyBounceProxyUrl();

        bpMock = new BounceProxyCommunicationMock(configuration.getAnyBounceProxyUrl(), receiverId);
    }

    @Test
    public void testSendAttachedByteArray() throws Exception {
        String channelId = "AttachmentTest_" + createUuidString();
        try {
            bpMock.createChannel(channelId);
        } catch (Exception e) {
            logger.error("Exception: ", e);
        }

        sendAttachmentMessage(channelId);
    }

    private Optional<Response> sendAttachmentMessage(String channelId) throws Exception {
        long ttl_ms = 1000000;
        byte[] payload = "attachmentTest".getBytes(StandardCharsets.UTF_8);
        byte[] serializedMessageWrapper = bpMock.createImmutableMessage(ttl_ms, payload).getSerializedMessage();

        String content = "This is a binary content";
        Response response = given().multiPart("wrapper", serializedMessageWrapper, "application/json")
                                   .multiPart("attachment",
                                              "attachment.txt",
                                              new ByteArrayInputStream(content.getBytes()))
                                   .expect()
                                   .response()
                                   .statusCode(201)
                                   .log()
                                   .all()
                                   .when()
                                   .post("/channels/" + channelId + "/messageWithAttachment");

        logger.debug("Response: ", response);
        return Optional.ofNullable(response);
    }

    @Test
    public void testSendAndDownload() throws Exception {
        String channelId = "AttachmentTest_" + createUuidString();

        bpMock.createChannel(channelId);

        Response senMsgResponse = sendAttachmentMessage(channelId).get();

        String msgId = senMsgResponse.getHeader("msgId");

        Future<Response> response = bpMock.longPollInOwnThread(channelId, 1000000, 200);
        Response longPoll = response.get();

        List<ImmutableMessage> messages = bpMock.getJoynrMessagesFromResponse(longPoll);

        assertEquals(1, messages.size());

        ImmutableMessage message = messages.get(0);
        assertTrue(message.getUnencryptedBody() != null);

        Response attachment = getAttachment(channelId, msgId);
        logger.debug("Received attachment: ", convertStreamToString(attachment.getBody().asInputStream()));
    }

    public static String convertStreamToString(java.io.InputStream is) {
        java.util.Scanner s = new java.util.Scanner(is);
        s.useDelimiter("\\A");

        String string = s.hasNext() ? s.next() : "";
        s.close();
        return string;
    }

    private Response getAttachment(String channelId, String messageId) {
        Response response = bpMock.onrequest(100000)
                                  .with()
                                  .queryParam("attachmentId", String.valueOf(messageId))
                                  .expect()
                                  .statusCode(200)
                                  .log()
                                  .all()
                                  .when()
                                  .get("/channels/" + channelId + "/attachment/");

        return response;

    }

}
