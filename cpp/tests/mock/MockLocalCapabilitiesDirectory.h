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
#ifndef TESTS_MOCK_MOCKLOCALCAPABILITIESDIRECTORY_H
#define TESTS_MOCK_MOCKLOCALCAPABILITIESDIRECTORY_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "joynr/CapabilitiesStorage.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryQos.h"


class MockLocalCapabilitiesDirectory : public joynr::LocalCapabilitiesDirectory {
public:
    MockLocalCapabilitiesDirectory(joynr::ClusterControllerSettings& ccSettings,
                                   std::shared_ptr<joynr::IMessageRouter> mockMessageRouter,
                                   std::shared_ptr<joynr::capabilities::Storage> locallyRegisteredCapabilities,
                                   std::shared_ptr<joynr::capabilities::CachingStorage> globalLookupCache,
                                   boost::asio::io_service& ioService,
                                   std::int64_t defaultExpiryDateMs)
        : LocalCapabilitiesDirectory(ccSettings,
                                   nullptr,
                                   locallyRegisteredCapabilities,
                                   globalLookupCache,
                                   "localAddress",
                                   mockMessageRouter,
                                   ioService,
                                   "clusterControllerId",
                                   {"testGbid"},
                                   defaultExpiryDateMs)
   {}

    MOCK_METHOD5(
            lookup,
            void(
                const std::string& participantId,
                const joynr::types::DiscoveryQos& discoveryQos,
                const std::vector<std::string>& gbids,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
                std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError
            ));
};

#endif // TESTS_MOCK_MOCKLOCALCAPABILITIESDIRECTORY_H
