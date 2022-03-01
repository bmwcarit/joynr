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

import static io.joynr.util.VersionUtil.getVersionFromAnnotation;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.capabilities.ParticipantIdStorage;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.AddressOperation;
import io.joynr.messaging.routing.CcMessageRouter;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.DiscoverySettingsStorage;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.util.ObjectMapper;
import joynr.system.Discovery;
import joynr.system.Routing;
import joynr.system.RoutingProvider;
import joynr.system.RoutingTypes.Address;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class ClusterControllerRuntime extends JoynrRuntimeImpl {

    public static final Logger logger = LoggerFactory.getLogger(ClusterControllerRuntime.class);
    private ScheduledExecutorService scheduler;
    private ScheduledFuture<?> removeStaleScheduledFuture;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public ClusterControllerRuntime(ObjectMapper objectMapper,
                                    ProxyBuilderFactory proxyBuilderFactory,
                                    Dispatcher dispatcher,
                                    MessagingSkeletonFactory messagingSkeletonFactory,
                                    LocalDiscoveryAggregator localDiscoveryAggregator,
                                    RoutingTable routingTable,
                                    StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory,
                                    DiscoverySettingsStorage discoverySettingsStorage,
                                    ParticipantIdStorage participantIdStorage,
                                    @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                    @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                                    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress,
                                    final CcMessageRouter messageRouter,
                                    CapabilitiesRegistrar capabilitiesRegistrar,
                                    LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                    RoutingProvider routingProvider,
                                    @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                    @Named(SystemServicesSettings.PROPERTY_CC_REMOVE_STALE_DELAY_MS) long removeStaleDelayMs) {
        super(objectMapper,
              proxyBuilderFactory,
              messagingSkeletonFactory,
              localDiscoveryAggregator,
              messageRouter,
              statelessAsyncCallbackDirectory,
              discoverySettingsStorage);

        if (dispatcherAddress instanceof InProcessAddress) {
            ((InProcessAddress) dispatcherAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        routingTable.apply(new AddressOperation() {
            @Override
            public void perform(Address address) {
                if (address instanceof InProcessAddress && ((InProcessAddress) address).getSkeleton() == null) {
                    ((InProcessAddress) address).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
                }
            }
        });

        if (discoveryProviderAddress instanceof InProcessAddress) {
            ((InProcessAddress) discoveryProviderAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        final boolean isGloballyVisible = false;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = true;
        final String discoveryProviderParticipantId = participantIdStorage.getProviderParticipantId(systemServicesDomain,
                                                                                                    Discovery.INTERFACE_NAME,
                                                                                                    getVersionFromAnnotation(Discovery.class).getMajorVersion());
        final String routingProviderParticipantId = participantIdStorage.getProviderParticipantId(systemServicesDomain,
                                                                                                  Routing.INTERFACE_NAME,
                                                                                                  getVersionFromAnnotation(Routing.class).getMajorVersion());
        routingTable.put(discoveryProviderParticipantId,
                         discoveryProviderAddress,
                         isGloballyVisible,
                         expiryDateMs,
                         isSticky);
        routingTable.put(routingProviderParticipantId,
                         discoveryProviderAddress,
                         isGloballyVisible,
                         expiryDateMs,
                         isSticky);

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        final boolean awaitGlobalRegistration = false;
        capabilitiesRegistrar.registerProvider(systemServicesDomain,
                                               localCapabilitiesDirectory,
                                               providerQos,
                                               new String[]{},
                                               awaitGlobalRegistration);
        capabilitiesRegistrar.registerProvider(systemServicesDomain,
                                               routingProvider,
                                               providerQos,
                                               new String[]{},
                                               awaitGlobalRegistration);

        this.scheduler = scheduler;
        scheduleRemoveStale(localCapabilitiesDirectory, removeStaleDelayMs);
    }

    @Override
    public void shutdown(boolean clear) {
        if (removeStaleScheduledFuture != null) {
            removeStaleScheduledFuture.cancel(false);
        }
        super.shutdown(clear);
    }

    private void scheduleRemoveStale(LocalCapabilitiesDirectory localCapabilitiesDirectory, long removeStaleDelayMs) {
        removeStaleScheduledFuture = scheduler.schedule(new Runnable() {
            @Override
            public void run() {
                localCapabilitiesDirectory.removeStaleProvidersOfClusterController();
            }
        }, removeStaleDelayMs, TimeUnit.MILLISECONDS);
    }
}
