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
#include "libjoynr/dbus/DbusCapabilitiesStubAdapter.h"
#include "libjoynr/dbus/DbusMessagingEndpointAddress.h"
#include "joynr/types/ProviderQos.h"
#include "libjoynr/dbus/DbusCapabilitiesUtil.h"

namespace joynr {

using namespace joynr_logging;

DbusCapabilitiesStubAdapter::DbusCapabilitiesStubAdapter(QString serviceAddress)
    : IDbusStubWrapper(serviceAddress)
{
    // init logger
    logger = Logging::getInstance()->getLogger("MSG", "DbusCapabilitiesStubAdapter");
    LOG_INFO(logger, "Get dbus proxy on address: " + serviceAddress);

    // init the stub
    init();
}

QList<CapabilityEntry> DbusCapabilitiesStubAdapter::lookup(const QString &domain, const QString &interfaceName, const DiscoveryQos& discoveryQos){
    logMethodCall("lookup");
    // transform data
    joynr::messaging::types::Types::DiscoveryQos dbusDiscoveryQos;
    DbusCapabilitiesUtil::copyJoynrDiscoveryQosToDbus(discoveryQos, dbusDiscoveryQos);

    // call
    CommonAPI::CallStatus status;
    joynr::messaging::types::Types::CapabilityEntryList capaEntryList;
    proxy->lookup1(domain.toStdString(), interfaceName.toStdString(), dbusDiscoveryQos, status, capaEntryList);
    printCallStatus(status, "lookup");

    // transform result
    QList<CapabilityEntry> list;
    DbusCapabilitiesUtil::copyDbusCapaEntryListToJoynr(capaEntryList, list);

    return list;
}

QList<CapabilityEntry> DbusCapabilitiesStubAdapter::lookup(const QString &participantId, const DiscoveryQos& discoveryQos){
    logMethodCall("lookup");

    // transform data
    joynr::messaging::types::Types::DiscoveryQos dbusDiscoveryQos;
    DbusCapabilitiesUtil::copyJoynrDiscoveryQosToDbus(discoveryQos, dbusDiscoveryQos);

    // call
    CommonAPI::CallStatus status;
    joynr::messaging::types::Types::CapabilityEntryList capaEntryList;
    proxy->lookup2(participantId.toStdString(), dbusDiscoveryQos, status, capaEntryList);
    printCallStatus(status, "lookup");

    // transform result
    QList<CapabilityEntry> list;
    DbusCapabilitiesUtil::copyDbusCapaEntryListToJoynr(capaEntryList, list);

    return list;
}

void DbusCapabilitiesStubAdapter::addEndpoint(const QString &participantId, QSharedPointer<joynr::system::Address> messagingStubAddress, const qint64& timeout_ms){
    logMethodCall("addEndpoint");

    CommonAPI::CallStatus status;
    auto addr = dynamic_cast<DbusMessagingEndpointAddress*>(messagingStubAddress.data());
    joynr::messaging::types::Types::EndpointAddressBase endPoint(addr->getServiceAddress().toStdString());
    proxy->addEndPoint(participantId.toStdString(), endPoint, timeout_ms, status);
    printCallStatus(status, "addEndpoint");
}

void DbusCapabilitiesStubAdapter::add(const QString &domain, const QString &interfaceName, const QString &participantId, const types::ProviderQos &qos, QList<QSharedPointer<joynr::system::Address> > endpointAddressList, QSharedPointer<joynr::system::Address> messagingStubAddress, const qint64& timeout_ms){
    logMethodCall("add");

    joynr::messaging::types::Types::ProviderQos dbusQos;
    DbusCapabilitiesUtil::copyJoynrProviderQosToDbus(qos, dbusQos);

    joynr::messaging::types::Types::EndpointAddressList endPointList;
    DbusCapabilitiesUtil::copyJoynrEndPointListToDbus(endpointAddressList, endPointList);

    auto addr = dynamic_cast<DbusMessagingEndpointAddress*>(messagingStubAddress.data());
    joynr::messaging::types::Types::EndpointAddressBase dbusEndPoint(addr->getServiceAddress().toStdString());

    CommonAPI::CallStatus status;
    proxy->add(domain.toStdString(), interfaceName.toStdString(), participantId.toStdString(), dbusQos, endPointList, dbusEndPoint, timeout_ms, status);
    printCallStatus(status, "add");
}

void DbusCapabilitiesStubAdapter::remove(const QString& participantId, const qint64& timeout_ms){
    logMethodCall("remove");
    CommonAPI::CallStatus status;
    proxy->remove(participantId.toStdString(), timeout_ms, status);
    printCallStatus(status, "remove");
}

} // namespace joynr
