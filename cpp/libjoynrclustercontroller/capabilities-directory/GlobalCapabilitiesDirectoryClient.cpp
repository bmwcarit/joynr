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

/*
 * Client for the global capabilities directory.
 */

#include "GlobalCapabilitiesDirectoryClient.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Message.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

namespace joynr
{

GlobalCapabilitiesDirectoryClient::GlobalCapabilitiesDirectoryClient(
        const ClusterControllerSettings& clusterControllerSettings)
        : _capabilitiesProxy(nullptr),
          _messagingQos(),
          _touchTtl(static_cast<std::uint64_t>(
                  clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs().count())),
          _removeStaleTtl(3600000)
{
}

void GlobalCapabilitiesDirectoryClient::add(
        const types::GlobalDiscoveryEntry& entry,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos addMessagingQos = _messagingQos;
    addMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    _capabilitiesProxy->addAsync(entry,
                                 std::move(gbids),
                                 std::move(onSuccess),
                                 std::move(onError),
                                 std::move(onRuntimeError),
                                 addMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::remove(
        const std::string& participantId,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos removeMessagingQos = _messagingQos;
    removeMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    _capabilitiesProxy->removeAsync(participantId,
                                    std::move(gbids),
                                    std::move(onSuccess),
                                    std::move(onError),
                                    std::move(onRuntimeError),
                                    std::move(removeMessagingQos));
}

void GlobalCapabilitiesDirectoryClient::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const std::vector<std::string>& gbids,
        std::int64_t messagingTtl,
        std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos lookupMessagingQos = _messagingQos;
    lookupMessagingQos.setTtl(static_cast<std::uint64_t>(messagingTtl));
    lookupMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    _capabilitiesProxy->lookupAsync(domains,
                                    interfaceName,
                                    std::move(gbids),
                                    std::move(onSuccess),
                                    std::move(onError),
                                    std::move(onRuntimeError),
                                    lookupMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::lookup(
        const std::string& participantId,
        const std::vector<std::string>& gbids,
        std::int64_t messagingTtl,
        std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos lookupMessagingQos = _messagingQos;
    lookupMessagingQos.setTtl(static_cast<std::uint64_t>(messagingTtl));
    lookupMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    _capabilitiesProxy->lookupAsync(participantId,
                                    std::move(gbids),
                                    [onSuccess = std::move(onSuccess)](
                                            const joynr::types::GlobalDiscoveryEntry& capability) {
                                        std::vector<joynr::types::GlobalDiscoveryEntry> result;
                                        result.push_back(capability);
                                        onSuccess(result);
                                    },
                                    std::move(onError),
                                    std::move(onRuntimeError),
                                    lookupMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::touch(
        const std::string& clusterControllerId,
        const std::vector<std::string>& participantIds,
        const std::string& gbid,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError)
{
    MessagingQos touchMessagingQos = _messagingQos;
    touchMessagingQos.setTtl(_touchTtl);
    touchMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbid);
    _capabilitiesProxy->touchAsync(clusterControllerId,
                                   participantIds,
                                   std::move(onSuccess),
                                   std::move(onError),
                                   touchMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::setProxy(
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy,
        MessagingQos messagingQos)
{
    this->_capabilitiesProxy = std::move(capabilitiesProxy);
    this->_messagingQos = std::move(messagingQos);
}

void GlobalCapabilitiesDirectoryClient::removeStale(
        const std::string& clusterControllerId,
        std::int64_t maxLastSeenDateMs,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError)
{
    MessagingQos removeStaleMessagingQos = _messagingQos;
    removeStaleMessagingQos.setTtl(_removeStaleTtl);
    _capabilitiesProxy->removeStaleAsync(clusterControllerId,
                                         maxLastSeenDateMs,
                                         std::move(onSuccess),
                                         std::move(onRuntimeError),
                                         removeStaleMessagingQos);
}

} // namespace joynr
