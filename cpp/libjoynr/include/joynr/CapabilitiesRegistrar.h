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

#include "joynr/RequestCallerFactory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/IDispatcher.h"
#include "joynr/MessageRouter.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/joynrlogging.h"
#include "joynr/types/QtDiscoveryEntry.h"
#include "joynr/Future.h"

#include <QString>
#include <string>
#include <QList>
#include <QSharedPointer>
#include <cassert>
#include <memory>

namespace joynr
{

/**
 * Class that handles provider registration/deregistration
 */
class JOYNR_EXPORT CapabilitiesRegistrar
{
public:
    CapabilitiesRegistrar(QList<IDispatcher*> dispatcherList,
                          joynr::system::IDiscoverySync& discoveryProxy,
                          QSharedPointer<joynr::system::QtAddress> messagingStubAddress,
                          QSharedPointer<ParticipantIdStorage> participantIdStorage,
                          QSharedPointer<joynr::system::QtAddress> dispatcherAddress,
                          QSharedPointer<MessageRouter> messageRouter);

    template <class T>
    std::string add(const std::string& domain, std::shared_ptr<T> provider)
    {

        QSharedPointer<RequestCaller> caller = RequestCallerFactory::create<T>(provider);

        std::string interfaceName = provider->getInterfaceName();

        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);

        foreach (IDispatcher* currentDispatcher, dispatcherList) {
            // TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher != NULL);
            currentDispatcher->addRequestCaller(participantId, caller);
        }

        std::vector<joynr::types::CommunicationMiddleware::Enum> connections = {
                joynr::types::CommunicationMiddleware::JOYNR};
        joynr::RequestStatus status;
        joynr::types::DiscoveryEntry entry(
                domain, interfaceName, participantId, provider->getProviderQos(), connections);
        discoveryProxy.add(status, entry);
        if (!status.successful()) {
            LOG_ERROR(logger,
                      QString("Unable to add provider (participant ID: %1, domain: %2, interface: "
                              "%3) "
                              "to discovery. Status code: %4.")
                              .arg(QString::fromStdString(participantId))
                              .arg(QString::fromStdString(domain))
                              .arg(QString::fromStdString(interfaceName))
                              .arg(status.getCode().toString()));
        }

        // add next hop to dispatcher
        QSharedPointer<joynr::Future<void>> future(new Future<void>());
        auto onSuccess = [future]() { future->onSuccess(); };
        messageRouter->addNextHop(participantId, dispatcherAddress, onSuccess);
        future->waitForFinished();

        return participantId;
    }

    void remove(const std::string& participantId);

    template <class T>
    std::string remove(const std::string& domain, std::shared_ptr<T> provider)

    {
        Q_UNUSED(provider)

        std::string interfaceName = provider->getInterfaceName();

        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);

        foreach (IDispatcher* currentDispatcher, dispatcherList) {
            // TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher != NULL);
            currentDispatcher->removeRequestCaller(participantId);
        }

        joynr::RequestStatus status;
        discoveryProxy.remove(status, participantId);
        if (!status.successful()) {
            LOG_ERROR(logger,
                      QString("Unable to remove provider (participant ID: %1, domain: %2, "
                              "interface: %3) "
                              "to discovery. Status code: %4.")
                              .arg(QString::fromStdString(participantId))
                              .arg(QString::fromStdString(domain))
                              .arg(QString::fromStdString(interfaceName))
                              .arg(status.getCode().toString()));
        }

        QSharedPointer<joynr::Future<void>> future(new Future<void>());
        auto callbackFct = [future]() { future->onSuccess(); };
        messageRouter->removeNextHop(participantId, callbackFct);
        future->waitForFinished();

        if (!future->getStatus().successful()) {
            LOG_ERROR(logger,
                      QString("Unable to remove next hop (participant ID: %1) from message router.")
                              .arg(QString::fromStdString(participantId)));
        }

        return participantId;
    }

    void addDispatcher(IDispatcher* dispatcher);
    void removeDispatcher(IDispatcher* dispatcher);

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrar);
    QList<IDispatcher*> dispatcherList;
    joynr::system::IDiscoverySync& discoveryProxy;
    QSharedPointer<joynr::system::QtAddress> messagingStubAddress;
    QSharedPointer<ParticipantIdStorage> participantIdStorage;
    QSharedPointer<joynr::system::QtAddress> dispatcherAddress;
    QSharedPointer<MessageRouter> messageRouter;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // CAPABILITIESREGISTRAR_H
