/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.jeeintegration;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.DiscoverySettingsStorage;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.runtime.ClusterControllerRuntime;
import io.joynr.runtime.ProviderRegistrar;
import io.joynr.runtime.SystemServicesSettings;
import joynr.system.RoutingProvider;
import joynr.system.RoutingTypes.Address;
import joynr.types.ProviderQos;

class JeeJoynrRuntimeImpl extends ClusterControllerRuntime implements JeeJoynrRuntime {

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public JeeJoynrRuntimeImpl(ObjectMapper objectMapper,
                               ProxyBuilderFactory proxyBuilderFactory,
                               Dispatcher dispatcher,
                               MessagingSkeletonFactory messagingSkeletonFactory,
                               LocalDiscoveryAggregator localDiscoveryAggregator,
                               RoutingTable routingTable,
                               StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory,
                               DiscoverySettingsStorage discoverySettingsStorage,
                               @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                               @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                               @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress,
                               CapabilitiesRegistrar capabilitiesRegistrar,
                               LocalCapabilitiesDirectory localCapabilitiesDirectory,
                               RoutingProvider routingProvider) {
        // CHECKSTYLE:ON
        super(objectMapper,
              proxyBuilderFactory,
              dispatcher,
              messagingSkeletonFactory,
              localDiscoveryAggregator,
              routingTable,
              statelessAsyncCallbackDirectory,
              discoverySettingsStorage,
              systemServicesDomain,
              dispatcherAddress,
              discoveryProviderAddress,
              capabilitiesRegistrar,
              localCapabilitiesDirectory,
              routingProvider);
    }

    @Override
    public Future<Void> registerProvider(String domain,
                                         JoynrProvider provider,
                                         ProviderQos providerQos,
                                         String[] gbids,
                                         boolean awaitGlobalRegistration,
                                         final Class<?> interfaceClass) {
        if (interfaceClass == null) {
            throw new IllegalArgumentException("Cannot registerProvider: interfaceClass must not be null");
        }
        registerInterfaceClassTypes(interfaceClass, "Cannot registerProvider");

        ProviderRegistrar registrar = getProviderRegistrar(domain, provider).withProviderQos(providerQos)
                                                                            .withGbids(gbids);
        if (awaitGlobalRegistration) {
            registrar.awaitGlobalRegistration();
        }
        return registrar.register();
    }

}
