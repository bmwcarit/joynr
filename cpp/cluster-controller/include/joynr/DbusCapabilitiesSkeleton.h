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
#ifndef DBUSCAPABILITESSKELETON_H
#define DBUSCAPABILITESSKELETON_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"

#include "joynr/ICapabilities.h"

// save the GCC diagnostic state
#pragma GCC diagnostic push
// Disable compiler warnings in this CommonAPI generated includes.
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Weffc++"
// include CommonAPI stuff here:
#include "common-api/joynr/messaging/ICapabilitiesStubDefault.h"
// restore the old GCC diagnostic state
#pragma GCC diagnostic pop

namespace joynr {

class JOYNRCLUSTERCONTROLLER_EXPORT DbusCapabilitiesSkeleton: public joynr::messaging::ICapabilitiesStubDefault {
public:

    DbusCapabilitiesSkeleton(ICapabilities& callBack);

    virtual void add(std::string domain,
                    std::string interfaceName,
                    std::string participantId,
                    joynr::messaging::types::Types::ProviderQos qos,
                    joynr::messaging::types::Types::EndpointAddressList endpointAddressList,
                    joynr::messaging::types::Types::EndpointAddressBase messagingStubAddress,
                    int64_t timeout_ms);

    virtual void addEndPoint(std::string participantId,
                    joynr::messaging::types::Types::EndpointAddressBase messagingStubAddress,
                    int64_t timeout_ms);

    virtual void lookup1(std::string domain,
                    std::string interfaceName,
                    joynr::messaging::types::Types::ProviderQosRequirement qos,
                    joynr::messaging::types::Types::DiscoveryQos discoveryQos,
                    joynr::messaging::types::Types::CapabilityEntryList& result);

    virtual void lookup2(std::string participandId,
                    joynr::messaging::types::Types::DiscoveryQos discoveryQos,
                    joynr::messaging::types::Types::CapabilityEntryList& result);

    virtual void remove(std::string participantId, int64_t timeout_ms);

private:
    DISALLOW_COPY_AND_ASSIGN(DbusCapabilitiesSkeleton);
    ICapabilities& callBack;
};


} // namespace joynr
#endif // DBUSCAPABILITESSKELETON_H
