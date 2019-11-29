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

import javax.annotation.CheckForNull;

import io.joynr.arbitration.ArbitrationConstants;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Version;

public class DiscoveryResult {

    private List<DiscoveryEntry> discoveryEntries;

    public DiscoveryResult(Collection<DiscoveryEntryWithMetaInfo> discoveryEntries) {
        this.discoveryEntries = new ArrayList<>();
        for (DiscoveryEntry entry : discoveryEntries) {
            this.discoveryEntries.add(new DiscoveryEntry(entry));
        }
    }

    public DiscoveryEntry getLastSeen() {
        DiscoveryEntry lastSeenEntry = discoveryEntries.get(0);
        for (DiscoveryEntry entry : discoveryEntries) {
            if (entry.getLastSeenDateMs() > lastSeenEntry.getLastSeenDateMs()) {
                lastSeenEntry = entry;
            }
        }
        return lastSeenEntry;
    }

    public DiscoveryEntry getHighestPriority() {
        DiscoveryEntry entryWithHighestPriority = discoveryEntries.get(0);
        for (DiscoveryEntry entry : discoveryEntries) {
            if (entry.getQos().getPriority() > entryWithHighestPriority.getQos().getPriority()) {
                entryWithHighestPriority = entry;
            }
        }
        return entryWithHighestPriority;
    }

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

    public Collection<DiscoveryEntry> getAllDiscoveryEntries() {
        return discoveryEntries;
    }

    public DiscoveryEntry getParticipantId(String participantId) {
        for (DiscoveryEntry entry : discoveryEntries) {
            if (entry.getParticipantId().equals(participantId)) {
                return entry;
            }
        }
        return null; //Or throw exception?
    }

    public Collection<DiscoveryEntry> getWithKeyword(String keyword) {
        List<DiscoveryEntry> entriesWithKeyword = new ArrayList<>();
        for (DiscoveryEntry entry : discoveryEntries) {
            CustomParameter keywordParameter = findQosParameter(entry, ArbitrationConstants.KEYWORD_PARAMETER);
            if (keywordParameter != null && keywordParameter.getValue().equals(keyword)) {
                entriesWithKeyword.add(entry);
            }
        }
        return entriesWithKeyword;
    }

    @CheckForNull
    private CustomParameter findQosParameter(DiscoveryEntry discoveryEntry, String parameterName) {
        for (CustomParameter parameter : discoveryEntry.getQos().getCustomParameters()) {
            if (parameterName.equals(parameter.getName())) {
                return parameter;
            }
        }
        return null;

    }
}
