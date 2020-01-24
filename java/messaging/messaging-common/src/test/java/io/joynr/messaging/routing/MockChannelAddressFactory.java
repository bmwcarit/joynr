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

import java.util.Optional;

import io.joynr.messaging.routing.GlobalAddressFactory;
import joynr.system.RoutingTypes.ChannelAddress;

public class MockChannelAddressFactory extends GlobalAddressFactory<ChannelAddress> {

    @Override
    public ChannelAddress create() {
        return new ChannelAddress("messagingEndpointUrl", "channelId");
    }

    @Override
    public boolean supportsTransport(Optional<String> transport) {
        return "longpolling".equals(transport.isPresent() ? transport.get() : null);
    }
}
