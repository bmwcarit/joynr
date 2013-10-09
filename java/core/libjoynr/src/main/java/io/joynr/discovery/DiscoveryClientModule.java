package io.joynr.discovery;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.CapabilitiesRegistrarImpl;
import io.joynr.capabilities.CapabilitiesStore;
import io.joynr.capabilities.CapabilitiesStoreImpl;
import io.joynr.capabilities.DefaultCapabilitiesProvisioning;
import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.capabilities.LocalCapabilitiesDirectoryImpl;
import io.joynr.capabilities.ParticipantIdStorage;
import io.joynr.capabilities.PropertiesFileParticipantIdStorage;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.LocalChannelUrlDirectoryClientImpl;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderDefaultImpl;
import io.joynr.pubsub.subscription.SubscriptionManager;

import javax.annotation.CheckForNull;

import joynr.infrastructure.ChannelUrlDirectoryProxy;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

public class DiscoveryClientModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(LocalCapabilitiesDirectory.class).to(LocalCapabilitiesDirectoryImpl.class).in(Singleton.class);
        bind(CapabilitiesProvisioning.class).to(DefaultCapabilitiesProvisioning.class);
        bind(CapabilitiesRegistrar.class).to(CapabilitiesRegistrarImpl.class);
        bind(LocalChannelUrlDirectoryClient.class).to(LocalChannelUrlDirectoryClientImpl.class).in(Singleton.class);
        bind(CapabilitiesStore.class).to(CapabilitiesStoreImpl.class);
        bind(ParticipantIdStorage.class).to(PropertiesFileParticipantIdStorage.class);
        requestStaticInjection(ArbitratorFactory.class);
    }

    @CheckForNull
    @Provides
    @Singleton
    ChannelUrlDirectoryProxy provideChannelUrlDirectoryClient(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                                              RequestReplySender messageSender,
                                                              RequestReplyDispatcher dispatcher,
                                                              @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String disocveryDirectoriesDomain,
                                                              SubscriptionManager subscriptionManager) {
        MessagingQos messagingQos = new MessagingQos(5 * 60 * 1000);

        DiscoveryQos discoveryQos = new DiscoveryQos(1000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_THEN_GLOBAL);

        ProxyBuilder<ChannelUrlDirectoryProxy> proxyBuilder = new ProxyBuilderDefaultImpl<ChannelUrlDirectoryProxy>(localCapabilitiesDirectory,
                                                                                                                    disocveryDirectoriesDomain,
                                                                                                                    ChannelUrlDirectoryProxy.class,
                                                                                                                    messageSender,
                                                                                                                    dispatcher,
                                                                                                                    subscriptionManager);

        ChannelUrlDirectoryProxy proxy = null;
        try {
            proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        } catch (JoynrIllegalStateException e) {
            e.printStackTrace();
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        return proxy;
    }

    @CheckForNull
    @Provides
    public// @Singleton
    GlobalCapabilitiesDirectoryClient provideCapabilitiesDirectoryClient(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                                                         RequestReplySender messageSender,
                                                                         RequestReplyDispatcher dispatcher,
                                                                         @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                                                         @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_CLIENT_REQUEST_TIMEOUT) long capabilitiesClientRequestTimeoutMs,
                                                                         SubscriptionManager subscriptionManager) {

        MessagingQos messagingQos = new MessagingQos(capabilitiesClientRequestTimeoutMs);
        DiscoveryQos discoveryQos = new DiscoveryQos(1000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_THEN_GLOBAL);

        // ProxyBuilder<GlobalCapabilitiesDirectoryClient> proxyBuilder =
        // runtime.getProxyBuilder(CapabilitiesDirectory.CAPABILITIES_DIRECTORY_PARTICIPANTID,
        // CapabilitiesDirectory.CAPABILITIES_DIRECTORY_DOMAIN,
        // GlobalCapabilitiesDirectoryClient.class);

        ProxyBuilder<GlobalCapabilitiesDirectoryClient> proxyBuilder = new ProxyBuilderDefaultImpl<GlobalCapabilitiesDirectoryClient>(localCapabilitiesDirectory,
                                                                                                                                      discoveryDirectoriesDomain,
                                                                                                                                      GlobalCapabilitiesDirectoryClient.class,
                                                                                                                                      messageSender,
                                                                                                                                      dispatcher,
                                                                                                                                      subscriptionManager);

        GlobalCapabilitiesDirectoryClient proxy = null;
        try {
            proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        } catch (JoynrIllegalStateException e) {
            e.printStackTrace();
        } catch (JoynrArbitrationException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        return proxy;
    }

}
