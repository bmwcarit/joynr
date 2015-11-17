/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#include <QString>
#include <string>
#include <stdint.h>
#include <cassert>

namespace joynr
{

joynr_logging::Logger* CapabilitiesClient::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "CapabilitiesClient");

CapabilitiesClient::CapabilitiesClient(const std::string& localChannelId)
        : defaultRequestTTL(30000),
          defaultRequestRoundtripTTL(40000),
          capabilitiesClientParticipantId(),
          localChannelId(localChannelId),
          capabilitiesProxy(NULL)
{
    // We will be deserializing QtCapabilityInformation - register the metatypes
    qRegisterMetaType<joynr::types::QtCapabilityInformation>(
            "joynr::types::QtCapabilityInformation");
    qRegisterMetaType<joynr__types__QtCapabilityInformation>("joynr__types__CapabilityInformation");
}

CapabilitiesClient::~CapabilitiesClient()
{
}

std::string CapabilitiesClient::getLocalChannelId()
{
    return localChannelId;
}

void CapabilitiesClient::add(std::vector<types::CapabilityInformation> capabilitiesInformationList)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method
    if (localChannelId.empty()) {
        assert(false); // "Assertion in CapabilitiesClient: Local channelId is empty. Tried to
                       // register capabilities before messaging was started(no queueing implemented
                       // yet;
    } else {
        for (uint32_t i = 0; i < capabilitiesInformationList.size(); i++) {
            capabilitiesInformationList[i].setChannelId(localChannelId);
        }
        RequestStatus rs;
        // TM switching from sync to async
        // capabilitiesProxy->add(rs, capabilitiesInformationList);

        std::function<void(const exceptions::JoynrException&)> onError =
                [](const exceptions::JoynrException& error) {
            (void)error;
            LOG_ERROR(logger,
                      QString("Error occured during the execution of capabilitiesProxy->add"));
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

std::vector<types::CapabilityInformation> CapabilitiesClient::lookup(
        const std::string& domain,
        const std::string& interfaceName)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method

    std::vector<types::CapabilityInformation> result;
    capabilitiesProxy->lookup(result, domain, interfaceName);
    return result;
}

void CapabilitiesClient::lookup(
        const std::string& domain,
        const std::string& interfaceName,
        std::function<void(const std::vector<types::CapabilityInformation>& result)> onSuccess,
        std::function<void(const exceptions::JoynrException& error)> onError)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method

    capabilitiesProxy->lookupAsync(domain, interfaceName, onSuccess, onError);
}

void CapabilitiesClient::lookup(
        const std::string& participantId,
        std::function<void(const std::vector<joynr::types::CapabilityInformation>& result)>
                onSuccess,
        std::function<void(const exceptions::JoynrException& error)> onError)
{
    assert(capabilitiesProxy); // calls to the capabilitiesClient are only allowed, once
                               // the capabilitiesProxy has been set via the init method
    capabilitiesProxy->lookupAsync(
            participantId,
            [onSuccess](const joynr::types::CapabilityInformation& capability) {
                std::vector<joynr::types::CapabilityInformation> result;
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
