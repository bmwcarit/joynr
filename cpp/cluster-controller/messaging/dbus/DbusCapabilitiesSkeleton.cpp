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
#include "joynr/DbusCapabilitiesSkeleton.h"
#include "libjoynr/dbus/DbusCapabilitiesUtil.h"
#include "libjoynr/dbus/DbusMessagingEndpointAddress.h"

namespace joynr {

DbusCapabilitiesSkeleton::DbusCapabilitiesSkeleton(ICapabilities& callBack):
    callBack(callBack)
{
}

void DbusCapabilitiesSkeleton::add(std::string domain,
                std::string interfaceName,
                std::string participantId,
                joynr::messaging::types::Types::ProviderQos qos,
                joynr::messaging::types::Types::EndpointAddressList endpointAddressList,
                joynr::messaging::types::Types::EndpointAddressBase messagingStubAddress,
                int64_t timeout_ms) {
    // tranform data
    types::ProviderQos joynrQos;
    DbusCapabilitiesUtil::copyDbusProviderQosToJoynr(qos, joynrQos);

    DbusMessagingEndpointAddress* dbusAddr = new DbusMessagingEndpointAddress(QString::fromStdString(messagingStubAddress.endPointAddress));
    QSharedPointer<DbusMessagingEndpointAddress >dbusAddrPointer(dbusAddr);

    QList<QSharedPointer<joynr::system::Address>> joynrList;
    DbusCapabilitiesUtil::copyDbusEndPointListToJoynr(endpointAddressList, joynrList);
    // call
    callBack.add(QString::fromStdString(domain), QString::fromStdString(interfaceName), QString::fromStdString(participantId), joynrQos, joynrList, dbusAddrPointer, timeout_ms);
}

void DbusCapabilitiesSkeleton::addEndPoint(std::string participantId,
                joynr::messaging::types::Types::EndpointAddressBase messagingStubAddress,
                int64_t timeout_ms) {

    DbusMessagingEndpointAddress* dbusAddr = new DbusMessagingEndpointAddress(QString::fromStdString(messagingStubAddress.endPointAddress));
    QSharedPointer<DbusMessagingEndpointAddress >dbusAddrPointer(dbusAddr);
    callBack.addEndpoint(QString::fromStdString(participantId), dbusAddrPointer, timeout_ms);
}

void DbusCapabilitiesSkeleton::lookup1(std::string domain,
                std::string interfaceName,
                joynr::messaging::types::Types::ProviderQosRequirement qos,
                joynr::messaging::types::Types::DiscoveryQos discoveryQos,
                joynr::messaging::types::Types::CapabilityEntryList& result) {

    // transform data
    types::ProviderQosRequirements joynrQos;
    DbusCapabilitiesUtil::copyDbusProviderQosRequirementsToJoynr(qos, joynrQos);

    DiscoveryQos joynrDiscoveryQos;
    DbusCapabilitiesUtil::copyDbusDiscoveryQosToJoynr(discoveryQos, joynrDiscoveryQos);

    // call the function
    QList<CapabilityEntry> joynrResult = callBack.lookup(QString::fromStdString(domain), QString::fromStdString(interfaceName), joynrQos, joynrDiscoveryQos);

    // transform result
    DbusCapabilitiesUtil::copyJoynrCapaEntryListToDbus(joynrResult, result);
}

void DbusCapabilitiesSkeleton::lookup2(std::string participandId,
                joynr::messaging::types::Types::DiscoveryQos discoveryQos,
                joynr::messaging::types::Types::CapabilityEntryList& result) {

    DiscoveryQos joynrDiscoveryQos;
    DbusCapabilitiesUtil::copyDbusDiscoveryQosToJoynr(discoveryQos, joynrDiscoveryQos);

    // call the function
    QList<CapabilityEntry> joynrResult = callBack.lookup(QString::fromStdString(participandId), joynrDiscoveryQos);

    // transform result
    DbusCapabilitiesUtil::copyJoynrCapaEntryListToDbus(joynrResult, result);
}

void DbusCapabilitiesSkeleton::remove(std::string participantId, int64_t timeout_ms) {
    callBack.remove(QString::fromStdString(participantId), timeout_ms);
}

} // namespace joynr
