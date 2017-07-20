package io.joynr.runtime;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.MessagingSkeletonFactory;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.system.RoutingProvider;
import joynr.system.RoutingTypes.Address;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class ClusterControllerRuntime extends JoynrRuntimeImpl {

    public static final Logger logger = LoggerFactory.getLogger(ClusterControllerRuntime.class);
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;

    // CHECKSTYLE:OFF
    @Inject
    public ClusterControllerRuntime(ObjectMapper objectMapper,
                                    ProxyBuilderFactory proxyBuilderFactory,
                                    Dispatcher dispatcher,
                                    MessagingSkeletonFactory messagingSkeletonFactory,
                                    LocalDiscoveryAggregator localDiscoveryAggregator,
                                    RoutingTable routingTable,
                                    @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                    @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                                    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress,
                                    CapabilitiesRegistrar capabilitiesRegistrar,
                                    LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                    RoutingProvider routingProvider) {
        super(objectMapper,
              proxyBuilderFactory,
              dispatcher,
              messagingSkeletonFactory,
              localDiscoveryAggregator,
              routingTable,
              systemServicesDomain,
              dispatcherAddress,
              discoveryProviderAddress);
        // CHECKSTYLE:ON

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        this.localCapabilitiesDirectory = localCapabilitiesDirectory;
        capabilitiesRegistrar.registerProvider(systemServicesDomain, localCapabilitiesDirectory, providerQos);
        capabilitiesRegistrar.registerProvider(systemServicesDomain, routingProvider, providerQos);
    }

    @Override
    public void shutdown(boolean clear) {
        localCapabilitiesDirectory.shutdown(false);
        super.shutdown(clear);
    }
}
