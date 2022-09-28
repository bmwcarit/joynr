/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

public class DummyRoutingTable implements RoutingTable {
    @Override
    public Address get(String participantId) {
        return null;
    }

    @Override
    public Address get(String participantId, String gbid) {
        return null;
    }

    @Override
    public void setGcdParticipantId(final String gcdParticipantId) {
    }

    @Override
    public boolean put(String participantId,
                       Address address,
                       boolean isGloballyVisible,
                       long expiryDateMs,
                       boolean isSticky) {
        return true;
    }

    @Override
    public boolean put(String participantId, Address address, boolean isGloballyVisible, long expiryDateMs) {
        return true;
    }

    @Override
    public boolean containsKey(String participantId) {
        return true;
    }

    @Override
    public boolean getIsGloballyVisible(String participantId) {
        return true;
    }

    @Override
    public long getExpiryDateMs(String participantId) {
        return Long.MAX_VALUE;
    }

    @Override
    public boolean getIsSticky(String participantId) {
        return true;
    }

    @Override
    public void remove(String participantId) {
    }

    @Override
    public void apply(AddressOperation addressOperation) {
    }

    @Override
    public void purge() {
    }

    @Override
    public void incrementReferenceCount(String participantId) {
    }

}
