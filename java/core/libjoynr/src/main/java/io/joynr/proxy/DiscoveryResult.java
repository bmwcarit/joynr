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
package io.joynr.proxy;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Optional;

import io.joynr.arbitration.ArbitrationConstants;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Version;

/**
 * This class represents a List of DiscoveryEntries resulting from an arbitration.
 * It provides several filtering methods that allow to retrieve DiscoveryEntries by
 * different criteria.
 */
public class DiscoveryResult {

    private List<DiscoveryEntry> discoveryEntries;

    /**
     * Constructor for internal use only.
     *
     * @param discoveryEntries DiscoveryEntries for initialization.
     */
    public DiscoveryResult(Collection<DiscoveryEntryWithMetaInfo> discoveryEntries) {
        this.discoveryEntries = new ArrayList<>();
        for (DiscoveryEntry entry : discoveryEntries) {
            this.discoveryEntries.add(new DiscoveryEntry(entry));
        }
    }

    /**
     * Returns the DiscoveryEntry with the most recent last seen date from this DiscoveryResult.
     * <p>
     * The last seen date of a provider is periodically updated by joynr at a configurable interval, property:
     * {@link io.joynr.runtime.SystemServicesSettings#PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS}.
     * <p>
     * Providers with a very outdated last seen date are most likely not reachable anymore, for example because of
     * connectivity issues or a crash of the provider application.
     *
     * @return Returns the DiscoveryEntry with the most recent last seen date from this DiscoveryResult.
     */
    public DiscoveryEntry getLastSeen() {
        DiscoveryEntry lastSeenEntry = discoveryEntries.get(0);
        for (DiscoveryEntry entry : discoveryEntries) {
            if (entry.getLastSeenDateMs() > lastSeenEntry.getLastSeenDateMs()) {
                lastSeenEntry = entry;
            }
        }
        return lastSeenEntry;
    }

    /**
     * Returns the DiscoveryEntry with the highest priority from this DiscoveryResult.
     * <p>
     * The priority of a provider can be set by an application when registering the provider, see
     * {@link joynr.types.ProviderQos#setPriority(Long)}.
     *
     * @return Returns the DiscoveryEntry with the highest priority from this DiscoveryResult.
     * @see joynr.types.ProviderQos#setPriority(Long)
     */
    public DiscoveryEntry getHighestPriority() {
        DiscoveryEntry entryWithHighestPriority = discoveryEntries.get(0);
        for (DiscoveryEntry entry : discoveryEntries) {
            if (entry.getQos().getPriority() > entryWithHighestPriority.getQos().getPriority()) {
                entryWithHighestPriority = entry;
            }
        }
        return entryWithHighestPriority;
    }

    /**
     * Returns the DiscoveryEntry with the highest Version number from this DiscoveryResult.
     *
     * @return Returns the DiscoveryEntry with the highest Version from this DiscoveryResult.
     */
    public DiscoveryEntry getLatestVersion() {
        DiscoveryEntry entryWithLatestVersion = discoveryEntries.get(0);
        for (DiscoveryEntry entry : discoveryEntries) {
            Version currentLatest = entryWithLatestVersion.getProviderVersion();
            Version inspectedVersion = entry.getProviderVersion();
            if (inspectedVersion.getMajorVersion() > currentLatest.getMajorVersion()) {
                entryWithLatestVersion = entry;
            } else if (inspectedVersion.getMajorVersion().equals(currentLatest.getMajorVersion())) {
                if (inspectedVersion.getMinorVersion() > currentLatest.getMinorVersion()) {
                    entryWithLatestVersion = entry;
                }
            }
        }
        return entryWithLatestVersion;
    }

    /**
     * Returns a Collection with all DiscoveryEntries in this DiscoveryResult.
     *
     * @return Returns a Collection with all DiscoveryEntries in this DiscoveryResult.
     */
    public Collection<DiscoveryEntry> getAllDiscoveryEntries() {
        return discoveryEntries;
    }

    /**
     * Returns the DiscoveryEntry with the given participantId, or null if it doesn't exist.
     *
     * @param participantId ParticipantId of the required DiscoveryEntry.
     * @return Returns the DiscoveryEntry with the given participantId, or null if it doesn't exist.
     */
    public DiscoveryEntry getParticipantId(String participantId) {
        for (DiscoveryEntry entry : discoveryEntries) {
            if (entry.getParticipantId().equals(participantId)) {
                return entry;
            }
        }
        return null; //Or throw exception?
    }

    /**
     * Returns a Collection of DiscoveryEntries that have the given keyword in their custom parameters.
     * <p>
     * A keyword for a provider can be set by an application when registering the provider by setting a custom parameter
     * in the {@link joynr.types.ProviderQos} with the special key {@link ArbitrationConstants#KEYWORD_PARAMETER}, see
     * {@link joynr.types.ProviderQos#setCustomParameters(CustomParameter[])}.
     *
     * @param keyword The custom parameters of the required DiscoveryEntries should contain this keyword.
     * @return Returns a Collection of DiscoveryEntries that have the given keyword in their custom parameters.
     * @see joynr.types.ProviderQos#setCustomParameters(CustomParameter[])
     */
    public Collection<DiscoveryEntry> getWithKeyword(String keyword) {
        List<DiscoveryEntry> entriesWithKeyword = new ArrayList<>();
        for (DiscoveryEntry entry : discoveryEntries) {
            Optional<CustomParameter> keywordParameter = findQosParameter(entry,
                                                                          ArbitrationConstants.KEYWORD_PARAMETER);
            if (keywordParameter.isPresent() && keywordParameter.get().getValue().equals(keyword)) {
                entriesWithKeyword.add(entry);
            }
        }
        return entriesWithKeyword;
    }

    private Optional<CustomParameter> findQosParameter(DiscoveryEntry discoveryEntry, String parameterName) {
        for (CustomParameter parameter : discoveryEntry.getQos().getCustomParameters()) {
            if (parameterName.equals(parameter.getName())) {
                return Optional.of(parameter);
            }
        }
        return Optional.empty();

    }
}
