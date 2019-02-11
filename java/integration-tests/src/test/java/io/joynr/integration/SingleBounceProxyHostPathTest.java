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

import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.response.Response;

import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.MessagingPropertyKeys;

public class SingleBounceProxyHostPathTest {

    private Server server;
    private String serverUrl;

    @Before
    public void setUp() throws Exception {
        serverUrl = "http://my-public-internet-domain.com:8119";
        System.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, serverUrl);
        server = ServersUtil.startBounceproxy();
        String bounceproxyUrlString = System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);
        RestAssured.baseURI = bounceproxyUrlString;
    }

    @After
    public void tearDown() throws Exception {
        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
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

        String servletHostpath = System.getProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH);
        Assert.assertEquals(servletHostpath + "/bounceproxy/channels/some-channel-Id/", response.header("Location"));
        Assert.assertEquals(servletHostpath + "/bounceproxy/channels/some-channel-Id/", response.body().asString());
    }

}
