/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <string>
#include <cstdint>
#include <cassert>

#include "cluster-controller/capabilities-client/CapabilitiesClient.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/Future.h"
#include "joynr/MessagingQos.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/types/DiscoveryQos.h"

namespace joynr
{

INIT_LOGGER(CapabilitiesClient);

// The capabilitiesProxyBuilder will use as MessagingQoS the one provided by the CapabilitiesClient
// user. If nothing is provided the default MessageQoS::ttl will be used.
//
// Additionally, if the capabilitiesProxyBuilder is not configured correctly an excpetion will
// be thrown (see ProxyBuilder::build())
// The configuration excpects that a discoveryQoS is set (see ProxyBuilder::setDiscoveryQos())
// The capabilitiesClient should not be responsible to change the DiscoveryQoS.

CapabilitiesClient::CapabilitiesClient()
        : defaultCapabilitiesProxy(nullptr), capabilitiesProxyBuilder(nullptr)
{
}

std::unique_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> CapabilitiesClient::
        getGlobalCapabilitiesDirectoryProxy(std::int64_t messagingTtl)
{
    assert(capabilitiesProxyBuilder);
    return std::unique_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy>(
            capabilitiesProxyBuilder->setMessagingQos(MessagingQos(messagingTtl))->build());
}

void CapabilitiesClient::add(
        const std::vector<types::GlobalDiscoveryEntry>& capabilitiesInformationList)
{
    if (capabilitiesInformationList.empty()) {
        return;
    }

    std::function<void(const exceptions::JoynrException&)> onError =
            [&](const exceptions::JoynrException& error) {
        std::ignore = error;
        JOYNR_LOG_ERROR(logger, "Error occured during the execution of capabilitiesProxy->add");
    };
    assert(defaultCapabilitiesProxy);
    defaultCapabilitiesProxy->addAsync(capabilitiesInformationList, nullptr, onError);
}

void CapabilitiesClient::remove(const std::string& participantId)
{
    assert(defaultCapabilitiesProxy);
    defaultCapabilitiesProxy->remove(participantId);
}

void CapabilitiesClient::remove(std::vector<std::string> participantIdList)
{
    assert(defaultCapabilitiesProxy);
    defaultCapabilitiesProxy->remove(participantIdList);
}

std::vector<types::GlobalDiscoveryEntry> CapabilitiesClient::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const std::int64_t messagingTtl)
{
    std::unique_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> proxy =
            getGlobalCapabilitiesDirectoryProxy(messagingTtl);
    std::vector<types::GlobalDiscoveryEntry> result;
    proxy->lookup(result, domains, interfaceName);
    return result;
}

void CapabilitiesClient::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const std::int64_t messagingTtl,
        std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    std::unique_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> proxy =
            getGlobalCapabilitiesDirectoryProxy(messagingTtl);
    proxy->lookupAsync(domains, interfaceName, onSuccess, onError);
}

void CapabilitiesClient::lookup(
        const std::string& participantId,
        std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    assert(defaultCapabilitiesProxy);
    defaultCapabilitiesProxy->lookupAsync(
            participantId,
            [onSuccess](const joynr::types::GlobalDiscoveryEntry& capability) {
                std::vector<joynr::types::GlobalDiscoveryEntry> result;
                result.push_back(capability);
                onSuccess(result);
            },
            onError);
}

void CapabilitiesClient::setProxyBuilder(std::unique_ptr<
        IProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>> inCapabilitiesProxyBuilder)
{
    assert(inCapabilitiesProxyBuilder);
    capabilitiesProxyBuilder = std::move(inCapabilitiesProxyBuilder);
    defaultCapabilitiesProxy.reset(capabilitiesProxyBuilder->build());
}

} // namespace joynr
