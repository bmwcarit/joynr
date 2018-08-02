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
#include <memory>
#include <vector>

#include "joynr/Dispatcher.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MessageSender.h"
#include "joynr/MessageQueue.h"
#include "joynr/LibJoynrMessageRouter.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/PublicationManager.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/Util.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/system/RoutingProxy.h"
#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"

namespace joynr
{

class ITransportStatus;

LibJoynrRuntime::LibJoynrRuntime(std::unique_ptr<Settings> settings,
                                 std::shared_ptr<IKeychain> keyChain)
        : JoynrRuntimeImpl(*settings, std::move(keyChain)),
          subscriptionManager(nullptr),
          messageSender(nullptr),
          joynrDispatcher(nullptr),
          settings(std::move(settings)),
          libjoynrSettings(new LibjoynrSettings(*this->settings)),
          dispatcherMessagingSkeleton(nullptr),
          libJoynrMessageRouter(nullptr),
          libJoynrRuntimeIsShuttingDown(false)
{
    libjoynrSettings->printSettings();
    singleThreadIOService->start();
}

LibJoynrRuntime::~LibJoynrRuntime()
{
    // make sure shutdown() has been called earlier
    assert(libJoynrRuntimeIsShuttingDown);
}

void LibJoynrRuntime::shutdown()
{
    // protect against parallel and multiple calls of shutdown()
    bool previousValue = std::atomic_exchange_explicit(
            &libJoynrRuntimeIsShuttingDown, true, std::memory_order_acquire);
    assert(!previousValue);
    // bail out in case assert is disabled
    if (previousValue) {
        return;
    }

    std::lock_guard<std::mutex> lock(proxyBuildersMutex);
    for (auto proxyBuilder : proxyBuilders) {
        proxyBuilder->stop();
        proxyBuilder.reset();
    }
    proxyBuilders.clear();

    if (joynrDispatcher) {
        joynrDispatcher->shutdown();
        joynrDispatcher.reset();
    }
    if (publicationManager) {
        publicationManager->shutdown();
    }
    if (subscriptionManager) {
        subscriptionManager->shutdown();
    }
    if (libjoynrSettings) {
        delete libjoynrSettings;
        libjoynrSettings = nullptr;
    }
    if (libJoynrMessageRouter) {
        libJoynrMessageRouter->shutdown();
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
            systemServicesSettings.getCcRoutingProviderParticipantId();
    // create message router
    libJoynrMessageRouter = std::make_shared<LibJoynrMessageRouter>(
            messagingSettings,
            libjoynrMessagingAddress,
            std::move(messagingStubFactory),
            singleThreadIOService->getIOService(),
            std::move(addressCalculator),
            libjoynrSettings->isMessageRouterPersistencyEnabled(),
            std::vector<std::shared_ptr<ITransportStatus>>{},
            std::make_unique<MessageQueue<std::string>>(),
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>());
    libJoynrMessageRouter->init();

    libJoynrMessageRouter->loadRoutingTable(
            libjoynrSettings->getMessageRouterPersistenceFilename());
    libJoynrMessageRouter->setParentAddress(routingProviderParticipantId, ccMessagingAddress);
    startLibJoynrMessagingSkeleton(libJoynrMessageRouter);

    messageSender = std::make_shared<MessageSender>(
            libJoynrMessageRouter, keyChain, messagingSettings.getTtlUpliftMs());
    joynrDispatcher =
            std::make_shared<Dispatcher>(messageSender, singleThreadIOService->getIOService());
    messageSender->registerDispatcher(joynrDispatcher);

    // create the inprocess skeleton for the dispatcher
    dispatcherMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(joynrDispatcher);
    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(dispatcherMessagingSkeleton);

    publicationManager = std::make_shared<PublicationManager>(
            singleThreadIOService->getIOService(),
            messageSender,
            libjoynrSettings->isSubscriptionPersistencyEnabled(),
            messagingSettings.getTtlUpliftMs());
    publicationManager->loadSavedAttributeSubscriptionRequestsMap(
            libjoynrSettings->getSubscriptionRequestPersistenceFilename());
    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(
            libjoynrSettings->getBroadcastSubscriptionRequestPersistenceFilename());

    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadIOService->getIOService(), libJoynrMessageRouter);

    auto joynrMessagingConnectorFactory =
            std::make_shared<JoynrMessagingConnectorFactory>(messageSender, subscriptionManager);

    proxyFactory = std::make_unique<ProxyFactory>(joynrMessagingConnectorFactory);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    // initialize the dispatchers
    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = std::make_shared<LocalDiscoveryAggregator>(getProvisionedEntries());

    buildInternalProxies(joynrMessagingConnectorFactory);

    auto globalAddressFuture = ccRoutingProxy->getGlobalAddressAsync();
    auto onSuccessWrapper = [
        this,
        onSuccess = std::move(onSuccess),
        onError,
        globalAddressFuture = std::move(globalAddressFuture)
    ](const std::string& replyAddress)
    {
        std::string globalAddress;
        try {
            globalAddressFuture->get(globalAddress);
        } catch (const exceptions::JoynrRuntimeException& error) {
            const std::string errorMessage =
                    "Failed to retrieve global address from cluster controller: " +
                    error.getMessage();
            JOYNR_LOG_FATAL(logger(), errorMessage);
            exceptions::JoynrRuntimeException wrappedError(errorMessage);
            if (onError) {
                onError(wrappedError);
            }
            return;
        }
        messageSender->setReplyToAddress(replyAddress);

        std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
        dispatcherList.push_back(joynrDispatcher);

        capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
                dispatcherList,
                discoveryProxy,
                participantIdStorage,
                dispatcherAddress,
                libJoynrMessageRouter,
                messagingSettings.getDiscoveryEntryExpiryIntervalMs(),
                publicationManager,
                globalAddress);

        if (onSuccess) {
            onSuccess();
        }
    };

