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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.messaging.IMessagingMulticastSubscriber;
import io.joynr.messaging.IMessagingSkeleton;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

public abstract class AbstractGlobalMessagingSkeleton implements IMessagingSkeleton, IMessagingMulticastSubscriber {

    private static final Logger logger = LoggerFactory.getLogger(AbstractGlobalMessagingSkeleton.class);

    final private RoutingTable routingTable;

    @Inject
    protected AbstractGlobalMessagingSkeleton(RoutingTable routingTable) {
        this.routingTable = routingTable;
    }

    protected void registerGlobalRoutingEntry(final ImmutableMessage message, String gbid) {
        final Message.MessageType messageType = message.getType();
        if (!messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST)
                && !messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                && !messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST)
                && !messageType.equals(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)) {
            logger.trace("Message type is: {}, no global routing entry added to the routing table for it ",
                         messageType);
            return;
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
            routingTable.put(message.getSender(), address, isGloballyVisible, expiryDateMs);
        }
    }
}
