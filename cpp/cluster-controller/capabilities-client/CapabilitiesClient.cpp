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
*   Client for the capabilities directory. It uses a joynr proxy provided by
*   an instance of libjoynr running on the clusterController.
*
*/

#include "cluster-controller/capabilities-client/CapabilitiesClient.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/Future.h"
#include "joynr/types/GlobalDiscoveryEntry.h"
#include <string>
#include <cstdint>
#include <cassert>

namespace joynr
{

INIT_LOGGER(CapabilitiesClient);

CapabilitiesClient::CapabilitiesClient(const std::string& localChannelId)
        : localChannelId(localChannelId), capabilitiesProxy(nullptr)
{
}

std::string CapabilitiesClient::getLocalChannelId()
{
    return localChannelId;
}

void CapabilitiesClient::add(std::vector<types::GlobalDiscoveryEntry> capabilitiesInformationList)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method
    if (localChannelId.empty()) {
        assert(false); // "Assertion in CapabilitiesClient: Local channelId is empty. Tried to
                       // register capabilities before messaging was started(no queueing implemented
                       // yet;
    } else {
        for (std::uint32_t i = 0; i < capabilitiesInformationList.size(); i++) {
            capabilitiesInformationList[i].setAddress(localChannelId);
        }
        // TM switching from sync to async
        // capabilitiesProxy->add(capabilitiesInformationList);

        std::function<void(const exceptions::JoynrException&)> onError =
                [&](const exceptions::JoynrException& error) {
            std::ignore = error;
            JOYNR_LOG_ERROR(logger, "Error occured during the execution of capabilitiesProxy->add");
        };
        capabilitiesProxy->addAsync(capabilitiesInformationList, nullptr, onError);
    }
}

void CapabilitiesClient::remove(const std::string& participantId)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method
    capabilitiesProxy->remove(participantId);
}

void CapabilitiesClient::remove(std::vector<std::string> participantIdList)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method
    capabilitiesProxy->remove(participantIdList);
}

std::vector<types::GlobalDiscoveryEntry> CapabilitiesClient::lookup(
        const std::string& domain,
        const std::string& interfaceName)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method

    std::vector<types::GlobalDiscoveryEntry> result;
    capabilitiesProxy->lookup(result, domain, interfaceName);
    return result;
}

void CapabilitiesClient::lookup(
        const std::string& domain,
        const std::string& interfaceName,
        std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method

    capabilitiesProxy->lookupAsync(domain, interfaceName, onSuccess, onError);
}

void CapabilitiesClient::lookup(
        const std::string& participantId,
        std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method
    capabilitiesProxy->lookupAsync(
            participantId,
            [onSuccess](const joynr::types::GlobalDiscoveryEntry& capability) {
                std::vector<joynr::types::GlobalDiscoveryEntry> result;
                result.push_back(capability);
                onSuccess(result);
            },
            onError);
}

void CapabilitiesClient::init(
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy)
{
    this->capabilitiesProxy = capabilitiesProxy;
}

} // namespace joynr
