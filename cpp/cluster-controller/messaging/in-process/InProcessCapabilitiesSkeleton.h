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
#ifndef INPROCESSCAPABILITIESSKELETON_H
#define INPROCESSCAPABILITIESSKELETON_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/ICapabilities.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/ClusterControllerDirectories.h"


namespace joynr {

namespace joynr_logging { class Logger; }

/**
  *
  *
  */


class JOYNRCLUSTERCONTROLLER_EXPORT InProcessCapabilitiesSkeleton : public ICapabilities {
public:
    InProcessCapabilitiesSkeleton(
            IMessagingEndpointDirectory* messagingEndpointDirectory,
            QSharedPointer<LocalCapabilitiesDirectory> localCapabilitiesDirectory,
            QString ccChannelId
    );
    void add(
            const QString &domain,
            const QString &interfaceName,
            const QString &participantId,
            const types::ProviderQos &qos,
            QList<QSharedPointer<joynr::system::Address> > endpointAddressList,
            QSharedPointer<joynr::system::Address> messagingStubAddress,
            const qint64& timeout_ms
    );
    void addEndpoint(
            const QString& participantId,
            QSharedPointer<joynr::system::Address> messagingStubAddress,
            const qint64& timeout_ms
    );
    QList<CapabilityEntry> lookup(
            const QString &domain,
            const QString &interfaceName,
            const DiscoveryQos& discoveryQos
    );
    QList<CapabilityEntry> lookup(
            const QString& participantId,
            const DiscoveryQos& discoveryQos
    );
    void remove(const QString& participantId, const qint64& timeout_ms);

private:
    DISALLOW_COPY_AND_ASSIGN(InProcessCapabilitiesSkeleton);
    static joynr_logging::Logger* logger;

    IMessagingEndpointDirectory* messagingEndpointDirectory;
    QSharedPointer<LocalCapabilitiesDirectory> localCapabilitiesDirectory;
    QString ccChannelId;
};

// NOTE: This future is used to convert the synchronous call of the middleware
// to an asynchronous call to the local capabilities directory. It could be remmoved
// once we have an asynchronous middleware to communicate between libJoynr and CC.
class DummyCapabilitiesFuture : public ILocalCapabilitiesCallback {
public:
    DummyCapabilitiesFuture();
    void capabilitiesReceived(QList<CapabilityEntry> capabilities);
    QList<CapabilityEntry> get();
    QList<CapabilityEntry> get(const qint64& timeout_ms);
    virtual ~DummyCapabilitiesFuture(){}
private:
    QSemaphore futureSemaphore;
    DISALLOW_COPY_AND_ASSIGN(DummyCapabilitiesFuture);
    QList<CapabilityEntry> capabilities;
};


} // namespace joynr
#endif //DUMMYCAPABILITIESSKELETON_H
