package io.joynr.runtime;

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

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.websocket.WebSocketMessagingSkeleton;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.system.RoutingTypes.Address;

public class CCJoynrRuntime extends InProcessRuntime {

    private final WebSocketMessagingSkeleton clusterControllerMessagingSkeleton;

    // CHECKSTYLE:OFF
    @Inject
    public CCJoynrRuntime(ObjectMapper objectMapper,
                          ProxyBuilderFactory proxyBuilderFactory,
                          RequestCallerDirectory requestCallerDirectory,
                          ReplyCallerDirectory replyCallerDirectory,
                          Dispatcher dispatcher,
                          LocalDiscoveryAggregator localDiscoveryAggregator,
                          @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                          @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress,
                          @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabilitiesDirectoryAddress,
                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS) Address channelUrlDirectoryAddress,
                          @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS) Address domainAccessControllerAddress,
                          @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_ADDRESS) Address discoveryProviderAddress,
                          @Named(ConfigurableMessagingSettings.PROPERTY_CLUSTERCONTROLER_MESSAGING_SKELETON) WebSocketMessagingSkeleton clusterControllerMessagingSkeleton,
                          CapabilitiesRegistrar capabilitiesRegistrar,
                          LocalCapabilitiesDirectory localCapabilitiesDirectory,
                          MessageReceiver messageReceiver,
                          MessageRouter messageRouter) {
        super(objectMapper,
              proxyBuilderFactory,
              requestCallerDirectory,
              replyCallerDirectory,
              dispatcher,
              localDiscoveryAggregator,
              systemServicesDomain,
              libjoynrMessagingAddress,
              capabilitiesDirectoryAddress,
              channelUrlDirectoryAddress,
              domainAccessControllerAddress,
              discoveryProviderAddress,
              clusterControllerMessagingSkeleton,
              capabilitiesRegistrar,
              localCapabilitiesDirectory,
              messageReceiver);
        this.clusterControllerMessagingSkeleton = clusterControllerMessagingSkeleton;
        this.clusterControllerMessagingSkeleton.initializeConnection();
        capabilitiesRegistrar.registerProvider(systemServicesDomain, messageRouter);
    }

    // CHECKSTYLE:ON

    @Override
    public void shutdown(boolean clear) {
        super.shutdown(clear);
        clusterControllerMessagingSkeleton.shutdown();
    }
}
