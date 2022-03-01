/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.runtime;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.arbitration.VersionCompatibilityChecker;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.LibJoynrMessageRouter;
import io.joynr.messaging.sender.LibJoynrMessageSender;
import io.joynr.proxy.DiscoverySettingsStorage;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;

public class LibjoynrRuntime extends JoynrRuntimeImpl {

    public static final Logger logger = LoggerFactory.getLogger(LibjoynrRuntime.class);

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LibjoynrRuntime(ObjectMapper objectMapper,
                           ProxyBuilderFactory proxyBuilderFactory,
                           MessagingSkeletonFactory messagingSkeletonFactory,
                           LocalDiscoveryAggregator localDiscoveryAggregator,
                           StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory,
                           DiscoverySettingsStorage discoverySettingsStorage,
                           VersionCompatibilityChecker versionCompatibilityChecker,
                           @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                           @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) final Address ccMessagingAddress,
                           final LibJoynrMessageRouter messageRouter,
                           final LibJoynrMessageSender messageSender,
                           @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) final String parentRoutingProviderParticipantId) {
        super(objectMapper,
              proxyBuilderFactory,
              messagingSkeletonFactory,
              localDiscoveryAggregator,
              messageRouter,
              statelessAsyncCallbackDirectory,
              discoverySettingsStorage,
              versionCompatibilityChecker);

        final ProxyBuilder<RoutingProxy> proxyBuilder = getProxyBuilder(systemServicesDomain, RoutingProxy.class);

        ProxyCreatedCallback<RoutingProxy> routingProxyCreatedCallback = new ProxyCreatedCallback<RoutingProxy>() {

            @Override
            public void onProxyCreationFinished(RoutingProxy routingProxy) {
                try {
                    messageRouter.setParentRouter(routingProxy,
                                                  ccMessagingAddress,
                                                  parentRoutingProviderParticipantId,
                                                  proxyBuilder.getParticipantId());
                    messageSender.setReplyToAddress(routingProxy.getReplyToAddress(), routingProxy.getGlobalAddress());
                } catch (Exception e) {
                    logger.error("Error during runtime creation, communication with other joynr runtimes might not be possible.",
                                 e);
                }
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                logger.error("Fatal error during runtime creation, no communication with other joynr runtimes is possible:"
                        + " Routing proxy creation failed:", error);
            }
        };
        proxyBuilder.build(routingProxyCreatedCallback);
    }
}
