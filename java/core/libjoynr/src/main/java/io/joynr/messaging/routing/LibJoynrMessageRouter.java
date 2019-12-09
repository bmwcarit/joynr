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
package io.joynr.messaging.routing;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import io.joynr.statusmetrics.StatusReceiver;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.BrowserAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

/**
 * MessageRouter implementation which adds hops to its parent and tries to resolve unknown addresses at its parent
 */
@Singleton
public class LibJoynrMessageRouter extends AbstractMessageRouter {
    private Logger logger = LoggerFactory.getLogger(LibJoynrMessageRouter.class);

    private static interface DeferrableRegistration {
        void register();
    }

    private static class ParticipantIdAndIsGloballyVisibleHolder {
        final String participantId;
        final boolean isGloballyVisible;

        public ParticipantIdAndIsGloballyVisibleHolder(String participantId, boolean isGloballyVisible) {
            this.participantId = participantId;
            this.isGloballyVisible = isGloballyVisible;
        }
    }

    private Address parentRouterMessagingAddress;
    private RoutingProxy parentRouter;
    private Address incomingAddress;
    private Set<ParticipantIdAndIsGloballyVisibleHolder> deferredParentHopsParticipantIds = new HashSet<>();
    private Map<String, DeferrableRegistration> deferredMulticastRegistrations = new HashMap<>();
    private boolean ready = false;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LibJoynrMessageRouter(RoutingTable routingTable,
                                 @Named(SystemServicesSettings.LIBJOYNR_MESSAGING_ADDRESS) Address incomingAddress,
                                 @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs,
                                 MessagingStubFactory messagingStubFactory,
                                 MessagingSkeletonFactory messagingSkeletonFactory,
                                 AddressManager addressManager,
                                 MulticastReceiverRegistry multicastReceiverRegistry,
                                 MessageQueue messageQueue,
                                 ShutdownNotifier shutdownNotifier,
                                 StatusReceiver statusReceiver) {
        // CHECKSTYLE:ON
        super(routingTable,
              scheduler,
              sendMsgRetryIntervalMs,
              maxParallelSends,
              routingTableCleanupIntervalMs,
              messagingStubFactory,
              messagingSkeletonFactory,
              addressManager,
              multicastReceiverRegistry,
              messageQueue,
              shutdownNotifier,
              statusReceiver);
        this.incomingAddress = incomingAddress;
    }

    @Override
    protected Set<Address> getAddresses(ImmutableMessage message) {
        Set<Address> result = super.getAddresses(message);

        if (result.isEmpty() && parentRouter != null
                && message.getType() != Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST) {
            String toParticipantId = message.getRecipient();
            Boolean parentHasNextHop = parentRouter.resolveNextHop(toParticipantId);
            if (parentHasNextHop) {
                super.addNextHop(toParticipantId, parentRouterMessagingAddress, true); // TODO: use appropriate boolean value in subsequent patch
                result.add(parentRouterMessagingAddress);
            }
        }

        return result;
    }

    @Override
    public void setToKnown(final String participantId) {
        logger.trace("setToKnown called for participantId {}", participantId);
        if (parentRouterMessagingAddress == null) {
            logger.debug("setToKnown called before parentRouterAddress is available");
            return;
        }
        // isGloballyVisible has no influence in libjoynr runtime
        final boolean isGloballyVisible = false;
        super.addNextHop(participantId, parentRouterMessagingAddress, isGloballyVisible);
    }

    @Override
    public void addNextHop(final String participantId, final Address address, final boolean isGloballyVisible) {
        super.addNextHop(participantId, address, isGloballyVisible);
        synchronized (this) {
            if (!ready) {
                deferredParentHopsParticipantIds.add(new ParticipantIdAndIsGloballyVisibleHolder(participantId,
                                                                                                 isGloballyVisible));
                return;
            }
        }
        addNextHopToParent(participantId, isGloballyVisible);
    }

