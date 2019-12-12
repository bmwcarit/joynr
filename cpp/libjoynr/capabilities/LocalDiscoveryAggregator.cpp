/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/LocalDiscoveryAggregator.h"

#include <cassert>
#include <memory>
#include <utility>

#include "joynr/Future.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

LocalDiscoveryAggregator::LocalDiscoveryAggregator(
        std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> provisionedDiscoveryEntries)
        : _discoveryProxy(), _provisionedDiscoveryEntries(std::move(provisionedDiscoveryEntries))
{
}

void LocalDiscoveryAggregator::setDiscoveryProxy(std::shared_ptr<IDiscoveryAsync> discoveryProxy)
{
    this->_discoveryProxy = std::move(discoveryProxy);
}

#define REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(FUTURE_TYPE)                            \
    if (!_discoveryProxy) {                                                                        \
        const std::string errorMsg("internal discoveryProxy not set");                             \
        if (onRuntimeError) {                                                                      \
            onRuntimeError(exceptions::JoynrRuntimeException(errorMsg));                           \
        }                                                                                          \
        auto retFuture = std::make_shared<joynr::Future<FUTURE_TYPE>>();                           \
        retFuture->onError(std::make_shared<exceptions::JoynrRuntimeException>(errorMsg));         \
        return retFuture;                                                                          \
    }

std::shared_ptr<joynr::Future<void>> LocalDiscoveryAggregator::addAsync(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(void)
    assert(_discoveryProxy);
    return _discoveryProxy->addAsync(discoveryEntry,
                                     std::move(onSuccess),
                                     std::move(onRuntimeError),
                                     std::move(messagingQos));
}

std::shared_ptr<joynr::Future<void>> LocalDiscoveryAggregator::addAsync(
        const types::DiscoveryEntry& discoveryEntry,
        const bool& awaitGlobalRegistration,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(void)
    assert(_discoveryProxy);
    return _discoveryProxy->addAsync(discoveryEntry,
                                     awaitGlobalRegistration,
                                     std::move(onSuccess),
                                     std::move(onRuntimeError),
                                     std::move(messagingQos));
}

std::shared_ptr<joynr::Future<void>> LocalDiscoveryAggregator::addAsync(
        const joynr::types::DiscoveryEntry& discoveryEntry,
        const bool& awaitGlobalRegistration,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(void)
    assert(_discoveryProxy);
    return _discoveryProxy->addAsync(discoveryEntry,
                                     awaitGlobalRegistration,
                                     gbids,
                                     std::move(onSuccess),
                                     std::move(onApplicationError),
                                     std::move(onRuntimeError),
                                     std::move(messagingQos));
}

std::shared_ptr<joynr::Future<void>> LocalDiscoveryAggregator::addToAllAsync(
        const joynr::types::DiscoveryEntry& discoveryEntry,
        const bool& awaitGlobalRegistration,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(void)
    assert(_discoveryProxy);
    return _discoveryProxy->addToAllAsync(discoveryEntry,
                                          awaitGlobalRegistration,
                                          std::move(onSuccess),
                                          std::move(onApplicationError),
                                          std::move(onRuntimeError),
                                          std::move(messagingQos));
}

std::shared_ptr<joynr::Future<std::vector<types::DiscoveryEntryWithMetaInfo>>>
LocalDiscoveryAggregator::lookupAsync(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const types::DiscoveryQos& discoveryQos,
        std::function<void(const std::vector<types::DiscoveryEntryWithMetaInfo>&)> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(
            std::vector<types::DiscoveryEntryWithMetaInfo>)
    assert(_discoveryProxy);
    return _discoveryProxy->lookupAsync(domains,
                                        interfaceName,
                                        discoveryQos,
                                        std::move(onSuccess),
                                        std::move(onRuntimeError),
                                        std::move(messagingQos));
}

std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>
LocalDiscoveryAggregator::lookupAsync(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(
            std::vector<types::DiscoveryEntryWithMetaInfo>)
    assert(_discoveryProxy);
    return _discoveryProxy->lookupAsync(domains,
                                        interfaceName,
                                        discoveryQos,
                                        gbids,
                                        std::move(onSuccess),
                                        std::move(onApplicationError),
                                        std::move(onRuntimeError),
                                        std::move(messagingQos));
}

std::shared_ptr<joynr::Future<types::DiscoveryEntryWithMetaInfo>> LocalDiscoveryAggregator::
        findProvisionedEntry(
                const std::string& participantId,
                std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess) noexcept
{
    auto entry = _provisionedDiscoveryEntries.find(participantId);
    if (entry != _provisionedDiscoveryEntries.cend()) {
        if (onSuccess) {
            onSuccess(entry->second);
        }
        auto future = std::make_shared<joynr::Future<types::DiscoveryEntryWithMetaInfo>>();
        future->onSuccess(entry->second);
        return future;
    }
    return nullptr;
}

std::shared_ptr<joynr::Future<types::DiscoveryEntryWithMetaInfo>> LocalDiscoveryAggregator::
        lookupAsync(const std::string& participantId,
                    std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess,
                    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError,
                    boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    if (auto future = findProvisionedEntry(participantId, onSuccess)) {
        return future;
    } else {
        REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(types::DiscoveryEntryWithMetaInfo)
        assert(_discoveryProxy);
        return _discoveryProxy->lookupAsync(participantId,
                                            std::move(onSuccess),
                                            std::move(onRuntimeError),
                                            std::move(messagingQos));
    }
}

std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>
LocalDiscoveryAggregator::lookupAsync(
        const std::string& participantId,
        const joynr::types::DiscoveryQos& discoveryQos,
        const std::vector<std::string>& gbids,
        std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onApplicationError,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    if (auto future = findProvisionedEntry(participantId, onSuccess)) {
        return future;
    } else {
        REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(types::DiscoveryEntryWithMetaInfo)
        assert(_discoveryProxy);
        return _discoveryProxy->lookupAsync(participantId,
                                            discoveryQos,
                                            gbids,
                                            std::move(onSuccess),
                                            std::move(onApplicationError),
                                            std::move(onRuntimeError),
                                            std::move(messagingQos));
    }
}

std::shared_ptr<joynr::Future<void>> LocalDiscoveryAggregator::removeAsync(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    REPORT_ERROR_AND_RETURN_IF_DISCOVERY_PROXY_NOT_SET(void)
    assert(_discoveryProxy);
    return _discoveryProxy->removeAsync(participantId,
                                        std::move(onSuccess),
                                        std::move(onRuntimeError),
                                        std::move(messagingQos));
}

} // namespace joynr
