/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include "joynr/ICapabilities.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/IDispatcher.h"

#include <QString>
#include <QList>
#include <QSharedPointer>
#include <cassert>

namespace joynr {

class JOYNR_EXPORT CapabilitiesRegistrar {
public:
    CapabilitiesRegistrar(QList<IDispatcher*> dispatcherList,
                          QSharedPointer<ICapabilities> capabilitiesAggregator,
                          QSharedPointer<EndpointAddressBase> messagingStubAddress,
                          QSharedPointer<ParticipantIdStorage> participantIdStorage);

    template <class T>
    QString registerCapability(const QString& domain, QSharedPointer<T> provider, QString authenticationToken){

        QSharedPointer<RequestCaller> caller = RequestCallerFactory::create<T>(provider);
        QList<QSharedPointer<EndpointAddressBase> > endpointAddresses;

        // Get the provider participant Id - the persisted provider Id has priority
        QString participantId =
                participantIdStorage->getProviderParticipantId(T::getInterfaceName(),
                                                               authenticationToken);

        foreach (IDispatcher* currentDispatcher, dispatcherList) {
            //TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher!=NULL);
            currentDispatcher->addRequestCaller(participantId,caller);
        }

        // Get the provider participant id
        capabilitiesAggregator->add(domain, T::getInterfaceName(),
                                    participantId,
                                    provider->getProviderQos(),
                                    endpointAddresses,
                                    messagingStubAddress,
                                    ICapabilities::NO_TIMEOUT());
        return participantId;
    }

    void unregisterCapability(QString participantId);

    template <class T>
    QString unregisterCapability(const QString& domain, QSharedPointer<T> provider, QString authenticationToken){
        Q_UNUSED(domain)
        Q_UNUSED(provider)

        // Get the provider participant Id - the persisted provider Id has priority
        QString participantId =
                participantIdStorage->getProviderParticipantId(T::getInterfaceName(),
                                                               authenticationToken);

        foreach (IDispatcher* currentDispatcher, dispatcherList) {
            //TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher!=NULL);
            currentDispatcher->removeRequestCaller(participantId);
        }

        capabilitiesAggregator->remove(participantId, ICapabilities::NO_TIMEOUT());
        return participantId;
    }

    void addDispatcher(IDispatcher* dispatcher);
    void removeDispatcher(IDispatcher* dispatcher);

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrar);
    QList<IDispatcher*> dispatcherList;
    QSharedPointer<ICapabilities> capabilitiesAggregator;
    QSharedPointer<EndpointAddressBase> messagingStubAddress;
    QSharedPointer<ParticipantIdStorage> participantIdStorage;
};


} // namespace joynr
#endif //CAPABILITIESREGISTRAR_H
