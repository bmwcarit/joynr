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
#ifndef LOCALCAPABILITIESDIRECTORYSTORE_H
#define LOCALCAPABILITIESDIRECTORYSTORE_H

#include <boost/optional.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "joynr/InterfaceAddress.h"
#include "joynr/Logger.h"
#include "joynr/types/DiscoveryScope.h"

namespace joynr
{
class DiscoveryEntry;
class ILocalCapabilitiesCallback;

namespace capabilities
{
class Storage;
class CachingStorage;
} // namespace capabilities

namespace types
{
class DiscoveryEntry;
class DiscoveryQos;
} // namespace types

class LocalCapabilitiesDirectoryStore
{

public:
    LocalCapabilitiesDirectoryStore();
    virtual ~LocalCapabilitiesDirectoryStore();

    /*
     * Returns a list of locally cached capabilitiy entries. This method is used
     * when capabilities from the global directory are received, to check if a new
     * local provider was registered in the meantime.
     */
    virtual std::vector<types::DiscoveryEntry> getLocalCapabilities(
            const std::string& participantId);
    virtual std::vector<types::DiscoveryEntry> getLocalCapabilities(
            const std::vector<InterfaceAddress>& interfaceAddress);

    virtual std::vector<types::DiscoveryEntry> getAllGlobalCapabilities() const;

    std::vector<types::DiscoveryEntry> getCachedGlobalDiscoveryEntries() const;

    std::size_t countGlobalCapabilities() const;

    virtual bool getLocalAndCachedCapabilities(
            const std::vector<InterfaceAddress>& interfaceAddress,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::shared_ptr<ILocalCapabilitiesCallback> callback);
    virtual bool getLocalAndCachedCapabilities(
            const std::string& participantId,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::shared_ptr<ILocalCapabilitiesCallback> callback);

    /*
     * removes all discovery entries
     */
    void clear();

    virtual void insertInLocalCapabilitiesStorage(
            const types::DiscoveryEntry& entry,
            bool awaitGlobalRegistration,
            const std::vector<std::string>& gbids = std::vector<std::string>());
    virtual void insertInGlobalLookupCache(const types::DiscoveryEntry& entry,
                                           const std::vector<std::string>& gbids);

    virtual std::vector<std::string> getGbidsForParticipantId(
            const std::string& participantId,
            const std::unique_lock<std::recursive_mutex>& cacheLock);
    std::vector<types::DiscoveryEntry> searchLocal(
            const std::vector<InterfaceAddress>& interfaceAddress,
            const types::DiscoveryScope::Enum& scope = types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    std::recursive_mutex& getCacheLock();
    virtual void eraseParticipantIdToGbidMapping(
            const std::string& participantId,
            const std::unique_lock<std::recursive_mutex>& cacheLock);
    virtual void eraseParticipantIdToAwaitGlobalRegistrationMapping(
            const std::string& participantId,
            const std::unique_lock<std::recursive_mutex>& cacheLock);
    virtual std::shared_ptr<capabilities::CachingStorage> getGlobalLookupCache(
            const std::unique_lock<std::recursive_mutex>& cacheLock);
    virtual std::shared_ptr<capabilities::Storage> getLocallyRegisteredCapabilities(
            const std::unique_lock<std::recursive_mutex>& cacheLock);

    virtual bool getAwaitGlobalRegistration(
            const std::string& participantId,
            const std::unique_lock<std::recursive_mutex>& cacheLock);

private:
    boost::optional<types::DiscoveryEntry> searchLocal(const std::string& participantId,
                                                       const types::DiscoveryScope::Enum& scope);

    boost::optional<types::DiscoveryEntry> searchGlobalCache(const std::string& participantId,
                                                             const std::vector<std::string>& gbids,
                                                             std::chrono::milliseconds maxCacheAge);

    std::vector<types::DiscoveryEntry> searchGlobalCache(
            const std::vector<InterfaceAddress>& interfaceAddress,
            const std::vector<std::string>& gbids,
            std::chrono::milliseconds maxCacheAge);
    bool callReceiverIfPossible(
            joynr::types::DiscoveryScope::Enum& scope,
            const std::vector<joynr::types::DiscoveryEntry>& localCapabilities,
            const std::vector<joynr::types::DiscoveryEntry>& globallyCachedCapabilities,
            std::shared_ptr<ILocalCapabilitiesCallback> callback);

    void mapGbidsToGlobalProviderParticipantId(const std::string& participantId,
                                               std::vector<std::string>& allGbids);

    const std::unordered_set<types::DiscoveryScope::Enum> _includeLocalScopes{
            types::DiscoveryScope::LOCAL_ONLY, types::DiscoveryScope::LOCAL_AND_GLOBAL,
            types::DiscoveryScope::LOCAL_THEN_GLOBAL};
    const std::unordered_set<types::DiscoveryScope::Enum> _includeGlobalScopes{
            types::DiscoveryScope::GLOBAL_ONLY, types::DiscoveryScope::LOCAL_AND_GLOBAL,
            types::DiscoveryScope::LOCAL_THEN_GLOBAL};

    std::shared_ptr<capabilities::Storage> _locallyRegisteredCapabilities;
    std::shared_ptr<capabilities::CachingStorage> _globalLookupCache;
    std::unordered_map<std::string, std::vector<std::string>> _globalParticipantIdsToGbidsMap;
    std::unordered_map<std::string, bool> _participantIdToAwaitGlobalRegistrationMap;
    mutable std::recursive_mutex _cacheLock;
    ADD_LOGGER(LocalCapabilitiesDirectoryStore)
};
} // namespace joynr

#endif // LOCALCAPABILITIESDIRECTORYSTORE_H
