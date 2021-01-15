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
    MOCK_METHOD1(getGbidsForParticipantId, std::vector<std::string> (const std::string& participantId));
    MOCK_CONST_METHOD0(getAllGlobalCapabilities, std::vector<types::DiscoveryEntry> ());
    MOCK_METHOD1(eraseParticipantIdToGbidMapping, void(const std::string& participantId));
    MOCK_METHOD0(getGlobalLookupCache, std::shared_ptr<capabilities::CachingStorage>());
    MOCK_METHOD0(getLocallyRegisteredCapabilities, std::shared_ptr<capabilities::Storage>());
};

} //namespace joynr


#endif // TESTS_MOCK_MOCKLOCALCAPABILITIESDIRECTORYSTORE_H