    ccRoutingProxy->getReplyToAddressAsync(std::move(onSuccessWrapper), std::move(onError));
}

void LibJoynrRuntime::buildInternalProxies(
        std::shared_ptr<JoynrMessagingConnectorFactory> connectorFactory)
{
    const std::uint64_t messagingTtl = 60000;
    const MessagingQos joynrInternalMessagingQos(messagingTtl);
    const bool isGloballyVisible = false;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = true;
    const std::string systemServicesDomain = systemServicesSettings.getDomain();

    ccRoutingProxy = std::make_shared<joynr::system::RoutingProxy>(
            shared_from_this(), connectorFactory, systemServicesDomain, joynrInternalMessagingQos);
    const std::string ccRoutingProviderParticipantId =
            systemServicesSettings.getCcRoutingProviderParticipantId();
    const joynr::types::DiscoveryEntryWithMetaInfo ccRoutingDiscoveryEntry =
            getProvisionedEntries()[ccRoutingProviderParticipantId];
    ccRoutingProxy->handleArbitrationFinished(ccRoutingDiscoveryEntry);
    libJoynrMessageRouter->addNextHop(ccRoutingProxy->getProxyParticipantId(),
                                      dispatcherAddress,
                                      isGloballyVisible,
                                      expiryDateMs,
                                      isSticky,
                                      allowUpdate);
    libJoynrMessageRouter->setParentRouter(ccRoutingProxy);

    auto clusterControllerDiscovery = std::make_shared<joynr::system::DiscoveryProxy>(
            shared_from_this(), connectorFactory, systemServicesDomain, joynrInternalMessagingQos);
    const std::string ccDiscoveryProviderParticipantId =
            systemServicesSettings.getCcDiscoveryProviderParticipantId();
    const joynr::types::DiscoveryEntryWithMetaInfo ccDiscoveryEntry =
            getProvisionedEntries()[ccDiscoveryProviderParticipantId];
    clusterControllerDiscovery->handleArbitrationFinished(ccDiscoveryEntry);
    libJoynrMessageRouter->addNextHop(clusterControllerDiscovery->getProxyParticipantId(),
                                      dispatcherAddress,
                                      isGloballyVisible,
                                      expiryDateMs,
                                      isSticky,
                                      allowUpdate);
    discoveryProxy->setDiscoveryProxy(std::move(clusterControllerDiscovery));
}

std::shared_ptr<IMessageRouter> LibJoynrRuntime::getMessageRouter()
{
    return libJoynrMessageRouter;
}

} // namespace joynr
