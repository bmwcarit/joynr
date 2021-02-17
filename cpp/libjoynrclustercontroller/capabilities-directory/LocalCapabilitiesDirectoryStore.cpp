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

#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "joynr/LocalCapabilitiesDirectoryStore.h"
#include "joynr/CapabilitiesStorage.h"
#include "joynr/CapabilityUtils.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/LCDUtil.h"
#include "joynr/Util.h"

namespace joynr
{

LocalCapabilitiesDirectoryStore::LocalCapabilitiesDirectoryStore()
        : _locallyRegisteredCapabilities(std::make_shared<capabilities::Storage>()),
          _globalLookupCache(std::make_shared<capabilities::CachingStorage>()),
          _cacheLock()
{
}

LocalCapabilitiesDirectoryStore::~LocalCapabilitiesDirectoryStore()
{
    clear();
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::
        getCachedGlobalDiscoveryEntries() const
{
    std::lock_guard<std::recursive_mutex> globalCachedRetrievalLock(_cacheLock);

    return std::vector<types::DiscoveryEntry>(
            _globalLookupCache->cbegin(), _globalLookupCache->cend());
}

std::size_t LocalCapabilitiesDirectoryStore::countGlobalCapabilities() const
{
    std::size_t counter = 0;
    std::lock_guard<std::recursive_mutex> lock4(_cacheLock);
    for (const auto& capability : *_locallyRegisteredCapabilities) {
        if (capability.getQos().getScope() == types::ProviderScope::GLOBAL) {
            counter++;
        }
    }
    return counter;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::getAllGlobalCapabilities() const
{
    std::vector<types::DiscoveryEntry> allGlobalEntries;
    std::lock_guard<std::recursive_mutex> storeLock(_cacheLock);
    for (const auto& capability : *_locallyRegisteredCapabilities) {
        if (LCDUtil::isGlobal(capability)) {
            allGlobalEntries.push_back(capability);
        }
    }
    return allGlobalEntries;
}

bool LocalCapabilitiesDirectoryStore::getLocalAndCachedCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    auto localCapabilities = searchLocalCache(interfaceAddresses);
    auto cachedCapabilities =
            scope == types::DiscoveryScope::LOCAL_ONLY
                    ? std::vector<types::DiscoveryEntry>{}
                    : searchGlobalCache(interfaceAddresses,
                                        gbids,
                                        std::chrono::milliseconds(discoveryQos.getCacheMaxAge()));

    return callReceiverIfPossible(scope,
                                  std::move(localCapabilities),
                                  std::move(cachedCapabilities),
                                  std::move(callback));
}

bool LocalCapabilitiesDirectoryStore::getLocalAndCachedCapabilities(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    boost::optional<types::DiscoveryEntry> localOrCachedCapability = searchCaches(
            participantId, scope, gbids, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()));
    auto localCapabilities = LCDUtil::optionalToVector(std::move(localOrCachedCapability));
    std::vector<types::DiscoveryEntry> globalCapabilities(localCapabilities);

    return callReceiverIfPossible(scope,
                                  std::move(localCapabilities),
                                  std::move(globalCapabilities),
                                  std::move(callback));
}

bool LocalCapabilitiesDirectoryStore::callReceiverIfPossible(
        joynr::types::DiscoveryScope::Enum& scope,
        std::vector<types::DiscoveryEntry>&& localCapabilities,
        std::vector<types::DiscoveryEntry>&& globalCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // return only local capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_ONLY) {
        auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
        callback->capabilitiesReceived(std::move(localCapabilitiesWithMetaInfo));
        return true;
    }

