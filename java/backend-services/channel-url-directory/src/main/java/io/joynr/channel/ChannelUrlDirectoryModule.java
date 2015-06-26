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

import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRequestInterruptedException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.provider.PromiseKeeper;
import io.joynr.provider.PromiseListener;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.infrastructure.ChannelUrlDirectoryAbstractProvider;
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
        bind(ChannelUrlDirectoryAbstractProvider.class).to(ChannelUrlDirectoyImpl.class);
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
                try {
                    PromiseKeeper keeper = new PromiseKeeper();
                    channelUrlDirectory.getUrlsForChannel(channelId).then(keeper);
                    Object[] outValues = keeper.getValues();
                    if (outValues == null) {
                        throw new JoynrIllegalStateException("Calling method with out parameters didn't return anything.");
                    }
                    return (ChannelUrlInformation) outValues[0];
                } catch (InterruptedException e) {
                    throw new JoynrRequestInterruptedException("interrupted while calling getUrlsForChannel("
                            + channelId + ")");
                }
            }

            @Override
            public void registerChannelUrls(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                            @JoynrRpcParam("channelId") String channelId,
                                            @JoynrRpcParam("channelUrlInformation") ChannelUrlInformation channelUrlInformation) {
                channelUrlDirectory.registerChannelUrls(channelId, channelUrlInformation);
                callback.onSuccess(null);
            }

            @Override
            public void unregisterChannelUrls(@JoynrRpcCallback(deserialisationType = VoidToken.class) final Callback<Void> callback,
                                              @JoynrRpcParam("channelId") String channelId) {
                channelUrlDirectory.unregisterChannelUrls(channelId).then(new PromiseListener() {

                    @Override
                    public void onRejection(JoynrRuntimeException error) {
                        callback.onFailure(error);
                    }

                    @Override
                    public void onFulfillment(Object... values) {
                        callback.onSuccess(null);
                    }
                });
                ;
            }

            @Override
            public Future<ChannelUrlInformation> getUrlsForChannel(@JoynrRpcCallback(deserialisationType = joynr.infrastructure.ChannelUrlDirectorySync.ChannelUrlInformationToken.class) final Callback<ChannelUrlInformation> callback,
                                                                   @JoynrRpcParam("channelId") String channelId) {
                final Future<ChannelUrlInformation> future = new Future<ChannelUrlInformation>();
                channelUrlDirectory.getUrlsForChannel(channelId).then(new PromiseListener() {

                    @Override
                    public void onRejection(JoynrRuntimeException error) {
                        callback.onFailure(error);
                        future.onFailure(error);
                    }

                    @Override
                    public void onFulfillment(Object... values) {
                        ChannelUrlInformation channelUrlInfo = (ChannelUrlInformation) values[0];
                        callback.onSuccess(channelUrlInfo);
                        future.onSuccess(channelUrlInfo);
                    }
                });
                return future;
            }

            @Override
            public void registerChannelUrls(@JoynrRpcParam("channelId") String channelId,
                                            @JoynrRpcParam("channelUrlInformation") ChannelUrlInformation channelUrlInformation) {
                PromiseKeeper keeper = new PromiseKeeper();
                channelUrlDirectory.registerChannelUrls(channelId, channelUrlInformation).then(keeper);
                try {
                    keeper.waitForSettlement();
                } catch (InterruptedException e) {
                    throw new JoynrRequestInterruptedException("interrupted while calling registerChannelUrls("
                            + channelId + ", " + channelUrlInformation + ")");
                }
            }

            @Override
            public void unregisterChannelUrls(@JoynrRpcParam("channelId") String channelId) {
                PromiseKeeper keeper = new PromiseKeeper();
                channelUrlDirectory.unregisterChannelUrls(channelId).then(keeper);
                try {
                    keeper.waitForSettlement();
                } catch (InterruptedException e) {
                    throw new JoynrRequestInterruptedException("interrupted while calling unregisterChannelUrls("
                            + channelId + ")");
                }
            }

        };
    }
}
