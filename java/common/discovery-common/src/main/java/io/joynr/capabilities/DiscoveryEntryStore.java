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

    public abstract boolean hasDiscoveryEntry(T discoveryEntry);

    public abstract void touch(String clusterControllerId);
}
