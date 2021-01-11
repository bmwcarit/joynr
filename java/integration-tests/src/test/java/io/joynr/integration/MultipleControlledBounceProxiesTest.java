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
import static io.joynr.integration.matchers.ChannelServiceResponseMatchers.containsChannel;
import static io.joynr.integration.matchers.MonitoringServiceResponseMatchers.containsBounceProxy;
import static io.joynr.integration.util.BounceProxyTestConstants.HEADER_BOUNCEPROXY_ID;
import static io.joynr.integration.util.BounceProxyTestConstants.HEADER_LOCATION;
import static io.joynr.integration.util.BounceProxyTestConstants.SESSIONID_APPENDIX;
import static io.joynr.integration.util.BounceProxyTestConstants.X_ATMOSPHERE_TRACKING_ID;
import static org.hamcrest.CoreMatchers.allOf;
import static org.hamcrest.CoreMatchers.anyOf;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;

//@RunWith(MultipleBounceProxySetupsTestRunner.class)
//@BounceProxyServerSetups(value = { ControlledBounceProxyCluster.class })
public class MultipleControlledBounceProxiesTest {

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    @Before
    public void setUp() {
        RestAssured.baseURI = configuration.getBounceProxyControllerUrl();
    }

    @Test
    @Ignore("need cleanup of other tests (i.e. implementation of delete channel")
    public void testSimpleChannelSetupOnTwoBounceProxies() throws Exception {

        // get bounce proxies list
        /* @formatter:off */
        JsonPath listBps = given().get("bounceproxies").getBody().jsonPath();
        /* @formatter:on */
        assertThat(listBps,
                   allOf( //
                         anyOf(containsBounceProxy("X.Y", "ALIVE"), containsBounceProxy("X.Y", "ACTIVE")), //
                         anyOf(containsBounceProxy("A.B", "ALIVE"), containsBounceProxy("A.B", "ACTIVE"))));

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateFirstChannel = //
                given().header(X_ATMOSPHERE_TRACKING_ID, "trackingId-123").post("channels?ccid=channel-123");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateFirstChannel.getStatusCode());
        assertEquals("X.Y", responseCreateFirstChannel.getHeader(HEADER_BOUNCEPROXY_ID));
        assertEquals("http://www.joynX.de/bp/channels/channel-123" + SESSIONID_APPENDIX + ".Y",
                     responseCreateFirstChannel.getHeader(HEADER_LOCATION));

        // create channel on different bounce proxy
        /* @formatter:off */
        Response responseCreateSecondChannel = //
                given().header(X_ATMOSPHERE_TRACKING_ID, "trackingId-abc").post("channels?ccid=channel-abc");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateSecondChannel.getStatusCode());
        assertEquals("A.B", responseCreateSecondChannel.getHeader(HEADER_BOUNCEPROXY_ID));
        assertEquals("http://www.joynA.de/bp/channels/channel-abc" + SESSIONID_APPENDIX + ".B",
                     responseCreateSecondChannel.getHeader(HEADER_LOCATION));

        // list channels
        /* @formatter:off */
        JsonPath listChannels = given().get("channels").getBody().jsonPath();
        /* @formatter:on */
        // TODO remove until delete channel is implemented
        // assertThat(listChannels, is(numberOfChannels(2)));
        assertThat(listChannels, containsChannel("channel-123"));
        assertThat(listChannels, containsChannel("channel-abc"));
    }
}
