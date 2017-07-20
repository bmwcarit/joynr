package io.joynr.integration;

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

import static com.jayway.restassured.RestAssured.given;
import static io.joynr.integration.matchers.ChannelServiceResponseMatchers.containsChannel;
import static io.joynr.integration.matchers.ChannelServiceResponseMatchers.isChannelUrlwithJsessionId;
import static io.joynr.integration.matchers.MonitoringServiceResponseMatchers.containsBounceProxy;
import static io.joynr.integration.util.BounceProxyTestConstants.HEADER_BOUNCEPROXY_ID;
import static io.joynr.integration.util.BounceProxyTestConstants.HEADER_LOCATION;
import static io.joynr.integration.util.BounceProxyTestConstants.SESSIONID_APPENDIX;
import static io.joynr.integration.util.BounceProxyTestConstants.SESSIONID_NAME;
import static io.joynr.integration.util.BounceProxyTestConstants.X_ATMOSPHERE_TRACKING_ID;
import static org.hamcrest.CoreMatchers.anyOf;
import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.SingleControlledBounceProxy;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;
import io.joynr.messaging.util.Utilities;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

//@RunWith(MultipleBounceProxySetupsTestRunner.class)
//@BounceProxyServerSetups(value = { SingleControlledBounceProxy.class })
public class SingleControlledBounceProxyTest {

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    @Before
    public void setUp() {
        RestAssured.baseURI = configuration.getBounceProxyControllerUrl();
    }

    @Test(timeout = 20000)
    @Ignore("Ignore until servers are started in a separate JVM. Guice static problem")
    public void testSimpleChannelSetupAndDeletion() throws Exception {

        String bpUrl = configuration.getBounceProxyUrl(SingleControlledBounceProxy.ID);

        // get bounce proxies list
        JsonPath listBps = given().get("bounceproxies").body().jsonPath();
        assertThat(listBps,
                   anyOf(containsBounceProxy(SingleControlledBounceProxy.ID, "ALIVE"),
                         containsBounceProxy(SingleControlledBounceProxy.ID, "ACTIVE")));

        // create channel on bounce proxy
        /* @formatter:off */
        Response responseCreateChannel = //
        given().header(X_ATMOSPHERE_TRACKING_ID, "test-trackingId").post("channels?ccid=test-channel");
        /* @formatter:on */
        assertEquals(201 /* Created */, responseCreateChannel.getStatusCode());
        assertEquals(SingleControlledBounceProxy.ID, responseCreateChannel.getHeader(HEADER_BOUNCEPROXY_ID));

        String channelUrl = responseCreateChannel.getHeader(HEADER_LOCATION);
        assertThat(channelUrl, isChannelUrlwithJsessionId(bpUrl, "test-channel", SESSIONID_NAME));
        String sessionId = Utilities.getSessionId(channelUrl, SESSIONID_NAME);

        // list channels
        JsonPath listChannels = given().get("channels").getBody().jsonPath();
        // TODO uncomment as soon as channel deletion is implemented
        // assertThat(listChannels, is(numberOfChannels(1)));
        assertThat(listChannels, containsChannel("test-channel"));

        RestAssured.baseURI = bpUrl;

        JsonPath listBpChannels = given().get("channels" + SESSIONID_APPENDIX + sessionId).getBody().jsonPath();
        // TODO uncomment as soon as channel deletion is implemented
        // assertThat(listBpChannels, is(numberOfChannels(2)));
        assertThat(listBpChannels, containsChannel("test-channel"));
        assertThat(listBpChannels, containsChannel("/*"));

        assertEquals(200 /* OK */, given().delete("channels/test-channel" + SESSIONID_APPENDIX + sessionId + "/")
                                           .thenReturn()
                                           .statusCode());
        JsonPath listBpChannelsAfterDelete = given().get("channels" + SESSIONID_APPENDIX + sessionId)
                                                    .getBody()
                                                    .jsonPath();
        // TODO uncomment as soon as channel deletion is implemented
        // assertThat(listBpChannelsAfterDelete, is(numberOfChannels(1)));
        assertThat(listBpChannelsAfterDelete, not(containsChannel("test-channel")));
    }
}
