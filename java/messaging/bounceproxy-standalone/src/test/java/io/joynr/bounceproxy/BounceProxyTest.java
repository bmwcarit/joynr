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
import io.joynr.common.ExpiryDate;
import io.joynr.messaging.MessagingModule;

import java.io.IOException;
import java.net.SocketTimeoutException;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadFactory;

import joynr.JoynrMessage;

import org.junit.After;
import org.junit.Before;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.jayway.restassured.RestAssured;
import com.jayway.restassured.config.HttpClientConfig;
import com.jayway.restassured.config.RestAssuredConfig;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;
import com.jayway.restassured.specification.RequestSpecification;
import com.jayway.restassured.specification.ResponseSpecification;

public class BounceProxyTest {

    protected static final Logger logger = LoggerFactory.getLogger(BounceProxyTest.class);
    ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("BounceProxyTestAsyncCalls-%d").build();
    protected ExecutorService asyncCallsExecutorService = Executors.newFixedThreadPool(2, namedThreadFactory);

    protected String contextId = UUID.randomUUID().toString();

    protected Random generator = new Random();

    protected ObjectMapper objectMapper;
    protected LocalGrizzlyBounceProxy server;
    protected String atmosphereTrackingId = UUID.randomUUID().toString();

    @Before
    public void setUp() throws Exception {
        Injector injector = Guice.createInjector(new MessagingModule());
        objectMapper = injector.getInstance(ObjectMapper.class);

        useLocalServer();
        // useJoynServer();
    }

    private void useLocalServer() throws Exception {
        server = new LocalGrizzlyBounceProxy();
        server.start();

        RestAssured.port = server.getPort();
    }

    @SuppressWarnings("unused")
    private void useJoynServer() {
        RestAssured.baseURI = "http://localhost";
        RestAssured.port = 8080;

    }

    @After
    public void tearDown() throws Exception {
        if (server != null) {
            server.stop();
        }
    }

    // ///////////////////////
    // HELPERS

    /**
     * initialize a RequestSpecification with no timeout
     * 
     * @return a rest-assured RequestSpecification
     */
    protected RequestSpecification onrequest() {
        return onrequest(0);
    }

    /**
     * initialize a RequestSpecification with the given timeout
     * 
     * @param timeout_ms
     *            : a SocketTimeoutException will be thrown if no response is received in this many milliseconds
     * @return
     */
    protected RequestSpecification onrequest(int timeout_ms) {
        return given().contentType(ContentType.JSON)
                      .header(BounceProxyConstants.X_ATMOSPHERE_TRACKING_ID, atmosphereTrackingId)
                      .log()
                      .everything()
                      .config(RestAssuredConfig.config().httpClient(HttpClientConfig.httpClientConfig()
                                                                                    .setParam("http.socket.timeout",
                                                                                              timeout_ms)));
    }

    /**
     * Create a channel with the given channelId
     * 
     * @param channelId
     */
    protected void createChannel(String channelId) {
        onrequest().with().queryParam("ccid", channelId).expect().statusCode(201).when().post("/bounceproxy/channels/");
    }

    /**
     * Delete the channel with the given channelId
     * 
     * @param channelId
     */
    protected void deleteChannel(String channelId) {
        onrequest().expect().statusCode(200).when().delete("/bounceproxy/channels/" + channelId + "/");
    }

    /**
     * Post the given payload string in a messageWrapper to thh given channelID
     * 
     * @param channelId
     * @param relativeTtlMs
     * @param payload
     * @throws JsonGenerationException
     * @throws JsonMappingException
     * @throws IOException
     */
    protected void postMessage(String channelId, long relativeTtlMs, String payload) throws JsonGenerationException,
                                                                                    JsonMappingException, IOException {
        postMessage(channelId, relativeTtlMs, payload, 201);
    }

    /**
     * Post the given payload string in a messageWrapper to thh given channelID
     * 
     * @param channelId
     * @param relativeTtlMs
     * @param payload
     * @param expectedStatusCode
     * @throws JsonGenerationException
     * @throws JsonMappingException
     * @throws IOException
     */
    protected void postMessage(String channelId, long relativeTtlMs, String payload, int expectedStatusCode)
                                                                                                            throws JsonGenerationException,
                                                                                                            JsonMappingException,
                                                                                                            IOException {
        JoynrMessage joynrMessage = new JoynrMessage();
        ExpiryDate expirationDate = ExpiryDate.fromRelativeTtl(relativeTtlMs);
        joynrMessage.setExpirationDate(expirationDate);
        joynrMessage.setPayload(payload);
        String serializedMessage = objectMapper.writeValueAsString(joynrMessage);
        /* @formatter:off */
        onrequest().with()
                   .body(serializedMessage)
                   .expect()
                   .statusCode(expectedStatusCode)
                   .when()
                   .post("/bounceproxy/channels/" + channelId + "/message/");
        /* @formatter:on */
    }

    /**
     * Long poll on the given channel
     * 
     * @param channelId
     * @param timeout_ms
     * @param cacheIndex
     * @return
     * @throws SocketTimeoutException
     */
    protected Future<Response> longPoll(String channelId, int timeout_ms) throws SocketTimeoutException {
        Future<Response> response = longPoll(channelId, timeout_ms, 200);

        return response;
    }

    /**
     * Long poll on the given channel, expecting the given status code
     * 
     * @param channelId
     * @param timeout_ms
     * @param cacheIndex
     * @return
     * @throws SocketTimeoutException
     */
    protected Future<Response> longPoll(String channelId, int timeout_ms, int statusCode) throws SocketTimeoutException {
        ResponseSpecification responseSpecification = onrequest(timeout_ms).expect().statusCode(statusCode).log().all();
        return getAsynchronously(responseSpecification, "/bounceproxy/channels/" + channelId + "/");
    }

    protected Future<Response> getAsynchronously(final ResponseSpecification responseSpecification, final String path) {
        Callable<Response> callable = new Callable<Response>() {

            @Override
            public Response call() throws Exception {
                return responseSpecification.get(path);
            }

        };
        return asyncCallsExecutorService.submit(callable);
    }

    protected JoynrMessage createJoynrMessage(String payload, long relativeTtl) {
        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        message.setPayload(payload);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(relativeTtl));
        return message;
    }

}
