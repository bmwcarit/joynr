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
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "joynr/IDispatcher.h"
#include "joynr/IMessageRouter.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/ProviderScope.h"
#include "joynr/types/Version.h"

namespace joynr
{

class MulticastBroadcastListener;
class PublicationManager;
class RequestCaller;

namespace system
{
namespace RoutingTypes
{
class Address;
} // namespace RoutingTypes
} // namespace system

/**
 * Class that handles provider registration/deregistration
 */
class JOYNR_EXPORT CapabilitiesRegistrar
{
public:
    CapabilitiesRegistrar(
            std::shared_ptr<IDispatcher> dispatcher,
            std::shared_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
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
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError,
            bool persist = true,
            bool awaitGlobalRegistration = false,
            bool addToAll = false,
            std::vector<std::string> gbids = std::vector<std::string>()) noexcept
    {
        const std::string interfaceName = T::INTERFACE_NAME();
        const std::string participantId = _participantIdStorage->getProviderParticipantId(
                domain, interfaceName, T::MAJOR_VERSION);
        std::shared_ptr<RequestCaller> caller = RequestCallerFactory::create<T>(provider);
        auto multicastBroadcastListener =
                std::make_shared<MulticastBroadcastListener>(participantId, _publicationManager);
        provider->registerBroadcastListener(multicastBroadcastListener);

        const std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::system_clock::now().time_since_epoch())
                                         .count();
        const std::int64_t lastSeenDateMs = now;
        const std::int64_t defaultExpiryDateMs = now + _defaultExpiryIntervalMs;
        const std::string defaultPublicKeyId("");
        joynr::types::Version providerVersion(T::MAJOR_VERSION, T::MINOR_VERSION);
        auto customParameters = providerQos.getCustomParameters();
        bool isInternalProvider =
                (customParameters.size() == 1 &&
                 customParameters.front().getName() == "___CC.InternalProvider___");
        const std::int64_t discoveryEntryExpiryDateMs =
                (isInternalProvider ? std::numeric_limits<std::int64_t>::max()
                                    : defaultExpiryDateMs);
        joynr::types::DiscoveryEntry entry(providerVersion,
                                           domain,
                                           interfaceName,
                                           participantId,
                                           providerQos,
                                           lastSeenDateMs,
                                           discoveryEntryExpiryDateMs,
                                           defaultPublicKeyId);
        bool isGloballyVisible = providerQos.getScope() == types::ProviderScope::GLOBAL;
        auto onErrorWrapper = [provider, multicastBroadcastListener, onError = std::move(onError)](
                                      const joynr::exceptions::JoynrRuntimeException& error) {
            provider->unregisterBroadcastListener(multicastBroadcastListener);
            onError(error);
        };
        for (auto&& gbid : gbids) {
            if (gbid.empty()) {
                onErrorWrapper(
                        joynr::exceptions::JoynrRuntimeException("gbid(s) must not be empty"));
                return participantId;
            }
        }
        // dispatcher is captured here to avoid compilation problem with gcc 7.x
        auto onSuccessWrapper = [domain,
                                 interfaceName,
                                 majorVersion = T::MAJOR_VERSION,
                                 dispatcher = this->_dispatcher,
                                 caller,
                                 participantIdStorage = util::as_weak_ptr(_participantIdStorage),
                                 messageRouter = util::as_weak_ptr(_messageRouter),
                                 participantId,
                                 discoveryProxy = util::as_weak_ptr(_discoveryProxy),
                                 entry = std::move(entry),
                                 awaitGlobalRegistration,
                                 onSuccess = std::move(onSuccess),
                                 onErrorWrapper,
                                 persist,
                                 isInternalProvider,
                                 addToAll,
                                 gbids]() {
            if (persist) {
                // Sync persistency to disk now that registration is done.
                if (auto participantIdStoragePtr = participantIdStorage.lock()) {
                    participantIdStoragePtr->setProviderParticipantId(
                            domain, interfaceName, majorVersion, participantId);
                }
            }

            auto onErrorWrapperInternal =
                    [participantId,
                     messageRouter = std::move(messageRouter),
                     onErrorWrapper = std::move(onErrorWrapper)](
                            const joynr::exceptions::JoynrRuntimeException& error) {
                        if (auto ptr = messageRouter.lock()) {
                            ptr->removeNextHop(participantId);
                        }
                        onErrorWrapper(error);
                    };

            auto onApplicationErrorWrapperInternal =
                    [participantId,
                     messageRouter = std::move(messageRouter),
                     onErrorWrapper = std::move(onErrorWrapper)](
                            const joynr::types::DiscoveryError::Enum& errorEnum) {
                        if (auto ptr = messageRouter.lock()) {
                            ptr->removeNextHop(participantId);
                        }
                        onErrorWrapper(joynr::exceptions::JoynrRuntimeException(
                                "Registration failed with DiscoveryError " +
                                joynr::types::DiscoveryError::getLiteral(errorEnum)));
                    };

            if (auto discoveryProxyPtr = discoveryProxy.lock()) {
                if (isInternalProvider) {
                    MessagingQos messagingQos =
                            MessagingQos(std::numeric_limits<std::int64_t>::max());
                    if (addToAll) {
                        discoveryProxyPtr->addToAllAsync(
                                entry,
                                awaitGlobalRegistration,
                                [domain, interfaceName, participantId, onSuccess]() {
                                    JOYNR_LOG_INFO(logger(),
                                                   "Registered internal Provider: "
                                                   "participantId: {}, domain: {}, "
                                                   "interfaceName: {}",
                                                   participantId,
                                                   domain,
                                                   interfaceName);
                                    onSuccess();
                                },
                                std::move(onApplicationErrorWrapperInternal),
                                std::move(onErrorWrapperInternal),
                                messagingQos);
                    } else {
                        discoveryProxyPtr->addAsync(
                                entry,
                                awaitGlobalRegistration,
                                gbids,
                                [domain, interfaceName, participantId, onSuccess]() {
                                    JOYNR_LOG_INFO(logger(),
                                                   "Registered internal Provider: "
                                                   "participantId: {}, domain: {}, "
                                                   "interfaceName: {}",
                                                   participantId,
                                                   domain,
                                                   interfaceName);
                                    onSuccess();
                                },
                                std::move(onApplicationErrorWrapperInternal),
                                std::move(onErrorWrapperInternal),
                                messagingQos);
                    }
                } else {
                    if (addToAll) {
                        discoveryProxyPtr->addToAllAsync(
                                entry,
                                awaitGlobalRegistration,
                                [domain, interfaceName, participantId, onSuccess]() {
                                    JOYNR_LOG_INFO(logger(),
                                                   "Registered Provider: "
                                                   "participantId: {}, domain: {}, "
                                                   "interfaceName: {}",
                                                   participantId,
                                                   domain,
                                                   interfaceName);
                                    onSuccess();
                                },
                                std::move(onApplicationErrorWrapperInternal),
                                std::move(onErrorWrapperInternal));
                    } else {
                        discoveryProxyPtr->addAsync(
                                entry,
                                awaitGlobalRegistration,
                                gbids,
                                [domain, interfaceName, participantId, onSuccess]() {
                                    JOYNR_LOG_INFO(logger(),
                                                   "Registered Provider: "
                                                   "participantId: {}, domain: {}, "
                                                   "interfaceName: {}",
                                                   participantId,
                                                   domain,
                                                   interfaceName);
                                    onSuccess();
                                },
                                std::move(onApplicationErrorWrapperInternal),
                                std::move(onErrorWrapperInternal));
                    }
                }
            } else {
                const joynr::exceptions::JoynrRuntimeException error(
                        "runtime and required discovery proxy have been already destroyed");
                onErrorWrapperInternal(error);
            }
        };

