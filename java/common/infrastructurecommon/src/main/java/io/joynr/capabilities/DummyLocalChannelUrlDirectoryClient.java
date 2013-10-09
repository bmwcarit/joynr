package io.joynr.capabilities;

/*
 * #%L
 * joynr::java::common::infrastructurecommon
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

import io.joynr.messaging.LocalChannelUrlDirectoryClient;

import java.util.HashMap;

import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;

public final class DummyLocalChannelUrlDirectoryClient implements LocalChannelUrlDirectoryClient {
    private static final Logger logger = LoggerFactory.getLogger(DummyLocalChannelUrlDirectoryClient.class);

    private HashMap<String, ChannelUrlInformation> registeredChannels = Maps.newHashMap();
    private static LocalChannelUrlDirectoryClient dummyChannelUrlClient = new DummyLocalChannelUrlDirectoryClient();

    @Override
    public void unregisterChannelUrls(String channelId) {
        logger.info("!!!!!!!!!!!!!!!unregisterChannelUrls: " + channelId);
        registeredChannels.remove(channelId);
    }

    @Override
    public void registerChannelUrls(String channelId, ChannelUrlInformation channelUrlInformation) {
        logger.info("!!!!!!!!!!!!!!!registerChannelUrls: " + channelId + ": " + channelUrlInformation);
        registeredChannels.put(channelId, channelUrlInformation);

    }

    @Override
    public ChannelUrlInformation getUrlsForChannel(String channelId) {
        logger.info("!!!!!!!!!!!!!!!getUrlsForChannel: " + channelId);
        return registeredChannels.get(channelId);
    }

    public static void setInstance(LocalChannelUrlDirectoryClient dummyChannelUrlClient) {
        DummyLocalChannelUrlDirectoryClient.dummyChannelUrlClient = dummyChannelUrlClient;
    }

    public static LocalChannelUrlDirectoryClient getInstance() {
        return dummyChannelUrlClient;
    }
}
