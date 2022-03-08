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
#ifndef TESTS_MOCK_MOCKLOCALCAPABILITIESDIRECTORYSTORE_H
#define TESTS_MOCK_MOCKLOCALCAPABILITIESDIRECTORYSTORE_H

#include "joynr/LocalCapabilitiesDirectoryStore.h"

namespace joynr
{

class MockLocalCapabilitiesDirectoryStore : public joynr::LocalCapabilitiesDirectoryStore {
public:
    MockLocalCapabilitiesDirectoryStore() = default;
    MockLocalCapabilitiesDirectoryStore(
            std::shared_ptr<capabilities::CachingStorage> globalLookupCache,
            std::shared_ptr<capabilities::Storage> locallyRegisteredCapabilities) :
        _globalLookupCache{globalLookupCache},
        _locallyRegisteredCapabilities{locallyRegisteredCapabilities} {}

    MOCK_METHOD1(getLocalCapabilities, std::vector<types::DiscoveryEntry> (const std::string& participantId));
    MOCK_METHOD1(getLocalCapabilities, std::vector<types::DiscoveryEntry> (const std::vector<InterfaceAddress>&
                                                                           interfaceAddresses));
    MOCK_METHOD1(insertInLocalCapabilitiesStorage, void (const types::DiscoveryEntry& entry));
    MOCK_METHOD2(insertInGlobalLookupCache, void (const types::DiscoveryEntry& entry,
                                                  const std::vector<std::string>& gbids));
    MOCK_METHOD4(getLocalAndCachedCapabilities, bool (const std::string& participantId,
                                                      const joynr::types::DiscoveryQos& discoveryQos,
                                                      const std::vector<std::string>& gbids,
                                                      std::shared_ptr<ILocalCapabilitiesCallback> callback));
    MOCK_METHOD4(getLocalAndCachedCapabilities, bool (const std::vector<InterfaceAddress>& interfaceAddresses,
                                                          const joynr::types::DiscoveryQos& discoveryQos,
                                                          const std::vector<std::string>& gbids,
                                                          std::shared_ptr<ILocalCapabilitiesCallback> callback));
    MOCK_METHOD2(getGbidsForParticipantId, std::vector<std::string> (const std::string& participantId,
                                                                     const std::unique_lock<std::recursive_mutex>& cacheLock));
    MOCK_CONST_METHOD0(getAllGlobalCapabilities, std::vector<types::DiscoveryEntry> ());
    MOCK_METHOD2(eraseParticipantIdToGbidMapping, void(const std::string& participantId,
                                                       const std::unique_lock<std::recursive_mutex>& cacheLock));
    std::shared_ptr<capabilities::CachingStorage> getGlobalLookupCache(const std::unique_lock<std::recursive_mutex>& cacheLock) override {
        if(_globalLookupCache) {
            return _globalLookupCache;
        }
        return joynr::LocalCapabilitiesDirectoryStore::getGlobalLookupCache(cacheLock);
    }
    std::shared_ptr<capabilities::Storage> getLocallyRegisteredCapabilities(const std::unique_lock<std::recursive_mutex>& cacheLock) override {
        if(_locallyRegisteredCapabilities) {
            return _locallyRegisteredCapabilities;
        }
        return joynr::LocalCapabilitiesDirectoryStore::getLocallyRegisteredCapabilities(cacheLock);
    }
private:
    std::shared_ptr<capabilities::CachingStorage> _globalLookupCache;
    std::shared_ptr<capabilities::Storage> _locallyRegisteredCapabilities;
};

} //namespace joynr


#endif // TESTS_MOCK_MOCKLOCALCAPABILITIESDIRECTORYSTORE_H
