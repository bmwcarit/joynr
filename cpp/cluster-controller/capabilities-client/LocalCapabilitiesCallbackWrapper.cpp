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
#include "cluster-controller/capabilities-client/LocalCapabilitiesCallbackWrapper.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/CapabilityEntry.h"

namespace joynr
{

LocalCapabilitiesCallbackWrapper::LocalCapabilitiesCallbackWrapper(
        LocalCapabilitiesDirectory* localCapabilitiesDirectory,
        QSharedPointer<ILocalCapabilitiesCallback> wrappedCallback,
        const std::string& participantId,
        const joynr::system::DiscoveryQos& discoveryQos)
        : localCapabilitiesDirectory(localCapabilitiesDirectory),
          wrappedCallback(wrappedCallback),
          participantId(participantId),
          interfaceAddress(),
          discoveryQos(discoveryQos)
{
}

LocalCapabilitiesCallbackWrapper::LocalCapabilitiesCallbackWrapper(
        LocalCapabilitiesDirectory* localCapabilitiesDirectory,
        QSharedPointer<ILocalCapabilitiesCallback> wrappedCallback,
        const InterfaceAddress& interfaceAddress,
        const joynr::system::DiscoveryQos& discoveryQos)
        : localCapabilitiesDirectory(localCapabilitiesDirectory),
          wrappedCallback(wrappedCallback),
          participantId(""),
          interfaceAddress(interfaceAddress),
          discoveryQos(discoveryQos)
{
}

void LocalCapabilitiesCallbackWrapper::capabilitiesReceived(
        QList<types::CapabilityInformation> results)
{
    QMap<std::string, CapabilityEntry> capabilitiesMap;
    QList<CapabilityEntry> mergedEntries;

    foreach (types::CapabilityInformation capInfo, results) {
        QList<joynr::system::CommunicationMiddleware::Enum> connections;
        connections.append(joynr::system::CommunicationMiddleware::JOYNR);
        CapabilityEntry capEntry(capInfo.getDomain(),
                                 capInfo.getInterfaceName(),
                                 capInfo.getProviderQos(),
                                 capInfo.getParticipantId(),
                                 connections,
                                 true);
        capabilitiesMap.insertMulti(capInfo.getChannelId().toStdString(), capEntry);
        mergedEntries.append(capEntry);
    }
    localCapabilitiesDirectory->registerReceivedCapabilities(capabilitiesMap);

    if (discoveryQos.getDiscoveryScope() == joynr::system::DiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryQos.getDiscoveryScope() == joynr::system::DiscoveryScope::LOCAL_AND_GLOBAL) {
        // look if in the meantime there are some local providers registered
        // lookup in the local directory to get local providers which were registered in the
        // meantime.
        if (participantId.empty()) {
            mergedEntries +=
                    localCapabilitiesDirectory->getCachedLocalCapabilities(interfaceAddress);
        } else {
            mergedEntries += localCapabilitiesDirectory->getCachedLocalCapabilities(participantId);
        }
    }
    wrappedCallback->capabilitiesReceived(mergedEntries);
}

} // namespace joynr
