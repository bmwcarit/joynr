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
#ifndef TESTS_MOCK_MOCKDISCOVERY_H
#define TESTS_MOCK_MOCKDISCOVERY_H

#include <gmock/gmock.h>

#include "joynr/system/IDiscovery.h"

class MockDiscovery : public joynr::system::IDiscovery {
public:
    MOCK_METHOD1(
            add,
            void(
                const joynr::types::DiscoveryEntry& entry
            )
    );
    MOCK_METHOD2(
            lookup,
            void(
                joynr::types::DiscoveryEntryWithMetaInfo& result,
                const std::string& participantId
            )
    );
    MOCK_METHOD4(
            lookup,
            void(
                std::vector<joynr::types::DiscoveryEntryWithMetaInfo> & result,
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos
            )
    );
    MOCK_METHOD1(
            remove,
            void(
                const std::string& participantId
            )
    );
    std::shared_ptr<joynr::Future<void>> addAsync (
                const joynr::types::DiscoveryEntry& discoveryEntry,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            ) noexcept override
    {
        return addAsyncMock(discoveryEntry, std::move(onSuccess), std::move(onRuntimeError));
    }
    MOCK_METHOD3(
            addAsyncMock,
            std::shared_ptr<joynr::Future<void>>(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            ) noexcept override
    {
        return lookupAsyncMock(participantId, std::move(onSuccess), std::move(onRuntimeError));
    }
    MOCK_METHOD3(
            lookupAsyncMock,
            std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>> lookupAsync(
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            ) noexcept override
    {
        return lookupAsyncMock(domain, interfaceName, discoveryQos, std::move(onSuccess), std::move(onRuntimeError));
    }
    MOCK_METHOD5(
            lookupAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>(
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
    std::shared_ptr<joynr::Future<void>> removeAsync(
                const std::string& participantId,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            ) noexcept override
    {
        return removeAsyncMock(participantId, std::move(onSuccess), std::move(onRuntimeError));
    }
    MOCK_METHOD3(
            removeAsyncMock,
            std::shared_ptr<joynr::Future<void>>(
                const std::string& participantId,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError
            )
    );
};

#endif // TESTS_MOCK_MOCKDISCOVERY_H