    // return local then global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
        auto globalCapabilitiesWithMetaInfo = util::convert(false, globalCapabilities);
        if (!localCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(localCapabilitiesWithMetaInfo));
            return true;
        }
        if (!globalCapabilities.empty()) {
            callback->capabilitiesReceived(std::move(globalCapabilitiesWithMetaInfo));
            return true;
        }
    }

    // return local and global capabilities
    if (scope == joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL) {
        if (!globalCapabilities.empty()) {
            auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
            auto globalCapabilitiesWithMetaInfo = util::convert(false, globalCapabilities);

            // remove duplicates
            auto resultVec = LCDUtil::filterDuplicates(std::move(localCapabilitiesWithMetaInfo),
                                                       std::move(globalCapabilitiesWithMetaInfo));
            callback->capabilitiesReceived(std::move(resultVec));
            return true;
        }
    }

    // return the global cached entries
    if (scope == joynr::types::DiscoveryScope::GLOBAL_ONLY) {
        auto resultWithDuplicates = util::convert(false, globalCapabilities);
        auto localCapabilitiesWithMetaInfo = util::convert(true, localCapabilities);
        for (const auto& entry : localCapabilitiesWithMetaInfo) {
            if (entry.getQos().getScope() == joynr::types::ProviderScope::GLOBAL) {
                resultWithDuplicates.push_back(entry);
            }
        }
        if (!resultWithDuplicates.empty()) {
            // remove duplicates
            std::unordered_set<types::DiscoveryEntryWithMetaInfo,
                               joynr::DiscoveryEntryHash,
                               joynr::DiscoveryEntryKeyEq>
                    resultSet(std::make_move_iterator(resultWithDuplicates.begin()),
                              std::make_move_iterator(resultWithDuplicates.end()));
            std::vector<types::DiscoveryEntryWithMetaInfo> result(
                    resultSet.begin(), resultSet.end());
            callback->capabilitiesReceived(std::move(result));
            return true;
        }
    }
    return false;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::getLocalCapabilities(
        const std::string& participantId)
{
    std::lock_guard<std::recursive_mutex> localCachedRetrievalLock(_cacheLock);
    return LCDUtil::optionalToVector(
            _locallyRegisteredCapabilities->lookupByParticipantId(participantId));
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::getLocalCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    return searchLocalCache(interfaceAddresses);
}

void LocalCapabilitiesDirectoryStore::clear()
{
    std::lock_guard<std::recursive_mutex> clearingLock(_cacheLock);
    _locallyRegisteredCapabilities->clear();
    _globalLookupCache->clear();
    _globalParticipantIdsToGbidsMap.clear();
}

void LocalCapabilitiesDirectoryStore::insertInLocalCapabilitiesStorage(
        const types::DiscoveryEntry& entry)
{
    std::lock_guard<std::recursive_mutex> localInsertionLock(_cacheLock);

    auto found = _globalParticipantIdsToGbidsMap.find(entry.getParticipantId());
    _locallyRegisteredCapabilities->insert(entry,
                                           found != _globalParticipantIdsToGbidsMap.cend()
                                                   ? found->second
                                                   : std::vector<std::string>{});
    JOYNR_LOG_INFO(logger(),
                   "Added local capability to cache {}, #localCapabilities: {}",
                   entry.toString(),
                   _locallyRegisteredCapabilities->size());
}

