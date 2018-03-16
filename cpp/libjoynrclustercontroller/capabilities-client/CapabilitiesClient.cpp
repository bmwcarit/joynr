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

#include "libjoynrclustercontroller/capabilities-client/CapabilitiesClient.h"

namespace joynr
{

CapabilitiesClient::CapabilitiesClient() : capabilitiesProxy(nullptr), messagingQos()
{
}

void CapabilitiesClient::add(
        const types::GlobalDiscoveryEntry& entry,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    capabilitiesProxy->addAsync(entry, onSuccess, onError);
}

void CapabilitiesClient::add(
        const std::vector<joynr::types::GlobalDiscoveryEntry>& globalDiscoveryEntries,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    capabilitiesProxy->addAsync(globalDiscoveryEntries, onSuccess, onRuntimeError);
}

void CapabilitiesClient::remove(const std::string& participantId)
{
    capabilitiesProxy->removeAsync(participantId);
}

void CapabilitiesClient::remove(std::vector<std::string> participantIdList)
{
    capabilitiesProxy->removeAsync(participantIdList);
}

void CapabilitiesClient::lookup(
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

void CapabilitiesClient::lookup(
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

void CapabilitiesClient::touch(
        const std::string& clusterControllerId,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError)
{
    capabilitiesProxy->touchAsync(clusterControllerId, std::move(onSuccess), std::move(onError));
}

void CapabilitiesClient::setProxy(
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy,
        MessagingQos messagingQos)
{
    this->capabilitiesProxy = std::move(capabilitiesProxy);
    this->messagingQos = std::move(messagingQos);
}

} // namespace joynr
