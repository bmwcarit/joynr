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

import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

public class AddressManager {
    private static final Logger logger = LoggerFactory.getLogger(AddressManager.class);
    public static final String multicastAddressCalculatorParticipantId = "joynr.internal.multicastAddressCalculatorParticipantId";

    private final MulticastReceiverRegistry multicastReceiversRegistry;

    private RoutingTable routingTable;
    private MulticastAddressCalculator multicastAddressCalculator;

    protected static class PrimaryGlobalTransportHolder {
        @Inject(optional = true)
        @Named(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT)
        private String primaryGlobalTransport;

        public PrimaryGlobalTransportHolder() {
        }

        // For testing only
        protected PrimaryGlobalTransportHolder(String primaryGlobalTransport) {
            this.primaryGlobalTransport = primaryGlobalTransport;
        }

        public String get() {
            return primaryGlobalTransport;
        }
    }

    @Inject
    public AddressManager(RoutingTable routingTable,
                          PrimaryGlobalTransportHolder primaryGlobalTransport,
                          Set<MulticastAddressCalculator> multicastAddressCalculators,
                          MulticastReceiverRegistry multicastReceiverRegistry) {
        logger.trace("Initialised with routingTable: {}, primaryGlobalTransport: {}, multicastAddressCalculators: {}, multicastReceiverRegistry: {}",
                     routingTable,
                     primaryGlobalTransport.get(),
                     multicastAddressCalculators,
                     multicastReceiverRegistry);
        this.routingTable = routingTable;
        this.multicastReceiversRegistry = multicastReceiverRegistry;
        if (multicastAddressCalculators.size() > 1 && primaryGlobalTransport.get() == null) {
            throw new JoynrIllegalStateException("Multiple multicast address calculators registered, but no primary global transport set.");
        }
        if (multicastAddressCalculators.size() == 1) {
            this.multicastAddressCalculator = multicastAddressCalculators.iterator().next();
        } else {
            for (MulticastAddressCalculator multicastAddressCalculator : multicastAddressCalculators) {
                if (multicastAddressCalculator.supports(primaryGlobalTransport.get())) {
                    this.multicastAddressCalculator = multicastAddressCalculator;
                    break;
                }
            }
        }
    }

    /**
     * Get the participantIds to which the passed in message should be sent to.
     * @param message the message for which we want to find the participantIds to send it to.
     * @return set of participantIds to send the message to. Will not be null, because if an participantId
     * can't be determined, the returned set of addresses will be empty.
     */
    public Set<String> getParticipantIdsForImmutableMessage(ImmutableMessage message) {
        Set<String> result = new HashSet<>();
        String toParticipantId = message.getRecipient();
        if (Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(message.getType())) {
            getRecipientsForMulticast(message, result);
        } else if (toParticipantId != null) {
            result.add(toParticipantId);
        }
        logger.trace("Found the following recipients for {}: {}", message, result);

        return result;
    }

    /**
     * Get the address to which the passed in message should be sent to.
     * This can be an address contained in the {@link RoutingTable}, or a
     * multicast address calculated from the header content of the message.
     *
     * @param message the message for which we want to find an address to send it to.
     * @return Optional of an address to send the message to. Will not be null, because if an address
     * can't be determined, the returned Optional will be empty.
     */
    public Optional<Address> getAddressForDelayableImmutableMessage(DelayableImmutableMessage message) {
        Map<String, String> customHeader = message.getMessage().getCustomHeaders();
        final String gbidVal = customHeader.get(Message.CUSTOM_HEADER_GBID_KEY);
        Address address = null;
        String recipient = message.getRecipient();
        if (recipient.startsWith(multicastAddressCalculatorParticipantId)) {
            address = determineAddressFromMulticastAddressCalculator(message, recipient);
        } else if (gbidVal == null) {
            address = routingTable.get(recipient);
        } else {
            address = routingTable.get(recipient, gbidVal);
        }
        return address == null ? Optional.empty() : Optional.of(address);
    }

    private Address determineAddressFromMulticastAddressCalculator(DelayableImmutableMessage message,
                                                                   String recipient) {
        Address address = null;
        Set<Address> addressSet = multicastAddressCalculator.calculate(message.getMessage());
        if (addressSet.size() <= 1) {
            for (Address calculatedAddress : addressSet) {
                address = calculatedAddress;
            }
        } else {
            // This case can only happen if we have multiple backends, which can only happen in case of MQTT
            for (Address calculatedAddress : addressSet) {
                MqttAddress mqttAddress = (MqttAddress) calculatedAddress;
                String brokerUri = mqttAddress.getBrokerUri();
                if (recipient.equals(multicastAddressCalculatorParticipantId + "_" + brokerUri)) {
                    address = calculatedAddress;
                    break;
                }
            }
        }
        return address;
    }

    private void getRecipientsForMulticast(ImmutableMessage message, Set<String> result) {
        if (!message.isReceivedFromGlobal() && multicastAddressCalculator != null) {
            if (multicastAddressCalculator.createsGlobalTransportAddresses()) {
                // only global providers should multicast to the "outside world"
                if (isProviderGloballyVisible(message.getSender())) {
                    addReceiversFromAddressCalculator(message, result);
                }
            } else {
                // in case the address calculator does not provide an address
                // to the "outside world" it is safe to forward the message
                // regardless of the provider being globally visible or not
                addReceiversFromAddressCalculator(message, result);
            }
        }
        addLocalMulticastReceiversFromRegistry(message, result);
    }

    private boolean isProviderGloballyVisible(String participantId) {
        boolean isGloballyVisible = false;

        try {
            isGloballyVisible = routingTable.getIsGloballyVisible(participantId);
        } catch (JoynrRuntimeException e) {
            // This should never happen
            logger.error("No routing entry found for Multicast Provider {}. The message will not be published globally.",
                         participantId);
        }
        return isGloballyVisible;
    }

    private void addReceiversFromAddressCalculator(ImmutableMessage message, Set<String> result) {
        Set<Address> calculatedAddresses = multicastAddressCalculator.calculate(message);
        if (calculatedAddresses.size() == 1) {
            result.add(multicastAddressCalculatorParticipantId);
        } else {
            // This case can only happen if we have multiple backends, which can only happen in case of MQTT
            for (Address address : calculatedAddresses) {
                MqttAddress mqttAddress = (MqttAddress) address;
                result.add(multicastAddressCalculatorParticipantId + "_" + mqttAddress.getBrokerUri());
            }
        }
    }

    private void addLocalMulticastReceiversFromRegistry(ImmutableMessage message, Set<String> result) {
        Set<String> receivers = multicastReceiversRegistry.getReceivers(message.getRecipient());
        result.addAll(receivers);
    }
}
