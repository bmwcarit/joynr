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

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Message.h"
#include "libjoynrclustercontroller/capabilities-client/GlobalCapabilitiesDirectoryClient.h"

namespace joynr
{

GlobalCapabilitiesDirectoryClient::GlobalCapabilitiesDirectoryClient(
        const ClusterControllerSettings& clusterControllerSettings)
        : capabilitiesProxy(nullptr),
          messagingQos(),
          touchTtl(clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs().count())
{
}

void GlobalCapabilitiesDirectoryClient::add(
        const types::GlobalDiscoveryEntry& entry,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    capabilitiesProxy->addAsync(entry, onSuccess, onError);
}

void GlobalCapabilitiesDirectoryClient::add(
        const std::vector<joynr::types::GlobalDiscoveryEntry>& globalDiscoveryEntries,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    capabilitiesProxy->addAsync(globalDiscoveryEntries, onSuccess, onRuntimeError);
}

void GlobalCapabilitiesDirectoryClient::add(
        const types::GlobalDiscoveryEntry& entry,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos addMessagingQos = messagingQos;
    addMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    capabilitiesProxy->addAsync(entry,
                                std::move(gbids),
                                std::move(onSuccess),
                                std::move(onError),
                                std::move(onRuntimeError),
                                addMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::remove(const std::string& participantId)
{
    capabilitiesProxy->removeAsync(participantId);
}

void GlobalCapabilitiesDirectoryClient::remove(
        const std::string& participantId,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos removeMessagingQos = messagingQos;
    removeMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    capabilitiesProxy->removeAsync(participantId,
                                   std::move(gbids),
                                   std::move(onSuccess),
                                   std::move(onError),
                                   std::move(onRuntimeError),
                                   std::move(removeMessagingQos));
}

void GlobalCapabilitiesDirectoryClient::remove(std::vector<std::string> participantIdList)
{
    capabilitiesProxy->removeAsync(participantIdList);
}

void GlobalCapabilitiesDirectoryClient::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        std::int64_t messagingTtl,
        std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    MessagingQos lookupMessagingQos = messagingQos;
    lookupMessagingQos.setTtl(messagingTtl);
    capabilitiesProxy->lookupAsync(
            domains, interfaceName, std::move(onSuccess), std::move(onError), lookupMessagingQos);
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
    MessagingQos lookupMessagingQos = messagingQos;
    lookupMessagingQos.setTtl(messagingTtl);
    lookupMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    capabilitiesProxy->lookupAsync(domains,
                                   interfaceName,
                                   std::move(gbids),
                                   std::move(onSuccess),
                                   std::move(onError),
                                   std::move(onRuntimeError),
                                   lookupMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::lookup(
        const std::string& participantId,
        std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    capabilitiesProxy->lookupAsync(participantId,
                                   [onSuccess = std::move(onSuccess)](
                                           const joynr::types::GlobalDiscoveryEntry& capability) {
                                       std::vector<joynr::types::GlobalDiscoveryEntry> result;
                                       result.push_back(capability);
                                       onSuccess(result);
                                   },
                                   std::move(onError));
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
    MessagingQos lookupMessagingQos = messagingQos;
    lookupMessagingQos.setTtl(messagingTtl);
    lookupMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    capabilitiesProxy->lookupAsync(participantId,
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
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError)
{
    MessagingQos touchMessagingQos = messagingQos;
    touchMessagingQos.setTtl(touchTtl);
    capabilitiesProxy->touchAsync(
            clusterControllerId, std::move(onSuccess), std::move(onError), touchMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::setProxy(
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy,
        MessagingQos messagingQos)
{
    this->capabilitiesProxy = std::move(capabilitiesProxy);
    this->messagingQos = std::move(messagingQos);
}

} // namespace joynr
