/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.hamcrest.Matchers.hasItems;
import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThat;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.Version;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.MapperFeature;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.fasterxml.jackson.databind.jsontype.TypeDeserializer;
import com.fasterxml.jackson.databind.jsontype.TypeResolverBuilder;
import com.fasterxml.jackson.databind.module.SimpleModule;
import com.fasterxml.jackson.databind.type.SimpleType;
import com.jayway.restassured.RestAssured;
import com.jayway.restassured.response.Response;

import io.joynr.integration.util.BounceProxyCommunicationMock;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.datatypes.JoynrMessagingError;
import io.joynr.messaging.datatypes.JoynrMessagingErrorCode;
import io.joynr.messaging.serialize.JoynrEnumSerializer;
import io.joynr.messaging.serialize.JoynrListSerializer;
import io.joynr.messaging.serialize.JoynrUntypedObjectDeserializer;
import io.joynr.messaging.util.Utilities;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.test.JoynrTestLoggingRule;

/**
 * This test sends bursts of 2 messages to a broadcaster on a bounceproxy and
 * long polls for the results Reproducer for current server-side problems
 *
 */
public abstract class AbstractBounceProxyServerTest {

    private static final Logger log = LoggerFactory.getLogger(AbstractBounceProxyServerTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(log);

    private static final String DEFAULT_SERVER = "http://localhost";
    private static final int DEFAULT_PORT = 8080;
    private static final String DEFAULT_PATH = "/bounceproxy";

    private static final String payload = "payload";

    private String channelId;

    Random generator = new Random();

    ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(4);
    protected ObjectMapper objectMapper;
    private String receiverId = "bounceproxytest-" + createUuidString();

    protected BounceProxyCommunicationMock bpMock;

    @Before
    public void setUp() throws Exception {

        channelId = "testSendAndReceiveMessagesOnServer_" + System.currentTimeMillis();

        objectMapper = getObjectMapper();

        String serverUrl = System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);
        bpMock = new BounceProxyCommunicationMock(serverUrl != null ? serverUrl : DEFAULT_SERVER,
                                                  receiverId,
                                                  objectMapper);

        log.info("server: " + RestAssured.baseURI + " (default was: " + DEFAULT_SERVER + ")");
        log.info("port: " + RestAssured.port + " (default was: " + DEFAULT_PORT + ")");
        log.info("basePath: " + RestAssured.baseURI + " (default was: " + DEFAULT_PATH + ")");
    }

    @After
    public void tearDown() throws Exception {
        bpMock.deleteChannel(channelId, 30000, 200);
    }

