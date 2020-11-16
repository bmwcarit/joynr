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
package io.joynr.capabilities;

import java.util.Collection;
import java.util.Optional;
import java.util.Set;

import joynr.types.DiscoveryEntry;

public interface DiscoveryEntryStore<T extends DiscoveryEntry> {

    public abstract void add(T discoveryEntry);

    public abstract void add(Collection<T> interfaces);

    public abstract boolean remove(String participantId);

    public abstract void remove(Collection<String> participantIds);

    public abstract Collection<T> lookup(String[] domain, String interfaceName, long cacheMaxAge);

    public abstract Collection<T> lookup(String[] domain, String interfaceName);

    public abstract Optional<T> lookup(String participantId, long cacheMaxAge);

    public abstract Set<T> getAllDiscoveryEntries();

    public abstract Set<T> getAllGlobalEntries();

    public abstract boolean hasDiscoveryEntry(T discoveryEntry);

    /**
     * Update last seen date and expiry date of every discovery entry saved in this store.
     * @param lastSeenDateMs - last seen date in milliseconds to set
     * @param expiryDateMs - expiry date in milliseconds to set
     * @return a list of participant IDs of the updated discovery entries with scope global
     */
    public abstract String[] touchDiscoveryEntries(long lastSeenDateMs, long expiryDateMs);

    /**
     * Update last seen date and expiry date of discovery entries with particular participant IDs
     * @param participantIds - list of participant IDs of discovery entries to be updated
     * @param lastSeenDateMs - last seen date in milliseconds to set
     * @param expiryDateMs - expiry date in milliseconds to set
     */
    public abstract void touchDiscoveryEntries(String[] participantIds, long lastSeenDateMs, long expiryDateMs);
}
