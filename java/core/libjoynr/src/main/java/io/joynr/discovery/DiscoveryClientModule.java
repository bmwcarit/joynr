package io.joynr.discovery;

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

import io.joynr.arbitration.ArbitrationConstants;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.CapabilitiesCache;
import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.CapabilitiesRegistrarImpl;
import io.joynr.capabilities.CapabilitiesStore;
import io.joynr.capabilities.CapabilitiesStoreImpl;
import io.joynr.capabilities.InProcessCapabilitiesProvisioning;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.capabilities.LocalCapabilitiesDirectoryImpl;
import io.joynr.capabilities.ParticipantIdStorage;
import io.joynr.capabilities.PropertiesFileParticipantIdStorage;
import io.joynr.messaging.ChannelUrlStore;
import io.joynr.messaging.ChannelUrlStoreImpl;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.LocalChannelUrlDirectoryClient;
import io.joynr.messaging.LocalChannelUrlDirectoryClientImpl;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;

import javax.annotation.CheckForNull;

import io.joynr.runtime.SystemServicesSettings;
import joynr.infrastructure.ChannelUrlDirectoryProxy;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import joynr.system.DiscoveryAsync;
import joynr.system.DiscoveryProxy;

public class DiscoveryClientModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(LocalDiscoveryAggregator.class).in(Singleton.class);
        bind(DiscoveryAsync.class).to(LocalDiscoveryAggregator.class);
        bind(ChannelUrlStore.class).to(ChannelUrlStoreImpl.class).in(Singleton.class);
        bind(LocalCapabilitiesDirectory.class).to(LocalCapabilitiesDirectoryImpl.class).in(Singleton.class);
        bind(CapabilitiesProvisioning.class).to(InProcessCapabilitiesProvisioning.class);
        bind(CapabilitiesRegistrar.class).to(CapabilitiesRegistrarImpl.class);
        bind(LocalChannelUrlDirectoryClient.class).to(LocalChannelUrlDirectoryClientImpl.class).in(Singleton.class);
        bind(CapabilitiesStore.class).to(CapabilitiesStoreImpl.class);
        bind(CapabilitiesCache.class);
        bind(ParticipantIdStorage.class).to(PropertiesFileParticipantIdStorage.class);
        requestStaticInjection(ArbitratorFactory.class);
    }

    @CheckForNull
    @Provides
    @Singleton
    ChannelUrlDirectoryProxy provideChannelUrlDirectoryClient(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoriesDomain,
                                                              @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT) long discoveryRequestTimeoutMs,
                                                              ProxyBuilderFactory proxyBuilderFactory) {
        MessagingQos messagingQos = new MessagingQos(discoveryRequestTimeoutMs);

        DiscoveryQos discoveryQos = new DiscoveryQos(1000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_THEN_GLOBAL);

        ProxyBuilder<ChannelUrlDirectoryProxy> proxyBuilder = proxyBuilderFactory.get(discoveryDirectoriesDomain,
                                                                                      ChannelUrlDirectoryProxy.class);

        return proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
    }

    @CheckForNull
    @Provides
    @Singleton
    DiscoveryProxy provideDiscoveryProxy(@Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                         @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT) long discoveryRequestTimeoutMs,
                                         @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                         ProxyBuilderFactory proxyBuilderFactory) {
        MessagingQos messagingQos = new MessagingQos(discoveryRequestTimeoutMs);

        DiscoveryQos discoveryQos = new DiscoveryQos(1000,
                                                     ArbitrationStrategy.FixedChannel,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        discoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD, discoveryProviderParticipantId);
        ProxyBuilder<DiscoveryProxy> proxyBuilder = proxyBuilderFactory.get(systemServicesDomain, DiscoveryProxy.class);

        return proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
    }
}
