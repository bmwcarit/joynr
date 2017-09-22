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
#include <memory>
#include <string>
#include <vector>

#include "joynr/Future.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessageRouter.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MulticastBroadcastListener.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/Util.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/Version.h"

namespace joynr
{

/**
 * Class that handles provider registration/deregistration
 */
class JOYNR_EXPORT CapabilitiesRegistrar
{
public:
    CapabilitiesRegistrar(
            std::vector<std::shared_ptr<IDispatcher>> dispatcherList,
            joynr::system::IDiscoveryAsync& discoveryProxy,
            std::shared_ptr<ParticipantIdStorage> participantIdStorage,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
            std::shared_ptr<IMessageRouter> messageRouter,
            std::int64_t defaultExpiryIntervalMs,
            std::weak_ptr<PublicationManager> publicationManager,
            const std::string& globalAddress);

    template <class T>
    std::string addAsync(
            const std::string& domain,
            std::shared_ptr<T> provider,
            const types::ProviderQos& providerQos,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError)
    {

        std::shared_ptr<RequestCaller> caller = RequestCallerFactory::create<T>(provider);

        std::string interfaceName = T::INTERFACE_NAME();

        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);

        provider->registerBroadcastListener(
                std::make_shared<MulticastBroadcastListener>(participantId, publicationManager));

        for (std::shared_ptr<IDispatcher> currentDispatcher : dispatcherList) {
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
        bool isGloballyVisible = entry.getQos().getScope() == types::ProviderScope::GLOBAL;
        auto onSuccessWrapper = [
            isGloballyVisible,
            messageRouter = util::as_weak_ptr(messageRouter),
            participantId,
            dispatcherAddress = dispatcherAddress,
            onSuccess = std::move(onSuccess),
            onError
        ]
        {
            // add next hop to dispatcher
            if (auto ptr = messageRouter.lock()) {
                constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
                const bool isSticky = false;
                ptr->addNextHop(participantId,
                                dispatcherAddress,
                                isGloballyVisible,
                                expiryDateMs,
                                isSticky,
                                std::move(onSuccess),
                                std::move(onError));
            }
        };

        discoveryProxy.addAsync(entry, std::move(onSuccessWrapper), std::move(onError));
        return participantId;
    }

    void removeAsync(
            const std::string& participantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError);

    template <class T>
    std::string removeAsync(
            const std::string& domain,
            std::shared_ptr<T> provider,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError)
    {
        std::string interfaceName = provider->getInterfaceName();
        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId =
                participantIdStorage->getProviderParticipantId(domain, interfaceName);
        removeAsync(participantId, std::move(onSuccess), std::move(onError));
        return participantId;
    }

    void addDispatcher(std::shared_ptr<IDispatcher> dispatcher);
    void removeDispatcher(std::shared_ptr<IDispatcher> dispatcher);

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrar);
    std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
    joynr::system::IDiscoveryAsync& discoveryProxy;
    std::shared_ptr<ParticipantIdStorage> participantIdStorage;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress;
    std::shared_ptr<IMessageRouter> messageRouter;
    std::int64_t defaultExpiryIntervalMs;
    std::weak_ptr<PublicationManager> publicationManager;
    const std::string globalAddress;
    ADD_LOGGER(CapabilitiesRegistrar);
};

} // namespace joynr
#endif // CAPABILITIESREGISTRAR_H
