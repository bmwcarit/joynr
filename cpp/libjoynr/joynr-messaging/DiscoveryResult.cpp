/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include "joynr/DiscoveryResult.h"

#include "joynr/DiscoveryQos.h"
#include "joynr/types/Version.h"

namespace joynr
{

DiscoveryResult::DiscoveryResult(const std::vector<types::DiscoveryEntry>& discoveryEntries)
        : _discoveryEntries(discoveryEntries)
{
}

const std::vector<joynr::types::DiscoveryEntry>& DiscoveryResult::getAllDiscoveryEntries() const
{
    return _discoveryEntries;
}

boost::optional<types::DiscoveryEntry> DiscoveryResult::getLastSeen() const
{
    boost::optional<joynr::types::DiscoveryEntry> entryWithLastSeenDate;
    if (_discoveryEntries.empty()) {
        return entryWithLastSeenDate;
    }

    entryWithLastSeenDate = _discoveryEntries.front();
    for (const auto& entry : _discoveryEntries) {
        if (entry.getLastSeenDateMs() > entryWithLastSeenDate.value().getLastSeenDateMs()) {
            entryWithLastSeenDate = entry;
        }
    }
    return entryWithLastSeenDate;
}

boost::optional<types::DiscoveryEntry> DiscoveryResult::getWithHighestPriority() const
{
    boost::optional<joynr::types::DiscoveryEntry> entryWithHighestPriority;
    if (_discoveryEntries.empty()) {
        return entryWithHighestPriority;
    }

    entryWithHighestPriority = _discoveryEntries.front();
    for (const auto& entry : _discoveryEntries) {
        if (entry.getQos().getPriority() >
            entryWithHighestPriority.value().getQos().getPriority()) {
            entryWithHighestPriority = entry;
        }
    }
    return entryWithHighestPriority;
}

boost::optional<types::DiscoveryEntry> DiscoveryResult::getWithLatestVersion() const
{
    boost::optional<joynr::types::DiscoveryEntry> entryWithLatestVersion;
    if (_discoveryEntries.empty()) {
        return entryWithLatestVersion;
    }

    entryWithLatestVersion = _discoveryEntries.front();
    for (const auto& entry : _discoveryEntries) {
        joynr::types::Version currentVersion = entryWithLatestVersion.value().getProviderVersion();
        joynr::types::Version inspectedVersion = entry.getProviderVersion();

        if (inspectedVersion.getMajorVersion() > currentVersion.getMajorVersion()) {
            entryWithLatestVersion = entry;
        } else if (inspectedVersion.getMajorVersion() == currentVersion.getMajorVersion()) {
            if (inspectedVersion.getMinorVersion() > currentVersion.getMinorVersion()) {
                entryWithLatestVersion = entry;
            }
        }
    }
    return entryWithLatestVersion;
}

boost::optional<types::DiscoveryEntry> DiscoveryResult::getWithParticipantId(
        const std::string& participantId) const
{
    boost::optional<joynr::types::DiscoveryEntry> entryWithParticipantId;
    if (_discoveryEntries.empty()) {
        return entryWithParticipantId;
    }

    for (const auto& entry : _discoveryEntries) {
        if (entry.getParticipantId() == participantId) {
            entryWithParticipantId = entry;
            break;
        }
    }
    return entryWithParticipantId;
}

std::vector<joynr::types::DiscoveryEntry> DiscoveryResult::getWithKeyword(
        const std::string& keyword) const
{
    std::vector<joynr::types::DiscoveryEntry> entriesWithKeyword;
    for (const auto& entry : _discoveryEntries) {
        boost::optional<joynr::types::CustomParameter> keywordParameter =
                findQosParameter(entry, DiscoveryQos::KEYWORD_PARAMETER());
        if (keywordParameter && keywordParameter.get().getValue() == keyword) {
            entriesWithKeyword.push_back(entry);
        }
    }
    return entriesWithKeyword;
}

boost::optional<joynr::types::CustomParameter> DiscoveryResult::findQosParameter(
        const types::DiscoveryEntry& discoveryEntry,
        const std::string& parameterName) const
{
    boost::optional<joynr::types::CustomParameter> customParameter;
    for (const auto& parameter : discoveryEntry.getQos().getCustomParameters()) {
        if (parameterName == parameter.getName()) {
            customParameter = parameter;
            break;
        }
    }
    return customParameter;
}

} // namespace joynr