        _dispatcher->addRequestCaller(participantId, caller);

        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;
        _messageRouter->addNextHop(participantId,
                                   _dispatcherAddress,
                                   isGloballyVisible,
                                   expiryDateMs,
                                   isSticky,
                                   std::move(onSuccessWrapper),
                                   std::move(onErrorWrapper));

        return participantId;
    }

    void removeAsync(const std::string& participantId,
                     std::function<void()> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                             onError) noexcept;

    template <class T>
    std::string removeAsync(
            const std::string& domain,
            std::shared_ptr<T> /*provider*/,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onError) noexcept
    {
        std::string interfaceName = T::INTERFACE_NAME();
        // Get the provider participant Id - the persisted provider Id has priority
        std::string participantId = _participantIdStorage->getProviderParticipantId(
                domain, interfaceName, T::MAJOR_VERSION);
        removeAsync(
                participantId,
                [participantId, domain, interfaceName, onSuccess]() {
                    JOYNR_LOG_INFO(logger(),
                                   "Unregistered Provider: participantId: {}, domain: {}, "
                                   "interfaceName: {}",
                                   participantId,
                                   domain,
                                   interfaceName);

                    onSuccess();
                },
                std::move(onError));
        return participantId;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrar);
    std::shared_ptr<IDispatcher> _dispatcher;
    std::shared_ptr<joynr::system::IDiscoveryAsync> _discoveryProxy;
    std::shared_ptr<ParticipantIdStorage> _participantIdStorage;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _dispatcherAddress;
    std::shared_ptr<IMessageRouter> _messageRouter;
    std::int64_t _defaultExpiryIntervalMs;
    std::weak_ptr<PublicationManager> _publicationManager;
    const std::string _globalAddress;
    ADD_LOGGER(CapabilitiesRegistrar)
};

} // namespace joynr
#endif // CAPABILITIESREGISTRAR_H
