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
#ifndef CAPABILITIESREGISTRAR_H
#define CAPABILITIESREGISTRAR_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"

#include "joynr/Provider.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/IDispatcher.h"
#include "joynr/MessageRouter.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/joynrlogging.h"
#include "joynr/system/DiscoveryEntry.h"

#include <QString>
#include <QList>
#include <QSharedPointer>
#include <cassert>

namespace joynr {

class JOYNR_EXPORT CapabilitiesRegistrar {
public:
    CapabilitiesRegistrar(
            QList<IDispatcher*> dispatcherList,
            joynr::system::IDiscoverySync& discoveryProxy,
            QSharedPointer<joynr::system::Address> messagingStubAddress,
            QSharedPointer<ParticipantIdStorage> participantIdStorage,
            QSharedPointer<joynr::system::Address> dispatcherAddress,
            QSharedPointer<MessageRouter> messageRouter
    );

    template <class T>
    QString add(
            const QString& domain,
            QSharedPointer<T> provider,
            QString authenticationToken
    ) {

        QSharedPointer<RequestCaller> caller = RequestCallerFactory::create<T>(provider);

        // Get the provider participant Id - the persisted provider Id has priority
        QString participantId = participantIdStorage->getProviderParticipantId(
                    domain,
                    T::getInterfaceName(),
                    authenticationToken
        );

        foreach (IDispatcher* currentDispatcher, dispatcherList) {
            //TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher!=NULL);
            currentDispatcher->addRequestCaller(participantId,caller);
        }

        QList<joynr::system::CommunicationMiddleware::Enum> connections;
        connections.append(joynr::system::CommunicationMiddleware::JOYNR);
        joynr::RequestStatus status;
        joynr::system::DiscoveryEntry entry(domain,
            T::getInterfaceName(),
            participantId,
            provider->getProviderQos(),
            connections);
        discoveryProxy.add(
                    status,
                    entry
        );
        if(!status.successful()) {
            LOG_ERROR(
                        logger,
                        QString("Unable to add provider (participant ID: %1, domain: %2, interface: %3) "
                                "to discovery. Status code: %4."
                        )
                        .arg(participantId)
                        .arg(domain)
                        .arg(T::getInterfaceName())
                        .arg(status.getCode().toString())
            );
        }

        // add next hop to dispatcher
        messageRouter->addNextHop(participantId, dispatcherAddress);

        return participantId;
    }

    void remove(const QString& participantId);

    template <class T>
    QString remove(
            const QString& domain,
            QSharedPointer<T> provider,
            QString authenticationToken
    ) {
        Q_UNUSED(provider)

        // Get the provider participant Id - the persisted provider Id has priority
        QString participantId = participantIdStorage->getProviderParticipantId(
                    domain,
                    T::getInterfaceName(),
                    authenticationToken
         );

        foreach (IDispatcher* currentDispatcher, dispatcherList) {
            //TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher!=NULL);
            currentDispatcher->removeRequestCaller(participantId);
        }

        joynr::RequestStatus status;
        discoveryProxy.remove(status, participantId);
        if(!status.successful()) {
            LOG_ERROR(
                        logger,
                        QString("Unable to remove provider (participant ID: %1, domain: %2, interface: %3) "
                                "to discovery. Status code: %4."
                        )
                        .arg(participantId)
                        .arg(domain)
                        .arg(T::getInterfaceName())
                        .arg(status.getCode().toString())
            );
        }

        messageRouter->removeNextHop(status, participantId);
        if(!status.successful()) {
            LOG_ERROR(
                        logger,
                        QString("Unable to remove next hop (participant ID: %1) from message router.")
                        .arg(participantId)
            );
        }
        return participantId;
    }

    void addDispatcher(IDispatcher* dispatcher);
    void removeDispatcher(IDispatcher* dispatcher);

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrar);
    QList<IDispatcher*> dispatcherList;
    joynr::system::IDiscoverySync& discoveryProxy;
    QSharedPointer<joynr::system::Address> messagingStubAddress;
    QSharedPointer<ParticipantIdStorage> participantIdStorage;
    QSharedPointer<joynr::system::Address> dispatcherAddress;
    QSharedPointer<MessageRouter> messageRouter;
    static joynr_logging::Logger* logger;
};


} // namespace joynr
#endif //CAPABILITIESREGISTRAR_H
