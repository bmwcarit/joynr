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

#include "joynr/Logger.h"
#include "joynr/types/DiscoveryScope.h"

namespace joynr
{
class DiscoveryEntry;
class InterfaceAddress;
class ILocalCapabilitiesCallback;

namespace capabilities
{
class Storage;
class CachingStorage;
}

namespace types
{
class DiscoveryEntry;
class DiscoveryQos;
}

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
    virtual std::vector<types::DiscoveryEntry> getCachedLocalCapabilities(
            const std::string& participantId);
    std::vector<types::DiscoveryEntry> getCachedLocalCapabilities(
            const std::vector<InterfaceAddress>& interfaceAddress);

    std::vector<types::DiscoveryEntry> getCachedGlobalDiscoveryEntries() const;

    std::size_t countGlobalCapabilities() const;

    bool getLocalAndCachedCapabilities(const std::vector<InterfaceAddress>& interfaceAddress,
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

    virtual void insertInLocalCapabilitiesStorage(const types::DiscoveryEntry& entry);
    virtual void insertInGlobalLookupCache(const types::DiscoveryEntry& entry,
                                           const std::vector<std::string>& gbids);

    std::vector<types::DiscoveryEntry> searchGlobalCache(
            const std::vector<InterfaceAddress>& interfaceAddress,
            const std::vector<std::string>& gbids,
            std::chrono::milliseconds maxCacheAge);
    std::vector<types::DiscoveryEntry> searchLocalCache(
            const std::vector<InterfaceAddress>& interfaceAddress);
    boost::optional<types::DiscoveryEntry> searchCaches(const std::string& participantId,
                                                        types::DiscoveryScope::Enum scope,
                                                        const std::vector<std::string>& gbids,
                                                        std::chrono::milliseconds maxCacheAge);
    std::recursive_mutex& getCacheLock();
    void eraseParticipantIdToGbidMapping(const std::string& participantId);
    std::vector<std::string> getGbidsForParticipantId(const std::string& participantId);
    std::shared_ptr<capabilities::CachingStorage> getGlobalLookupCache();
    std::shared_ptr<capabilities::Storage> getLocallyRegisteredCapabilities();

private:
    bool callReceiverIfPossible(joynr::types::DiscoveryScope::Enum& scope,
                                std::vector<types::DiscoveryEntry>&& localCapabilities,
                                std::vector<types::DiscoveryEntry>&& globalCapabilities,
                                std::shared_ptr<ILocalCapabilitiesCallback> callback);

    std::shared_ptr<capabilities::Storage> _locallyRegisteredCapabilities;
    std::shared_ptr<capabilities::CachingStorage> _globalLookupCache;
    std::unordered_map<std::string, std::vector<std::string>> _globalParticipantIdsToGbidsMap;
    mutable std::recursive_mutex _cacheLock;
    ADD_LOGGER(LocalCapabilitiesDirectoryStore)
};
} // namespace joynr

#endif // LOCALCAPABILITIESDIRECTORYSTORE_H
