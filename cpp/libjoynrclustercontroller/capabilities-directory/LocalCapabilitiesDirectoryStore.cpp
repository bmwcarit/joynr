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

#include "joynr/CapabilitiesStorage.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/LCDUtil.h"
#include "joynr/LocalCapabilitiesDirectoryStore.h"
#include "joynr/Util.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/types/DiscoveryQos.h"

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

    auto localCapabilities = searchLocal(interfaceAddresses, scope);
    std::vector<types::DiscoveryEntry> globallyCachedEntries;
    if (_includeGlobalScopes.find(scope) != _includeGlobalScopes.end()) {
        globallyCachedEntries =
                searchGlobalCache(interfaceAddresses,
                                  gbids,
                                  std::chrono::milliseconds(discoveryQos.getCacheMaxAge()));
    }

    if (areMissingDomains(interfaceAddresses, localCapabilities, globallyCachedEntries)) {
        return false;
    } else {
        return collectCapabilities(
                scope, localCapabilities, globallyCachedEntries, std::move(callback));
    }
}

bool LocalCapabilitiesDirectoryStore::getLocalAndCachedCapabilities(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    joynr::types::DiscoveryScope::Enum scope = discoveryQos.getDiscoveryScope();

    boost::optional<types::DiscoveryEntry> localCapability = searchLocal(participantId, scope);
    boost::optional<types::DiscoveryEntry> globalCachedCapability = boost::none;

    if (localCapability) {
        if (scope == types::DiscoveryScope::GLOBAL_ONLY) {
            globalCachedCapability = localCapability;
            localCapability = boost::none;
        }
    } else if (_includeGlobalScopes.find(scope) != _includeGlobalScopes.end()) {
        globalCachedCapability = searchGlobalCache(
                participantId, gbids, std::chrono::milliseconds(discoveryQos.getCacheMaxAge()));
    }

    auto localCapabilities = LCDUtil::optionalToVector(std::move(localCapability));
    auto globallyCachedEntries = LCDUtil::optionalToVector(std::move(globalCachedCapability));

    if (areMissingDomains(scope, participantId, localCapabilities, globallyCachedEntries)) {
        return false;
    } else {
        return collectCapabilities(
                scope, localCapabilities, globallyCachedEntries, std::move(callback));
    }
}

bool LocalCapabilitiesDirectoryStore::getAwaitGlobalRegistration(
        const std::string& participantId,
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;
    auto it = _participantIdToAwaitGlobalRegistrationMap.find(participantId);
    if (it != _participantIdToAwaitGlobalRegistrationMap.end()) {
        return it->second;
    }
    // if no entry was found for given participantId, continue as it was registered with false
    return false;
}

std::vector<std::string> LocalCapabilitiesDirectoryStore::
        touchAndReturnGlobalParticipantIdsFromLocalCapabilities(
                const std::unique_lock<std::recursive_mutex>& cacheLock,
                const std::int64_t& newLastSeenDateMs,
                const std::int64_t& newExpiryDateMs)
{
    return getLocallyRegisteredCapabilities(cacheLock)->touchAndReturnGlobalParticipantIds(
            newLastSeenDateMs, newExpiryDateMs);
}

void LocalCapabilitiesDirectoryStore::touchSelectedGlobalParticipant(
        const std::unique_lock<std::recursive_mutex>& cacheLock,
        const std::vector<std::string>& participantIds,
        const std::int64_t& newLastSeenDateMs,
        const std::int64_t& newExpiryDateMs)
{
    getGlobalLookupCache(cacheLock)->touchSelected(
            participantIds, newLastSeenDateMs, newExpiryDateMs);
}

void LocalCapabilitiesDirectoryStore::insertIntoLocallyRegisteredCapabilities(
        const std::unique_lock<std::recursive_mutex>& cacheLock,
        const joynr::types::DiscoveryEntry& capability,
        std::vector<std::string> gbids)
{
    auto locallyRegisteredCapabilities{getLocallyRegisteredCapabilities(cacheLock)};
    if (!gbids.empty()) {
        locallyRegisteredCapabilities->insert(capability, gbids);
    } else {
        locallyRegisteredCapabilities->insert(capability);
    }
}

void LocalCapabilitiesDirectoryStore::insertIntoGlobalCachedCapabilities(
        const std::unique_lock<std::recursive_mutex>& cacheLock,
        const joynr::types::DiscoveryEntry& capability)
{
    getGlobalLookupCache(cacheLock)->insert(capability);
}

