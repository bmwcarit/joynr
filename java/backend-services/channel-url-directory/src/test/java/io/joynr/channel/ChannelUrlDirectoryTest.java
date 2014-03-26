package io.joynr.channel;

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

import java.util.Arrays;
import java.util.List;
import java.util.UUID;

import joynr.types.ChannelUrlInformation;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class ChannelUrlDirectoryTest {
    private ChannelUrlDirectoyImpl fixture;

    @Before
    public void setup() {
        fixture = new ChannelUrlDirectoyImpl(500);
    }

    @Test
    public void testDelayedCleanup() throws Exception {

        String testChannelId = "testDelayedCleanup" + UUID.randomUUID().toString();
        String[] urls = { "http://testurl.com/" + testChannelId + "/" };
        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        channelUrlInformation.setUrls(Arrays.asList(urls));

        fixture.registerChannelUrls(testChannelId, channelUrlInformation);

        ChannelUrlInformation urlsForChannelId = fixture.getUrlsForChannel(testChannelId);

        List<String> urlsFromServer = urlsForChannelId.getUrls();
        Assert.assertArrayEquals(urls, urlsFromServer.toArray(new String[urlsFromServer.size()]));
        fixture.unregisterChannelUrls(testChannelId);
        /* after deletion, url shall still be a valid channelurl, as the unregistration shall only affect after
         * fixture.channelurInactiveTimeInMS
         */

        urlsForChannelId = fixture.getUrlsForChannel(testChannelId);
        urlsFromServer = urlsForChannelId.getUrls();
        Assert.assertArrayEquals(urls, urlsFromServer.toArray(new String[urlsFromServer.size()]));

        synchronized (this) {
            this.wait(fixture.channelurInactiveTimeInMS * 2);
        }
        urlsForChannelId = fixture.getUrlsForChannel(testChannelId);
        urlsFromServer = urlsForChannelId.getUrls();
        Assert.assertEquals(0, urlsForChannelId.getUrls().size());

    }

    @Test
    public void testDelayedCleanupWithReactivate() throws Exception {

        String testChannelId = "testDelayedCleanupWithReactivate" + UUID.randomUUID().toString();
        String[] urls = { "http://testurl.com/" + testChannelId + "/" };
        ChannelUrlInformation channelUrlInformation = new ChannelUrlInformation();
        channelUrlInformation.setUrls(Arrays.asList(urls));

        fixture.registerChannelUrls(testChannelId, channelUrlInformation);

        fixture.unregisterChannelUrls(testChannelId);
        ChannelUrlInformation urlsForChannelId = fixture.getUrlsForChannel(testChannelId);
        List<String> urlsFromServer = urlsForChannelId.getUrls();
        Assert.assertArrayEquals(urls, urlsFromServer.toArray(new String[urlsFromServer.size()]));
        Assert.assertEquals(1, fixture.inactiveChannelIds.size());
        Assert.assertNotNull(fixture.inactiveChannelIds.get(testChannelId));
        fixture.registerChannelUrls(testChannelId, channelUrlInformation);
        Assert.assertEquals(0, fixture.inactiveChannelIds.size());
        synchronized (this) {
            this.wait(fixture.channelurInactiveTimeInMS * 2);
        }
        urlsForChannelId = fixture.getUrlsForChannel(testChannelId);
        urlsFromServer = urlsForChannelId.getUrls();
        Assert.assertArrayEquals(urls, urlsFromServer.toArray(new String[urlsFromServer.size()]));

    }
}
