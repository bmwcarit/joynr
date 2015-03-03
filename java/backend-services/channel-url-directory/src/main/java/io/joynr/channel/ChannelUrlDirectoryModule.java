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

import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.proxy.Future;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.infrastructure.ChannelUrlDirectoryAbstractProvider;
import joynr.infrastructure.ChannelUrlDirectoryProviderAsync;
import joynr.infrastructure.ChannelUrlDirectoryProxy;
import joynr.types.ChannelUrlInformation;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import com.google.inject.name.Names;

/**
 * Overrides the ChannelUrlDirectoryClient Provider to return a NO-OP provider
 * 
 * @author david.katz
 * 
 */
public class ChannelUrlDirectoryModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(ChannelUrlDirectoryProviderAsync.class).to(ChannelUrlDirectoyImpl.class);
        bind(Long.class).annotatedWith(Names.named(ChannelUrlDirectoyImpl.CHANNELURL_INACTIVE_TIME_IN_MS))
                        .toInstance(5000l);
    }

    @Provides
    @Named(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL)
    String provideChannelUrlDirectoryDomain(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String channelUrlDirectoryDomain) {
        return channelUrlDirectoryDomain;
    }

    @Provides
    @Named(MessagingPropertyKeys.CHANNELID)
    String provideChannelUrlDirectoryChannelId(@Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId) {
        return channelUrlDirectoryChannelId;
    }

    /**
     * passes local queries for channelurls to itself (ie asks global via method call and not via joynr proxy)
     * 
     * @param channelUrlDirectory
     * @return
     */
    @Provides
    @Singleton
    ChannelUrlDirectoryProxy provideChannelUrlDirectoryClient(final ChannelUrlDirectoryAbstractProvider channelUrlDirectory) {

        return new ChannelUrlDirectoryProxy() {

            @Override
            public ChannelUrlInformation getUrlsForChannel(String channelId) {
                return channelUrlDirectory.getUrlsForChannel(channelId);
            }

            @Override
            public void registerChannelUrls(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                            @JoynrRpcParam("channelId") String channelId,
                                            @JoynrRpcParam("channelUrlInformation") ChannelUrlInformation channelUrlInformation) {
                channelUrlDirectory.registerChannelUrls(channelId, channelUrlInformation);
                callback.onSuccess(null);
            }

            @Override
            public void unregisterChannelUrls(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                              @JoynrRpcParam("channelId") String channelId) {
                channelUrlDirectory.unregisterChannelUrls(channelId);
                callback.onSuccess(null);
            }

            @Override
            public Future<ChannelUrlInformation> getUrlsForChannel(@JoynrRpcCallback(deserialisationType = joynr.infrastructure.ChannelUrlDirectorySync.ChannelUrlInformationToken.class) Callback<ChannelUrlInformation> callback,
                                                                   @JoynrRpcParam("channelId") String channelId) {
                callback.onSuccess(channelUrlDirectory.getUrlsForChannel(channelId));
                Future<ChannelUrlInformation> future = new Future<ChannelUrlInformation>();
                future.onSuccess(channelUrlDirectory.getUrlsForChannel(channelId));
                return future;
            }

            @Override
            public void registerChannelUrls(@JoynrRpcParam("channelId") String channelId,
                                            @JoynrRpcParam("channelUrlInformation") ChannelUrlInformation channelUrlInformation) {
                channelUrlDirectory.registerChannelUrls(channelId, channelUrlInformation);
            }

            @Override
            public void unregisterChannelUrls(@JoynrRpcParam("channelId") String channelId) {
                channelUrlDirectory.unregisterChannelUrls(channelId);
            }

        };
    }
}
