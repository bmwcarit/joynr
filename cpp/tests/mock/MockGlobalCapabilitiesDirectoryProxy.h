/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
#ifndef MOCKGLOBALCAPABILITIESDIRECTORYPROXY
#define MOCKGLOBALCAPABILITIESDIRECTORYPROXY

#include <vector>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/JoynrRuntimeImpl.h"
#include "joynr/MessagingQos.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

using namespace joynr;
using namespace types;
using namespace exceptions;

class MockGlobalCapabilitiesDirectoryProxy
        : virtual public infrastructure::GlobalCapabilitiesDirectoryProxy
{
public:
    MockGlobalCapabilitiesDirectoryProxy(
            std::weak_ptr<JoynrRuntimeImpl> runtime,
            std::shared_ptr<JoynrMessagingConnectorFactory> connectorFactory)
            : ProxyBase(runtime, connectorFactory, "testdomain", MessagingQos(10000)),
              GlobalCapabilitiesDirectoryProxyBase(runtime,
                                                   connectorFactory,
                                                   "testdomain",
                                                   MessagingQos(10000)),
              GlobalCapabilitiesDirectorySyncProxy(runtime,
                                                   connectorFactory,
                                                   "testdomain",
                                                   MessagingQos(10000)),
              GlobalCapabilitiesDirectoryAsyncProxy(runtime,
                                                    connectorFactory,
                                                    "testdomain",
                                                    MessagingQos(10000)),
              GlobalCapabilitiesDirectoryProxy(runtime,
                                               connectorFactory,
                                               "testdomain",
                                               MessagingQos(10000))
    {
    }

    std::shared_ptr<Future<void>> addAsync(
            const GlobalDiscoveryEntry& globalDiscoveryEntry,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const DiscoveryError::Enum&)> onApplicationError,
            std::function<void(const JoynrRuntimeException&)> onRuntimeError,
            boost::optional<MessagingQos> qos) noexcept override
    {
        return addAsyncMock(globalDiscoveryEntry, gbids, onSuccess, onApplicationError,
                            onRuntimeError, std::make_shared<MessagingQos>(qos.get()));
    }
    MOCK_METHOD6(
            addAsyncMock,
            std::shared_ptr<Future<void>>(
                    const GlobalDiscoveryEntry& globalDiscoveryEntry,
                    const std::vector<std::string>& gbids,
                    std::function<void()> onSuccess,
                    std::function<void(const DiscoveryError::Enum& errorEnum)> onApplicationError,
                    std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
                    std::shared_ptr<MessagingQos> qos));

    std::shared_ptr<Future<std::vector<GlobalDiscoveryEntry>>> lookupAsync(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const std::vector<std::string>& gbids,
            std::function<void(const std::vector<GlobalDiscoveryEntry>& result)> onSuccess,
            std::function<void(const DiscoveryError::Enum& errorEnum)> onApplicationError,
            std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<MessagingQos> qos) noexcept override
    {
        return lookupAsyncMock(domains, interfaceName, gbids, onSuccess, onApplicationError,
                               onRuntimeError, std::make_shared<MessagingQos>(qos.get()));
    }
    MOCK_METHOD7(
            lookupAsyncMock,
            std::shared_ptr<Future<std::vector<GlobalDiscoveryEntry>>>(
                    const std::vector<std::string>& domains,
                    const std::string& interfaceName,
                    const std::vector<std::string>& gbids,
                    std::function<void(const std::vector<GlobalDiscoveryEntry>& result)> onSuccess,
                    std::function<void(const DiscoveryError::Enum& errorEnum)> onApplicationError,
                    std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
                    std::shared_ptr<MessagingQos> qos));

    std::shared_ptr<Future<GlobalDiscoveryEntry>> lookupAsync(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::function<void(const GlobalDiscoveryEntry& result)> onSuccess,
            std::function<void(const DiscoveryError::Enum& errorEnum)> onApplicationError,
            std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<MessagingQos> qos) noexcept override
    {
        return lookupAsyncMock(participantId, gbids, onSuccess, onApplicationError, onRuntimeError,
                               std::make_shared<MessagingQos>(qos.get()));
    }
    MOCK_METHOD6(
            lookupAsyncMock,
            std::shared_ptr<Future<GlobalDiscoveryEntry>>(
                    const std::string& participantId,
                    const std::vector<std::string>& gbids,
                    std::function<void(const GlobalDiscoveryEntry& result)> onSuccess,
                    std::function<void(const DiscoveryError::Enum& errorEnum)> onApplicationError,
                    std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
                    std::shared_ptr<MessagingQos> qos));

    std::shared_ptr<Future<void>> removeAsync(
            const std::string& participantId,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess,
            std::function<void(const DiscoveryError::Enum& errorEnum)> onApplicationError,
            std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<MessagingQos> qos) noexcept override
    {
        return removeAsyncMock(participantId, gbids, onSuccess, onApplicationError, onRuntimeError,
                               std::make_shared<MessagingQos>(qos.get()));
    }
    MOCK_METHOD6(
            removeAsyncMock,
            std::shared_ptr<Future<void>>(
                    const std::string& participantId,
                    const std::vector<std::string>& gbids,
                    std::function<void()> onSuccess,
                    std::function<void(const DiscoveryError::Enum& errorEnum)> onApplicationError,
                    std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
                    std::shared_ptr<MessagingQos> qos));

    std::shared_ptr<Future<void>> touchAsync(
            const std::string& clusterControllerId,
            std::function<void()> onSuccess,
            std::function<void(const JoynrRuntimeException&)> onRuntimeError,
            boost::optional<MessagingQos> qos) noexcept override
    {
        return touchAsyncMock(clusterControllerId, onSuccess, onRuntimeError,
                              std::make_shared<MessagingQos>(qos.get()));
    }
    MOCK_METHOD4(touchAsyncMock,
                 std::shared_ptr<Future<void>>(
                         const std::string& clusterControllerId,
                         std::function<void()> onSuccess,
                         std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
                         std::shared_ptr<MessagingQos> qos));

    std::shared_ptr<Future<void>> touchAsync(
            const std::string& clusterControllerId,
            const std::vector<std::string>& participantIds,
            std::function<void()> onSuccess,
            std::function<void(const JoynrRuntimeException&)> onRuntimeError,
            boost::optional<MessagingQos> qos) noexcept override
    {
        return touchAsyncMock(clusterControllerId, participantIds, onSuccess, onRuntimeError,
                              std::make_shared<MessagingQos>(qos.get()));
    }
    MOCK_METHOD5(touchAsyncMock,
                 std::shared_ptr<Future<void>>(
                         const std::string& clusterControllerId,
                         const std::vector<std::string>& participantIds,
                         std::function<void()> onSuccess,
                         std::function<void(const JoynrRuntimeException& error)> onRuntimeError,
                         std::shared_ptr<MessagingQos> qos));

    std::shared_ptr<joynr::Future<void>> removeStaleAsync(
            const std::string& clusterControllerId,
            const std::int64_t& maxLastSeenDateMs,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError,
            boost::optional<MessagingQos> qos) noexcept override
    {
        return removeStaleAsyncMock(clusterControllerId, maxLastSeenDateMs, onSuccess,
                                    onRuntimeError, std::make_shared<MessagingQos>(qos.get()));
    }
    MOCK_METHOD5(removeStaleAsyncMock,
                 std::shared_ptr<Future<void>>(
                         const std::string& clusterControllerId,
                         const std::int64_t& maxLastSeenDateMs,
                         std::function<void()> onSuccess,
                         std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                                 onRuntimeError,
                         std::shared_ptr<MessagingQos> qos));
};

#endif // MOCKGLOBALCAPABILITIESDIRECTORYPROXY
