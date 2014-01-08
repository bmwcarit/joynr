package io.joynr.messaging.httpoperation;

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
import static org.hamcrest.Matchers.containsString;
import io.joynr.bounceproxy.LocalGrizzlyBounceProxy;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingTestModule;
import io.joynr.runtime.JoynrBaseModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.io.IOException;
import java.util.Properties;
import java.util.UUID;

import joynr.JoynrMessage;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.Injector;
import com.jayway.restassured.RestAssured;
import com.jayway.restassured.config.HttpClientConfig;
import com.jayway.restassured.config.RestAssuredConfig;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.specification.RequestSpecification;

/**
 * HttpCommunicationManager tested by sending a request to the local host and checking the received message.
 * 
 */

@RunWith(MockitoJUnitRunner.class)
public class HttpCommunicationManagerTest {

    LongPollingMessageReceiver longpollingMessageReceiver;
    MessageSender messageSender;
    private String testChannelId = "HttpCommunicationManagerTest-" + UUID.randomUUID().toString();
    private int port;

    public HttpCommunicationManagerTest() throws IOException {
        LocalGrizzlyBounceProxy server = new LocalGrizzlyBounceProxy();
        port = server.start();
    }

    @Mock
    private RequestReplyDispatcher dispatcher;
    private String bounceProxyUrlString;

    @Before
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value="ST_WRITE_TO_STATIC_FROM_INSTANCE_METHOD", justification="correct use of RestAssured API")
    public void setUp() throws Exception {

        RestAssured.port = port;
        String basePath = "/bounceproxy/";
        RestAssured.basePath = basePath;

        bounceProxyUrlString = "http://localhost:" + port + basePath;

        Properties properties = new Properties();
        properties.put(MessagingPropertyKeys.CHANNELID, testChannelId);
        properties.put(MessagingPropertyKeys.BOUNCE_PROXY_URL, bounceProxyUrlString);

        Injector injector = new JoynrInjectorFactory(new JoynrBaseModule(properties, new MessagingTestModule())).getInjector();

        longpollingMessageReceiver = injector.getInstance(LongPollingMessageReceiver.class);
        messageSender = injector.getInstance(MessageSender.class);
    }

    @After
    public void tearDown() throws Exception {
        longpollingMessageReceiver.shutdown(true);
    }

    @Test
    public void testCreateOpenAndDeleteChannel() throws Exception {

        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(30000));
        message.setPayload("testMessage");

        longpollingMessageReceiver.registerMessageListener(dispatcher);
        longpollingMessageReceiver.startReceiver();

        // post to the channel to see if it exists

        onrequest(1000).with().body(message).expect().statusCode(201).when().post("/channels/" + testChannelId
                + "/message/");

        longpollingMessageReceiver.shutdown(true);

        // post again; this time it should be missing (NO_CONTENT)
        onrequest(1000).with()
                       .body(message)
                       .expect()
                       .statusCode(400)
                       .body(containsString("Channel not found"))
                       .when()
                       .post("/channels/" + testChannelId + "/message/");

    }

    // ///////////////////////
    // HELPERS

    /**
     * initialize a RequestSpecification with the given timeout
     * 
     * @param timeout_ms
     *            : a SocketTimeoutException will be thrown if no response is received in this many milliseconds
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

}
