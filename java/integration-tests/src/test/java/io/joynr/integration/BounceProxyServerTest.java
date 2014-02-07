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
import static org.hamcrest.Matchers.hasItems;
import static org.hamcrest.Matchers.lessThan;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import io.joynr.common.ExpiryDate;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.datatypes.JoynrMessagingError;
import io.joynr.messaging.datatypes.JoynrMessagingErrorCode;
import io.joynr.messaging.serialize.JoynrEnumSerializer;
import io.joynr.messaging.serialize.JoynrListSerializer;
import io.joynr.messaging.serialize.JoynrUntypedObjectDeserializer;
import io.joynr.messaging.serialize.NumberSerializer;
import io.joynr.messaging.util.Utilities;

import java.io.IOException;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.Callable;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;

import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import com.fasterxml.jackson.annotation.JsonTypeName;
import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.Version;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.MapperFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.fasterxml.jackson.databind.jsontype.TypeDeserializer;
import com.fasterxml.jackson.databind.jsontype.TypeResolverBuilder;
import com.fasterxml.jackson.databind.module.SimpleModule;
import com.fasterxml.jackson.databind.type.SimpleType;
import com.jayway.restassured.RestAssured;
import com.jayway.restassured.config.HttpClientConfig;
import com.jayway.restassured.config.RestAssuredConfig;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;
import com.jayway.restassured.specification.RequestSpecification;

/**
 * This test sends bursts of 2 messages to a broadcaster on a bounceproxy and
 * long polls for the results Reproducer for current server-side problems
 * 
 */
public class BounceProxyServerTest {

    private static final String DEFAULT_SERVER = "http://localhost";
    private static final int DEFAULT_PORT = 8080;
    private static final String DEFAULT_PATH = "/bounceproxy";

    private static final long maxTimePerRun = 5000;
    private static final int maxRuns = 100;

    private static final String channelId = "testSendAndReceiveMessagesOnServer_" + System.currentTimeMillis();
    private static final String payload = "payload";
    private static Server server;

    Random generator = new Random();

    ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(4);
    private ObjectMapper objectMapper;
    private String receiverId = "bounceproxytest-" + UUID.randomUUID().toString();

    @BeforeClass
    public static void startServer() throws Exception {
        server = ServersUtil.startBounceproxy();
    }

    @AfterClass
    public static void stopServer() throws Exception {
        server.stop();
    }

    @Before
    public void setUp() throws Exception {

        String serverUrl = System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);

        RestAssured.baseURI = serverUrl != null ? serverUrl : DEFAULT_SERVER;

