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

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.IMessaging;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;

import java.io.IOException;

import javax.annotation.CheckForNull;
import javax.inject.Inject;
import javax.inject.Singleton;

import joynr.JoynrMessage;
import joynr.system.RoutingAbstractProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BrowserAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.CommonApiDbusAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MessageRouterImpl extends RoutingAbstractProvider implements MessageRouter {

    private Logger logger = LoggerFactory.getLogger(MessageRouterImpl.class);
    private final RoutingTable routingTable;
    private MessagingStubFactory messagingStubFactory;
    private static final int UUID_TAIL = 32;

    @Inject
    @Singleton
    public MessageRouterImpl(RoutingTable routingTable, MessagingStubFactory messagingStubFactory) {
        this.routingTable = routingTable;
        this.messagingStubFactory = messagingStubFactory;
    }

    protected Promise<DeferredVoid> addNextHopInternal(String participantId, Address address) {
        routingTable.put(participantId, address);
        final DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, ChannelAddress channelAddress) {
        return addNextHopInternal(participantId, channelAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, CommonApiDbusAddress commonApiDbusAddress) {
        return addNextHopInternal(participantId, commonApiDbusAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, BrowserAddress browserAddress) {
        return addNextHopInternal(participantId, browserAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, WebSocketAddress webSocketAddress) {
        return addNextHopInternal(participantId, webSocketAddress);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, WebSocketClientAddress webSocketClientAddress) {
        return addNextHopInternal(participantId, webSocketClientAddress);
    }

    @Override
    public Promise<DeferredVoid> removeNextHop(String participantId) {
        routingTable.remove(participantId);
        DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<ResolveNextHopDeferred> resolveNextHop(String participantId) {
        ResolveNextHopDeferred deferred = new ResolveNextHopDeferred();
        deferred.resolve(routingTable.containsKey(participantId));
        return new Promise<ResolveNextHopDeferred>(deferred);
    }

    @CheckForNull
    protected Address getAddress(String toParticipantId) {
        Address address = null;
        if (toParticipantId != null && routingTable.containsKey(toParticipantId)) {
            address = routingTable.get(toParticipantId);
        }
        return address;
    }

    @Override
    public void route(JoynrMessage message) throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                           IOException {
        String toParticipantId = message.getTo();
        Address address = getAddress(toParticipantId);
        if (address != null) {
            routeMessageByAddress(message, address);
        } else {
            throw new JoynrMessageNotSentException("Failed to send Request: No route for given participantId: "
                    + toParticipantId);
        }
    }

    private void routeMessageByAddress(JoynrMessage message, Address address) throws JoynrSendBufferFullException,
                                                                             JoynrMessageNotSentException, IOException {

        String messageId = message.getId().substring(UUID_TAIL);
        logger.info(">>>>> SEND  ID:{}:{} from: {} to: {} header: {}", new String[]{ messageId, message.getType(),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID), message.getHeader().toString() });
        logger.debug(">>>>> body  ID:{}:{}: {}", new String[]{ messageId, message.getType(), message.getPayload() });
        IMessaging messagingStub = messagingStubFactory.create(address);
        messagingStub.transmit(message);
    }

    @Override
    public void addNextHop(String participantId, Address address) {
        addNextHopInternal(participantId, address);
    }

    @Override
    public void shutdown() {
        messagingStubFactory.shutdown();
    }

}
