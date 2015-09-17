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

import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingStub;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;

import java.io.IOException;

import javax.inject.Inject;
import javax.inject.Singleton;

import joynr.JoynrMessage;
import joynr.system.RoutingAbstractProvider;
import joynr.system.routingtypes.Address;
import joynr.system.routingtypes.BrowserAddress;
import joynr.system.routingtypes.ChannelAddress;
import joynr.system.routingtypes.CommonApiDbusAddress;
import joynr.system.routingtypes.WebSocketAddress;
import joynr.system.routingtypes.WebSocketClientAddress;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

public class MessageRouterImpl extends RoutingAbstractProvider implements MessageRouter {

    private static final Logger logger = LoggerFactory.getLogger(MessageRouterImpl.class);
    private final RoutingTable routingTable;
    private final MessageSender messageSender;

    @Inject
    @Singleton
    public MessageRouterImpl(RoutingTable routingTable, MessageSender messageSender) {
        this.routingTable = routingTable;
        this.messageSender = messageSender;
    }

    private Promise<DeferredVoid> addNextHopInternal(String participantId, Address address) {
        routingTable.put(participantId, address);
        DeferredVoid deferred = new DeferredVoid();
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

    public void route(JoynrMessage message) throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                           JsonGenerationException, JsonMappingException, IOException {
        String toParticipantId = message.getTo();
        if (toParticipantId != null && routingTable.containsKey(toParticipantId)) {
            Address address = routingTable.get(toParticipantId);
            routeMessageByAddress(message, address);
        } else {
            throw new JoynrCommunicationException("Failed to send Request: No route for given participantId: "
                    + toParticipantId);
        }
    }

    private void routeMessageByAddress(JoynrMessage message, Address address) throws JoynrSendBufferFullException,
                                                                             JoynrMessageNotSentException,
                                                                             JsonGenerationException,
                                                                             JsonMappingException, IOException {

        if (address instanceof ChannelAddress) {
            logger.info("SEND messageId: {} type: {} from: {} to: {} header: {}",
                        new String[]{ message.getId(), message.getType(),
                                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                                message.getHeader().toString() });
            logger.debug("\r\n>>>>>>>>>>>>>>>>\r\n:{}", message.toLogMessage());

            String destinationChannelId = ((ChannelAddress) address).getChannelId();
            messageSender.sendMessage(destinationChannelId, message);
        } else if (address instanceof InProcessAddress) {
            /*
             * This creation should be done by a factory, avoiding that the MessageRouter is aware of the
             * different address types
             */
            new InProcessMessagingStub(((InProcessAddress) address).getSkeleton()).transmit(message);
        } else {
            throw new JoynrCommunicationException("Failed to send Request: Address type not supported");
        }
    }

    @Override
    public void addNextHop(String participantId, Address address) {
        if (address instanceof ChannelAddress) {
            addNextHop(participantId, (ChannelAddress) address);
        } else if (address instanceof BrowserAddress) {
            addNextHop(participantId, (BrowserAddress) address);
        } else if (address instanceof CommonApiDbusAddress) {
            addNextHop(participantId, (CommonApiDbusAddress) address);
        } else if (address instanceof WebSocketAddress) {
            addNextHop(participantId, (WebSocketAddress) address);
        } else if (address instanceof WebSocketClientAddress) {
            addNextHop(participantId, (WebSocketClientAddress) address);
        }
    }

    @Override
    public void shutdown() {
        messageSender.shutdown();
    }

}
