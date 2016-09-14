package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;

import javax.inject.Inject;
import joynr.system.RoutingAbstractProvider;
import joynr.system.RoutingTypes.BrowserAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.CommonApiDbusAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

/**
 * Implements the RoutingProvider interface to receive routing information from connected libjoynrs
 *
 */
public class RoutingProviderImpl extends RoutingAbstractProvider {
    private MessageRouter messageRouter;

    /**
     * @param messageRouter handles the logic for the RoutingProvider
     */
    @Inject
    public RoutingProviderImpl(MessageRouter messageRouter) {
        this.messageRouter = messageRouter;
    }

    private Promise<DeferredVoid> resolvedDeferred() {
        final DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, ChannelAddress address) {
        messageRouter.addNextHop(participantId, address);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, MqttAddress address) {
        messageRouter.addNextHop(participantId, address);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, CommonApiDbusAddress address) {
        messageRouter.addNextHop(participantId, address);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, BrowserAddress address) {
        messageRouter.addNextHop(participantId, address);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, WebSocketAddress address) {
        messageRouter.addNextHop(participantId, address);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, WebSocketClientAddress address) {
        messageRouter.addNextHop(participantId, address);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> removeNextHop(String participantId) {
        messageRouter.removeNextHop(participantId);
        return resolvedDeferred();
    }

    @Override
    public Promise<ResolveNextHopDeferred> resolveNextHop(String participantId) {
        boolean resolved = messageRouter.resolveNextHop(participantId);
        ResolveNextHopDeferred deferred = new ResolveNextHopDeferred();
        deferred.resolve(resolved);
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> addMulticastReceiver(String multicastId,
                                                      String subscriberParticipantId,
                                                      String providerParticipantId) {
        assert false : "method not implemented";
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> removeMulticastReceiver(String multicastId,
                                                         String subscriberParticipantId,
                                                         String providerParticipantId) {
        assert false : "method not implemented";
        return resolvedDeferred();
    }
}
