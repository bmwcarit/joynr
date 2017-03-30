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
#include "joynr/InProcessDispatcher.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/PublicationManager.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/LibJoynrMessageRouter.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/Util.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"

namespace joynr
{

LibJoynrRuntime::LibJoynrRuntime(std::unique_ptr<Settings> settings)
        : JoynrRuntime(*settings),
          subscriptionManager(nullptr),
          joynrMessagingSendStub(nullptr),
          joynrMessageSender(nullptr),
          joynrDispatcher(nullptr),
          inProcessDispatcher(nullptr),
          settings(std::move(settings)),
          libjoynrSettings(new LibjoynrSettings(*this->settings)),
          dispatcherMessagingSkeleton(nullptr)
{
    libjoynrSettings->printSettings();
    singleThreadIOService->start();
}

LibJoynrRuntime::~LibJoynrRuntime()
{
    delete inProcessDispatcher;
    delete joynrDispatcher;
    delete libjoynrSettings;
    libjoynrSettings = nullptr;

    delete inProcessPublicationSender;
    inProcessPublicationSender = nullptr;
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
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress) {
        messagingStubFactory->remove(destinationAddress);
    });
    messagingStubFactory->registerStubFactory(middlewareMessagingStubFactory);
    messagingStubFactory->registerStubFactory(std::make_shared<InProcessMessagingStubFactory>());

    // create message router
    libJoynrMessageRouter =
            std::make_shared<LibJoynrMessageRouter>(libjoynrMessagingAddress,
                                                    std::move(messagingStubFactory),
                                                    singleThreadIOService->getIOService(),
                                                    std::move(addressCalculator));

    libJoynrMessageRouter->loadRoutingTable(
            libjoynrSettings->getMessageRouterPersistenceFilename());
    startLibJoynrMessagingSkeleton(libJoynrMessageRouter);

    joynrMessageSender = std::make_shared<JoynrMessageSender>(
            libJoynrMessageRouter, messagingSettings.getTtlUpliftMs());
    joynrDispatcher = new Dispatcher(joynrMessageSender, singleThreadIOService->getIOService());
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    // create the inprocess skeleton for the dispatcher
    dispatcherMessagingSkeleton =
            std::make_shared<InProcessLibJoynrMessagingSkeleton>(joynrDispatcher);
    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(dispatcherMessagingSkeleton);

    publicationManager = new PublicationManager(singleThreadIOService->getIOService(),
                                                joynrMessageSender.get(),
                                                messagingSettings.getTtlUpliftMs());
    publicationManager->loadSavedAttributeSubscriptionRequestsMap(
            libjoynrSettings->getSubscriptionRequestPersistenceFilename());
    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(
            libjoynrSettings->getBroadcastSubscriptionRequestPersistenceFilename());

    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadIOService->getIOService(), libJoynrMessageRouter);
    inProcessDispatcher = new InProcessDispatcher(singleThreadIOService->getIOService());

    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);
    auto inProcessConnectorFactory = std::make_unique<InProcessConnectorFactory>(
            subscriptionManager.get(),
            publicationManager,
            inProcessPublicationSender,
            dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher));
    auto joynrMessagingConnectorFactory = std::make_unique<JoynrMessagingConnectorFactory>(
            joynrMessageSender, subscriptionManager);

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            std::move(inProcessConnectorFactory), std::move(joynrMessagingConnectorFactory));
    proxyFactory =
            std::make_unique<ProxyFactory>(libjoynrMessagingAddress, std::move(connectorFactory));

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    // initialize the dispatchers
    std::vector<IDispatcher*> dispatcherList;
    dispatcherList.push_back(inProcessDispatcher);
    dispatcherList.push_back(joynrDispatcher);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = std::make_unique<LocalDiscoveryAggregator>(getProvisionedEntries());

    requestCallerDirectory = dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher);

    std::string systemServicesDomain = systemServicesSettings.getDomain();
    std::string routingProviderParticipantId =
            systemServicesSettings.getCcRoutingProviderParticipantId();

    DiscoveryQos routingProviderDiscoveryQos;
    routingProviderDiscoveryQos.setCacheMaxAgeMs(1000);
    routingProviderDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    routingProviderDiscoveryQos.addCustomParameter(
            "fixedParticipantId", routingProviderParticipantId);
    routingProviderDiscoveryQos.setDiscoveryTimeoutMs(50);

    std::unique_ptr<ProxyBuilder<joynr::system::RoutingProxy>> routingProxyBuilder =
            createProxyBuilder<joynr::system::RoutingProxy>(systemServicesDomain);
    auto routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(10000))
                                ->setDiscoveryQos(routingProviderDiscoveryQos)
                                ->build();
    libJoynrMessageRouter->setParentRouter(
            std::move(routingProxy), ccMessagingAddress, routingProviderParticipantId);

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

    std::unique_ptr<ProxyBuilder<joynr::system::DiscoveryProxy>> discoveryProxyBuilder =
            createProxyBuilder<joynr::system::DiscoveryProxy>(systemServicesDomain);

    auto proxy = discoveryProxyBuilder->setMessagingQos(MessagingQos(40000))
                         ->setDiscoveryQos(discoveryProviderDiscoveryQos)
                         ->build();

    discoveryProxy->setDiscoveryProxy(std::move(proxy));
    capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
            dispatcherList,
            *discoveryProxy,
            participantIdStorage,
            dispatcherAddress,
            libJoynrMessageRouter,
            messagingSettings.getDiscoveryEntryExpiryIntervalMs(),
            *publicationManager);

    libJoynrMessageRouter->queryGlobalClusterControllerAddress(
            std::move(onSuccess), std::move(onError));
}

std::shared_ptr<IMessageRouter> LibJoynrRuntime::getMessageRouter()
{
    return libJoynrMessageRouter;
}

} // namespace joynr
