/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.messaging.IMessagingMulticastSubscriber;
import io.joynr.messaging.IMessagingSkeleton;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Message.MessageType;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

public abstract class AbstractGlobalMessagingSkeleton implements IMessagingSkeleton, IMessagingMulticastSubscriber {

    private final static Set<Message.MessageType> MESSAGE_TYPE_REQUESTS = new HashSet<MessageType>(Arrays.asList(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST,
                                                                                                                 Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                                                                                                                 Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                                                                                                                 Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST));

    private static final Logger logger = LoggerFactory.getLogger(AbstractGlobalMessagingSkeleton.class);

    final private RoutingTable routingTable;

    @Inject
    protected AbstractGlobalMessagingSkeleton(RoutingTable routingTable) {
        this.routingTable = routingTable;
    }

    protected boolean registerGlobalRoutingEntry(final ImmutableMessage message, String gbid) {
        final Message.MessageType messageType = message.getType();
        if (!MESSAGE_TYPE_REQUESTS.contains(messageType)) {
            logger.trace("Message type is: {}, no global routing entry added to the routing table for it ",
                         messageType);
            return false;
        }

        final String replyTo = message.getReplyTo();
        if (replyTo != null && !replyTo.isEmpty()) {
            Address address = RoutingTypesUtil.fromAddressString(replyTo);
            if (address instanceof MqttAddress) {
                MqttAddress mqttAddress = (MqttAddress) address;
                mqttAddress.setBrokerUri(gbid);
                logger.trace("Register Global Routing Entry of incoming request message {} for MqttAddress with topic: {} for gbid: {}",
                             message.getId(),
                             mqttAddress.getTopic(),
                             mqttAddress.getBrokerUri());
            }
            // As the message was received from global, the sender is globally visible by definition.
            final boolean isGloballyVisible = true;
            final long expiryDateMs = message.getTtlMs();
            return routingTable.put(message.getSender(), address, isGloballyVisible, expiryDateMs);
        }
        logger.error("Message ({}) has no replyTo. Reply might not be routable.", message.getTrackingInfo());
        return false;
    }

    protected void removeGlobalRoutingEntry(final ImmutableMessage message, boolean routingEntryRegistered) {
        if (message.isMessageProcessed()) {
            return;
        }
        message.messageProcessed();
        if (routingEntryRegistered) {
            routingTable.remove(message.getSender());
        }
    }
}
