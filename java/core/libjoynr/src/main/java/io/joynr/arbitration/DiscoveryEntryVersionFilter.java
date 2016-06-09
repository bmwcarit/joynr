package io.joynr.arbitration;

import java.util.Iterator;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.Set;

import com.google.inject.Inject;

import joynr.types.DiscoveryEntry;
import joynr.types.Version;

/**
 * Given the version of an interface for which a proxy is being created (the caller) and a set of {@link DiscoveryEntry
 * discovery entries}, this class' {@link #filter(Version, Set<DiscoveryEntry>)} method can be used to filter out all
 * discovery entries which are not compatible with the caller version.
 * Uses the {@link VersionCompatibilityChecker} to determine whether the versions
 * are compatible or not.
 */
public class DiscoveryEntryVersionFilter {

    private VersionCompatibilityChecker versionCompatibilityChecker;

    @Inject
    public DiscoveryEntryVersionFilter(VersionCompatibilityChecker versionCompatibilityChecker) {
        this.versionCompatibilityChecker = versionCompatibilityChecker;
    }

    /**
     * Reduces the passed in set of {@link DiscoveryEntry discovery entries} by
     * removing all of those entries which are incompatible with the specified
     * caller {@link Version}.
     *
     * @param callerVersion the version of the caller. Must not be <code>null</code>.
     * @param discoveryEntries the discovery entries which are to be filtered by versions.
     * Must not be <code>null</code>.
     *
     * @return the filtered discovery entry set.
     */
    public Set<DiscoveryEntry> filter(Version callerVersion, Set<DiscoveryEntry> discoveryEntries) {
        if (callerVersion == null || discoveryEntries == null) {
            throw new IllegalArgumentException(String.format("Neither callerVersion (%s) nor discoveryEntries (%s) can be null.",
                                                             callerVersion,
                                                             discoveryEntries));
        }
        Iterator<DiscoveryEntry> iterator = discoveryEntries.iterator();
        while (iterator.hasNext()) {
            DiscoveryEntry discoveryEntry = iterator.next();
            if (!versionCompatibilityChecker.check(callerVersion, discoveryEntry.getProviderVersion())) {
                iterator.remove();
            }
        }
        return discoveryEntries;
    }

}
