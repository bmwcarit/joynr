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

import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.inprocess.InProcessAddress;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.UdsAddress;
import joynr.system.RoutingTypes.UdsClientAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

public class CcRoutingTableAddressValidator implements RoutingTableAddressValidator {

    private static final Logger logger = LoggerFactory.getLogger(CcRoutingTableAddressValidator.class);

    private Set<Address> ownAddresses;

    @Inject(optional = true)
    @Named(MessagingPropertyKeys.GLOBAL_ADDRESS)
    private Address globalAddress = new Address();
    @Inject(optional = true)
    @Named(MessagingPropertyKeys.REPLY_TO_ADDRESS)
    private Address replyToAddress = new Address();

    public CcRoutingTableAddressValidator() {
        ownAddresses = new HashSet<>();
    }

    @Inject
    public void init() {
        ownAddresses.add(globalAddress);
        ownAddresses.add(replyToAddress);
    }

    @Override
    public boolean isValidForRoutingTable(final Address address) {
        if (address instanceof WebSocketAddress || address instanceof UdsAddress) {
            final String addressType = address instanceof WebSocketAddress ? "WebSocketAddress" : "UdsAddress";
            logger.error("{} will not be used for CC Routing Table: {}",
                         addressType.getClass().getSimpleName(),
                         address);
            return false;
        }
        try {
            if (ownAddresses.stream().anyMatch(ownAddress -> ownAddress.equals(address))) {
                logger.trace("Address will not be used for Routing Table since it refers to ourselves: {}", address);
                return false;
            }
        } catch (Exception e) {
            logger.error("Exception in isValidForRoutingTable", e);
            return false;
        }
        return true;
    }

    @Override
    public boolean allowUpdate(final RoutingEntry oldEntry, final RoutingEntry newEntry) {
        // precedence: InProcessAddress > WebSocketClientAddress/UdsClientAddress > MqttAddress > WebSocketAddress/UdsAddress
        if (newEntry.address instanceof InProcessAddress) {
            return true;
        }
        if (!(oldEntry.getAddress() instanceof InProcessAddress)) {
            if (newEntry.address instanceof WebSocketClientAddress || newEntry.address instanceof UdsClientAddress
                    || newEntry.address instanceof BinderAddress) {
                return true;
            } else if (!(oldEntry.getAddress() instanceof WebSocketClientAddress)
                    && !(oldEntry.getAddress() instanceof UdsClientAddress)
                    && !(oldEntry.getAddress() instanceof BinderAddress)) {
                // old address is MqttAddress or WebSocketAddress or UdsAddress
                if (newEntry.address instanceof MqttAddress) {
                    return true;
                } else if (oldEntry.getAddress() instanceof WebSocketAddress
                        || oldEntry.getAddress() instanceof UdsAddress) {
                    // old address is WebSocketAddress or UdsAddress
                    if (newEntry.getAddress() instanceof WebSocketAddress
                            || newEntry.getAddress() instanceof UdsAddress) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

}
