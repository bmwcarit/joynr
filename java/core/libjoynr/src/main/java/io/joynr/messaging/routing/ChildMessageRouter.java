package io.joynr.messaging.routing;

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

import com.google.inject.Inject;
import io.joynr.exceptions.JoynrException;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.proxy.Callback;
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

import javax.annotation.CheckForNull;
import java.util.LinkedList;
import java.util.List;

/**
 * MessageRouter implementation which adds hops to its parent and tries to resolve unknown addresses at its parent
 */
public class ChildMessageRouter extends MessageRouterImpl {

    private Logger logger = LoggerFactory.getLogger(ChildMessageRouter.class);

    private Address parentRouterMessagingAddress;
    private RoutingProxy parentRouter;
    private Address incomingAddress;
    private List<Runnable> deferredParentHops = new LinkedList<>();

    @Inject
    public ChildMessageRouter(RoutingTable routingTable, MessagingStubFactory messagingStubFactory) {
        super(routingTable, messagingStubFactory);
    }

    @Override
    protected Address getAddress(String toParticipantId) {
        Address address = super.getAddress(toParticipantId);
        if (address == null && parentRouter != null) {
            Boolean parentHasNextHop = parentRouter.resolveNextHop(toParticipantId);
            if (parentHasNextHop) {
                super.addNextHopInternal(toParticipantId, parentRouterMessagingAddress);
                address = parentRouterMessagingAddress;
            }
        }
        return address;
    }

    @Override
    protected Promise<DeferredVoid> addNextHopInternal(final String participantId, final Address address) {
        super.addNextHopInternal(participantId, address);
        final DeferredVoid deferred = new DeferredVoid();
        if (parentRouter != null) {
            addNextHopToParent(participantId, deferred);
        } else {
            deferredParentHops.add(new Runnable() {
                @Override public void run() {
                    addNextHopToParent(participantId, deferred);
                }
            });
        }
        return new Promise<DeferredVoid>(deferred);
    }

    private void addNextHopToParent(String participantId, final DeferredVoid deferred) {
        logger.debug("Adding next hop with participant id " + participantId + " to parent router");
        Callback<Void> callback = new Callback<Void>() {
            @Override
            public void onSuccess(@CheckForNull Void result) {
                deferred.resolve();
            }

            @Override
            public void onFailure(JoynrException error) {
                deferred.reject(new ProviderRuntimeException("Failed to add next hop to parent: " + error));
            }
        };
        if (incomingAddress instanceof ChannelAddress) {
            parentRouter.addNextHop(callback, participantId, (ChannelAddress) incomingAddress);
        } else if (incomingAddress instanceof CommonApiDbusAddress) {
            parentRouter.addNextHop(callback, participantId, (CommonApiDbusAddress) incomingAddress);
        } else if (incomingAddress instanceof BrowserAddress) {
            parentRouter.addNextHop(callback, participantId, (BrowserAddress) incomingAddress);
        } else if (incomingAddress instanceof WebSocketAddress) {
            parentRouter.addNextHop(callback, participantId, (WebSocketAddress) incomingAddress);
        } else if (incomingAddress instanceof WebSocketClientAddress) {
            parentRouter.addNextHop(callback, participantId, (WebSocketClientAddress) incomingAddress);
        } else {
            deferred.reject(new ProviderRuntimeException("Failed to add next hop to parent: unknown address type"
                    + incomingAddress.getClass().getSimpleName()));
        }
    }

    public void setParentRouter(RoutingProxy parentRouter,
                                Address parentRouterMessagingAddress,
                                String parentRoutingProviderParticipantId,
                                String routingProxyParticipantId) {
        this.parentRouter = parentRouter;
        this.parentRouterMessagingAddress = parentRouterMessagingAddress;

        super.addNextHopInternal(parentRoutingProviderParticipantId, parentRouterMessagingAddress);
        addNextHopToParent(routingProxyParticipantId, new DeferredVoid());
        for (Runnable deferredParentHop : deferredParentHops) {
            deferredParentHop.run();
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
