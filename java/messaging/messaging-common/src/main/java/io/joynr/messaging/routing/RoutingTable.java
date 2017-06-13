package io.joynr.messaging.routing;

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

import joynr.system.RoutingTypes.Address;

public interface RoutingTable {
    Address get(String participantId);

    Address put(String participantId, Address address, boolean isGloballyVisible, long expiryDateMs, boolean isSticky);

    boolean containsKey(String participantId);

    /**
     * Query the routing table for the status of isGloballyVisible parameter
     * @param participantId
     * @return true if participantId is globally visible,
     *         false if participantId is not globally visible
     * @throws JoynrRuntimeException if no entry exists for the given participantId
     */
    boolean getIsGloballyVisible(String participantId);

    /**
     * Sets the isSticky attribute of the Routing Entry for the participantId.
     * If true, the routing entry will not be get purged from routing table
     * by the cleanup thread.
     * @param participantId
     * @param isSticky
     * @throws JoynrRuntimeException if no entry exists for the given participantId
     */
    void setIsSticky(String participantId, boolean isSticky);

    void remove(String participantId);

    /**
     * Apply the specified operation to all addresses currently held in the routing table.
     *
     * @param addressOperation
     *            the address operation to perform.
     */
    void apply(AddressOperation addressOperation);

    /**
     * Purge all expired routing entries from the table
     */
    void purge();
}
