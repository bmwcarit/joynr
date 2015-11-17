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
#ifndef ICAPABILITIESCLIENT_H
#define ICAPABILITIESCLIENT_H

#include "joynr/types/CapabilityInformation.h"

#include <QSharedPointer>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

class IGlobalCapabilitiesCallback;
class RequestStatus;
class ICapabilitiesClient
{
public:
    virtual ~ICapabilitiesClient()
    {
    }
    virtual void add(std::vector<types::CapabilityInformation> capabilitiesInformationList) = 0;
    virtual void remove(const std::string& participantId) = 0;
    virtual void remove(std::vector<std::string> capabilitiesInformationList) = 0;
    virtual std::vector<types::CapabilityInformation> lookup(const std::string& domain,
                                                             const std::string& interfaceName) = 0;
    virtual void lookup(
            const std::string& domain,
            const std::string& interfaceName,
            std::function<void(const std::vector<joynr::types::CapabilityInformation>&
                                       capabilities)> onSuccess,
            std::function<void(const exceptions::JoynrException& error)> onError = nullptr) = 0;
    virtual void lookup(
            const std::string& participantId,
            std::function<void(const std::vector<joynr::types::CapabilityInformation>&
                                       capabilities)> onSuccess,
            std::function<void(const exceptions::JoynrException& error)> onError = nullptr) = 0;
    virtual std::string getLocalChannelId() = 0;
};

} // namespace joynr
#endif // ICAPABILITIESCLIENT_H
