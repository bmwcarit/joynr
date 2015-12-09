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
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> messagingStubAddress,
        std::shared_ptr<ParticipantIdStorage> participantIdStorage,
        std::shared_ptr<joynr::system::RoutingTypes::QtAddress> dispatcherAddress,
        std::shared_ptr<MessageRouter> messageRouter)
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
    for (IDispatcher* currentDispatcher : dispatcherList) {
        currentDispatcher->removeRequestCaller(participantId);
    }
    try {
        discoveryProxy.remove(participantId);
    } catch (exceptions::JoynrException& e) {
        LOG_ERROR(logger,
                  FormatString("Unable to remove provider (participant ID: %1) "
                               "to discovery. Error: %2.")
                          .arg(participantId)
                          .arg(e.getMessage())
                          .str());
    }

    std::shared_ptr<joynr::Future<void>> future(new Future<void>());
    auto onSuccess = [future]() { future->onSuccess(); };
    messageRouter->removeNextHop(participantId, onSuccess);
    future->wait();

    if (!future->getStatus().successful()) {
        LOG_ERROR(
                logger,
                FormatString("Unable to remove next hop (participant ID: %1) from message router.")
                        .arg(participantId)
                        .str());
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