    @Override
    public void removeNextHop(final String participantId) {
        super.removeNextHop(participantId);
        if (parentRouter != null) {
            removeNextHopFromParent(participantId);
        }
    }

    private void addNextHopToParent(String participantId, boolean isGloballyVisible) {
        logger.trace("Adding next hop with participant id " + participantId + " to parent router");
        if (incomingAddress instanceof ChannelAddress) {
            parentRouter.addNextHop(participantId, (ChannelAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof BrowserAddress) {
            parentRouter.addNextHop(participantId, (BrowserAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof WebSocketAddress) {
            parentRouter.addNextHop(participantId, (WebSocketAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof WebSocketClientAddress) {
            parentRouter.addNextHop(participantId, (WebSocketClientAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof BinderAddress) {
            parentRouter.addNextHop(participantId, (BinderAddress) incomingAddress, isGloballyVisible);
        } else {
            throw new ProviderRuntimeException("Failed to add next hop to parent: unknown address type"
                    + incomingAddress.getClass().getSimpleName());
        }
    }

    private void removeNextHopFromParent(String participantId) {
        logger.trace("Removing next hop with participant id " + participantId + " from parent router");
        parentRouter.removeNextHop(participantId);
    }

    @Override
    public void addMulticastReceiver(final String multicastId,
                                     final String subscriberParticipantId,
                                     final String providerParticipantId) {
        super.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        DeferrableRegistration registerWithParent = new DeferrableRegistration() {
            @Override
            public void register() {
                Address providerAddress = routingTable.get(providerParticipantId);
                if (providerAddress == null || !(providerAddress instanceof InProcessAddress)) {
                    parentRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
                }
            }
        };
        synchronized (this) {
            if (!ready) {
                deferredMulticastRegistrations.put(multicastId + subscriberParticipantId + providerParticipantId,
                                                   registerWithParent);
                return;
            }
        }
        registerWithParent.register();
    }

    @Override
    public void removeMulticastReceiver(String multicastId,
                                        String subscriberParticipantId,
                                        String providerParticipantId) {
        super.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        synchronized (this) {
            if (!ready) {
                deferredMulticastRegistrations.remove(multicastId + subscriberParticipantId + providerParticipantId);
                return;
            }
        }
        Address providerAddress = routingTable.get(providerParticipantId);
        if (providerAddress == null || !(providerAddress instanceof InProcessAddress)) {
            parentRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        }
    }

    public void setParentRouter(RoutingProxy parentRouter,
                                Address parentRouterMessagingAddress,
                                String parentRoutingProviderParticipantId,
                                String routingProxyParticipantId) {
        this.parentRouterMessagingAddress = parentRouterMessagingAddress;
        this.parentRouter = parentRouter;

        // because the routing provider is local, therefore isGloballyVisible is false
        final boolean isGloballyVisible = false;
        super.addNextHop(parentRoutingProviderParticipantId, parentRouterMessagingAddress, isGloballyVisible);
        addNextHopToParent(routingProxyParticipantId, isGloballyVisible);
        synchronized (this) {
            for (ParticipantIdAndIsGloballyVisibleHolder participantIds : deferredParentHopsParticipantIds) {
                addNextHopToParent(participantIds.participantId, participantIds.isGloballyVisible);
            }
            deferredParentHopsParticipantIds.clear();
            for (DeferrableRegistration registerWithParent : deferredMulticastRegistrations.values()) {
                registerWithParent.register();
            }
            deferredMulticastRegistrations.clear();
            ready = true;
        }
    }

    /**
     * Sets the address which will be registered at the parent router for the next hop
     * to contact this child message router
     * @param incomingAddress address of this libjoynr instance. Used by the cluster controller's
     *                        message router to forward messages
     */
    public void setIncomingAddress(Address incomingAddress) {
        this.incomingAddress = incomingAddress;
    }
}
