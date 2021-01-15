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

#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <ostream>
#include <memory>
#include <vector>
#include <string>
#include <boost/optional.hpp>

#include "joynr/LCDUtil.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/GlobalDiscoveryEntry.h"
#include "joynr/types/ProviderScope.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

ValidateGBIDsEnum::Enum LCDUtil::validateGbids(std::vector<std::string> gbids,
                                               std::unordered_set<std::string> validGbids)
{
    std::unordered_set<std::string> gbidSet;
    for (auto gbid : gbids) {
        if (gbid.empty() || (gbidSet.find(gbid) != gbidSet.cend())) {
            JOYNR_LOG_ERROR(
                    logger(), "INVALID_GBID: provided GBID is empty or duplicate: >{}<.", gbid);
            return ValidateGBIDsEnum::INVALID;
        }
        gbidSet.insert(gbid);
        if (validGbids.find(gbid) == validGbids.cend()) {
            JOYNR_LOG_ERROR(logger(), "UNKNOWN_GBID: provided GBID is unknown: >{}<.", gbid);
            return ValidateGBIDsEnum::UNKNOWN;
        }
    }
    return ValidateGBIDsEnum::OK;
}

std::vector<types::DiscoveryEntry> LCDUtil::filterDiscoveryEntriesByGbids(
        const std::unique_lock<std::recursive_mutex>& cacheLock,
        const std::vector<types::DiscoveryEntry>& entries,
        const std::unordered_set<std::string>& gbids,
        std::unordered_map<std::string, std::vector<std::string>> globalParticipantIdsToGbidsMap)
{
    assert(cacheLock.owns_lock());
    std::vector<types::DiscoveryEntry> result;

    for (const auto& entry : entries) {
        if (isEntryForGbid(cacheLock, entry, gbids, globalParticipantIdsToGbidsMap)) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<types::DiscoveryEntryWithMetaInfo> LCDUtil::filterDuplicates(
        std::vector<types::DiscoveryEntryWithMetaInfo>&& localCapabilitiesWithMetaInfo,
        std::vector<types::DiscoveryEntryWithMetaInfo>&& globalCapabilitiesWithMetaInfo)
{
    // use custom DiscoveryEntryHash and custom DiscoveryEntryKeyEq to compare only the
    // participantId and to ignore the isLocal flag of DiscoveryEntryWithMetaInfo.
    // prefer local entries if there are local and global entries for the same provider.
    std::unordered_set<types::DiscoveryEntryWithMetaInfo,
                       joynr::DiscoveryEntryHash,
                       joynr::DiscoveryEntryKeyEq>
            resultSet(std::make_move_iterator(localCapabilitiesWithMetaInfo.begin()),
                      std::make_move_iterator(localCapabilitiesWithMetaInfo.end()));
    resultSet.insert(std::make_move_iterator(globalCapabilitiesWithMetaInfo.begin()),
                     std::make_move_iterator(globalCapabilitiesWithMetaInfo.end()));
    std::vector<types::DiscoveryEntryWithMetaInfo> resultVec(resultSet.begin(), resultSet.end());
    return resultVec;
}

bool LCDUtil::containsOnlyEmptyString(const std::vector<std::string> gbids)
{
    return gbids.size() == 1 && gbids[0] == "";
}

void LCDUtil::replaceGbidWithEmptyString(
        std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)
{
    JOYNR_LOG_TRACE(logger(), "replacing GBID of GDEs with empty string");
    for (auto& cap : capabilities) {
        const auto& serializedAddress = cap.getAddress();
        std::shared_ptr<system::RoutingTypes::Address> address;
        try {
            joynr::serializer::deserializeFromJson(address, serializedAddress);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(
                    logger(),
                    "could not deserialize Address for GBID replacement from {} - error: {}",
                    serializedAddress,
                    e.what());
            continue;
        }
        if (auto mqttAddress = dynamic_cast<system::RoutingTypes::MqttAddress*>(address.get())) {
            mqttAddress->setBrokerUri("");
            cap.setAddress(joynr::serializer::serializeToJson(*mqttAddress));
        }
        // other address types do not contain a GBID, default GBID will be used then for
        // globalParticipantIdsToGbidsMap
    }
}

std::vector<types::DiscoveryEntry> LCDUtil::optionalToVector(
        boost::optional<types::DiscoveryEntry> optionalEntry)
{
    std::vector<types::DiscoveryEntry> vec;
    if (optionalEntry) {
        vec.push_back(std::move(*optionalEntry));
    }
    return vec;
}

bool LCDUtil::isGlobal(const types::DiscoveryEntry& discoveryEntry)
{
    return discoveryEntry.getQos().getScope() == types::ProviderScope::GLOBAL;
}

std::string LCDUtil::joinToString(const std::vector<types::DiscoveryEntry>& discoveryEntries)
{
    std::ostringstream outputStream;

    std::transform(
            discoveryEntries.cbegin(),
            discoveryEntries.cend(),
            std::ostream_iterator<std::string>(outputStream, ", "),
            [](const types::DiscoveryEntry& discoveryEntry) { return discoveryEntry.toString(); });

    return outputStream.str();
}

bool LCDUtil::isEntryForGbid(
        const std::unique_lock<std::recursive_mutex>& cacheLock,
        const types::DiscoveryEntry& entry,
        const std::unordered_set<std::string> gbids,
        std::unordered_map<std::string, std::vector<std::string>> globalParticipantIdsToGbidsMap)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;

    const auto foundMapping = globalParticipantIdsToGbidsMap.find(entry.getParticipantId());
    if (foundMapping != globalParticipantIdsToGbidsMap.cend() && !foundMapping->second.empty()) {
        for (const auto& entryGbid : foundMapping->second) {
            if (gbids.find(entryGbid) != gbids.cend()) {
                return true;
            }
        }
    } else {
        JOYNR_LOG_WARN(logger(), "No GBIDs found for DiscoveryEntry {}", entry.getParticipantId());
    }
    return false;
}

types::GlobalDiscoveryEntry LCDUtil::toGlobalDiscoveryEntry(
        const types::DiscoveryEntry& discoveryEntry,
        const std::string& localAddress)
{
    return types::GlobalDiscoveryEntry(discoveryEntry.getProviderVersion(),
                                       discoveryEntry.getDomain(),
                                       discoveryEntry.getInterfaceName(),
                                       discoveryEntry.getParticipantId(),
                                       discoveryEntry.getQos(),
                                       discoveryEntry.getLastSeenDateMs(),
                                       discoveryEntry.getExpiryDateMs(),
                                       discoveryEntry.getPublicKeyId(),
                                       localAddress);
}

std::vector<InterfaceAddress> LCDUtil::getInterfaceAddresses(
        const std::vector<std::string>& domains,
        const std::string& interfaceName)
{
    std::vector<InterfaceAddress> interfaceAddresses;
    interfaceAddresses.reserve(domains.size());
    for (const auto& domain : domains) {
        interfaceAddresses.push_back(InterfaceAddress(domain, interfaceName));
    }
    return interfaceAddresses;
}

} // namespace joynr
