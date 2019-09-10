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
import static io.joynr.integration.matchers.InputStreamMatchers.equalsLineByLine;
import static io.joynr.integration.util.BounceProxyTestConstants.X_ATMOSPHERE_TRACKING_ID;
import static io.joynr.integration.util.ChannelServiceTestUtils.getChannelIdsOnChannelsHtml;
import static org.hamcrest.CoreMatchers.hasItem;
import static org.hamcrest.CoreMatchers.hasItems;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;

import java.io.InputStream;
import java.util.List;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import com.gargoylesoftware.htmlunit.WebClient;
import com.jayway.restassured.RestAssured;

import io.joynr.integration.setup.BounceProxyServerSetup;
import io.joynr.integration.setup.testrunner.BounceProxyServerContext;

//@RunWith(MultipleBounceProxySetupsTestRunner.class)
// NOTE: for some reason the order of these classes in the array matters.
//@BounceProxyServerSetups(value = { SingleBounceProxy.class, SingleControlledBounceProxy.class })
public class ChannelsHtmlPageTest {

    @BounceProxyServerContext
    public BounceProxyServerSetup configuration;

    private WebClient webClient;

    @Before
    public void setUp() {
        RestAssured.baseURI = configuration.getAnyBounceProxyUrl();

        webClient = new WebClient();
    }

    @After
    public void tearDown() {
        webClient.close();
    }

    @Test
    @Ignore("Ignore until servers are started in a separate JVM. Guice static problem")
    public void testHtmlPage() throws Exception {

        String bpUrl = configuration.getAnyBounceProxyUrl();
        String channelsHtmlUrl = bpUrl + "channels.html";

        // html page is always the same. Content is added to DOM by Javascript
        InputStream channelsHtmlIn = given().get("channels.html").body().asInputStream();
        assertThat(channelsHtmlIn, equalsLineByLine("expectedChannelsHtml.html"));

        // inspect DOM tree for actual content
        List<String> displayedChannelIds = getChannelIdsOnChannelsHtml(webClient, channelsHtmlUrl);

        assertEquals(1, displayedChannelIds.size());
        assertThat(displayedChannelIds, hasItem("/*"));

        // create channel on bounce proxy
        given().header(X_ATMOSPHERE_TRACKING_ID, "test-trackingId")
               .expect()
               .statusCode(201)
               .post("channels?ccid=channel1");

        channelsHtmlIn = given().get("channels.html").body().asInputStream();
        assertThat(channelsHtmlIn, equalsLineByLine("expectedChannelsHtml.html"));

        displayedChannelIds = getChannelIdsOnChannelsHtml(webClient, channelsHtmlUrl);

        assertEquals(2, displayedChannelIds.size());
        assertThat(displayedChannelIds, hasItems("/*", "channel1"));
    }
}
