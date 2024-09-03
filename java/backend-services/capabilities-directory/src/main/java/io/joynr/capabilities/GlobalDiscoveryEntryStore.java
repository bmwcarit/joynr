/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import joynr.types.GlobalDiscoveryEntry;

public interface GlobalDiscoveryEntryStore<T extends GlobalDiscoveryEntry> {

    public abstract void add(T discoveryEntry, String[] gbids);

    public abstract int remove(String participantId, String[] gbids);

    public abstract Collection<T> lookup(String[] domain, String interfaceName);

    public abstract Optional<Collection<T>> lookup(String participantId);

    public abstract void touch(String clusterControllerId);

    public abstract void touch(String clusterControllerId, String[] participantIds);

    public int removeStale(String clusterControllerId, Long maxLastSeenDateMs);

}
