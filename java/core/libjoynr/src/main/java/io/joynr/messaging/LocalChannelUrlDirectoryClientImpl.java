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

import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import joynr.types.ChannelUrlInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class LocalChannelUrlDirectoryClientImpl implements LocalChannelUrlDirectoryClient {

    private static final Logger logger = LoggerFactory.getLogger(LocalChannelUrlDirectoryClient.class);

    private final GlobalChannelUrlDirectoryClient channelUrlDirectoryClient;
    private final ChannelUrlStore channelUrlStore;

    @Inject
    // CHECKSTYLE:OFF
    public LocalChannelUrlDirectoryClientImpl(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                              @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId,
                                              @Named(MessagingPropertyKeys.CHANNELURLDIRECTORYURL) String channelUrlDirectoryUrl,
                                              @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabilitiesDirectoryChannelId,
                                              @Named(MessagingPropertyKeys.CAPABILITIESDIRECTORYURL) String capabilitiesDirectoryUrl,
                                              LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                              ChannelUrlStore channelUrlStore,
                                              MessagingSettings settings,
                                              ProxyInvocationHandlerFactory proxyInvocationHandlerFactory) {
        // CHECKSTYLE:ON
        this.channelUrlDirectoryClient = new GlobalChannelUrlDirectoryClient(discoveryDirectoriesDomain,
                                                                             localCapabilitiesDirectory,
                                                                             proxyInvocationHandlerFactory);
        this.channelUrlStore = channelUrlStore;
        channelUrlStore.registerChannelUrl(channelUrlDirectoryChannelId, channelUrlDirectoryUrl);
        channelUrlStore.registerChannelUrl(capabilitiesDirectoryChannelId, capabilitiesDirectoryUrl);

    }

    @Override
    public void registerChannelUrls(final String channelId, final ChannelUrlInformation channelUrlInformation) {
        logger.debug("registered {} for {}", channelId, channelUrlInformation);

        channelUrlStore.registerChannelUrls(channelId, channelUrlInformation);
        try {
            channelUrlDirectoryClient.registerChannelUrls(new Callback<Void>() {

                @Override
                public void onSuccess(Void result) {
                    logger.debug("successfully registered channelId: {} channelUrls: {}",
                                 channelId,
                                 channelUrlInformation.getUrls());
                }

                @Override
                public void onFailure(JoynrRuntimeException e) {
                    //Currently not retrying. Using long TTL instead.
                    logger.error("exception while registering channelId: {} reason: {}", channelId, e.getMessage());

                }
            }, channelId, channelUrlInformation);

        } catch (JoynrRuntimeException e) {
            logger.error("exception while registering channelId: {} reason: {}", channelId, e.getMessage());
        }
    }

    @Override
    public void unregisterChannelUrls(String channelId) {
        logger.debug("removing {}", channelId);

        channelUrlStore.removeChannelUrls(channelId);
        channelUrlDirectoryClient.unregisterChannelUrls(channelId);
    }

    @Override
    public ChannelUrlInformation getUrlsForChannel(String channelId) {
        logger.debug("get URLs for Channel: {}", channelId);

        // retrieve from cache
        ChannelUrlInformation channelUrlInformation = channelUrlStore.findChannelEntry(channelId);
        if (channelUrlInformation.getUrls().isEmpty()) {
            // retrieve from remote store
            synchronized (channelUrlInformation) {
                if (channelUrlInformation.getUrls().isEmpty()) {
                    ChannelUrlInformation remoteChannelUrlInformation = channelUrlDirectoryClient.getUrlsForChannel(channelId);
                    if (remoteChannelUrlInformation == null || remoteChannelUrlInformation.getUrls() == null
                            || remoteChannelUrlInformation.getUrls().isEmpty()) {
                        logger.error("No channelurls found for channel {}", channelId);
                    } else {
                        channelUrlInformation.setUrls(remoteChannelUrlInformation.getUrls());
                    }
                }
            }
        }

        return channelUrlInformation;
    }

}
