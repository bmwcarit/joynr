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

#include "tests/utils/Gmock.h"

#include "joynr/system/IDiscovery.h"

class MockDiscovery : public joynr::system::IDiscovery {
public:
    MOCK_METHOD2(
            add,
            void(
                const joynr::types::DiscoveryEntry& entry,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD3(
            add,
            void(
                const joynr::types::DiscoveryEntry& entry,
                const bool& awaitGlobalRegistration,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD4(
            add,
            void(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                const std::vector<std::string> & gbids,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD3 (
            addToAll,
            void(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD3(
            lookup,
            void(
                joynr::types::DiscoveryEntryWithMetaInfo& result,
                const std::string& participantId,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD5(
            lookup,
            void(
                joynr::types::DiscoveryEntryWithMetaInfo& result,
                const std::string& participantId,
                const joynr::types::DiscoveryQos& discoveryQos,
                const std::vector<std::string>& gbids,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD5(
            lookup,
            void(
                std::vector<joynr::types::DiscoveryEntryWithMetaInfo> & result,
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD6(
            lookup,
            void(
                std::vector<joynr::types::DiscoveryEntryWithMetaInfo> & result,
                const std::vector<std::string> & domains,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                const std::vector<std::string>& gbids,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD2(
            remove,
            void(
                const std::string& participantId,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<void>> addAsync (
                const joynr::types::DiscoveryEntry& discoveryEntry,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return addAsyncMock(discoveryEntry,
                            std::move(onSuccess),
                            std::move(onRuntimeError),
                            std::move(qos));
    }
    MOCK_METHOD4(
            addAsyncMock,
            std::shared_ptr<joynr::Future<void>>(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<void>> addAsync (
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return addAsyncMock(discoveryEntry,
                            awaitGlobalRegistration,
                            std::move(onSuccess),
                            std::move(onRuntimeError),
                            std::move(qos));
    }
    MOCK_METHOD5(
            addAsyncMock,
            std::shared_ptr<joynr::Future<void>>(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<void>> addAsync(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                const std::vector<std::string>& gbids,
                std::function<void()> onSuccess,
                std::function<void (const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos) noexcept override
    {
        return addAsyncMock(discoveryEntry,
                            awaitGlobalRegistration,
                            gbids,
                            std::move(onSuccess),
                            std::move(onApplicationError),
                            std::move(onRuntimeError),
                            std::move(qos));
    }
    MOCK_METHOD7(
            addAsyncMock,
            std::shared_ptr<joynr::Future<void>>(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                const std::vector<std::string>& gbids,
                std::function<void(void)> onSuccess,
                std::function<void (const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<void>> addToAllAsync(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                std::function<void()> onSuccess,
                std::function<void (const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
    ) noexcept override
    {
         return addToAllAsyncMock(discoveryEntry,
                             awaitGlobalRegistration,
                             std::move(onSuccess),
                             std::move(onApplicationError),
                             std::move(onRuntimeError),
                             std::move(qos));
    }
    MOCK_METHOD6(
            addToAllAsyncMock,
            std::shared_ptr<joynr::Future<void>>(
                const joynr::types::DiscoveryEntry& discoveryEntry,
                const bool& awaitGlobalRegistration,
                std::function<void(void)> onSuccess,
                std::function<void (const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return lookupAsyncMock(participantId, std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
    }
    MOCK_METHOD4(
            lookupAsyncMock,
            std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>(
                const std::string& participantId,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
                const std::string& participantId,
                const joynr::types::DiscoveryQos& discoveryQos,
                const std::vector<std::string>& gbids,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)>
                        onSuccess,
                std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                        onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                        onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return lookupAsyncMock(participantId, discoveryQos, gbids, std::move(onSuccess), std::move(onApplicationError), std::move(onRuntimeError), std::move(qos));
    }
    MOCK_METHOD7(
            lookupAsyncMock,
            std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>(
                const std::string& participantId,
                const joynr::types::DiscoveryQos& discoveryQos,
                const std::vector<std::string>& gbids,
                std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)>
                        onSuccess,
                std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                        onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                        onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>> lookupAsync(
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return lookupAsyncMock(domain, interfaceName, discoveryQos, std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
    }
    MOCK_METHOD6(
            lookupAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>(
                const std::vector<std::string>& domain,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                        onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>> lookupAsync(
                const std::vector<std::string>& domains,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                const std::vector<std::string>& gbids,
                std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                        onSuccess,
                std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                        onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                        onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return lookupAsyncMock(domains, interfaceName, discoveryQos, gbids, std::move(onSuccess), std::move(onApplicationError), std::move(onRuntimeError), std::move(qos));
    }
    MOCK_METHOD8(
            lookupAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>(
                const std::vector<std::string>& domains,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos,
                const std::vector<std::string>& gbids,
                std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                        onSuccess,
                std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                        onApplicationError,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                        onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<void>> removeAsync(
                const std::string& participantId,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return removeAsyncMock(participantId, std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
    }
    MOCK_METHOD4(
            removeAsyncMock,
            std::shared_ptr<joynr::Future<void>>(
                const std::string& participantId,
                std::function<void(void)> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                boost::optional<joynr::MessagingQos> qos
            )
    );
};

#endif // TESTS_MOCK_MOCKDISCOVERY_H
