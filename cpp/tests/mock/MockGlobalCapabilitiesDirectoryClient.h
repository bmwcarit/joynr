/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKGLOBALCAPABILITIESDIRECTORYCLIENT_H
#define TESTS_MOCK_MOCKGLOBALCAPABILITIESDIRECTORYCLIENT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "libjoynrclustercontroller/capabilities-directory/IGlobalCapabilitiesDirectoryClient.h"
#include "joynr/LocalCapabilitiesDirectoryStore.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

class MockGlobalCapabilitiesDirectoryClient : public joynr::IGlobalCapabilitiesDirectoryClient {
public:
    MOCK_METHOD5(add, void(const joynr::types::GlobalDiscoveryEntry& entry,
                           const std::vector<std::string>& gbids,
                           std::function<void()> onSuccess,
                           std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
                           std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD5(remove, void(const std::string& participantId,
                              const std::vector<std::string>& gbids,
                              std::function<void()> onSuccess,
                              std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
                              std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD7(lookup, void(
                     const std::vector<std::string>& domain,
                     const std::string& interfaceName,
                     const std::vector<std::string>& gbids,
                     std::int64_t messagingTtl,
                     std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)> onSuccess,
                     std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD6(lookup, void(
                     const std::string& participantId,
                     const std::vector<std::string>& gbids,
                     std::int64_t messagingTtl,
                     std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)> onSuccess,
                     std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));


    MOCK_METHOD5(removeStale, void(const std::string& clusterControllerId,
                                   std::int64_t maxLastSeenDateMs,
                                   const std::string gbid,
                                   std::function<void()> onSuccess,
                                   std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));

    MOCK_METHOD5(touch, void(const std::string& clusterControllerId,
                             const std::vector<std::string>& participantIds,
                             const std::string& gbid,
                             std::function<void()> onSuccess,
                             std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));

    MOCK_METHOD2(reAdd, void(std::shared_ptr<joynr::LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore,
                             const std::string& localAddress));
};

#endif // TESTS_MOCK_MOCKGLOBALCAPABILITIESDIRECTORYCLIENT_H
