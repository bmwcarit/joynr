package io.joynr.runtime;

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

import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.ProxyBuilderFactory;

import javax.inject.Named;

import joynr.system.RoutingTypes.Address;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class ServletJoynrRuntimeImpl extends ClusterControllerRuntime {

    // CHECKSTYLE:OFF
    @Inject
    public ServletJoynrRuntimeImpl(ObjectMapper objectMapper,
                                   ProxyBuilderFactory builderFactory,
                                   RequestCallerDirectory requestCallerDirectory,
                                   ReplyCallerDirectory replyCallerDirectory,
                                   MessageReceiver messageReceiver,
                                   Dispatcher dispatcher,
                                   MessagingStubFactory messagingStubFactory,
                                   MessagingSkeletonFactory messagingSkeletonFactory,
                                   LocalDiscoveryAggregator localDiscoveryAggregator,
                                   LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                   @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabilitiesDirectoryAddress,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS) Address channelUrlDirectoryAddress,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS) Address domainAccessControllerAddress,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_CLUSTERCONTROLER_MESSAGING_SKELETON) IMessagingSkeleton clusterControllerMessagingSkeleton,
                                   @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                   CapabilitiesRegistrar capabilitiesRegistrar,
                                   @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress,
                                   MessageRouter messageRouter) {
        // CHECKSTYLE:ON
        super(objectMapper,
              builderFactory,
              requestCallerDirectory,
              replyCallerDirectory,
              dispatcher,
              messagingStubFactory,
              messagingSkeletonFactory,
              localDiscoveryAggregator,
              systemServicesDomain,
              dispatcherAddress,
              capabilitiesDirectoryAddress,
              channelUrlDirectoryAddress,
              domainAccessControllerAddress,
              discoveryProviderAddress,
              clusterControllerMessagingSkeleton,
              capabilitiesRegistrar,
              localCapabilitiesDirectory,
              messageReceiver,
              messageRouter);
        // CHECKSTYLE:ON
    }

    @Override
    /**
     * Unregistering currently is not receiving any answers, cauing timrout exceptions
     * The reason is that the unregister happens in the ServletContextListener at contextDestroyed
     * which happens after the servlet has already been destroyed. Since the response to unregister
     * would have to arrive via the messaging receiver servlet, this is obviously too late to unregister,
     * but there is no obvious fix (other than create a long polling message receiver for the unregister)
     * since the servelet lifecycle does not consist of any further usful events.
     */
    public void unregisterProvider(String domain, JoynrProvider provider) {
        try {
            super.unregisterProvider(domain, provider);
        } catch (JoynrCommunicationException e) {

        }
    }

}
