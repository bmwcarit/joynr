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

import io.joynr.messaging.inprocess.InProcessAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

public abstract class AbstractRoutingTableAddressValidator implements RoutingTableAddressValidator {

    @Override
    public boolean allowUpdate(final RoutingEntry oldEntry, final RoutingEntry newEntry) {
        if (newEntry.address instanceof InProcessAddress) {
            return true;
        } else if (newEntry.address instanceof WebSocketClientAddress
                && !(oldEntry.getAddress() instanceof InProcessAddress)) {
            return true;
        } else if ((oldEntry.getAddress() instanceof MqttAddress || oldEntry.getAddress() instanceof ChannelAddress)
                && (newEntry.address instanceof MqttAddress || newEntry.address instanceof ChannelAddress
                        || newEntry.address instanceof WebSocketAddress)) {
            return true;
        }
        return false;
    }

}