        System.out.println("server: " + RestAssured.baseURI + " (default was: " + DEFAULT_SERVER + ")");
        System.out.println("port: " + RestAssured.port + " (default was: " + DEFAULT_PORT + ")");
        System.out.println("basePath: " + RestAssured.baseURI + " (default was: " + DEFAULT_PATH + ")");
        objectMapper = getObjectMapper();
    }

    @After
    public void tearDown() throws Exception {
        deleteChannel(channelId, 30000, 200);
    }

    @Test(timeout = 20000)
    // This is a test to see if the atmos bug still exists. If the bug exists,
    // the server will hang 20 secs
    public void testSendAndReceiveMessagesOnAtmosphereServer() throws Exception {
        createChannel(channelId);
        // createChannel(channelIdProvider);
        int index = 1;
        List<String> expectedPayloads = new ArrayList<String>();

        for (int i = 0; i < maxRuns; i++) {
            expectedPayloads.clear();

            long startTime_ms = System.currentTimeMillis();
            ScheduledFuture<Response> longPollConsumer = longPoll(channelId, 30000);

            String postPayload = payload + index++ + "-" + UUID.randomUUID().toString();
            expectedPayloads.add(postPayload);
            ScheduledFuture<Response> postMessage = postMessage(channelId, 5000, postPayload);

            String postPayload2 = payload + index++ + "-" + UUID.randomUUID().toString();
            expectedPayloads.add(postPayload2);
            ScheduledFuture<Response> postMessage2 = postMessage(channelId, 5000, postPayload2);

            // wait until the long poll returns
            Response responseLongPoll = longPollConsumer.get();
            String responseBody = responseLongPoll.getBody().asString();
            List<String> listOfJsonStrings = Utilities.splitJson(responseBody);

            // wait until the POSTs are finished.
            postMessage.get();
            postMessage2.get();

            long elapsedTime_ms = System.currentTimeMillis() - startTime_ms;

            if (listOfJsonStrings.size() < 2 && elapsedTime_ms < maxTimePerRun) {
                // Thread.sleep(100);
                Response responseLongPoll2 = longPoll(channelId, 30000).get();
                String responseBody2 = responseLongPoll2.getBody().asString();
                List<String> listOfJsonStrings2 = Utilities.splitJson(responseBody2);
                listOfJsonStrings.addAll(listOfJsonStrings2);
            }

            ArrayList<String> payloads = new ArrayList<String>();
            for (String json : listOfJsonStrings) {
                JoynrMessage message = objectMapper.readValue(json, JoynrMessage.class);
                payloads.add(message.getPayload());
            }

            elapsedTime_ms = System.currentTimeMillis() - startTime_ms;
            System.err.println(i + ": time elapsed to send messages and return long poll:" + elapsedTime_ms);
            assertThat("the long poll did not receive the messages in time", elapsedTime_ms, lessThan(maxTimePerRun));

            assertThat(payloads, hasItems(postPayload, postPayload2));
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
            createChannel(newChannelId);
            Thread.sleep(50);
        }

        Thread.sleep(5000);

        for (String channel : channels) {
            postMessage(channel, 10000, "payload-" + UUID.randomUUID().toString());
            Thread.sleep(50);

        }

        for (String channel : channels) {
            deleteChannel(channel, 1000, 200);
            Thread.sleep(50);

        }

    }

    @Test
    public void testPostMessageToNonExistingChannel() throws Exception {

        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(100000l));
        message.setPayload("payload-" + UUID.randomUUID().toString());

        String serializedMessage = objectMapper.writeValueAsString(message);
        /* @formatter:off */
        Response postMessageResponse = onrequest().with()
                                                  .body(serializedMessage)
                                                  .when()
                                                  .post("/channels/non-existing-channel/message/");

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

    /**
     * initialize a RequestSpecification with no timeout
     * 
     * @return a rest-assured RequestSpecification
     */
    private RequestSpecification onrequest() {
        return onrequest(0);
    }

    /**
     * initialize a RequestSpecification with the given timeout
     * 
     * @param timeout_ms
     *            : a SocketTimeoutException will be thrown if no response is
     *            received in this many milliseconds
     * @return
     */
    private RequestSpecification onrequest(int timeout_ms) {
        return given().contentType(ContentType.JSON)
                      .log()
                      .everything()
                      .config(RestAssuredConfig.config().httpClient(HttpClientConfig.httpClientConfig()
                                                                                    .setParam("http.socket.timeout",
                                                                                              timeout_ms)));
    }

    /**
     * Create a channel with the given channelId
     * 
     * @param myChannelId
     */
    void createChannel(String myChannelId) {
        onrequest().with().headers("X-Atmosphere-Tracking-Id", receiverId).with().queryParam("ccid", myChannelId)
        // .expect()
                   // .statusCode(201)
                   .when()
                   .post("/channels/");
    }

    /**
     * Post the given payload string in a messageWrapper to thh given channelID
     * 
     * @param myChannelId
     * @param relativeTtlMs
     * @param postPayload
     * @throws JsonGenerationException
     * @throws JsonMappingException
     * @throws IOException
     */
    private ScheduledFuture<Response> postMessage(final String myChannelId,
                                                  final long relativeTtlMs,
                                                  final String postPayload) throws JsonGenerationException,
                                                                           JsonMappingException, IOException {
        ScheduledFuture<Response> scheduledFuture = scheduler.schedule(new Callable<Response>() {

            public Response call() throws JsonGenerationException, JsonMappingException, IOException {

                JoynrMessage message = new JoynrMessage();
                message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
                message.setExpirationDate(ExpiryDate.fromRelativeTtl(relativeTtlMs));
                message.setPayload(postPayload);

                String serializedMessage = objectMapper.writeValueAsString(message);
                /* @formatter:off */
                Response response = onrequest().with()
                                               .body(serializedMessage)
                                               .expect()
                                               .statusCode(201)
                                               .when()
                                               .post("/channels/" + myChannelId + "/message/");
                /* @formatter:on */

                return response;
            }
        }, 0, TimeUnit.MILLISECONDS);
        return scheduledFuture;
    }

    /**
     * Long poll on the given channel
     * 
     * @param myChannelId
     * @param timeout_ms
     * @param cacheIndex
     * @return
     * @throws SocketTimeoutException
     */
    ScheduledFuture<Response> longPoll(String myChannelId, int timeout_ms) throws SocketTimeoutException {
        return longPoll(myChannelId, timeout_ms, 200);
    }

    /**
     * Long poll on the given channel, expecting the given status code
     * 
     * @param myChannelId
     * @param timeout_ms
     * @return
     * @throws SocketTimeoutException
     */
    ScheduledFuture<Response> longPoll(final String myChannelId, final int timeout_ms, final int statusCode)
                                                                                                            throws SocketTimeoutException {
        ScheduledFuture<Response> scheduledFuture = scheduler.schedule(new Callable<Response>() {
            public Response call() {
                Response response = onrequest(timeout_ms).with()
                                                         .header("X-Atmosphere-tracking-id", receiverId)
                                                         .expect()
                                                         .statusCode(statusCode)
                                                         .log()
                                                         .all()
                                                         .when()
                                                         .get("/channels/" + myChannelId + "/");

                return response;

            }
        }, 0, TimeUnit.MILLISECONDS);
        return scheduledFuture;
    }

    private void deleteChannel(final String myChannelId, final int timeout_ms, final int statusCode) {
        onrequest(timeout_ms)// .expect()
                             // .statusCode(statusCode)
                             .log()
                             .all()
                             .when()
                             .delete("/channels/" + myChannelId + "/");

    }

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
        module.addSerializer(Number.class, new NumberSerializer());
        module.addSerializer(new JoynrEnumSerializer());
        module.addSerializer(new JoynrListSerializer());

        TypeDeserializer typeDeserializer = joynrTypeResolverBuilder.buildTypeDeserializer(objectMapper.getDeserializationConfig(),
                                                                                           SimpleType.construct(Object.class),
                                                                                           null);

        module.addDeserializer(Object.class, new JoynrUntypedObjectDeserializer(typeDeserializer));
        objectMapper.registerModule(module);
        return objectMapper;
    }

    @JsonTypeInfo(use = JsonTypeInfo.Id.NAME, include = JsonTypeInfo.As.PROPERTY, property = "_typeName")
    @JsonTypeName(value = "MessageWrapper")
    class MessageWrapper {

        private String id;

        /**
         * used by the cache to order messages
         */

        private int index;

        /**
         * stamped when the msg first enters the bounceproxy, in ms
         */
        private long entryTimeStamp;

        /**
         * the absolute time until the message should have reached it's
         * destination
         */

        private long expiryDate;

        /**
         * the actual message
         */

        private String body;

        @JsonIgnore
        private boolean pickedUp = false;

        public boolean isPickedUp() {
            return pickedUp;
        }

        public void setPickedUp(boolean pickedUp) {
            this.pickedUp = pickedUp;
        }

        protected MessageWrapper() {
        }

        public MessageWrapper(String messageId, long expiryDate, String body) {
            this.id = messageId;
            this.expiryDate = expiryDate;
            this.body = body;
        }

        /**
         * Copy Constructor
         * 
         * @param messageWrapper
         */
        public MessageWrapper(MessageWrapper messageWrapper) {
            this.id = messageWrapper.id;
            this.entryTimeStamp = messageWrapper.entryTimeStamp;
            this.body = messageWrapper.body;
            this.index = messageWrapper.index;
            this.expiryDate = messageWrapper.expiryDate;
        }

        /**
         * Overriden to provide string represention of the message object for
         * Atmosphere used in response.
         * 
         * @return
         */
        @Override
        public String toString() {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.append("\r\nmsgId: ");
            stringBuilder.append(id);
            stringBuilder.append("\r\nindex: ");
            stringBuilder.append(getIndex());
            stringBuilder.append("\r\nms: ");
            stringBuilder.append(getEntryTimeStamp());
            stringBuilder.append("\r\nexpiryDate: ");
            stringBuilder.append(getExpiryDate());
            stringBuilder.append("\r\nbody: ");
            stringBuilder.append(getBody());
            stringBuilder.append("\r\n");
            return stringBuilder.toString();
        }

        //
        // public String print() {
        // return toString();
        // }

        public String getId() {
            return id;
        }

        public void setId(String msgId) {
            id = msgId;
        }

        /**
         * @return the index within the bounce proxy cache
         */

        public int getIndex() {
            return index;
        }

        /**
         * @param index
         *            : message index within the bounce proxy cache
         */
        public void setIndex(int index) {
            this.index = index;
        }

        /**
         * @return entryTimeStamp the time the message entered the cache
         */

        public long getEntryTimeStamp() {
            return entryTimeStamp;
        }

        /**
         * @param entryTimeStamp
         *            set time the message entered the cache
         */
        public void setEntryTimeStamp(long entryTimeStamp) {
            this.entryTimeStamp = entryTimeStamp;
        }

        /**
         * @return the time to live in ms
         */

        public long getExpiryDate() {
            return expiryDate;
        }

        /**
         * @param expiryDate
         *            set time to live in ms
         */
        public void setExpiryDate(long expiryDate) {
            this.expiryDate = expiryDate;
        }

        /**
         * @return the serialized message body
         */

        public String getBody() {
            return body;
        }

        /**
         * @param body
         */
        public void setBody(String body) {
            this.body = body;
        }

        // TODO bounceproxy does not accept messages with id / expired fields
        // but sends messages containing them
        @JsonIgnore
        public boolean isExpired() {
            return getExpiryDate() < System.currentTimeMillis();
        }

    }

    public static void main(String[] args) {

        org.junit.runner.JUnitCore.main(BounceProxyServerTest.class.getName());
    }
}
