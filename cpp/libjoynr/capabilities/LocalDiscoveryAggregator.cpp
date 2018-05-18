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
#include <chrono>
#include <limits>
#include <memory>
#include <utility>

#include "joynr/Future.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

LocalDiscoveryAggregator::LocalDiscoveryAggregator(
        std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> provisionedDiscoveryEntries)
        : discoveryProxy(), provisionedDiscoveryEntries(std::move(provisionedDiscoveryEntries))
{
}

void LocalDiscoveryAggregator::setDiscoveryProxy(std::shared_ptr<IDiscoveryAsync> discoveryProxy)
{
    this->discoveryProxy = std::move(discoveryProxy);
}

std::shared_ptr<joynr::Future<void>> LocalDiscoveryAggregator::addAsync(
        const types::DiscoveryEntry& discoveryEntry,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError,
        boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    assert(discoveryProxy);
    return discoveryProxy->addAsync(discoveryEntry,
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
    assert(discoveryProxy);
    return discoveryProxy->addAsync(discoveryEntry,
                                    awaitGlobalRegistration,
                                    std::move(onSuccess),
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
    assert(discoveryProxy);
    return discoveryProxy->lookupAsync(domains,
                                       interfaceName,
                                       discoveryQos,
                                       std::move(onSuccess),
                                       std::move(onRuntimeError),
                                       std::move(messagingQos));
}

std::shared_ptr<joynr::Future<types::DiscoveryEntryWithMetaInfo>> LocalDiscoveryAggregator::
        lookupAsync(const std::string& participantId,
                    std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess,
                    std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError,
                    boost::optional<joynr::MessagingQos> messagingQos) noexcept
{
    auto entry = provisionedDiscoveryEntries.find(participantId);
    if (entry != provisionedDiscoveryEntries.cend()) {
        if (onSuccess) {
            onSuccess(entry->second);
        }
        auto future = std::make_shared<joynr::Future<types::DiscoveryEntryWithMetaInfo>>();
        future->onSuccess(entry->second);
        return future;
    } else {
        assert(discoveryProxy);
        return discoveryProxy->lookupAsync(participantId,
                                           std::move(onSuccess),
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
    assert(discoveryProxy);
    return discoveryProxy->removeAsync(participantId,
                                       std::move(onSuccess),
                                       std::move(onRuntimeError),
                                       std::move(messagingQos));
}

} // namespace joynr
