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

import java.util.Set;

import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;

public interface MulticastAddressCalculator {

    /**
     * Call this method to calculate a multicast address for the given message.
     * The message must be of type {@link joynr.Message.MessageType#VALUE_MESSAGE_TYPE_MULTICAST}.
     *
     * @param message the message for which to calculate the address.
     *
     * @return the address calculated, or null if this wasn't possible.
     */
    Set<Address> calculate(ImmutableMessage message);

    /**
     * Used to determine if the address calculator supports providing multicast addresses for a given transport.
     *
     * @param transport the transport for which to check whether this calculator can provide addresses for.
     * @return <code>true</code> if it does, <code>false</code> otherwise.
     */
    boolean supports(String transport);

    /**
     * Used to determine if the addresses created by this calculator are global transport addresses.
     *
     * @return <code>true</code> if it does, <code>false</code> otherwise.
     */
    boolean createsGlobalTransportAddresses();
}
