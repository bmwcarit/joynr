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
#ifndef LOCALDISCOVERYAGGREGATOR_H
#define LOCALDISCOVERYAGGREGATOR_H

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/system/IDiscovery.h"

namespace joynr
{

namespace types
{

class DiscoveryEntry;
class DiscoveryEntryWithMetaInfo;
class DiscoveryQos;
} // namespace types

/**
 * @brief The LocalDiscoveryAggregator class is a wrapper for discovery proxies. It holds a list
 * of provisioned discovery entries (for example for the discovery and routing provider). If a
 * lookup is performed by using a participant ID, these entries are checked and returned first
 * before the request is forwarded to the wrapped discovery provider.
 */
class JOYNR_EXPORT LocalDiscoveryAggregator : public joynr::system::IDiscoveryAsync
{
public:
    LocalDiscoveryAggregator(std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo>
                                     provisionedDiscoveryEntries);

    void setDiscoveryProxy(std::shared_ptr<IDiscoveryAsync> discoveryProxy);

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            const bool& awaitGlobalRegistration,
            const std::vector<std::string>& gbids,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            const bool& awaitGlobalRegistration,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> addToAllAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            const bool& awaitGlobalRegistration,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>
    lookupAsync(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>
    lookupAsync(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess = nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
            const std::string& participantId,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess =
                    nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
            const std::string& participantId,
            const joynr::types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess =
                    nullptr,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)>
                    onApplicationError = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

    // inherited from joynr::system::IDiscoveryAsync
    std::shared_ptr<joynr::Future<void>> removeAsync(
            const std::string& participantId,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr,
            boost::optional<joynr::MessagingQos> messagingQos = boost::none) noexcept override;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryAggregator);

    std::shared_ptr<joynr::Future<types::DiscoveryEntryWithMetaInfo>> findProvisionedEntry(
            const std::string& participantId,
            std::function<void(const types::DiscoveryEntryWithMetaInfo&)> onSuccess) noexcept;

    std::shared_ptr<joynr::system::IDiscoveryAsync> _discoveryProxy;
    const std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo>
            _provisionedDiscoveryEntries;
};
} // namespace joynr
#endif // LOCALDISCOVERYAGGREGATOR_H
