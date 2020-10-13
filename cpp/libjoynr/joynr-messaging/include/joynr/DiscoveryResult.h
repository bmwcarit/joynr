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
#ifndef DISCOVERYRESULT_H
#define DISCOVERYRESULT_H

#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/CustomParameter.h"

namespace joynr
{
class DiscoveryResult
{
public:
    explicit DiscoveryResult(
            const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& discoveryEntries);
    explicit DiscoveryResult(const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries);

    DiscoveryResult() = default;
    ~DiscoveryResult() = default;

    boost::optional<joynr::types::DiscoveryEntry> getLastSeen() const;
    boost::optional<joynr::types::DiscoveryEntry> getHighestPriority() const;
    boost::optional<joynr::types::DiscoveryEntry> getLatestVersion() const;
    boost::optional<joynr::types::DiscoveryEntry> getParticipantId(
            const std::string& participantId) const;
    std::vector<types::DiscoveryEntry> getWithKeyword(const std::string& keyword) const;

    const std::vector<joynr::types::DiscoveryEntry>& getAllDiscoveryEntries() const;

private:
    boost::optional<joynr::types::CustomParameter> findQosParameter(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            const std::string& parameterName) const;

private:
    std::vector<joynr::types::DiscoveryEntry> _discoveryEntries;
};
} // namespace joynr

#endif // DISCOVERYRESULT_H
