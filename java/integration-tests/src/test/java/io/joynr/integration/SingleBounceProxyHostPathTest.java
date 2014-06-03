package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.MessagingPropertyKeys;

import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.response.Response;

public class SingleBounceProxyHostPathTest {

    private Server server;
    private String serverUrl;

    @Before
    public void setUp() throws Exception {
        System.setProperty("joynr.servlet.hostpath", "http://some-joyn-test-server.io/bounceproxy");
        server = ServersUtil.startBounceproxy();

        serverUrl = System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);

        RestAssured.baseURI = serverUrl;
    }

    @After
    public void tearDown() throws Exception {
        server.stop();
    }

    @Test
    public void testBounceProxyHostPathFromSystemProperties() {

        Response response = RestAssured.given()
                                       .with()
                                       .headers("X-Atmosphere-Tracking-Id", "some-tracking-Id")
                                       .queryParam("ccid", "some-channel-Id")
                                       .expect()
                                       .statusCode(201)
                                       .when()
                                       .post("/channels/");

        Assert.assertEquals("http://some-joyn-test-server.io/bounceproxy/channels/some-channel-Id/",
                            response.header("Location"));
        Assert.assertEquals("http://some-joyn-test-server.io/bounceproxy/channels/some-channel-Id/",
                            response.body().asString());
    }

}
