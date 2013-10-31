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

import java.util.HashMap;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The ChannelUrlStore stores a list of channelIds mapped to their ChannelUrls.
 * 
 */

public class ChannelUrlStore {
    private static final Logger logger = LoggerFactory.getLogger(ChannelUrlStore.class);
    private ConcurrentHashMap<String, ChannelUrlInformation> registeredChannels = new ConcurrentHashMap<String, ChannelUrlInformation>();

    public void registerChannelUrls(String channelId, ChannelUrlInformation channelUrlInformation) {
        registeredChannels.put(channelId, channelUrlInformation);
    }

    public void removeChannelUrls(String channelId) {
        registeredChannels.remove(channelId);
    }

    public ChannelUrlInformation findChannelEntry(String channelId) {
        ChannelUrlInformation channelUrlInformation = registeredChannels.get(channelId);
        if (channelUrlInformation == null) {
            channelUrlInformation = new ChannelUrlInformation();
        } else {
            logger.debug("ChannelUrls for channelId {} found: {}", channelId, channelUrlInformation.toString());
        }

        return channelUrlInformation;
    }

    public HashMap<String, ChannelUrlInformation> getAllChannelUrls() {
        return new HashMap<String, ChannelUrlInformation>(registeredChannels);
    }

    public void registerChannelUrl(String channelid, String channelUrl) {
        ChannelUrlInformation channelUrlInformation = registeredChannels.get(channelid);

        if (channelUrlInformation == null) {
            channelUrlInformation = new ChannelUrlInformation();
        }

        List<String> urls = channelUrlInformation.getUrls();
        urls.add(channelUrl);
        channelUrlInformation.setUrls(urls);
        registeredChannels.put(channelid, channelUrlInformation);

    }
}
