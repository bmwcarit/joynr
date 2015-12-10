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
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"

#include <QtConcurrent/QtConcurrent>

#include "joynr/Dispatcher.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/PublicationManager.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/MessageRouter.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/TypeUtil.h"
#include "joynr/Util.h"
#include <vector>

namespace joynr
{

LibJoynrRuntime::LibJoynrRuntime(Settings* settings)
        : JoynrRuntime(*settings),
          connectorFactory(NULL),
          subscriptionManager(NULL),
          inProcessPublicationSender(NULL),
          inProcessConnectorFactory(NULL),
          joynrMessagingConnectorFactory(NULL),
          joynrMessagingSendStub(NULL),
          joynrMessageSender(NULL),
          joynrDispatcher(NULL),
          inProcessDispatcher(NULL),
          settings(settings),
          libjoynrSettings(new LibjoynrSettings(*settings)),
          dispatcherMessagingSkeleton(NULL),
          runtimeExecutor(NULL)
{
    libjoynrSettings->printSettings();
}

LibJoynrRuntime::~LibJoynrRuntime()
{
    delete proxyFactory;
    delete inProcessDispatcher;
    delete capabilitiesRegistrar;
    delete joynrMessageSender;
    delete joynrDispatcher;
    delete libjoynrSettings;
    libjoynrSettings = nullptr;
    delete settings;
    if (runtimeExecutor != nullptr) {
        runtimeExecutor->stop();
        runtimeExecutor->deleteLater();
        runtimeExecutor = nullptr;
    }
}

void LibJoynrRuntime::init(
        IMiddlewareMessagingStubFactory* middlewareMessagingStubFactory,
        std::shared_ptr<joynr::system::RoutingTypes::Address> libjoynrMessagingAddress,
        std::shared_ptr<joynr::system::RoutingTypes::Address> ccMessagingAddress)
{
    // create messaging stub factory
    MessagingStubFactory* messagingStubFactory = new MessagingStubFactory();
    messagingStubFactory->registerStubFactory(middlewareMessagingStubFactory);
    messagingStubFactory->registerStubFactory(new InProcessMessagingStubFactory());

    // create message router
    messageRouter = std::shared_ptr<MessageRouter>(
            new MessageRouter(messagingStubFactory, libjoynrMessagingAddress));

    startLibJoynrMessagingSkeleton(*messageRouter);

    joynrMessageSender = new JoynrMessageSender(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender);
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    // create the inprocess skeleton for the dispatcher
    dispatcherMessagingSkeleton = std::shared_ptr<InProcessMessagingSkeleton>(
            new InProcessLibJoynrMessagingSkeleton(joynrDispatcher));
    dispatcherAddress = std::shared_ptr<joynr::system::RoutingTypes::Address>(
            new InProcessMessagingAddress(dispatcherMessagingSkeleton));

    publicationManager = new PublicationManager();
    subscriptionManager = new SubscriptionManager();
    inProcessDispatcher = new InProcessDispatcher();

    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);
    inProcessConnectorFactory = new InProcessConnectorFactory(
            subscriptionManager,
            publicationManager,
            inProcessPublicationSender,
            dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher));
    joynrMessagingConnectorFactory =
            new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    connectorFactory =
            new ConnectorFactory(inProcessConnectorFactory, joynrMessagingConnectorFactory);

    proxyFactory = new ProxyFactory(libjoynrMessagingAddress, connectorFactory, NULL);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage =
            std::shared_ptr<ParticipantIdStorage>(new ParticipantIdStorage(persistenceFilename));

    // initialize the dispatchers
    std::vector<IDispatcher*> dispatcherList;
    dispatcherList.push_back(inProcessDispatcher);
    dispatcherList.push_back(joynrDispatcher);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = new LocalDiscoveryAggregator(
            *dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher), systemServicesSettings);
    std::string systemServicesDomain = systemServicesSettings.getDomain();
    QString routingProviderParticipantId =
            TypeUtil::toQt(systemServicesSettings.getCcRoutingProviderParticipantId());

    DiscoveryQos routingProviderDiscoveryQos;
    routingProviderDiscoveryQos.setCacheMaxAge(1000);
    routingProviderDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    routingProviderDiscoveryQos.addCustomParameter(
            "fixedParticipantId", TypeUtil::toStd(routingProviderParticipantId));
    routingProviderDiscoveryQos.setDiscoveryTimeout(50);

    auto routingProxyBuilder =
            createProxyBuilder<joynr::system::RoutingProxy>(systemServicesDomain);
    auto routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(10000))
                                ->setCached(false)
                                ->setDiscoveryQos(routingProviderDiscoveryQos)
                                ->build();
    messageRouter->setParentRouter(
            routingProxy, ccMessagingAddress, routingProviderParticipantId.toStdString());
    delete routingProxyBuilder;

    // setup discovery
    QString discoveryProviderParticipantId =
            TypeUtil::toQt(systemServicesSettings.getCcDiscoveryProviderParticipantId());
    DiscoveryQos discoveryProviderDiscoveryQos;
    discoveryProviderDiscoveryQos.setCacheMaxAge(1000);
    discoveryProviderDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryProviderDiscoveryQos.addCustomParameter(
            "fixedParticipantId", TypeUtil::toStd(discoveryProviderParticipantId));
    discoveryProviderDiscoveryQos.setDiscoveryTimeout(1000);

    ProxyBuilder<joynr::system::DiscoveryProxy>* discoveryProxyBuilder =
            createProxyBuilder<joynr::system::DiscoveryProxy>(systemServicesDomain);
    discoveryProxy->setDiscoveryProxy(discoveryProxyBuilder->setMessagingQos(MessagingQos(10000))
                                              ->setCached(false)
                                              ->setDiscoveryQos(discoveryProviderDiscoveryQos)
                                              ->build());
    capabilitiesRegistrar = new CapabilitiesRegistrar(dispatcherList,
                                                      *discoveryProxy,
                                                      libjoynrMessagingAddress,
                                                      participantIdStorage,
                                                      dispatcherAddress,
                                                      messageRouter);
}

void LibJoynrRuntime::unregisterProvider(const std::string& participantId)
{
    assert(capabilitiesRegistrar);
    capabilitiesRegistrar->remove(participantId);
}

void LibJoynrRuntime::setRuntimeExecutor(JoynrRuntimeExecutor* runtimeExecutor)
{
    this->runtimeExecutor = runtimeExecutor;
}

LibJoynrRuntime* LibJoynrRuntime::create(JoynrRuntimeExecutor* runtimeExecutor)
{
    LibJoynrRuntime* runtime = runtimeExecutor->getRuntime();
    runtime->setRuntimeExecutor(runtimeExecutor);
    return runtime;
}

} // namespace joynr
