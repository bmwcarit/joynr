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
    /**
     * @param participantId participantId for which an Address will be returned from the
     * routing table
     * @return Address the stored address for the given participantId (unmodified, see {@link #get(String, String)}
     */
    Address get(String participantId);

    /**
     * @param participantId participantId for which an Address will be returned from the
     * routing table
     * @param gbid name of the backend (evaluated for gcdParticipantId only):
     * selects the backend specific address of GCD (if participantId is the participantId of the GCD)
     * @return Address the stored address for the given participantId<br>
     * - in case of gcdParticipantId, the gbid is evaluated to adapt the stored address for the selected
     *   backend before it is returned (this allows communication with GCD instances in different backends<br>
     *   even though there is only one address per participantId<br>
     *   return null if the gbid is unknown (not part of the configured list of
     *   GBIDs {@link io.joynr.messaging.ConfigurableMessagingSettings#PROPERTY_GBIDS}<br>
     * - return unmodified address otherwise
     */
    Address get(String participantId, String gbid);

    /**
     * Sets the participantId of the Global Capabilities Directory (GCD) before adding it to the routing table
     * @param gcdParticipantId the participantId of the GCD
     */
    public void setGcdParticipantId(final String gcdParticipantId);

    /**
     * Adds a new routing entry. If a routing entry for the provided participantId already exists, only the expiryDate
     * and the sticky-flag are updated unless the update is allowed (See RoutingTableAddressValidator).
     *
     * @param participantId participant id for which a routing entry shall be created
     * @param address Address which shall be associated with the participant id
     * @param isGloballyVisible States whether the endpoint is globally visible or not
     * @param expiryDateMs Expiry date of the routing entry in milliseconds
     * @param isSticky If set to true, the routing entry never expires and cannot be replaced
     */
    void put(String participantId, Address address, boolean isGloballyVisible, long expiryDateMs, boolean isSticky);

    /**
     * Overload of put method with isSticky set to false.
     *
     * @param participantId participant id for which a routing entry shall be created
     * @param address Address which shall be associated with the participant id
     * @param isGloballyVisible States whether the endpoint is globally visible or not
     * @param expiryDateMs Expiry date of the routing entry in milliseconds
     */
    void put(String participantId, Address address, boolean isGloballyVisible, long expiryDateMs);

    boolean containsKey(String participantId);

    /**
     * Query the routing table for the status of isGloballyVisible parameter
     * @param participantId participantId for which the visibility shall be looked up
     * @return true if participantId is globally visible,
     *         false if participantId is not globally visible
     * @throws io.joynr.exceptions.JoynrRuntimeException if no entry exists for the given participantId
     */
    boolean getIsGloballyVisible(String participantId);

    /**
     * Query the expiry date of a routing entry for a participant id.
     * @param participantId participantId for which the expiryDate shall be looked up
     * @return The routing entry's expiry date in ms.
     */
    long getExpiryDateMs(String participantId);

    /**
     * Query the sticky-flag of a routing entry for a participant id.
     * @param participantId participantId for which the sticky-flag shall be looked up
     * @return The routing entry's sticky state.
     */
    boolean getIsSticky(String participantId);

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
