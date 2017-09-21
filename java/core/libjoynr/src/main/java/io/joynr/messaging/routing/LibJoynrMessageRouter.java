package io.joynr.messaging.routing;

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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import joynr.ImmutableMessage;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BrowserAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.CommonApiDbusAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LibJoynrMessageRouter(RoutingTable routingTable,
                                 @Named(SystemServicesSettings.LIBJOYNR_MESSAGING_ADDRESS) Address incomingAddress,
                                 @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_GRACE_PERIOD_MS) long routingTableGracePeriodMs,
                                 @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs,
                                 MessagingStubFactory messagingStubFactory,
                                 MessagingSkeletonFactory messagingSkeletonFactory,
                                 AddressManager addressManager,
                                 MulticastReceiverRegistry multicastReceiverRegistry,
                                 DelayQueue<DelayableImmutableMessage> messageQueue,
                                 ShutdownNotifier shutdownNotifier) {
        // CHECKSTYLE:ON
        super(routingTable,
              scheduler,
              sendMsgRetryIntervalMs,
              maxParallelSends,
              routingTableGracePeriodMs,
              routingTableCleanupIntervalMs,
              messagingStubFactory,
              messagingSkeletonFactory,
              addressManager,
              multicastReceiverRegistry,
              messageQueue,
              shutdownNotifier);
        this.incomingAddress = incomingAddress;
    }

    @Override
    protected Set<Address> getAddresses(ImmutableMessage message) {
        Set<Address> result;
        JoynrRuntimeException noAddressException = null;
        try {
            result = super.getAddresses(message);
        } catch (JoynrMessageNotSentException | JoynrIllegalStateException e) {
            noAddressException = e;
            result = new HashSet<>();
        }
        String toParticipantId = message.getRecipient();
        if (result.isEmpty() && parentRouter != null) {
            Boolean parentHasNextHop = parentRouter.resolveNextHop(toParticipantId);
            if (parentHasNextHop) {
                super.addNextHop(toParticipantId, parentRouterMessagingAddress, true); // TODO: use appropriate boolean value in subsequent patch
                result.add(parentRouterMessagingAddress);
            }
        }
        if (result.isEmpty() && noAddressException != null) {
            throw noAddressException;
        }
        return result;
    }

    @Override
    public void addNextHop(final String participantId, final Address address, final boolean isGloballyVisible) {
        super.addNextHop(participantId, address, isGloballyVisible);
        if (parentRouter != null) {
            addNextHopToParent(participantId, isGloballyVisible);
        } else {
            deferredParentHopsParticipantIds.add(new ParticipantIdAndIsGloballyVisibleHolder(participantId,
                                                                                             isGloballyVisible));
        }
    }

    private void addNextHopToParent(String participantId, boolean isGloballyVisible) {
        logger.trace("Adding next hop with participant id " + participantId + " to parent router");
        if (incomingAddress instanceof ChannelAddress) {
            parentRouter.addNextHop(participantId, (ChannelAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof CommonApiDbusAddress) {
            parentRouter.addNextHop(participantId, (CommonApiDbusAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof BrowserAddress) {
            parentRouter.addNextHop(participantId, (BrowserAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof WebSocketAddress) {
            parentRouter.addNextHop(participantId, (WebSocketAddress) incomingAddress, isGloballyVisible);
        } else if (incomingAddress instanceof WebSocketClientAddress) {
            parentRouter.addNextHop(participantId, (WebSocketClientAddress) incomingAddress, isGloballyVisible);
        } else {
            throw new ProviderRuntimeException("Failed to add next hop to parent: unknown address type"
                    + incomingAddress.getClass().getSimpleName());
        }
    }

    @Override
    public void addMulticastReceiver(final String multicastId,
                                     final String subscriberParticipantId,
                                     final String providerParticipantId) {
        super.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        DeferrableRegistration registerWithParent = new DeferrableRegistration() {
            @Override
            public void register() {
                parentRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
            }
        };
        if (parentRouter != null) {
            registerWithParent.register();
        } else {
            synchronized (deferredMulticastRegistrations) {
                deferredMulticastRegistrations.put(multicastId + subscriberParticipantId + providerParticipantId,
                                                   registerWithParent);
            }
        }
    }

    @Override
    public void removeMulticastReceiver(String multicastId, String subscriberParticipantId, String providerParticipantId) {
        super.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        if (parentRouter == null) {
            synchronized (deferredMulticastRegistrations) {
                deferredMulticastRegistrations.remove(multicastId + subscriberParticipantId + providerParticipantId);
            }
        }
    }

    public void setParentRouter(RoutingProxy parentRouter,
                                Address parentRouterMessagingAddress,
                                String parentRoutingProviderParticipantId,
                                String routingProxyParticipantId) {
        this.parentRouter = parentRouter;
        this.parentRouterMessagingAddress = parentRouterMessagingAddress;

        // because the routing provider is local, therefore isGloballyVisible is false
        final boolean isGloballyVisible = false;
        super.addNextHop(parentRoutingProviderParticipantId, parentRouterMessagingAddress, isGloballyVisible);
        addNextHopToParent(routingProxyParticipantId, isGloballyVisible);
        for (ParticipantIdAndIsGloballyVisibleHolder participantIds : deferredParentHopsParticipantIds) {
            addNextHopToParent(participantIds.participantId, participantIds.isGloballyVisible);
        }
        synchronized (deferredMulticastRegistrations) {
            for (DeferrableRegistration registerWithParent : deferredMulticastRegistrations.values()) {
                registerWithParent.register();
            }
        }
        deferredParentHopsParticipantIds.clear();
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