void LocalCapabilitiesDirectoryStore::insertInGlobalLookupCache(
        const types::DiscoveryEntry& entry,
        const std::vector<std::string>& gbids)
{
    std::lock_guard<std::recursive_mutex> globalInsertionLock(_cacheLock);

    _globalLookupCache->insert(entry);

    const std::string& participantId = entry.getParticipantId();
    std::vector<std::string> allGbids(gbids);
    const auto foundMapping = _globalParticipantIdsToGbidsMap.find(participantId);
    if (foundMapping != _globalParticipantIdsToGbidsMap.cend()) {
        // entry already exists
        const auto& oldGbids = foundMapping->second;
        for (const auto& gbid : oldGbids) {
            if (std::find(allGbids.cbegin(), allGbids.cend(), gbid) == allGbids.cend()) {
                allGbids.emplace_back(gbid);
            }
        }
    }
    _globalParticipantIdsToGbidsMap[participantId] = allGbids;

    JOYNR_LOG_INFO(
            logger(),
            "Added global capability to cache {}, registered GBIDs: >{}<, #globalLookupCache: {}",
            entry.toString(),
            boost::algorithm::join(allGbids, ", "),
            _globalLookupCache->size());
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::searchGlobalCache(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::vector<std::string>& gbids,
        std::chrono::milliseconds maxCacheAge)
{
    std::unique_lock<std::recursive_mutex> globalSearchLock(_cacheLock);

    const std::unordered_set<std::string> gbidsSet(gbids.cbegin(), gbids.cend());
    std::vector<types::DiscoveryEntry> result;
    for (const auto& interfaceAddress : interfaceAddresses) {
        const std::string& domain = interfaceAddress.getDomain();
        const std::string& interface = interfaceAddress.getInterface();

        const auto entries =
                _globalLookupCache->lookupCacheByDomainAndInterface(domain, interface, maxCacheAge);
        const auto filteredEntries = LCDUtil::filterDiscoveryEntriesByGbids(
                globalSearchLock, entries, gbidsSet, _globalParticipantIdsToGbidsMap);
        result.insert(result.end(),
                      std::make_move_iterator(filteredEntries.begin()),
                      std::make_move_iterator(filteredEntries.end()));
    }
    return result;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::searchLocalCache(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    std::lock_guard<std::recursive_mutex> localSearchLock(_cacheLock);

    std::vector<types::DiscoveryEntry> result;
    for (const auto& interfaceAddress : interfaceAddresses) {
        const std::string& domain = interfaceAddress.getDomain();
        const std::string& interface = interfaceAddress.getInterface();

        auto entries =
                _locallyRegisteredCapabilities->lookupByDomainAndInterface(domain, interface);
        result.insert(result.end(),
                      std::make_move_iterator(entries.begin()),
                      std::make_move_iterator(entries.end()));
    }
    return result;
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::searchCaches(
        const std::string& participantId,
        joynr::types::DiscoveryScope::Enum scope,
        const std::vector<std::string>& gbids,
        std::chrono::milliseconds maxCacheAge)
{
    std::unique_lock<std::recursive_mutex> lock(_cacheLock);

    // first search locally
    auto entry = _locallyRegisteredCapabilities->lookupByParticipantId(participantId);
    if (scope == types::DiscoveryScope::LOCAL_ONLY ||
        (entry && scope != types::DiscoveryScope::GLOBAL_ONLY)) {
        return entry;
    }

    if (maxCacheAge == std::chrono::milliseconds(-1)) {
        entry = _globalLookupCache->lookupByParticipantId(participantId);
    } else {
        entry = _globalLookupCache->lookupCacheByParticipantId(participantId, maxCacheAge);
    }
    if (entry) {
        const std::unordered_set<std::string> gbidsSet(gbids.cbegin(), gbids.cend());
        if (!LCDUtil::isEntryForGbid(lock, *entry, gbidsSet, _globalParticipantIdsToGbidsMap)) {
            return boost::none;
        }
    }
    return entry;
}

std::recursive_mutex& LocalCapabilitiesDirectoryStore::getCacheLock()
{
    return _cacheLock;
}

void LocalCapabilitiesDirectoryStore::eraseParticipantIdToGbidMapping(
        const std::string& participantId)
{
    _globalParticipantIdsToGbidsMap.erase(participantId);
}

std::vector<std::string> LocalCapabilitiesDirectoryStore::getGbidsForParticipantId(
        const std::string& participantId)
{
    auto foundGbids = _globalParticipantIdsToGbidsMap.find(participantId);
    if (foundGbids == _globalParticipantIdsToGbidsMap.cend()) {
        return {};
    }
    return foundGbids->second;
}

std::shared_ptr<capabilities::CachingStorage> LocalCapabilitiesDirectoryStore::
        getGlobalLookupCache()
{
    return _globalLookupCache;
}

std::shared_ptr<capabilities::Storage> LocalCapabilitiesDirectoryStore::
        getLocallyRegisteredCapabilities()
{
    return _locallyRegisteredCapabilities;
}
}
