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
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/RequestStatus.h"

namespace joynr
{

joynr_logging::Logger* CapabilitiesRegistrar::logger =
        joynr_logging::Logging::getInstance()->getLogger("DIS", "CapabilitiesRegistrar");

CapabilitiesRegistrar::CapabilitiesRegistrar(
        QList<IDispatcher*> dispatcherList,
        joynr::system::IDiscoverySync& discoveryProxy,
        QSharedPointer<joynr::system::QtAddress> messagingStubAddress,
        QSharedPointer<ParticipantIdStorage> participantIdStorage,
        QSharedPointer<joynr::system::QtAddress> dispatcherAddress,
        QSharedPointer<MessageRouter> messageRouter)
        : dispatcherList(dispatcherList),
          discoveryProxy(discoveryProxy),
          messagingStubAddress(messagingStubAddress),
          participantIdStorage(participantIdStorage),
          dispatcherAddress(dispatcherAddress),
          messageRouter(messageRouter)
{
}

void CapabilitiesRegistrar::remove(const std::string& participantId)
{
    foreach (IDispatcher* currentDispatcher, dispatcherList) {
        currentDispatcher->removeRequestCaller(participantId);
    }
    joynr::RequestStatus status(discoveryProxy.remove(participantId));
    if (!status.successful()) {
        LOG_ERROR(logger,
                  QString("Unable to remove provider (participant ID: %1) "
                          "to discovery. Status code: %2.")
                          .arg(QString::fromStdString(participantId))
                          .arg(QString::fromStdString(status.getCode().toString())));
    }

    QSharedPointer<joynr::Future<void>> future(new Future<void>());
    auto onSuccess = [future]() { future->onSuccess(); };
    messageRouter->removeNextHop(participantId, onSuccess);
    future->waitForFinished();

    if (!future->getStatus().successful()) {
        LOG_ERROR(logger,
                  QString("Unable to remove next hop (participant ID: %1) from message router.")
                          .arg(QString::fromStdString(participantId)));
    }
}

void CapabilitiesRegistrar::addDispatcher(IDispatcher* dispatcher)
{
    dispatcherList.append(dispatcher);
}

void CapabilitiesRegistrar::removeDispatcher(IDispatcher* dispatcher)
{
    dispatcherList.removeAll(dispatcher);
}

} // namespace joynr
