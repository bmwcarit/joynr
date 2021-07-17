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
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/Dispatcher.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessageSender.h"
#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/IProxyBuilderBase.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/LibJoynrMessageRouter.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/MessageQueue.h"
#include "joynr/MessageSender.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/ProxyFactory.h"
#include "joynr/PublicationManager.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"

namespace joynr
{

class ITransportStatus;

namespace system
{
namespace RoutingTypes
{
class Address;
}
}

LibJoynrRuntime::LibJoynrRuntime(
        std::unique_ptr<Settings> settings,
        std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError,
        std::shared_ptr<IKeychain> keyChain)
        : JoynrRuntimeImpl(*settings, std::move(onFatalRuntimeError), std::move(keyChain)),
          _subscriptionManager(nullptr),
          _messageSender(nullptr),
          _joynrDispatcher(nullptr),
          _settings(std::move(settings)),
          _libjoynrSettings(new LibjoynrSettings(*this->_settings)),
          _dispatcherMessagingSkeleton(nullptr),
          _libJoynrMessageRouter(nullptr),
          _libJoynrRuntimeIsShuttingDown(false)
{
    _libjoynrSettings->printSettings();
    _singleThreadIOService->start();
}

LibJoynrRuntime::~LibJoynrRuntime()
{
    // make sure shutdown() has been called earlier
    assert(_libJoynrRuntimeIsShuttingDown);
}

void LibJoynrRuntime::shutdown()
{
    // protect against parallel and multiple calls of shutdown()
    bool previousValue = std::atomic_exchange_explicit(
            &_libJoynrRuntimeIsShuttingDown, true, std::memory_order_acquire);
    assert(!previousValue);
    // bail out in case assert is disabled
    if (previousValue) {
        return;
    }

    std::lock_guard<std::mutex> lock(_proxyBuildersMutex);
    for (auto proxyBuilder : _proxyBuilders) {
        proxyBuilder->stop();
        proxyBuilder.reset();
    }
    _proxyBuilders.clear();

    if (_joynrDispatcher) {
        _joynrDispatcher->shutdown();
        _joynrDispatcher.reset();
    }
    if (_publicationManager) {
        _publicationManager->shutdown();
    }
    if (_subscriptionManager) {
        _subscriptionManager->shutdown();
    }
    if (_libjoynrSettings) {
        delete _libjoynrSettings;
        _libjoynrSettings = nullptr;
    }
    if (_libJoynrMessageRouter) {
        _libJoynrMessageRouter->shutdown();
    }
}

void LibJoynrRuntime::init(
        std::shared_ptr<IMiddlewareMessagingStubFactory> middlewareMessagingStubFactory,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> libjoynrMessagingAddress,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> ccMessagingAddress,
        std::unique_ptr<IMulticastAddressCalculator> addressCalculator,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError)
{
    // create messaging stub factory
    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
    middlewareMessagingStubFactory->registerOnMessagingStubClosedCallback([messagingStubFactory](
            std::shared_ptr<const joynr::system::RoutingTypes::Address> destinationAddress) {
        messagingStubFactory->remove(std::move(destinationAddress));
    });
    messagingStubFactory->registerStubFactory(middlewareMessagingStubFactory);
    messagingStubFactory->registerStubFactory(std::make_shared<InProcessMessagingStubFactory>());

    std::string routingProviderParticipantId =
            _systemServicesSettings.getCcRoutingProviderParticipantId();
    // create message router
    _libJoynrMessageRouter = std::make_shared<LibJoynrMessageRouter>(
            _messagingSettings,
            libjoynrMessagingAddress,
            std::move(messagingStubFactory),
            _singleThreadIOService->getIOService(),
            std::move(addressCalculator),
            std::vector<std::shared_ptr<ITransportStatus>>{},
            std::make_unique<MessageQueue<std::string>>(),
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>());
    _libJoynrMessageRouter->init();

    _libJoynrMessageRouter->setParentAddress(routingProviderParticipantId, ccMessagingAddress);
    startLibJoynrMessagingSkeleton(_libJoynrMessageRouter);

    _messageSender = std::make_shared<MessageSender>(
            _libJoynrMessageRouter, _keyChain, _messagingSettings.getTtlUpliftMs());
    _joynrDispatcher =
            std::make_shared<Dispatcher>(_messageSender, _singleThreadIOService->getIOService());
    _messageSender->registerDispatcher(_joynrDispatcher);

    // create the inprocess skeleton for the dispatcher
    _dispatcherMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(_joynrDispatcher);
    _dispatcherAddress = std::make_shared<InProcessMessagingAddress>(_dispatcherMessagingSkeleton);

    _publicationManager =
            std::make_shared<PublicationManager>(_singleThreadIOService->getIOService(),
                                                 _messageSender,
                                                 _messagingSettings.getTtlUpliftMs());

    _subscriptionManager = std::make_shared<SubscriptionManager>(
            _singleThreadIOService->getIOService(), _libJoynrMessageRouter);

    auto joynrMessagingConnectorFactory =
            std::make_shared<JoynrMessagingConnectorFactory>(_messageSender, _subscriptionManager);

    _proxyFactory = std::make_unique<ProxyFactory>(joynrMessagingConnectorFactory);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = _libjoynrSettings->getParticipantIdsPersistenceFilename();
    _participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    // initialize the dispatchers
    _joynrDispatcher->registerPublicationManager(_publicationManager);
    _joynrDispatcher->registerSubscriptionManager(_subscriptionManager);

    _discoveryProxy = std::make_shared<LocalDiscoveryAggregator>(getProvisionedEntries());

    auto onSuccessBuildInternalProxies =
            [ thisSharedPtr = shared_from_this(), this, onSuccess, onError ]()
    {
        auto onSuccessGetGlobalAddress =
                [thisSharedPtr, this, onSuccess, onError](const std::string& globalAddress) {
            const std::string savedGlobalAddress(globalAddress);

            auto onSuccessGetReplyToAddress = [thisSharedPtr, this, onSuccess, savedGlobalAddress](
                    const std::string& replyAddress) {

                _messageSender->setReplyToAddress(replyAddress);

                std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
                dispatcherList.push_back(_joynrDispatcher);

                _capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
                        dispatcherList,
                        _discoveryProxy,
                        _participantIdStorage,
                        _dispatcherAddress,
                        _libJoynrMessageRouter,
                        _messagingSettings.getDiscoveryEntryExpiryIntervalMs(),
                        _publicationManager,
                        savedGlobalAddress);

                if (onSuccess) {
                    onSuccess();
                }
            };

            auto onErrorGetReplyToAddress =
                    [onError](const joynr::exceptions::JoynrRuntimeException& error) {
                JOYNR_LOG_FATAL(logger(),
                                "onErrorGetReplyToAddress: got exception: {}",
                                error.getMessage());
                if (onError) {
                    onError(error);
                }
            };

            _ccRoutingProxy->getReplyToAddressAsync(
                    std::move(onSuccessGetReplyToAddress), std::move(onErrorGetReplyToAddress));
        };

        auto onErrorGetGlobalAddress =
                [onError](const joynr::exceptions::JoynrRuntimeException& error) {
            JOYNR_LOG_FATAL(
                    logger(), "onErrorGetGlobalAddress: got exception: {}", error.getMessage());
            if (onError) {
                onError(error);
            }
        };

        _ccRoutingProxy->getGlobalAddressAsync(
                std::move(onSuccessGetGlobalAddress), std::move(onErrorGetGlobalAddress));
    };