    @Test(timeout = 20000)
    @Ignore
    // This is a test to see if the atmos bug still exists. If the bug exists,
    // the server will hang 20 secs
    public void testSendAndReceiveMessagesOnAtmosphereServer() throws Exception {

        final long maxTimePerRun = 5000;
        final int maxRuns = 100;

        bpMock.createChannel(channelId);
        // createChannel(channelIdProvider);

        RestAssured.baseURI = getBounceProxyBaseUri();

        int index = 1;
        List<byte[]> expectedPayloads = new ArrayList<byte[]>();

        for (int i = 0; i < maxRuns; i++) {
            expectedPayloads.clear();

            long startTime_ms = System.currentTimeMillis();
            ScheduledFuture<Response> longPollConsumer = bpMock.longPollInOwnThread(channelId, 30000);

            byte[] postPayload = (payload + index++ + "-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
            expectedPayloads.add(postPayload);
            ScheduledFuture<Response> postMessage = bpMock.postMessageInOwnThread(channelId, 5000, postPayload);

            byte[] postPayload2 = (payload + index++ + "-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
            expectedPayloads.add(postPayload2);
            ScheduledFuture<Response> postMessage2 = bpMock.postMessageInOwnThread(channelId, 5000, postPayload2);

            // wait until the long poll returns
            Response responseLongPoll = longPollConsumer.get();
            byte[] responseBody = responseLongPoll.getBody().asByteArray();
            List<ImmutableMessage> receivedMessages = Utilities.splitSMRF(responseBody);

            // wait until the POSTs are finished.
            postMessage.get();
            postMessage2.get();

            long elapsedTime_ms = System.currentTimeMillis() - startTime_ms;

            if (receivedMessages.size() < 2 && elapsedTime_ms < maxTimePerRun) {
                // Thread.sleep(100);
                Response responseLongPoll2 = bpMock.longPollInOwnThread(channelId, 30000).get();
                byte[] responseBody2 = responseLongPoll2.getBody().asByteArray();
                List<ImmutableMessage> receivedMessages2 = Utilities.splitSMRF(responseBody2);
                receivedMessages.addAll(receivedMessages2);
            }

            ArrayList<byte[]> payloads = new ArrayList<byte[]>();
            for (ImmutableMessage message : receivedMessages) {
                payloads.add(message.getUnencryptedBody());
            }

            elapsedTime_ms = System.currentTimeMillis() - startTime_ms;
            log.info(i + ": time elapsed to send messages and return long poll:" + elapsedTime_ms);
            assertThat("the long poll did not receive the messages in time", elapsedTime_ms, lessThan(maxTimePerRun));

            if (payloads.size() == 2) {
                assertFalse("Unresolved bug that causes duplicate messages to be sent",
                            payloads.get(0).equals(payloads.get(1)));
            }
            assertThat(payloads, hasItems(postPayload, postPayload2));
        }

    }

    @Test(timeout = 1000000)
    @Ignore
    // This is a test to see if sending and receiving messages at the same time
    // results in duplicate messages in the long poll.
    public void testSendAndReceiveMessagesConcurrently() throws Exception {

        final int maxRuns = 1000;

        bpMock.createChannel(channelId);
        // createChannel(channelIdProvider);

        RestAssured.baseURI = getBounceProxyBaseUri();

        List<byte[]> expectedPayloads = new ArrayList<byte[]>();

        for (int i = 0; i < maxRuns; i++) {

            expectedPayloads.clear();

            ScheduledFuture<Response> longPollConsumer = bpMock.longPollInOwnThread(channelId, 30000);

            byte[] postPayload = (payload + i + "-" + createUuidString()).getBytes(StandardCharsets.UTF_8);
            expectedPayloads.add(postPayload);
            ScheduledFuture<Response> postMessage = bpMock.postMessageInOwnThread(channelId, 5000, postPayload);

            // wait until the long poll returns
            Response responseLongPoll = longPollConsumer.get();
            byte[] responseBody = responseLongPoll.getBody().asByteArray();
            List<ImmutableMessage> receivedMessages = Utilities.splitSMRF(responseBody);

            // wait until the POSTs are finished.
            postMessage.get();

            ArrayList<byte[]> payloads = new ArrayList<byte[]>();
            for (ImmutableMessage message : receivedMessages) {
                payloads.add(message.getUnencryptedBody());
            }

            // assertFalse("Unresolved bug that causes duplicate messages to be sent", payloads.size() == 2);
            // assertEquals(1, payloads.size());
            assertThat(payloads, hasItems(postPayload));
        }

    }

    @Test
    @Ignore
    public void testOpenAndCloseChannels() throws Exception {

        int maxTries = 200;
        List<String> channels = new ArrayList<String>(maxTries);
        for (int i = 0; i < maxTries; i++) {
            String newChannelId = "JavaTest-Bounceproxy-testOpenAndCloseChannels-" + i;
            channels.add(newChannelId);
            bpMock.createChannel(newChannelId);
            Thread.sleep(50);
        }

        Thread.sleep(5000);

        for (String channel : channels) {
            bpMock.postMessageInOwnThread(channel,
                                          10000,
                                          ("payload-" + createUuidString()).getBytes(StandardCharsets.UTF_8));
            Thread.sleep(50);

        }

        for (String channel : channels) {
            bpMock.deleteChannel(channel, 1000, 200);
            Thread.sleep(50);

        }

    }

    @Test
    public void testPostMessageToNonExistingChannel() throws Exception {

        byte[] serializedMessage = bpMock.createImmutableMessage(100000l,
                                                                 "some-payload".getBytes(StandardCharsets.UTF_8))
                                         .getSerializedMessage();
        /* @formatter:off */
        Response postMessageResponse = bpMock.onrequest()
                                             .with()
                                             .body(serializedMessage)
                                             .when()
                                             .post("/channels/non-existing-channel/message/");
        /* @formatter:on */

        assertEquals(400 /* Bad Request */, postMessageResponse.getStatusCode());

        String body = postMessageResponse.getBody().asString();
        JoynrMessagingError error = objectMapper.readValue(body, JoynrMessagingError.class);
        assertNotNull(error);

        JoynrMessagingErrorCode joynrMessagingErrorCode = JoynrMessagingErrorCode.getJoynrMessagingErrorCode(error.getCode());
        assertNotNull(joynrMessagingErrorCode);
        assertEquals(JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTFOUND, joynrMessagingErrorCode);
    }

    // ///////////////////////
    // HELPERS

    private ObjectMapper getObjectMapper() {
        objectMapper = new ObjectMapper();
        objectMapper.configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);
        objectMapper.configure(SerializationFeature.WRAP_ROOT_VALUE, false);
        // objectMapper.configure(SerializationFeature.ORDER_MAP_ENTRIES_BY_KEYS,
        // true);
        objectMapper.configure(SerializationFeature.WRITE_NULL_MAP_VALUES, true);
        objectMapper.configure(SerializationFeature.FAIL_ON_EMPTY_BEANS, false);
        objectMapper.configure(MapperFeature.SORT_PROPERTIES_ALPHABETICALLY, true);

        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        TypeResolverBuilder<?> joynrTypeResolverBuilder = objectMapper.getSerializationConfig()
                                                                      .getDefaultTyper(SimpleType.construct(Object.class));

        SimpleModule module = new SimpleModule("NonTypedModule", new Version(1, 0, 0, "", "", ""));
        module.addSerializer(new JoynrEnumSerializer());
        module.addSerializer(new JoynrListSerializer());

        TypeDeserializer typeDeserializer = joynrTypeResolverBuilder.buildTypeDeserializer(objectMapper.getDeserializationConfig(),
                                                                                           SimpleType.construct(Object.class),
                                                                                           null);

        module.addDeserializer(Object.class, new JoynrUntypedObjectDeserializer(typeDeserializer));
        objectMapper.registerModule(module);
        return objectMapper;
    }

    /**
     * Returns the url of the bounce proxy which should be used for messaging.
     *
     * @return the url of the bounce proxy
     */
    protected abstract String getBounceProxyBaseUri();

}
