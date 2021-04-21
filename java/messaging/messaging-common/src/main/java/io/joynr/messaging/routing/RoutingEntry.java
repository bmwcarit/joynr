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

public class RoutingEntry {
    RoutingEntry(Address address, boolean isGloballyVisible, long expiryDateMs, boolean isSticky) {
        setAddress(address);
        setIsGloballyVisible(isGloballyVisible);
        this.expiryDateMs = expiryDateMs;
        this.isSticky = isSticky;
        this.refCount = 1L;
    }

    public Address getAddress() {
        return address;
    }

    public boolean getIsGloballyVisible() {
        return isGloballyVisible;
    }

    public long getExpiryDateMs() {
        return expiryDateMs;
    }

    public boolean getIsSticky() {
        return isSticky;
    }

    public long getRefCount() {
        return refCount;
    }

    public void setAddress(Address address) {
        this.address = address;
    }

    public void setIsGloballyVisible(boolean isGloballyVisible) {
        this.isGloballyVisible = isGloballyVisible;
    }

    public void setExpiryDateMs(long expiryDateMs) {
        this.expiryDateMs = expiryDateMs;
    }

    public void setIsSticky(boolean isSticky) {
        this.isSticky = isSticky;
    }

    public void setRefCount(long refCount) {
        this.refCount = refCount;
    }

    public long incRefCount() {
        return ++refCount;
    }

    public long decRefCount() throws IllegalStateException {
        if (refCount <= 0) {
            String exceptionMessage = "Reference count of the routing entry (" + address.toString()
                    + ") is less or equal to 0. It could not be decremented.";
            throw new IllegalStateException(exceptionMessage);
        }
        return --refCount;
    }

    Address address;
    boolean isGloballyVisible;
    long expiryDateMs;
    boolean isSticky;
    long refCount;
}
