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

import joynr.system.RoutingTypes.Address;

public interface RoutingTable {
    Address get(String participantId);

    /**
     * Adds a new routing entry. If a routing entry for the provided participantId already exists, only the expiryDate and the sticky-flag
     * are updated unless allowUpdate is set to true.
     *
     * @param participantId participant id for which a routing entry shall be created
     * @param address Address which shall be associated with the participant id
     * @param isGloballyVisible States whether the endpoint is globally visible or not
     * @param expiryDateMs Expiry date of the routing entry in milliseconds
     * @param isSticky If set to true, the routing entry never expires
     * @param allowUpdate If set to false, the address won't be changed if a routing entry for the provided participantId already exists.
     */
    void put(String participantId,
             Address address,
             boolean isGloballyVisible,
             long expiryDateMs,
             boolean isSticky,
             boolean allowUpdate);

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
     * Query the expiry date of a routing entry for a participant id.
     * @param participantId
     * @return The routing entry's expiry date in ms.
     */
    long getExpiryDateMs(String participantId);

    /**
     * Query the sticky-flag of a routing entry for a participant id.
     * @param participantId
     * @return The routing entry's sticky state.
     */
    boolean getIsSticky(String participantId);

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