std::vector<joynr::types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::
        removeExpiredCapabilitiesFromGlobalCache(
                const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    return getGlobalLookupCache(cacheLock)->removeExpired();
}

std::vector<joynr::types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::
        removeExpiredLocallyRegisteredCapabilities(
                const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    return getLocallyRegisteredCapabilities(cacheLock)->removeExpired();
}

std::size_t LocalCapabilitiesDirectoryStore::getLocallyRegisteredCapabilitiesCount(
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    return getLocallyRegisteredCapabilities(cacheLock)->size();
}

std::size_t LocalCapabilitiesDirectoryStore::getGlobalCachedCapabilitiesCount(
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    return getGlobalLookupCache(cacheLock)->size();
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::
        insertLocallyRegisteredCapabilitesToEntryList(
                const std::unique_lock<std::recursive_mutex>& cacheLock,
                const std::int64_t& currentTime,
                const std::int64_t& newExpiryDateMs)
{
    std::vector<types::DiscoveryEntry> entries{};
    for (auto capability : *(getLocallyRegisteredCapabilities(cacheLock))) {
        if (capability.getExpiryDateMs() < newExpiryDateMs) {
            capability.setExpiryDateMs(newExpiryDateMs);
        }
        if (capability.getLastSeenDateMs() < currentTime) {
            capability.setLastSeenDateMs(currentTime);
        }
        entries.push_back(capability);
    }
    return entries;
}
bool LocalCapabilitiesDirectoryStore::getLocalCapabilities(
        const std::vector<types::DiscoveryEntry>& localCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    // we call capabilitiesReceived for empty results, too
    const auto& localCapsWithMetaInfo = LCDUtil::convert(true, localCapabilities);
    callback->capabilitiesReceived(localCapsWithMetaInfo);
    return true;
}

bool LocalCapabilitiesDirectoryStore::getLocalThenGlobalCapabilities(
        const std::vector<types::DiscoveryEntry>& localCapabilities,
        const std::vector<types::DiscoveryEntry>& globallyCachedCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    bool areCapabilitiesAvailable{false};

    if (!localCapabilities.empty()) {
        const auto& localCapsWithMetaInfo = LCDUtil::convert(true, localCapabilities);
        callback->capabilitiesReceived(localCapsWithMetaInfo);
        areCapabilitiesAvailable = true;
    }
    if (!globallyCachedCapabilities.empty()) {
        const auto& globalCachedCapsWithMetaInfo =
                LCDUtil::convert(false, globallyCachedCapabilities);
        callback->capabilitiesReceived(globalCachedCapsWithMetaInfo);
        areCapabilitiesAvailable = true;
    }

    return areCapabilitiesAvailable;
}

bool LocalCapabilitiesDirectoryStore::getLocalAndGlobalCapabilities(
        const std::vector<types::DiscoveryEntry>& localCapabilities,
        const std::vector<types::DiscoveryEntry>& globallyCachedCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    bool areCapabilitiesAvailable{false};

    if (!globallyCachedCapabilities.empty()) {
        auto localCapsWithMetaInfo = LCDUtil::convert(true, localCapabilities);
        auto globalCachedCapsWithMetaInfo = LCDUtil::convert(false, globallyCachedCapabilities);

        // merge and remove duplicated entries. If duplicate entries with the same
        // participantId found, keep the local ones
        const auto& localAndGlobalCapsWithMetaInfo = LCDUtil::filterDuplicates(
                std::move(localCapsWithMetaInfo), std::move(globalCachedCapsWithMetaInfo));
        callback->capabilitiesReceived(localAndGlobalCapsWithMetaInfo);
        areCapabilitiesAvailable = true;
    }

    return areCapabilitiesAvailable;
}

bool LocalCapabilitiesDirectoryStore::getGlobalCapabilities(
        const std::vector<types::DiscoveryEntry>& localCapabilities,
        const std::vector<types::DiscoveryEntry>& globallyCachedCapabilities,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    bool areCapabilitiesAvailable{false};

    if (!globallyCachedCapabilities.empty()) {
        // lookup remote entries in GCD if there are no cached entries
        std::vector<types::DiscoveryEntry> globallyRegisteredEntries;
        for (const auto& capability : localCapabilities) {
            if (LCDUtil::isGlobal(capability)) {
                globallyRegisteredEntries.push_back(capability);
            }
        }
        auto globallyRegisteredCapsWithMetaInfo = LCDUtil::convert(true, globallyRegisteredEntries);
        auto globalCachedCapsWithMetaInfo = LCDUtil::convert(false, globallyCachedCapabilities);
        // merge and remove duplicated entries. If duplicate entries with the same
        // participantId found, keep the local ones
        const auto& allGlobalCaps =
                LCDUtil::filterDuplicates(std::move(globallyRegisteredCapsWithMetaInfo),
                                          std::move(globalCachedCapsWithMetaInfo));
        callback->capabilitiesReceived(allGlobalCaps);
        areCapabilitiesAvailable = true;
    }

    return areCapabilitiesAvailable;
}

void LocalCapabilitiesDirectoryStore::mapGbidsToGlobalProviderParticipantId(
        const std::string& participantId,
        std::vector<std::string>& allGbids)
{
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
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::lookupGlobalEntry(
        const std::string& participantId)
{
    return _globalLookupCache->lookupByParticipantId(participantId);
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::lookupLocalEntry(
        const std::string& participantId)
{
    return _locallyRegisteredCapabilities->lookupByParticipantId(participantId);
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::getLocalCapabilities(
        const std::string& participantId)
{
    std::lock_guard<std::recursive_mutex> localCachedRetrievalLock(_cacheLock);
    return LCDUtil::optionalToVector(lookupLocalEntry(participantId));
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::getLocalCapabilities(
        const std::vector<InterfaceAddress>& interfaceAddresses)
{
    return searchLocal(interfaceAddresses);
}

void LocalCapabilitiesDirectoryStore::clear()
{
    std::lock_guard<std::recursive_mutex> clearingLock(_cacheLock);
    _locallyRegisteredCapabilities->clear();
    _globalLookupCache->clear();
    _globalParticipantIdsToGbidsMap.clear();
    _participantIdToAwaitGlobalRegistrationMap.clear();
}

void LocalCapabilitiesDirectoryStore::insertInLocalCapabilitiesStorage(
        const types::DiscoveryEntry& entry,
        bool awaitGlobalRegistration,
        const std::vector<std::string>& gbids)
{
    std::unique_lock<std::recursive_mutex> localInsertionLock(_cacheLock);
    // always remove cached remote entries with the same participantId.
    auto cachedEntry = _globalLookupCache->lookupByParticipantId(entry.getParticipantId());
    if (cachedEntry) {
        JOYNR_LOG_WARN(logger(),
                       "Add participantId {} removes cached entry with the same participantId: {}",
                       entry.getParticipantId(),
                       cachedEntry->toString());
        _globalLookupCache->removeByParticipantId(entry.getParticipantId());
        eraseParticipantIdToGbidMapping(cachedEntry->getParticipantId(), localInsertionLock);
    }

    std::vector<std::string> allGbids(gbids);
    _participantIdToAwaitGlobalRegistrationMap[entry.getParticipantId()] = awaitGlobalRegistration;
    if (LCDUtil::isGlobal(entry)) {
        _locallyRegisteredCapabilities->insert(entry, allGbids);
        mapGbidsToGlobalProviderParticipantId(entry.getParticipantId(), allGbids);
    } else {
        _locallyRegisteredCapabilities->insert(entry);
    }

    JOYNR_LOG_INFO(logger(),
                   "Added local capability {}, #localCapabilities: {}",
                   entry.toString(),
                   _locallyRegisteredCapabilities->size());
}

void LocalCapabilitiesDirectoryStore::insertInGlobalLookupCache(
        const types::DiscoveryEntry& entry,
        const std::vector<std::string>& gbids)
{
    std::lock_guard<std::recursive_mutex> globalInsertionLock(_cacheLock);

    std::vector<std::string> allGbids(gbids);
    _globalLookupCache->insert(entry);
    mapGbidsToGlobalProviderParticipantId(entry.getParticipantId(), allGbids);

    JOYNR_LOG_INFO(
            logger(),
            "Added global capability to cache {}, registered GBIDs: >{}<, #globalLookupCache: {}",
            entry.toString(),
            boost::algorithm::join(allGbids, ", "),
            _globalLookupCache->size());
}

void LocalCapabilitiesDirectoryStore::insertRemoteEntriesIntoGlobalCache(
        const types::DiscoveryEntry& entry,
        const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address,
        const std::vector<std::string>& knownGbids)
{
    std::vector<std::string> gbids;
    if (auto mqttAddress = dynamic_cast<const system::RoutingTypes::MqttAddress*>(address.get())) {
        gbids.push_back(mqttAddress->getBrokerUri());
    } else {
        // use default GBID for all other address types
        gbids.push_back(knownGbids[0]);
    }
    insertInGlobalLookupCache(std::move(entry), std::move(gbids));
}

void LocalCapabilitiesDirectoryStore::removeLocallyRegisteredParticipant(
        const std::string& participantId,
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    getLocallyRegisteredCapabilities(cacheLock)->removeByParticipantId(participantId);
    eraseParticipantIdToAwaitGlobalRegistrationMapping(participantId, cacheLock);
}

void LocalCapabilitiesDirectoryStore::removeParticipant(
        const std::string& participantId,
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    eraseParticipantIdToGbidMapping(participantId, cacheLock);
    getGlobalLookupCache(cacheLock)->removeByParticipantId(participantId);
    getLocallyRegisteredCapabilities(cacheLock)->removeByParticipantId(participantId);
    eraseParticipantIdToAwaitGlobalRegistrationMapping(participantId, cacheLock);
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::searchGlobalCache(
        const std::string& participantId,
        const std::vector<std::string>& gbids,
        std::chrono::milliseconds maxCacheAge)
{
    std::unique_lock<std::recursive_mutex> lock(_cacheLock);
    boost::optional<types::DiscoveryEntry> entry = boost::none;
    if (maxCacheAge.count() >= 0) {
        entry = _globalLookupCache->lookupCacheByParticipantId(participantId, maxCacheAge);
    } else {
        entry = lookupGlobalEntry(participantId);
    }

    if (entry) {
        const std::unordered_set<std::string> gbidsSet(gbids.cbegin(), gbids.cend());
        if (!LCDUtil::isEntryForGbid(lock, *entry, gbidsSet, _globalParticipantIdsToGbidsMap)) {
            return boost::none;
        }
    }
    return entry;
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

bool LocalCapabilitiesDirectoryStore::areMissingDomains(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::vector<joynr::types::DiscoveryEntry>& localCapabilities,
        const std::vector<types::DiscoveryEntry>& globallyCachedEntries)
{
    bool areMissingDomains{false};
    std::vector<std::string> domainList{};
    if (!localCapabilities.empty() || !globallyCachedEntries.empty()) {
        for (const auto& interfaceAddress : interfaceAddresses) {
            domainList.push_back(interfaceAddress.getDomain());
        }
        for (const auto& localEntry : localCapabilities) {
            auto domainMatchIterator{
                    std::find(domainList.cbegin(), domainList.cend(), localEntry.getDomain())};
            if (domainMatchIterator != domainList.end()) {
                domainList.erase(domainMatchIterator);
            }
        }
        for (const auto& globalEntry : globallyCachedEntries) {
            auto domainMatchIterator{
                    std::find(domainList.cbegin(), domainList.cend(), globalEntry.getDomain())};
            if (domainMatchIterator != domainList.end()) {
                domainList.erase(domainMatchIterator);
            }
        }
    }
    if (!domainList.empty()) {
        areMissingDomains = true;
    }
    return areMissingDomains;
}

bool LocalCapabilitiesDirectoryStore::areMissingDomains(
        const joynr::types::DiscoveryScope::Enum& scope,
        const std::string& participantId,
        const std::vector<joynr::types::DiscoveryEntry>& localCapabilities,
        const std::vector<types::DiscoveryEntry>& globallyCachedEntries)
{
    bool areMissingEntries{false};
    auto localCapabilitesEntries{localCapabilities};
    auto globalCapabilitiesEntries{globallyCachedEntries};
    auto localEntry{lookupLocalEntry(participantId)};
    auto globalEntry{lookupGlobalEntry(participantId)};

    if (localEntry && scope == joynr::types::DiscoveryScope::GLOBAL_ONLY) {
        globalEntry = localEntry;
        localEntry = boost::none;
    }

    if (localEntry) {
        for (auto localEntryIterator = localCapabilitesEntries.cbegin();
             localEntryIterator != localCapabilitesEntries.cend();
             localEntryIterator++) {
            if (!localCapabilitesEntries.empty()) {
                if (localEntryIterator->getDomain() == localEntry->getDomain()) {
                    localCapabilitesEntries.erase(localEntryIterator);
                    break;
                }
            }
        }
    }

    if (globalEntry) {
        for (auto globalEntryInterator = globalCapabilitiesEntries.cbegin();
             globalEntryInterator != globalCapabilitiesEntries.cend();
             globalEntryInterator++) {
            if (globalEntryInterator->getDomain() == globalEntry->getDomain()) {
                globalCapabilitiesEntries.erase(globalEntryInterator);
                break;
            }
        }
    }

    if (!localCapabilitesEntries.empty() || (!globalCapabilitiesEntries.empty())) {
        areMissingEntries = true;
    }
    return areMissingEntries;
}

bool LocalCapabilitiesDirectoryStore::collectCapabilities(
        joynr::types::DiscoveryScope::Enum& scope,
        const std::vector<types::DiscoveryEntry>& localCapabilities,
        const std::vector<types::DiscoveryEntry>& globallyCachedEntries,
        std::shared_ptr<ILocalCapabilitiesCallback> callback)
{
    bool areCapabilitiesAvailable{false};
    switch (scope) {
    case joynr::types::DiscoveryScope::LOCAL_ONLY: {
        areCapabilitiesAvailable = getLocalCapabilities(localCapabilities, std::move(callback));
        break;
    }
    case joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL: {
        areCapabilitiesAvailable = getLocalThenGlobalCapabilities(
                localCapabilities, globallyCachedEntries, std::move(callback));
        break;
    }
    case joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL: {
        areCapabilitiesAvailable = getLocalAndGlobalCapabilities(
                localCapabilities, globallyCachedEntries, std::move(callback));
        break;
    }
    case joynr::types::DiscoveryScope::GLOBAL_ONLY: {
        areCapabilitiesAvailable = getGlobalCapabilities(
                localCapabilities, globallyCachedEntries, std::move(callback));
        break;
    }
    default: {
        std::string errorMsg = "Unknown or illegal DiscoveryScope value: ";
        errorMsg += scope;
        throw exceptions::DiscoveryException(errorMsg);
    }
    }
    return areCapabilitiesAvailable;
}

boost::optional<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::searchLocal(
        const std::string& participantId,
        const types::DiscoveryScope::Enum& scope)
{
    // search locally registered entry in local store
    std::lock_guard<std::recursive_mutex> localSearchLock(_cacheLock);
    boost::optional<types::DiscoveryEntry> entry = lookupLocalEntry(participantId);
    if (entry && (_includeLocalScopes.find(scope) == _includeLocalScopes.end() &&
                  entry->getQos().getScope() == types::ProviderScope::LOCAL)) {
        // ignore the found entry
        return boost::none;
    }
    return entry;
}

std::vector<types::DiscoveryEntry> LocalCapabilitiesDirectoryStore::searchLocal(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const types::DiscoveryScope::Enum& scope)
{
    std::unique_lock<std::recursive_mutex> localSearchLock(_cacheLock);

    std::vector<types::DiscoveryEntry> result;
    for (const auto& interfaceAddress : interfaceAddresses) {
        const std::string& domain = interfaceAddress.getDomain();
        const std::string& interface = interfaceAddress.getInterface();

        auto entries =
                _locallyRegisteredCapabilities->lookupByDomainAndInterface(domain, interface);
        if (_includeLocalScopes.find(scope) != _includeLocalScopes.cend()) {
            result.insert(result.end(),
                          std::make_move_iterator(entries.begin()),
                          std::make_move_iterator(entries.end()));
        } else {
            for (const auto& entry : entries) {
                if (LCDUtil::isGlobal(entry)) {
                    result.push_back(entry);
                }
            }
        }
    }
    return result;
}

std::recursive_mutex& LocalCapabilitiesDirectoryStore::getCacheLock()
{
    return _cacheLock;
}

void LocalCapabilitiesDirectoryStore::eraseParticipantIdToGbidMapping(
        const std::string& participantId,
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;
    _globalParticipantIdsToGbidsMap.erase(participantId);
}

void LocalCapabilitiesDirectoryStore::eraseParticipantIdToAwaitGlobalRegistrationMapping(
        const std::string& participantId,
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;
    _participantIdToAwaitGlobalRegistrationMap.erase(participantId);
}

std::vector<std::string> LocalCapabilitiesDirectoryStore::getGbidsForParticipantId(
        const std::string& participantId,
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;
    auto foundGbids = _globalParticipantIdsToGbidsMap.find(participantId);
    if (foundGbids == _globalParticipantIdsToGbidsMap.cend()) {
        return {};
    }
    return foundGbids->second;
}

std::shared_ptr<capabilities::CachingStorage> LocalCapabilitiesDirectoryStore::getGlobalLookupCache(
        const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;
    return _globalLookupCache;
}

std::shared_ptr<capabilities::Storage> LocalCapabilitiesDirectoryStore::
        getLocallyRegisteredCapabilities(const std::unique_lock<std::recursive_mutex>& cacheLock)
{
    assert(cacheLock.owns_lock());
    std::ignore = cacheLock;
    return _locallyRegisteredCapabilities;
}
} // namespace joynr
