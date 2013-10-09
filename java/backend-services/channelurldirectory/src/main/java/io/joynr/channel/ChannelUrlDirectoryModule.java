package io.joynr.channel;

/*
 * #%L
 * joynr::java::backend-services::channelurldirectory
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

import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.discovery.DiscoveryClientModule;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.proxy.Future;
import io.joynr.pubsub.subscription.SubscriptionManager;
import io.joynr.runtime.AbstractJoynrApplication;

import java.util.List;

import joynr.infrastructure.ChannelUrlDirectoryAbstractProvider;
import joynr.infrastructure.ChannelUrlDirectoryProxy;
import joynr.types.CapabilityInformation;
import joynr.types.ChannelUrlInformation;
import joynr.types.ProviderQosRequirements;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

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

    @Provides
    // @Singleton
    protected GlobalCapabilitiesDirectoryClient provideCapabilitiesDirectoryClient(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                                                                   RequestReplySender messageSender,
                                                                                   RequestReplyDispatcher dispatcher,
                                                                                   @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String capabilitiesDirectoryDomain,
                                                                                   @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_CLIENT_REQUEST_TIMEOUT) long capabilitiesClientRequestTimeoutMs,
                                                                                   SubscriptionManager subscriptionManager) {

        DiscoveryClientModule discoveryClientModule = new DiscoveryClientModule();
        final GlobalCapabilitiesDirectoryClient proxy = discoveryClientModule.provideCapabilitiesDirectoryClient(localCapabilitiesDirectory,
                                                                                                                 messageSender,
                                                                                                                 dispatcher,
                                                                                                                 capabilitiesDirectoryDomain,
                                                                                                                 capabilitiesClientRequestTimeoutMs,
                                                                                                                 subscriptionManager);

        return new GlobalCapabilitiesDirectoryClient() {

            @Override
            public void unregisterCapability(CapabilityInformation capInfo) {
                // Don't register capabilities globally for channelUrlDirectory
                return;
            }

            @Override
            public void unregisterCapabilities(List<CapabilityInformation> capabilities) {
                // Don't register capabilities globally for channelUrlDirectory
                return;
            }

            @Override
            public void registerCapability(CapabilityInformation capability) {
                // Don't register capabilities globally for channelUrlDirectory
                return;

            }

            @Override
            public void registerCapabilities(List<CapabilityInformation> capabilities) {
                // Don't register capabilities globally for channelUrlDirectory
                return;

            }

            @Override
            public void registerCapabilities(Callback<Void> callback, List<CapabilityInformation> capabilities) {
                // Don't register capabilities globally for channelUrlDirectory
                callback.onSuccess(null);
                return;

            }

            @Override
            public void registerCapability(Callback<Void> callback, CapabilityInformation capability) {
                // Don't register capabilities globally for channelUrlDirectory
                callback.onSuccess(null);
                return;

            }

            @Override
            public Future<List<CapabilityInformation>> lookupCapabilities(Callback<List<CapabilityInformation>> callback,
                                                                          String domain,
                                                                          String interfaceName,
                                                                          ProviderQosRequirements qos) {
                return proxy.lookupCapabilities(callback, domain, interfaceName, qos);
            }

            @Override
            public Future<List<CapabilityInformation>> getCapabilitiesForChannelId(Callback<List<CapabilityInformation>> callback,
                                                                                   String channelId) {

                return proxy.getCapabilitiesForChannelId(callback, channelId);
            }

            @Override
            public Future<List<CapabilityInformation>> getCapabilitiesForParticipantId(Callback<List<CapabilityInformation>> callback,
                                                                                       String participantId) {

                return proxy.getCapabilitiesForParticipantId(callback, participantId);
            }

            @Override
            public void unregisterCapabilities(Callback<Void> callback, List<CapabilityInformation> capabilities) {
                proxy.unregisterCapabilities(callback, capabilities);

            }

            @Override
            public void unregisterCapability(Callback<Void> callback, CapabilityInformation capability) {
                proxy.unregisterCapability(callback, capability);

            }

            @Override
            public List<CapabilityInformation> lookupCapabilities(String domain,
                                                                  String interfaceName,
                                                                  ProviderQosRequirements qos) {

                return proxy.lookupCapabilities(domain, interfaceName, qos);
            }

            @Override
            public List<CapabilityInformation> getCapabilitiesForChannelId(String channelId) {

                return proxy.getCapabilitiesForChannelId(channelId);
            }

            @Override
            public List<CapabilityInformation> getCapabilitiesForParticipantId(String participantId) {

                return proxy.getCapabilitiesForParticipantId(participantId);
            }

        };

    }
}
