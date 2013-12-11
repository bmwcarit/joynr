package io.joynr.messaging.bounceproxy.controller.integration;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;

import java.util.List;

import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.webapp.WebAppContext;
import org.hamcrest.BaseMatcher;
import static org.hamcrest.CoreMatchers.is;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

public class NormalOperationTest {

    private String serverUrl;

    private Server jettyServer;

    @Before
    public void setUp() throws Exception {

        // starts the server with a random port
        jettyServer = new Server(0);

        WebAppContext bpCtrlWebapp = new WebAppContext();
        bpCtrlWebapp.setResourceBase("./src/main/java");
        bpCtrlWebapp.setDescriptor("./src/main/resources/WEB-INF/web.xml");

        jettyServer.setHandler(bpCtrlWebapp);

        jettyServer.start();

        int port = jettyServer.getConnectors()[0].getLocalPort();
        serverUrl = String.format("http://localhost:%d", port);
    }

    @After
    public void tearDown() throws Exception {
        jettyServer.stop();
    }

    @Test
    public void testSimpleChannelSetupOnSingleBounceProxy() {

        // register new bounce proxy
        Response responseCreateBp = //
        given(). //
               when()
               .header("url4cc", "http://www.joynX.de/bp")
               .and()
               .header("url4bpc", "http://joynX.bmwgroup.net/bp")
               .post(serverUrl + "/controller?bpid=X.Y");
        assertEquals(201 /* Created */, responseCreateBp.getStatusCode());

        // get bounce proxies list
        Response listBps = given().get(serverUrl + "/controller");
        assertEquals("[\"X.Y\"]", listBps.getBody().asString());

        // create channel on bounce proxy
        Response responseCreateChannel = //
        given().post(serverUrl + "/channels?ccid=channel-123");
        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals("X.Y", responseCreateChannel.getHeader("bp"));
        assertEquals("http://www.joynX.de/bp/channels/channel-123;jsessionid=.Y",
                     responseCreateChannel.getHeader("Location"));

        // list channels
        JsonPath listChannels = given().get(serverUrl + "/channels").getBody().jsonPath();
        assertThat(listChannels, is(numberOfChannels(1)));
        assertThat(listChannels, containsChannel("channel-123"));
    }

    @Test
    public void testSimpleChannelSetupOnTwoBounceProxies() {

        // register two bounce proxies
        Response responseCreateFirstBp = //
        given(). //
               when()
               .header("url4cc", "http://www.joynX.de/bp")
               .and()
               .header("url4bpc", "http://joynX.bmwgroup.net/bp")
               .post(serverUrl + "/controller?bpid=X.Y");
        assertEquals(201 /* Created */, responseCreateFirstBp.getStatusCode());

        Response responseCreateSecondBp = //
        given(). //
               when()
               .header("url4cc", "http://www.joynA.de/bp")
               .and()
               .header("url4bpc", "http://joynA.bmwgroup.net/bp")
               .post(serverUrl + "/controller?bpid=A.B");
        assertEquals(201 /* Created */, responseCreateSecondBp.getStatusCode());

        // get bounce proxies list
        Response listBps = given().get(serverUrl + "/controller");
        assertEquals("[\"X.Y\",\"A.B\"]", listBps.getBody().asString());

        // create channel on bounce proxy
        Response responseCreateFirstChannel = //
        given().post(serverUrl + "/channels?ccid=channel-123");
        assertEquals(201 /* Created */, responseCreateFirstChannel.getStatusCode());
        assertEquals("X.Y", responseCreateFirstChannel.getHeader("bp"));
        assertEquals("http://www.joynX.de/bp/channels/channel-123;jsessionid=.Y",
                     responseCreateFirstChannel.getHeader("Location"));

        // create channel on different bounce proxy
        Response responseCreateSecondChannel = //
        given().post(serverUrl + "/channels?ccid=channel-abc");
        assertEquals(201 /* Created */, responseCreateSecondChannel.getStatusCode());
        assertEquals("A.B", responseCreateSecondChannel.getHeader("bp"));
        assertEquals("http://www.joynA.de/bp/channels/channel-abc;jsessionid=.B",
                     responseCreateSecondChannel.getHeader("Location"));

        // list channels
        JsonPath listChannels = given().get(serverUrl + "/channels").getBody().jsonPath();
        assertThat(listChannels, is(numberOfChannels(2)));
        assertThat(listChannels, containsChannel("channel-123"));
        assertThat(listChannels, containsChannel("channel-abc"));
    }

    private Matcher<JsonPath> containsChannel(final String channelId) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                List<String> channelIds = jsonPath.getList("channelId");

                for (String c : channelIds) {
                    if (c.equals(channelId)) {
                        return true;
                    }
                }

                return false;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains channel ID '" + channelId + "'");
            }

        };
    }

    private Matcher<JsonPath> numberOfChannels(final int size) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                List<String> channelIds = jsonPath.getList("channelId");
                return channelIds.size() == size;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains " + size + " channels");
            }

        };
    }
}
