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
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/Future.h"

#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include <tuple>

namespace joynr
{

/**
 * Class that handles provider registration/deregistration
 */
class JOYNR_EXPORT CapabilitiesRegistrar
{
public:
    CapabilitiesRegistrar(
            std::vector<IDispatcher*> dispatcherList,
            joynr::system::IDiscoverySync& discoveryProxy,
            std::shared_ptr<joynr::system::RoutingTypes::Address> messagingStubAddress,
            std::shared_ptr<ParticipantIdStorage> participantIdStorage,
            std::shared_ptr<joynr::system::RoutingTypes::Address> dispatcherAddress,
            std::shared_ptr<MessageRouter> messageRouter);

    template <class T>
    std::string add(const std::string& domain, std::shared_ptr<T> provider)
    {

        std::shared_ptr<RequestCaller> caller = RequestCallerFactory::create<T>(provider);

        std::string interfaceName = provider->getInterfaceName();

        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);

        for (IDispatcher* currentDispatcher : dispatcherList) {
            // TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher != NULL);
            currentDispatcher->addRequestCaller(participantId, caller);
        }

        std::vector<joynr::types::CommunicationMiddleware::Enum> connections = {
                joynr::types::CommunicationMiddleware::JOYNR};
        joynr::types::DiscoveryEntry entry(
                domain, interfaceName, participantId, provider->getProviderQos(), connections);
        try {
            discoveryProxy.add(entry);
        } catch (exceptions::JoynrException& e) {
            LOG_ERROR(logger,
                      FormatString(
                              "Unable to add provider (participant ID: %1, domain: %2, interface: "
                              "%3) "
                              "to discovery. Error: %4.")
                              .arg(participantId)
                              .arg(domain)
                              .arg(interfaceName)
                              .arg(e.getMessage())
                              .str());
        }

        // add next hop to dispatcher
        std::shared_ptr<joynr::Future<void>> future(new Future<void>());
        auto onSuccess = [future]() { future->onSuccess(); };
        messageRouter->addNextHop(participantId, dispatcherAddress, onSuccess);
        future->wait();

        return participantId;
    }

    void remove(const std::string& participantId);

    template <class T>
    std::string remove(const std::string& domain, std::shared_ptr<T> provider)

    {
        std::ignore = provider;

        std::string interfaceName = provider->getInterfaceName();

        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);

        for (IDispatcher* currentDispatcher : dispatcherList) {
            // TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher != NULL);
            currentDispatcher->removeRequestCaller(participantId);
        }

        try {
            discoveryProxy.remove(participantId);
        } catch (exceptions::JoynrException& e) {
            LOG_ERROR(logger,
                      FormatString("Unable to remove provider (participant ID: %1, domain: %2, "
                                   "interface: %3) "
                                   "to discovery. Status code: %4.")
                              .arg(participantId)
                              .arg(domain)
                              .arg(interfaceName)
                              .arg(e.getMessage())
                              .str());
        }

        std::shared_ptr<joynr::Future<void>> future(new Future<void>());
        auto callbackFct = [future]() { future->onSuccess(); };
        messageRouter->removeNextHop(participantId, callbackFct);
        future->wait();

        if (!future->getStatus().successful()) {
            LOG_ERROR(logger,
                      FormatString(
                              "Unable to remove next hop (participant ID: %1) from message router.")
                              .arg(participantId)
                              .str());
        }

        return participantId;
    }

    void addDispatcher(IDispatcher* dispatcher);
    void removeDispatcher(IDispatcher* dispatcher);

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrar);
    std::vector<IDispatcher*> dispatcherList;
    joynr::system::IDiscoverySync& discoveryProxy;
    std::shared_ptr<joynr::system::RoutingTypes::Address> messagingStubAddress;
    std::shared_ptr<ParticipantIdStorage> participantIdStorage;
    std::shared_ptr<joynr::system::RoutingTypes::Address> dispatcherAddress;
    std::shared_ptr<MessageRouter> messageRouter;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // CAPABILITIESREGISTRAR_H
