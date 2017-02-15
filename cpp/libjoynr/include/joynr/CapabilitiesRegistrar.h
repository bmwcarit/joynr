/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include <cassert>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include "joynr/IMessageRouter.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/IDispatcher.h"
#include "joynr/MulticastBroadcastListener.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/Logger.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/Future.h"
#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/Version.h"
#include "joynr/system/RoutingTypes/Address.h"

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
            std::shared_ptr<ParticipantIdStorage> participantIdStorage,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
            std::shared_ptr<IMessageRouter> messageRouter,
            std::int64_t defaultExpiryIntervalMs,
            PublicationManager& publicationManager);

    template <class T>
    std::string add(const std::string& domain,
                    std::shared_ptr<T> provider,
                    const types::ProviderQos& providerQos)
    {

        std::shared_ptr<RequestCaller> caller = RequestCallerFactory::create<T>(provider);

        std::string interfaceName = provider->getInterfaceName();

        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);

        provider->registerBroadcastListener(
                new MulticastBroadcastListener(participantId, publicationManager));

        for (IDispatcher* currentDispatcher : dispatcherList) {
            // TODO will the provider be registered at all dispatchers or
            //     should it be configurable which ones are used to contact it.
            assert(currentDispatcher != nullptr);
            currentDispatcher->addRequestCaller(participantId, caller);
        }

        const std::int64_t now =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
        const std::int64_t lastSeenDateMs = now;
        const std::int64_t defaultExpiryDateMs = now + defaultExpiryIntervalMs;
        const std::string defaultPublicKeyId("");
        joynr::types::Version providerVersion(provider->MAJOR_VERSION, provider->MINOR_VERSION);
        joynr::types::DiscoveryEntry entry(providerVersion,
                                           domain,
                                           interfaceName,
                                           participantId,
                                           providerQos,
                                           lastSeenDateMs,
                                           defaultExpiryDateMs,
                                           defaultPublicKeyId);
        try {
            discoveryProxy.add(entry);
        } catch (const exceptions::JoynrException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Unable to add provider (participant ID: {}, domain: {}, interface: "
                            "{}) to discovery. Error: {}",
                            participantId,
                            domain,
                            interfaceName,
                            e.getMessage());
        }

        // add next hop to dispatcher
        auto future = std::make_shared<joynr::Future<void>>();
        auto onSuccess = [future]() { future->onSuccess(); };
        messageRouter->addNextHop(participantId, dispatcherAddress, onSuccess);
        future->wait();

        return participantId;
    }

    void remove(const std::string& participantId);

    template <class T>
    std::string remove(const std::string& domain, std::shared_ptr<T> provider)
    {
        std::string interfaceName = provider->getInterfaceName();
        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);
        remove(participantId);
        return participantId;
    }

    void addDispatcher(IDispatcher* dispatcher);
    void removeDispatcher(IDispatcher* dispatcher);

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrar);
    std::vector<IDispatcher*> dispatcherList;
    joynr::system::IDiscoverySync& discoveryProxy;
    std::shared_ptr<ParticipantIdStorage> participantIdStorage;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress;
    std::shared_ptr<IMessageRouter> messageRouter;
    std::int64_t defaultExpiryIntervalMs;
    PublicationManager& publicationManager;
    ADD_LOGGER(CapabilitiesRegistrar);
};

} // namespace joynr
#endif // CAPABILITIESREGISTRAR_H
