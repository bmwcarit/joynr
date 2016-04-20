package io.joynr.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;

import javax.annotation.CheckForNull;
import javax.inject.Singleton;

import joynr.infrastructure.ChannelUrlDirectoryProxy;
import joynr.types.ChannelUrlInformation;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;

public class DefaultUrlDirectoryModule extends AbstractModule {

    @Override
    protected void configure() {
    }

    @CheckForNull
    @Provides
    @Singleton
    ChannelUrlDirectoryProxy provideChannelUrlDirectoryClient(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                                              @Named(MessagingPropertyKeys.BOUNCE_PROXY_URL) final String bounceProxyUrl) {
        return new ChannelUrlDirectoryProxy() {

            @Override
            public void unregisterChannelUrls(String channelId) throws DiscoveryException {

            }

            @Override
            public void registerChannelUrls(String channelId, ChannelUrlInformation channelUrlInformation)
                                                                                                          throws DiscoveryException {

            }

            @Override
            public ChannelUrlInformation getUrlsForChannel(String channelId) throws DiscoveryException {
                String[] urls = { bounceProxyUrl + "channels/" + channelId + "/" };
                return new ChannelUrlInformation(urls);
            }

            @Override
            public Future<Void> unregisterChannelUrls(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback,
                                                      String channelId) {
                Future<Void> future = new Future<Void>();
                future.onSuccess(null);
                return future;
            }

            @Override
            public Future<Void> registerChannelUrls(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback,
                                                    String channelId,
                                                    ChannelUrlInformation channelUrlInformation) {

                Future<Void> future = new Future<Void>();
                future.onSuccess(null);
                return future;

            }

            @Override
            public Future<ChannelUrlInformation> getUrlsForChannel(@JoynrRpcCallback(deserializationType = ChannelUrlInformation.class) Callback<ChannelUrlInformation> callback,
                                                                   String channelId) {
                ChannelUrlInformation urlsForChannel = getUrlsForChannel(channelId);
                Future<ChannelUrlInformation> future = new Future<ChannelUrlInformation>();
                future.onSuccess(urlsForChannel);
                return future;
            }
        };
    }

}
