package io.joynr.messaging.websocket;

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

import com.google.inject.Inject;
import com.google.inject.name.Named;
import io.joynr.messaging.routing.MulticastAddressCalculator;
import io.joynr.runtime.SystemServicesSettings;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;

public class WebSocketMulticastAddressCalculator implements MulticastAddressCalculator {

    private WebSocketAddress globalAddress;

    @Inject
    public WebSocketMulticastAddressCalculator(@Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address globalAddress) {
        if (globalAddress instanceof WebSocketAddress) {
            this.globalAddress = (WebSocketAddress) globalAddress;
        }
    }

    @Override
    public Address calculate(ImmutableMessage message) {
        return globalAddress;
    }

    @Override
    public boolean supports(String transport) {
        return transport != null && transport.toLowerCase().contains("websocket");
    }
}
