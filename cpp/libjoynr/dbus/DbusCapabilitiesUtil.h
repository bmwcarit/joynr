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
#ifndef DBUSCAPABILITIESUTIL_H
#define DBUSCAPABILITIESUTIL_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/types/ProviderQosRequirements.h"

#include "joynr/CapabilityEntry.h"
#include "common-api/joynr/messaging/ICapabilities.h"
#include "common-api/joynr/messaging/types/Types.h"

#include "joynr/DiscoveryQos.h"

namespace joynr {

class DbusCapabilitiesUtil
{
public:
    DbusCapabilitiesUtil();

    static void copyJoynrProviderQosRequirementsToDbus(const types::ProviderQosRequirements& joynrProvQosRec, joynr::messaging::types::Types::ProviderQosRequirement& dbusProvQosRec);
    static void copyDbusProviderQosRequirementsToJoynr(const joynr::messaging::types::Types::ProviderQosRequirement& dbusProvQosRec, types::ProviderQosRequirements& joynrProvQosRec);

    static void copyJoynrCapaEntryListToDbus(const QList<CapabilityEntry>& joynrResult, joynr::messaging::types::Types::CapabilityEntryList& dbusList);
    static void copyJoynrCapaEntryToDbus(const CapabilityEntry& joynrEntry, joynr::messaging::types::Types::CapabilityEntry& dbusEntry);

    static void copyDbusCapaEntryToJoynr(const joynr::messaging::types::Types::CapabilityEntry& dbusEntry, CapabilityEntry& joynrEntry);
    static void copyDbusCapaEntryListToJoynr(const joynr::messaging::types::Types::CapabilityEntryList& dbusList, QList<CapabilityEntry>& joynrResult);

    static void copyJoynrProviderQosToDbus(const types::ProviderQos& joynrQos, joynr::messaging::types::Types::ProviderQos& dbusQos);
    static void copyDbusProviderQosToJoynr(const joynr::messaging::types::Types::ProviderQos& dbusQos, types::ProviderQos& joynrQos);

    static void copyJoynrEndPointListToDbus(const QList<QSharedPointer<EndpointAddressBase> >& joynrList, joynr::messaging::types::Types::EndpointAddressList& dbusEndPointList);
    static void copyDbusEndPointListToJoynr(const joynr::messaging::types::Types::EndpointAddressList& dbusEndPointList, QList<QSharedPointer<EndpointAddressBase> >& joynrList);

    static void copyJoynrDiscoveryQosToDbus(const DiscoveryQos& joynrDiscoveryQos, joynr::messaging::types::Types::DiscoveryQos& dbusDiscoveryQos);
    static void copyDbusDiscoveryQosToJoynr(const joynr::messaging::types::Types::DiscoveryQos& dbusDiscoveryQos, DiscoveryQos& joynrDiscoveryQos);
private:
    DISALLOW_COPY_AND_ASSIGN(DbusCapabilitiesUtil);
};


} // namespace joynr
#endif // DBUSCAPABILITIESUTIL_H
