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
#include "joynr/exceptions/JoynrException.h"
#include "joynr/IKeychain.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/MessageSender.h"
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
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"

namespace joynr
{

LibJoynrRuntime::LibJoynrRuntime(std::unique_ptr<Settings> settings,
                                 std::shared_ptr<IKeychain> keyChain)
        : JoynrRuntime(*settings, std::move(keyChain)),
          subscriptionManager(nullptr),
          inProcessPublicationSender(),
          messageSender(nullptr),
          joynrDispatcher(nullptr),
          inProcessDispatcher(),
          settings(std::move(settings)),
          libjoynrSettings(new LibjoynrSettings(*this->settings)),
          dispatcherMessagingSkeleton(nullptr),
          libJoynrMessageRouter(nullptr)
{
    libjoynrSettings->printSettings();
    singleThreadIOService->start();
}

LibJoynrRuntime::~LibJoynrRuntime()
{
    if (inProcessDispatcher) {
        inProcessDispatcher->shutdown();
        inProcessDispatcher.reset();
    }
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
    if (inProcessPublicationSender) {
        inProcessPublicationSender.reset();
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
    libJoynrMessageRouter =
            std::make_shared<LibJoynrMessageRouter>(messagingSettings,
                                                    libjoynrMessagingAddress,
                                                    std::move(messagingStubFactory),
                                                    singleThreadIOService->getIOService(),
                                                    std::move(addressCalculator));
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

    publicationManager = std::make_shared<PublicationManager>(singleThreadIOService->getIOService(),
                                                              messageSender,
                                                              messagingSettings.getTtlUpliftMs());
    publicationManager->loadSavedAttributeSubscriptionRequestsMap(
            libjoynrSettings->getSubscriptionRequestPersistenceFilename());
    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(
            libjoynrSettings->getBroadcastSubscriptionRequestPersistenceFilename());

    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadIOService->getIOService(), libJoynrMessageRouter);
    inProcessDispatcher =
            std::make_shared<InProcessDispatcher>(singleThreadIOService->getIOService());

    inProcessPublicationSender = std::make_shared<InProcessPublicationSender>(subscriptionManager);
    // TODO: replace raw ptr to IRequestCallerDirectory
    auto inProcessConnectorFactory = std::make_unique<InProcessConnectorFactory>(
            subscriptionManager,
            publicationManager,
            inProcessPublicationSender,
            std::dynamic_pointer_cast<IRequestCallerDirectory>(inProcessDispatcher));
    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(messageSender, subscriptionManager);

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            std::move(inProcessConnectorFactory), std::move(joynrMessagingConnectorFactory));
    proxyFactory = std::make_unique<ProxyFactory>(std::move(connectorFactory));

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    // initialize the dispatchers
    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = std::make_unique<LocalDiscoveryAggregator>(getProvisionedEntries());

    requestCallerDirectory =
            std::dynamic_pointer_cast<IRequestCallerDirectory>(inProcessDispatcher);

    std::string systemServicesDomain = systemServicesSettings.getDomain();

    DiscoveryQos routingProviderDiscoveryQos;
    routingProviderDiscoveryQos.setCacheMaxAgeMs(1000);
    routingProviderDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    routingProviderDiscoveryQos.addCustomParameter(
            "fixedParticipantId", routingProviderParticipantId);
    routingProviderDiscoveryQos.setDiscoveryTimeoutMs(50);

    std::shared_ptr<ProxyBuilder<joynr::system::RoutingProxy>> routingProxyBuilder =
            createProxyBuilder<joynr::system::RoutingProxy>(systemServicesDomain);

    std::uint64_t routingProxyTtl = 10000;
    auto routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(routingProxyTtl))
                                ->setDiscoveryQos(routingProviderDiscoveryQos)
                                ->build();

    libJoynrMessageRouter->setParentRouter(routingProxy);

    auto globalAddressFuture = routingProxy->getGlobalAddressAsync();
    auto onSuccessWrapper = [
        this,
        onSuccess = std::move(onSuccess),
        onError,
        globalAddressFuture = std::move(globalAddressFuture),
        routingProxyTtl
    ](const std::string& replyAddress)
    {
        std::string globalAddress;
        try {
            globalAddressFuture->get(routingProxyTtl, globalAddress);
        } catch (const exceptions::JoynrRuntimeException& error) {
            exceptions::JoynrRuntimeException wrappedError(
                    "Failed to retrieve global address from cluster controller: " +
                    error.getMessage());
            if (onError) {
                onError(wrappedError);
            }
            return;
        }
        messageSender->setReplyToAddress(replyAddress);

        std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
        dispatcherList.push_back(inProcessDispatcher);
        dispatcherList.push_back(joynrDispatcher);

        capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
                dispatcherList,
                *discoveryProxy,
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

    routingProxy->getReplyToAddressAsync(std::move(onSuccessWrapper), std::move(onError));

    // setup discovery
    std::string discoveryProviderParticipantId =
            systemServicesSettings.getCcDiscoveryProviderParticipantId();
    DiscoveryQos discoveryProviderDiscoveryQos;
    discoveryProviderDiscoveryQos.setCacheMaxAgeMs(1000);
    discoveryProviderDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryProviderDiscoveryQos.addCustomParameter(
            "fixedParticipantId", discoveryProviderParticipantId);
    discoveryProviderDiscoveryQos.setDiscoveryTimeoutMs(1000);

    std::shared_ptr<ProxyBuilder<joynr::system::DiscoveryProxy>> discoveryProxyBuilder =
            createProxyBuilder<joynr::system::DiscoveryProxy>(systemServicesDomain);

    auto proxy = discoveryProxyBuilder->setMessagingQos(MessagingQos(40000))
                         ->setDiscoveryQos(discoveryProviderDiscoveryQos)
                         ->build();

    discoveryProxy->setDiscoveryProxy(std::move(proxy));
}

std::shared_ptr<IMessageRouter> LibJoynrRuntime::getMessageRouter()
{
    return libJoynrMessageRouter;
}

} // namespace joynr
