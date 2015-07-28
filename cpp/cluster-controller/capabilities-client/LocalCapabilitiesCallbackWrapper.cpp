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
        const joynr::types::StdDiscoveryQos& discoveryQos)
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
        const joynr::types::StdDiscoveryQos& discoveryQos)
        : localCapabilitiesDirectory(localCapabilitiesDirectory),
          wrappedCallback(wrappedCallback),
          participantId(""),
          interfaceAddress(interfaceAddress),
          discoveryQos(discoveryQos)
{
}

void LocalCapabilitiesCallbackWrapper::capabilitiesReceived(
        std::vector<types::StdCapabilityInformation> results)
{
    QMap<std::string, CapabilityEntry> capabilitiesMap;
    std::vector<CapabilityEntry> mergedEntries;

    foreach (types::StdCapabilityInformation capInfo, results) {
        QList<joynr::types::QtCommunicationMiddleware::Enum> connections;
        connections.append(joynr::types::QtCommunicationMiddleware::JOYNR);
        CapabilityEntry capEntry(QString::fromStdString(capInfo.getDomain()),
                                 QString::fromStdString(capInfo.getInterfaceName()),
                                 types::QtProviderQos::createQt(capInfo.getProviderQos()),
                                 QString::fromStdString(capInfo.getParticipantId()),
                                 connections,
                                 true);
        capabilitiesMap.insertMulti(capInfo.getChannelId(), capEntry);
        mergedEntries.push_back(capEntry);
    }
    localCapabilitiesDirectory->registerReceivedCapabilities(capabilitiesMap);

    if (discoveryQos.getDiscoveryScope() == joynr::types::StdDiscoveryScope::LOCAL_THEN_GLOBAL ||
        discoveryQos.getDiscoveryScope() == joynr::types::StdDiscoveryScope::LOCAL_AND_GLOBAL) {
        // look if in the meantime there are some local providers registered
        // lookup in the local directory to get local providers which were registered in the
        // meantime.
        if (participantId.empty()) {
            std::vector<CapabilityEntry> cachedCaps(
                    localCapabilitiesDirectory->getCachedLocalCapabilities(interfaceAddress));
            mergedEntries.insert(mergedEntries.end(), cachedCaps.begin(), cachedCaps.end());
            ;
        } else {
            std::vector<CapabilityEntry> cachedCaps(
                    localCapabilitiesDirectory->getCachedLocalCapabilities(participantId));
            mergedEntries.insert(mergedEntries.end(), cachedCaps.begin(), cachedCaps.end());
            ;
        }
    }
    wrappedCallback->capabilitiesReceived(mergedEntries);
}

} // namespace joynr
