/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/exceptions.h"
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
    // We will be deserializing CapabilityInformation - register the metatypes
    qRegisterMetaType<joynr::types::CapabilityInformation>("joynr::types::CapabilityInformation");
    qRegisterMetaType<joynr__types__CapabilityInformation>("joynr__types__CapabilityInformation");
}

CapabilitiesClient::~CapabilitiesClient()
{
}

std::string CapabilitiesClient::getLocalChannelId()
{
    return localChannelId;
}

void CapabilitiesClient::add(
        std::vector<types::StdCapabilityInformation> capabilitiesInformationList)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
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

        std::function<void(const RequestStatus& status)> callbackFct =
                [](const RequestStatus& status) {
            // check requestStatus?
            if (!status.successful()) {
                LOG_ERROR(logger,
                          QString("Error occured during the execution of capabilitiesProxy->add"));
            }
        };
        capabilitiesProxy->add(capabilitiesInformationList, callbackFct);
    }
}

void CapabilitiesClient::remove(const std::string& participantId)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method
    RequestStatus status;
    capabilitiesProxy->remove(status, participantId);
}

void CapabilitiesClient::remove(std::vector<std::string> participantIdList)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method
    RequestStatus status;
    capabilitiesProxy->remove(status, participantIdList);
}

std::vector<types::StdCapabilityInformation> CapabilitiesClient::lookup(
        const std::string& domain,
        const std::string& interfaceName)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method

    RequestStatus status;
    std::vector<types::StdCapabilityInformation> result;
    capabilitiesProxy->lookup(status, result, domain, interfaceName);
    return result;
}

void CapabilitiesClient::lookup(
        const std::string& domain,
        const std::string& interfaceName,
        std::function<void(const RequestStatus& status,
                           const std::vector<types::StdCapabilityInformation>& result)> callbackFct)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method

    capabilitiesProxy->lookup(domain, interfaceName, callbackFct);
}

void CapabilitiesClient::lookup(
        const std::string& participantId,
        std::function<void(const RequestStatus& status,
                           const std::vector<joynr::types::StdCapabilityInformation>& result)>
                callbackFct)
{
    assert(!capabilitiesProxy.isNull()); // calls to the capabilitiesClient are only allowed, once
                                         // the capabilitiesProxy has been set via the init method
    capabilitiesProxy->lookup(
            participantId,
            [callbackFct](const RequestStatus& status,
                          const joynr::types::StdCapabilityInformation& capability) {
                std::vector<joynr::types::StdCapabilityInformation> result;
                result.push_back(capability);
                callbackFct(status, result);
            });
}

void CapabilitiesClient::init(
        QSharedPointer<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy)
{
    this->capabilitiesProxy = capabilitiesProxy;
}

} // namespace joynr
