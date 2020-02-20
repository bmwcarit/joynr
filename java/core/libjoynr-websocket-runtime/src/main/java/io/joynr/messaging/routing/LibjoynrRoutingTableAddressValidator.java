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

import io.joynr.messaging.inprocess.InProcessAddress;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.UdsAddress;
import joynr.system.RoutingTypes.UdsClientAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

public class LibjoynrRoutingTableAddressValidator implements RoutingTableAddressValidator {

    private static final Logger logger = LoggerFactory.getLogger(LibjoynrRoutingTableAddressValidator.class);

    @Override
    public boolean isValidForRoutingTable(final Address address) {
        if (address instanceof WebSocketAddress || address instanceof InProcessAddress) {
            return true;
        }
        logger.error("An address which is neither of type WebSocketAddress nor InProcessAddress"
                + " will not be used for libjoynr Routing Table: {}", address);
        return false;
    }

    @Override
    public boolean allowUpdate(final RoutingEntry oldEntry, final RoutingEntry newEntry) {
        // precedence: InProcessAddress > WebSocketAddress/UdsAddress > WebSocketClientAddress/UdsClientAddress > MqttAddress/ChannelAddress
        if (newEntry.address instanceof InProcessAddress) {
            return true;
        }
        if (!(oldEntry.getAddress() instanceof InProcessAddress)) {
            if (newEntry.address instanceof WebSocketAddress || newEntry.address instanceof UdsAddress) {
                return true;
            } else if (!(oldEntry.getAddress() instanceof WebSocketAddress)
                    && !(oldEntry.getAddress() instanceof UdsAddress)) {
                // old address is WebSocketClientAddress or UdsClientAddress or MqttAddress/ChannelAddress
                if (newEntry.getAddress() instanceof WebSocketClientAddress
                        || newEntry.getAddress() instanceof UdsClientAddress) {
                    return true;
                } else if (!(oldEntry.getAddress() instanceof WebSocketClientAddress)
                        && !(oldEntry.getAddress() instanceof UdsClientAddress)) {
                    // old address is MqttAddress or ChannelAddress
                    if (newEntry.address instanceof MqttAddress || newEntry.address instanceof ChannelAddress) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

}
