package io.joynr.integration.util;

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
import io.joynr.common.ExpiryDate;

import java.io.IOException;
import java.net.SocketTimeoutException;
import java.util.concurrent.Callable;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import joynr.JoynrMessage;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonTypeInfo;
import com.fasterxml.jackson.annotation.JsonTypeName;
import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.jayway.restassured.config.HttpClientConfig;
import com.jayway.restassured.config.RestAssuredConfig;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;
import com.jayway.restassured.specification.RequestSpecification;

/**
 * Contains methods that can be used by different test classes that test
 * bounceproxy functionality.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyTestUtils {

    public static String receiverId;
    public static ScheduledThreadPoolExecutor scheduler;
    public static ObjectMapper objectMapper;

    /**
     * initialize a RequestSpecification with no timeout
     * 
     * @return a rest-assured RequestSpecification
     */
    public static RequestSpecification onrequest() {
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
    public static RequestSpecification onrequest(int timeout_ms) {
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
    public static void createChannel(String myChannelId) {
        onrequest().with()
                   .headers("X-Atmosphere-Tracking-Id", receiverId)
                   .with()
                   .queryParam("ccid", myChannelId)
                   .expect()
                   .statusCode(201)
                   .when()
                   .post("/channels/");
    }

    /**
     * Post the given payload string in a messageWrapper to the given channelID.
     * The message is posted in its own thread.
     * 
     * @param myChannelId
     * @param relativeTtlMs
     * @param postPayload
     * @throws JsonGenerationException
     * @throws JsonMappingException
     * @throws IOException
     */
    public static ScheduledFuture<Response> postMessageInOwnThread(final String myChannelId,
                                                                   final long relativeTtlMs,
                                                                   final String postPayload)
                                                                                            throws JsonGenerationException,
                                                                                            JsonMappingException,
                                                                                            IOException {
        ScheduledFuture<Response> scheduledFuture = scheduler.schedule(new Callable<Response>() {

            public Response call() throws JsonGenerationException, JsonMappingException, IOException {

                return postMessage(myChannelId, relativeTtlMs, postPayload);
            }
        }, 0, TimeUnit.MILLISECONDS);
        return scheduledFuture;
    }

    /**
     * Post the given payload string in a messageWrapper to the given channelID.
     * 
     * @param myChannelId
     * @param relativeTtlMs
     * @param postPayload
     * @return
     * @throws JsonProcessingException
     */
    public static Response postMessage(final String myChannelId, final long relativeTtlMs, final String postPayload)
                                                                                                                    throws JsonProcessingException {

        return postMessage(myChannelId, relativeTtlMs, postPayload, 201);
    }

    /**
     * Post the given payload string in a messageWrapper to the given channelID.
     * 
     * @param myChannelId
     * @param relativeTtlMs
     * @param postPayload
     * @param expectedStatusCode
     * @return
     * @throws JsonProcessingException
     */
    public static Response postMessage(final String myChannelId,
                                       final long relativeTtlMs,
                                       final String postPayload,
                                       int expectedStatusCode) throws JsonProcessingException {

        String serializedMessage = createSerializedJoynrMessage(relativeTtlMs, postPayload);
        /* @formatter:off */
        Response response = onrequest().with()
                                       .body(serializedMessage)
                                       .expect()
                                       .statusCode(expectedStatusCode)
                                       .when()
                                       .post("channels/" + myChannelId + "/message/");
        /* @formatter:on */

        return response;
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
    public static ScheduledFuture<Response> longPollInOwnThread(String myChannelId, int timeout_ms)
                                                                                                   throws SocketTimeoutException {
        return longPollInOwnThread(myChannelId, timeout_ms, 200);
    }

    /**
     * Long poll on the given channel, expecting the given status code. The long
     * poll is setup in its own thread.
     * 
     * @param myChannelId
     * @param timeout_ms
     * @return
     * @throws SocketTimeoutException
     */
    public static ScheduledFuture<Response> longPollInOwnThread(final String myChannelId,
                                                                final int timeout_ms,
                                                                final int statusCode) throws SocketTimeoutException {
        ScheduledFuture<Response> scheduledFuture = scheduler.schedule(new Callable<Response>() {
            public Response call() throws SocketTimeoutException {
                return longPoll(myChannelId, timeout_ms, statusCode);
            }
        }, 0, TimeUnit.MILLISECONDS);
        return scheduledFuture;
    }

    public static Response longPoll(final String myChannelId, final int timeout_ms, final int statusCode)
                                                                                                         throws SocketTimeoutException {
        return onrequest(timeout_ms).with()
                                    .header("X-Atmosphere-tracking-id", receiverId)
                                    .expect()
                                    .statusCode(statusCode)
                                    .log()
                                    .all()
                                    .when()
                                    .get("/channels/" + myChannelId + "/");
    }

    public static void deleteChannel(final String myChannelId, final int timeout_ms, final int statusCode) {
        onrequest(timeout_ms)// .expect()
                             // .statusCode(statusCode)
                             .log()
                             .all()
                             .when()
                             .delete("/channels/" + myChannelId + "/");

    }

    /**
     * Creates a serialized joynr message from a payload and a relative ttl.
     * 
     * @param relativeTtlMs
     * @param postPayload
     * @return
     * @throws JsonProcessingException
     */
    public static String createSerializedJoynrMessage(final long relativeTtlMs, final String postPayload)
                                                                                                         throws JsonProcessingException {
        return createSerializedJoynrMessage(relativeTtlMs, postPayload, null);
    }

    /**
     * Creates a serialized joynr message from a payload, a message ID and a
     * relative ttl.
     * 
     * @param relativeTtlMs
     * @param postPayload
     * @param msgId
     * @return
     * @throws JsonProcessingException
     */
    public static String createSerializedJoynrMessage(final long relativeTtlMs,
                                                      final String postPayload,
                                                      final String msgId) throws JsonProcessingException {
        JoynrMessage message = new JoynrMessage();
        if (msgId != null) {
            message.setHeaderValue(JoynrMessage.HEADER_NAME_MESSAGE_ID, msgId);
        }
        message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(relativeTtlMs));
        message.setPayload(postPayload);

        String serializedMessage = objectMapper.writeValueAsString(message);
        return serializedMessage;
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
}
