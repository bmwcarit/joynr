package io.joynr.channel;

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

import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;

import io.joynr.messaging.MessagingSettings;

import java.util.concurrent.ConcurrentHashMap;

import javax.annotation.CheckForNull;

import joynr.infrastructure.ChannelUrlDirectoryAbstractProvider;
import joynr.types.ChannelUrlInformation;
import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;

/**
 * The channelurldirectory stores channelIds mapped to channelUrls.
 * 
 * 
 * channelurls are stored in a concurrentHashMap. Using a in memory database could be possible optimization.
 */
// TODO Evaluate pro /cons of a in memory database

@Singleton
public class ChannelUrlDirectoyImpl extends ChannelUrlDirectoryAbstractProvider {
    private static final Logger logger = LoggerFactory.getLogger(ChannelUrlDirectoyImpl.class);

    @Inject
    MessagingSettings settings;
    private ConcurrentHashMap<String, ChannelUrlInformation> registeredChannels = new ConcurrentHashMap<String, ChannelUrlInformation>();

    ConcurrentHashMap<String, ChannelUrlInformation> getRegisteredChannels() {
        return registeredChannels;
    }

    @Override
    public ChannelUrlInformation getUrlsForChannel(String channelId) {

        ChannelUrlInformation channelUrlInformation = registeredChannels.get(channelId);
        if (channelUrlInformation == null) {
            channelUrlInformation = new ChannelUrlInformation();
            logger.warn("GLOBAL getUrlsForChannel for Channel: {} found nothing.", channelId, channelUrlInformation);
        } else {
            logger.debug("GLOBAL getUrlsForChannel ChannelUrls for channelId {} found: {}",
                         channelId,
                         channelUrlInformation);
        }

        return channelUrlInformation;

    }

    @Override
    @CheckForNull
    public ProviderQos getProviderQos() {
        return providerQos;
    }

    @Override
    public void registerChannelUrls(@JoynrRpcParam("channelId") String channelId,
                                    @JoynrRpcParam("channelUrlInformation") joynr.types.ChannelUrlInformation channelUrlInformation) {
        logger.debug("GLOBAL registerChannelUrls channelId: {} channelUrls: {}", channelId, channelUrlInformation);
        registeredChannels.put(channelId, channelUrlInformation);

    }

    @Override
    public void unregisterChannelUrls(@JoynrRpcParam("channelId") String channelId) {
        logger.debug("GLOBAL unregisterChannelUrls channelId: {}", channelId);
        registeredChannels.remove(channelId);
    }

}
