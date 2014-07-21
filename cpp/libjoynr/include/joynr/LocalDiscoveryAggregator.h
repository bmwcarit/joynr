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
#ifndef LOCALDISCOVERYAGGREGATOR_H
#define LOCALDISCOVERYAGGREGATOR_H
#include <QMap>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/ProxyBuilder.h"

#include "joynr/system/IDiscovery.h"

namespace joynr {
class JoynrRuntime;
class IRequestCallerDirectory;
class SystemServicesSettings;

namespace system {
class DiscoveryEntry;
class DiscoveryQos;
class DiscoveryProxy;
}

/**
 * @brief The LocalDiscoveryAggregator class is a wrapper for discovery proxies. On
 * lookup operations it checks whether the retrieved providers are available locally.
 * If yes, it adds an in-process connection to the provider's connection list.
 */
class JOYNR_EXPORT LocalDiscoveryAggregator : public joynr::system::IDiscoverySync {
public:
    // discoveryProxy pointer doesn't hold ownership
    /**
     * @brief LocalDiscoveryAggregator Constructs a local discovery aggregator needed
     * on cluster-controller side that holds a pointer to the local capabilities
     * directory. NOTE: it doesn't take ownership of the local capabilties directory
     * (discovery proxy) pointer.
     */
    LocalDiscoveryAggregator(
            IRequestCallerDirectory& requestCallerDirectory,
            const SystemServicesSettings& systemServicesSettings,
            joynr::system::IDiscoverySync* localCapabilitiesDirectory = NULL
    );

    virtual ~LocalDiscoveryAggregator();

    void setDiscoveryProxy(joynr::system::IDiscoverySync* discoveryProxy);

    // inherited from joynr::system::IDiscoverySync
    virtual void add(
            joynr::RequestStatus& joynrInternalStatus,
            joynr::system::DiscoveryEntry entry
    );

    // inherited from joynr::system::IDiscoverySync
    virtual void lookup(
            joynr::RequestStatus& joynrInternalStatus,
            QList<joynr::system::DiscoveryEntry>& result,
            QString domain,
            QString interfaceName,
            joynr::system::DiscoveryQos discoveryQos
    );

    // inherited from joynr::system::IDiscoverySync
    virtual void lookup(
            joynr::RequestStatus& joynrInternalStatus,
            joynr::system::DiscoveryEntry& result,
            QString participantId
    );

    // inherited from joynr::system::IDiscoverySync
    virtual void remove(
            joynr::RequestStatus& joynrInternalStatus,
            QString participantId
    );

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryAggregator);

    /**
     * @brief checkForLocalAvailabilityAndAddInProcessConnection Checks whether the provider
     * in the discovery entry is locally available. If yes, adds a in-process connection to
     * the discovery entry's connection list.
     *
     * @param discoveryEntry Discovery entry to check for local availability.
     */
    void checkForLocalAvailabilityAndAddInProcessConnection(
            joynr::system::DiscoveryEntry& discoveryEntry
    );

    // this pointer doesn't hold ownership in case the proxy builder is not set
    joynr::system::IDiscoverySync* discoveryProxy;
    bool hasOwnershipOfDiscoveryProxy;
    IRequestCallerDirectory& requestCallerDirectory;
    QMap<QString, joynr::system::DiscoveryEntry> provisionedDiscoveryEntries;
    const SystemServicesSettings& systemServicesSettings;
};
} // namespace joynr
#endif //LOCALDISCOVERYAGGREGATOR_H
