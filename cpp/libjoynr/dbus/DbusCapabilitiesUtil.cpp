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
#include "libjoynr/dbus/DbusCapabilitiesUtil.h"
#include "joynr/JoynrMessagingViaCCEndpointAddress.h"

namespace joynr {

void DbusCapabilitiesUtil::copyJoynrProviderQosRequirementsToDbus(const types::ProviderQosRequirements& joynrProvQosRec, joynr::messaging::types::Types::ProviderQosRequirement& dbusProvQosRec) {
    dbusProvQosRec.notYetImplemented = joynrProvQosRec.getNotYetImplemented();
}

void DbusCapabilitiesUtil::copyDbusProviderQosRequirementsToJoynr(const joynr::messaging::types::Types::ProviderQosRequirement& dbusProvQosRec, types::ProviderQosRequirements& joynrProvQosRec) {
    joynrProvQosRec.setNotYetImplemented(dbusProvQosRec.notYetImplemented);
}

void DbusCapabilitiesUtil::copyJoynrCapaEntryListToDbus(const QList<CapabilityEntry>& joynrResult, joynr::messaging::types::Types::CapabilityEntryList& dbusList) {
    for(auto it = joynrResult.begin(); it != joynrResult.end(); it++) {
        joynr::messaging::types::Types::CapabilityEntry entry;
        copyJoynrCapaEntryToDbus(*it, entry);
        dbusList.insert(dbusList.end(), entry);
    }
}

void DbusCapabilitiesUtil::copyJoynrCapaEntryToDbus(const CapabilityEntry& joynrEntry, joynr::messaging::types::Types::CapabilityEntry& dbusEntry) {
    dbusEntry.interfaceName = joynrEntry.getInterfaceName().toStdString();
    dbusEntry.domain = joynrEntry.getDomain().toStdString();
    dbusEntry.participantId = joynrEntry.getParticipantId().toStdString();
    dbusEntry.global = joynrEntry.isGlobal();

    // copy qos
    joynr::messaging::types::Types::ProviderQos qos;
    copyJoynrProviderQosToDbus(joynrEntry.getQos(), qos);
    dbusEntry.qos = qos;

    // at the moment only joynr messaging endpoint addresses are supported
    copyJoynrEndPointListToDbus(joynrEntry.getEndpointAddresses(), dbusEntry.endpointAdresses);
}

void DbusCapabilitiesUtil::copyJoynrProviderQosToDbus(const types::ProviderQos& joynrQos, joynr::messaging::types::Types::ProviderQos& dbusQos) {
    // copy the scope
    switch(joynrQos.getScope()) {
    case types::ProviderScope::LOCAL:
        dbusQos.scope = joynr::messaging::types::Types::ProviderScope::LOCAL;
        break;
    case types::ProviderScope::GLOBAL:
        dbusQos.scope = joynr::messaging::types::Types::ProviderScope::GLOBAL;
        break;
    }

    dbusQos.priority = joynrQos.getPriority();
    dbusQos.version1 = joynrQos.getProviderVersion();

    // copy the custom parameters
    auto parameterList = joynrQos.getCustomParameters();
    for(auto it = parameterList.begin(); it != parameterList.end(); it++) {
        types::CustomParameter joynrParam= *it;
        joynr::messaging::types::Types::CustomParameter dbusParam;
        dbusParam.name = joynrParam.getName().toStdString();
        dbusParam.value = joynrParam.getValue().toStdString();
        dbusQos.customParameters.insert(dbusQos.customParameters.end(), dbusParam);
    }

    dbusQos.supportsOnChangeSubscriptions = joynrQos.getSupportsOnChangeSubscriptions();
}

void DbusCapabilitiesUtil::copyJoynrEndPointListToDbus(const QList<QSharedPointer<joynr::system::Address> >& joynrList, joynr::messaging::types::Types::EndpointAddressList& dbusEndPointList) {
    int index = 0;
    for(auto it = joynrList.begin(); it != joynrList.end(); it++, index ++) {
        // at the moment only joynr messaging endpoint addresses are supported
        joynr::messaging::types::Types::EndpointAddressBase dbusAddr;
        dbusEndPointList.insert(dbusEndPointList.end(), dbusAddr);
    }
}

void DbusCapabilitiesUtil::copyDbusCapaEntryListToJoynr(const joynr::messaging::types::Types::CapabilityEntryList& dbusList, QList<CapabilityEntry>& joynrResult) {
    for(auto it = dbusList.begin(); it != dbusList.end(); it++){
        CapabilityEntry entry;
        copyDbusCapaEntryToJoynr(*it, entry);
        joynrResult.insert(joynrResult.end(), entry);
    }
}

void DbusCapabilitiesUtil::copyDbusCapaEntryToJoynr(const joynr::messaging::types::Types::CapabilityEntry& dbusEntry, CapabilityEntry& joynrEntry) {
    joynrEntry.setInterfaceName(QString::fromStdString(dbusEntry.interfaceName));
    joynrEntry.setDomain(QString::fromStdString(dbusEntry.domain));
    joynrEntry.setParticipantId(QString::fromStdString(dbusEntry.participantId));

    // copy qos
    types::ProviderQos qos;
    copyDbusProviderQosToJoynr(dbusEntry.qos, qos);
    joynrEntry.setQos(qos);

    // copy addressess
    // at the moment only joynr messaging endpoint addresses are supported
    QList<QSharedPointer<joynr::system::Address>> endPointAddrList;
    copyDbusEndPointListToJoynr(dbusEntry.endpointAdresses, endPointAddrList);
    joynrEntry.setEndpointAddresses(endPointAddrList);
}

void DbusCapabilitiesUtil::copyDbusProviderQosToJoynr(const joynr::messaging::types::Types::ProviderQos& dbusQos, types::ProviderQos& joynrQos) {
    // copy scope
    switch(dbusQos.scope) {
    case joynr::messaging::types::Types::ProviderScope::LOCAL:
        joynrQos.setScope(types::ProviderScope::LOCAL);
        break;
    case joynr::messaging::types::Types::ProviderScope::GLOBAL:
        joynrQos.setScope(types::ProviderScope::GLOBAL);
        break;
    }

    joynrQos.setPriority(dbusQos.priority);
    joynrQos.setProviderVersion(dbusQos.version1);

    // copy the custom parameters
    for(auto it = dbusQos.customParameters.begin(); it != dbusQos.customParameters.end(); it++) {
        joynr::messaging::types::Types::CustomParameter dbusParam = *it;
        types::CustomParameter joynrParam;
        joynrParam.setName(QString::fromStdString(dbusParam.name));
        joynrParam.setValue(QString::fromStdString(dbusParam.value));
        joynrQos.getCustomParameters().append(joynrParam);
    }

    joynrQos.setSupportsOnChangeSubscriptions(dbusQos.supportsOnChangeSubscriptions);
}

void DbusCapabilitiesUtil::copyDbusEndPointListToJoynr(const joynr::messaging::types::Types::EndpointAddressList& dbusEndPointList, QList<QSharedPointer<joynr::system::Address> >& joynrList) {
    for(auto it = dbusEndPointList.begin(); it != dbusEndPointList.end(); it++) {
        QString channelId = QString::fromStdString((*it).endPointAddress);
        JoynrMessagingViaCCEndpointAddress* joynrAddr = new JoynrMessagingViaCCEndpointAddress();
        QSharedPointer<JoynrMessagingViaCCEndpointAddress> joynrAddrPtr(joynrAddr);
        joynrList.append(joynrAddrPtr);
    }
}

void DbusCapabilitiesUtil::copyJoynrDiscoveryQosToDbus(const DiscoveryQos& joynrDiscoveryQos, joynr::messaging::types::Types::DiscoveryQos& dbusDiscoveryQos) {
    // copy arbitration strategy
    switch (joynrDiscoveryQos.getArbitrationStrategy()) {
    case DiscoveryQos::ArbitrationStrategy::FIXED_CHANNEL:
        dbusDiscoveryQos.arbitrationStrategy = joynr::messaging::types::Types::ArbitrationStrategy::FIXED_CHANNEL;
        break;
    case DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY:
        dbusDiscoveryQos.arbitrationStrategy = joynr::messaging::types::Types::ArbitrationStrategy::HIGHEST_PRIORITY;
        break;
    case DiscoveryQos::ArbitrationStrategy::KEYWORD:
        dbusDiscoveryQos.arbitrationStrategy = joynr::messaging::types::Types::ArbitrationStrategy::KEYWORD;
        break;
    case DiscoveryQos::ArbitrationStrategy::LOCAL_ONLY:
        dbusDiscoveryQos.arbitrationStrategy = joynr::messaging::types::Types::ArbitrationStrategy::LOCAL_ONLY;
        break;
    case DiscoveryQos::ArbitrationStrategy::NOT_SET:
        dbusDiscoveryQos.arbitrationStrategy = joynr::messaging::types::Types::ArbitrationStrategy::NOT_SET;
        break;
    default:
        assert(false);
    }

    switch(joynrDiscoveryQos.getDiscoveryScope()) {
    case DiscoveryQos::DiscoveryScope::GLOBAL_ONLY:
        dbusDiscoveryQos.discoveryScope = joynr::messaging::types::Types::DiscoveryScope::GLOBAL_ONLY;
        break;
    case DiscoveryQos::DiscoveryScope::LOCAL_ONLY:
        dbusDiscoveryQos.discoveryScope = joynr::messaging::types::Types::DiscoveryScope::LOCAL_ONLY;
        break;
    case DiscoveryQos::DiscoveryScope::LOCAL_THEN_GLOBAL:
        dbusDiscoveryQos.discoveryScope = joynr::messaging::types::Types::DiscoveryScope::LOCAL_THEN_GLOBAL;
        break;
    case DiscoveryQos::DiscoveryScope::LOCAL_AND_GLOBAL:
        dbusDiscoveryQos.discoveryScope = joynr::messaging::types::Types::DiscoveryScope::LOCAL_AND_GLOBAL;
        break;
    default:
        assert(false);
    }

    dbusDiscoveryQos.cacheMaxAge = joynrDiscoveryQos.getCacheMaxAge();
    dbusDiscoveryQos.discoveryTimeout = joynrDiscoveryQos.getDiscoveryTimeout();
    dbusDiscoveryQos.providerMustSupportOnChange = joynrDiscoveryQos.getProviderMustSupportOnChange();
    dbusDiscoveryQos.retryInterval = joynrDiscoveryQos.getRetryInterval();

    // copy the custom parameter
    auto parameterMap = joynrDiscoveryQos.getCustomParameters();
    for(auto it = parameterMap.begin(); it != parameterMap.end(); it++) {
        types::CustomParameter parameter = *it;
        // initialize dbus parameter
        joynr::messaging::types::Types::CustomParameter dbusParam;
        dbusParam.name = parameter.getName().toStdString();
        dbusParam.value = parameter.getValue().toStdString();
        // add to the map
        dbusDiscoveryQos.customParameters[dbusParam.name] = dbusParam;
    }
}

void DbusCapabilitiesUtil::copyDbusDiscoveryQosToJoynr(const joynr::messaging::types::Types::DiscoveryQos& dbusDiscoveryQos, DiscoveryQos& joynrDiscoveryQos) {
    // copy arbitration strategy
    switch (dbusDiscoveryQos.arbitrationStrategy) {
    case joynr::messaging::types::Types::ArbitrationStrategy::FIXED_CHANNEL:
        joynrDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_CHANNEL);
        break;
    case joynr::messaging::types::Types::ArbitrationStrategy::HIGHEST_PRIORITY:
        joynrDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        break;
    case joynr::messaging::types::Types::ArbitrationStrategy::KEYWORD:
        joynrDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
        break;
    case joynr::messaging::types::Types::ArbitrationStrategy::LOCAL_ONLY:
        joynrDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LOCAL_ONLY);
        break;
    case joynr::messaging::types::Types::ArbitrationStrategy::NOT_SET:
        joynrDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::NOT_SET);
        break;
    default:
        assert(false);
    }

    switch(dbusDiscoveryQos.discoveryScope) {
    case joynr::messaging::types::Types::DiscoveryScope::GLOBAL_ONLY:
        joynrDiscoveryQos.setDiscoveryScope(DiscoveryQos::DiscoveryScope::GLOBAL_ONLY);
        break;
    case joynr::messaging::types::Types::DiscoveryScope::LOCAL_ONLY:
        joynrDiscoveryQos.setDiscoveryScope(DiscoveryQos::DiscoveryScope::LOCAL_ONLY);
        break;
    case joynr::messaging::types::Types::DiscoveryScope::LOCAL_THEN_GLOBAL:
        joynrDiscoveryQos.setDiscoveryScope(DiscoveryQos::DiscoveryScope::LOCAL_THEN_GLOBAL);
        break;
    case joynr::messaging::types::Types::DiscoveryScope::LOCAL_AND_GLOBAL:
        joynrDiscoveryQos.setDiscoveryScope(DiscoveryQos::DiscoveryScope::LOCAL_AND_GLOBAL);
        break;
    default:
        assert(false);
    }

    joynrDiscoveryQos.setCacheMaxAge(dbusDiscoveryQos.cacheMaxAge);
    joynrDiscoveryQos.setDiscoveryTimeout(dbusDiscoveryQos.discoveryTimeout);
    joynrDiscoveryQos.setProviderMustSupportOnChange(dbusDiscoveryQos.providerMustSupportOnChange);
    joynrDiscoveryQos.setRetryInterval(dbusDiscoveryQos.retryInterval);

    // copy the custom parameters
    auto parameterMap = dbusDiscoveryQos.customParameters;
    for(auto it = parameterMap.begin(); it != parameterMap.end(); it++) {
        joynr::messaging::types::Types::CustomParameter parameter = it->second;
        // initialize the joynr parameter
        joynrDiscoveryQos.addCustomParameter(QString::fromStdString(parameter.name),
                        QString::fromStdString(parameter.value));
    }
}

} // namespace joynr
