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
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;

import java.util.ArrayList;
import java.util.List;

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
            public void unregisterChannelUrls(@JoynrRpcParam("channelId") String channelId)
                                                                                           throws JoynrArbitrationException {

            }

            @Override
            public void registerChannelUrls(@JoynrRpcParam("channelId") String channelId,
                                            @JoynrRpcParam("channelUrlInformation") ChannelUrlInformation channelUrlInformation)
                                                                                                                                throws JoynrArbitrationException {

            }

            @Override
            @JoynrRpcReturn(deserializationType = ChannelUrlInformationToken.class)
            public ChannelUrlInformation getUrlsForChannel(@JoynrRpcParam("channelId") String channelId)
                                                                                                        throws JoynrArbitrationException {
                List<String> urls = new ArrayList<String>();
                urls.add(bounceProxyUrl + "channels/" + channelId + "/");
                return new ChannelUrlInformation(urls);
            }

            @Override
            public Future<Void> unregisterChannelUrls(@JoynrRpcCallback(deserializationType = VoidToken.class) Callback<Void> callback,
                                                      @JoynrRpcParam("channelId") String channelId) {
                Future<Void> future = new Future<Void>();
                future.onSuccess(null);
                return future;
            }

            @Override
            public Future<Void> registerChannelUrls(@JoynrRpcCallback(deserializationType = VoidToken.class) Callback<Void> callback,
                                                    @JoynrRpcParam("channelId") String channelId,
                                                    @JoynrRpcParam("channelUrlInformation") ChannelUrlInformation channelUrlInformation) {

                Future<Void> future = new Future<Void>();
                future.onSuccess(null);
                return future;

            }

            @Override
            public Future<ChannelUrlInformation> getUrlsForChannel(@JoynrRpcCallback(deserializationType = ChannelUrlInformationToken.class) Callback<ChannelUrlInformation> callback,
                                                                   @JoynrRpcParam("channelId") String channelId) {
                ChannelUrlInformation urlsForChannel = getUrlsForChannel(channelId);
                Future<ChannelUrlInformation> future = new Future<ChannelUrlInformation>();
                future.onSuccess(urlsForChannel);
                return future;
            }
        };
    }

}
