package io.joynr.messaging;

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

import java.util.UUID;

import joynr.types.ChannelUrlInformation;

import org.junit.Assert;
import org.junit.Test;

/**
 * Tests the interaction of the dispatcher and communication manager.
 */
public class ChannelUrlStoreTest {

    @Test
    public void testStoreMultipleUrlsForChannelId() throws Exception {
        ChannelUrlStore store = new ChannelUrlStoreImpl();

        String channelId = "testStoreMultipleUrlsForChannelId" + UUID.randomUUID().toString();
        int numberChannelUrls = 10;
        for (int i = 0; i < numberChannelUrls; i++) {
            String channelUrl = "http://" + "myserver:8080/" + channelId + i + "/";
            store.registerChannelUrl(channelId, channelUrl);
        }

        ChannelUrlInformation channelEntry = store.findChannelEntry(channelId);
        Assert.assertEquals("entry contains all urls", numberChannelUrls, channelEntry.getUrls().length);

    }
}