    auto onErrorBuildInternalProxies =
            [onError](const joynr::exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_FATAL(
                logger(), "onErrorBuildInternalProxies: got exception: {}", error.getMessage());
        if (onError) {
            onError(error);
        }
    };

    buildInternalProxies(joynrMessagingConnectorFactory,
                         onSuccessBuildInternalProxies,
                         onErrorBuildInternalProxies);
}

void LibJoynrRuntime::buildInternalProxies(
        std::shared_ptr<JoynrMessagingConnectorFactory> connectorFactory,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError)
{
    /*
     * Extend the default ttl by 10 seconds in order to allow the cluster controller to handle
     * timeout for global discovery requests and send back the response to discoveryProxy.
     * This assumes that the communication to the cluster-controller is way faster then the one
     * from the cluster-controller to the backend.
     * This value should be greater then the TTL set on the cluster-controller side to communicate
     * with the global capabilities directory (per default Messaging::default_ttl).
     */
    const std::uint64_t messagingTtl = joynr::MessagingQos().getTtl() + 10000;
    const MessagingQos joynrInternalMessagingQos(messagingTtl);
    const bool isGloballyVisible = false;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = true;
    const std::string systemServicesDomain = _systemServicesSettings.getDomain();

    _ccRoutingProxy = std::make_shared<joynr::system::RoutingProxy>(
            shared_from_this(), connectorFactory, systemServicesDomain, joynrInternalMessagingQos);
    const std::string ccRoutingProviderParticipantId =
            _systemServicesSettings.getCcRoutingProviderParticipantId();
    const joynr::types::DiscoveryEntryWithMetaInfo ccRoutingDiscoveryEntry =
            getProvisionedEntries()[ccRoutingProviderParticipantId];
    _ccRoutingProxy->handleArbitrationFinished(ccRoutingDiscoveryEntry);

    // the following adds a RoutingEntry only to local RoutingTable since parentRouter is not set
    // yet
    auto onSuccessAddNextHopRoutingProxy = [
        onSuccess,
        onError,
        connectorFactory,
        systemServicesDomain,
        joynrInternalMessagingQos,
        thisSharedPtr = shared_from_this(),
        this
    ]()
    {

        // the following call invokes another addNextHopToParent after setting the parentRouter
        // this results in async call to cluster controller; we need to wait for it to finish
        // since without an entry in the RoutingTable of the cluster controller any further
        // responses to requests over the routingProxy could be discarded

        auto onSuccessSetParentRouter = [
            onSuccess,
            onError,
            connectorFactory,
            systemServicesDomain,
            joynrInternalMessagingQos,
            thisSharedPtr = shared_from_this(),
            this // need to capture this as well because shared_ptr does not allow to access
                 // protected / private members
        ]()
        {
            auto clusterControllerDiscovery =
                    std::make_shared<joynr::system::DiscoveryProxy>(thisSharedPtr,
                                                                    connectorFactory,
                                                                    systemServicesDomain,
                                                                    joynrInternalMessagingQos);
            const std::string ccDiscoveryProviderParticipantId =
                    _systemServicesSettings.getCcDiscoveryProviderParticipantId();
            const joynr::types::DiscoveryEntryWithMetaInfo ccDiscoveryEntry =
                    getProvisionedEntries()[ccDiscoveryProviderParticipantId];
            clusterControllerDiscovery->handleArbitrationFinished(ccDiscoveryEntry);

            auto onSuccessAddNextHopDiscoveryProxy =
                    [onSuccess, clusterControllerDiscovery, thisSharedPtr, this]() {
                _discoveryProxy->setDiscoveryProxy(clusterControllerDiscovery);
                onSuccess();
            };

            auto onErrorAddNextHopDiscoveryProxy =
                    [onError](const joynr::exceptions::ProviderRuntimeException& error) {
                JOYNR_LOG_FATAL(logger(),
                                "Failed to call add next hop for "
                                "clusterControllerDiscovery: {}",
                                error.getMessage());
                onError(error);
            };

            _libJoynrMessageRouter->addNextHop(clusterControllerDiscovery->getProxyParticipantId(),
                                               _dispatcherAddress,
                                               isGloballyVisible,
                                               expiryDateMs,
                                               isSticky,
                                               onSuccessAddNextHopDiscoveryProxy,
                                               onErrorAddNextHopDiscoveryProxy);
        };

        auto onErrorSetParentRouter =
                [onError](const joynr::exceptions::ProviderRuntimeException& error) {
            JOYNR_LOG_FATAL(logger(),
                            "Failed to call setParentRouter for RoutingProxy: {}",
                            error.getMessage());
            onError(error);
        };

        _libJoynrMessageRouter->setParentRouter(
                _ccRoutingProxy, onSuccessSetParentRouter, onErrorSetParentRouter);
    };

    auto onErrorAddNextHopRoutingProxy =
            [onError](const joynr::exceptions::ProviderRuntimeException& error) {
        JOYNR_LOG_FATAL(logger(), "Failed to call setParentRouter: {}", error.getMessage());
        onError(error);
    };

    _libJoynrMessageRouter->addNextHop(_ccRoutingProxy->getProxyParticipantId(),
                                       _dispatcherAddress,
                                       isGloballyVisible,
                                       expiryDateMs,
                                       isSticky,
                                       onSuccessAddNextHopRoutingProxy,
                                       onErrorAddNextHopRoutingProxy);
}

std::shared_ptr<IMessageRouter> LibJoynrRuntime::getMessageRouter()
{
    return _libJoynrMessageRouter;
}

} // namespace joynr
