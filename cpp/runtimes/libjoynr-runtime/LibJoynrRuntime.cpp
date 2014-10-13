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
#include "joynr/system/CommonApiDbusAddress.h"
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

#include "joynr/Util.h"

namespace joynr {

LibJoynrRuntime::LibJoynrRuntime(QSettings* settings):
    JoynrRuntime(*settings),
    connectorFactory(NULL),
    publicationManager(NULL),
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

LibJoynrRuntime::~LibJoynrRuntime() {
    delete proxyFactory;
    delete inProcessDispatcher;
    delete capabilitiesRegistrar;
    delete joynrMessageSender;
    delete joynrDispatcher;
    delete libjoynrSettings;
    libjoynrSettings = Q_NULLPTR;
    settings->clear();
    settings->deleteLater();
    if(runtimeExecutor != Q_NULLPTR) {
        runtimeExecutor->stop();
        runtimeExecutor->deleteLater();
        runtimeExecutor = Q_NULLPTR;
    }
}

void LibJoynrRuntime::init(
        IMiddlewareMessagingStubFactory *middlewareMessagingStubFactory,
        QSharedPointer<joynr::system::Address> libjoynrMessagingAddress,
        QSharedPointer<joynr::system::Address> ccMessagingAddress
) {
    // create messaging stub factory
    MessagingStubFactory* messagingStubFactory = new MessagingStubFactory();
    messagingStubFactory->registerStubFactory(middlewareMessagingStubFactory);
    messagingStubFactory->registerStubFactory(new InProcessMessagingStubFactory());

    // create message router
    messageRouter = QSharedPointer<MessageRouter>(
                new MessageRouter(messagingStubFactory, libjoynrMessagingAddress)
    );

    startLibJoynrMessagingSkeleton(*messageRouter);

    joynrMessageSender = new JoynrMessageSender(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender);
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    // create the inprocess skeleton for the dispatcher
    dispatcherMessagingSkeleton = QSharedPointer<InProcessMessagingSkeleton>(
                new InProcessLibJoynrMessagingSkeleton(joynrDispatcher)
    );
    dispatcherAddress = QSharedPointer<joynr::system::Address>(
                new InProcessMessagingAddress(dispatcherMessagingSkeleton)
    );

    publicationManager = new PublicationManager();
    subscriptionManager = new SubscriptionManager();
    inProcessDispatcher = new InProcessDispatcher();

    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);
    inProcessConnectorFactory = new InProcessConnectorFactory(
                subscriptionManager,
                publicationManager,
                inProcessPublicationSender,
                dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher)
    );
    joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    connectorFactory = new ConnectorFactory(inProcessConnectorFactory, joynrMessagingConnectorFactory);

    proxyFactory = new ProxyFactory(libjoynrMessagingAddress, connectorFactory, NULL);

    // Set up the persistence file for storing provider participant ids
    QString persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage = QSharedPointer<ParticipantIdStorage>(new ParticipantIdStorage(persistenceFilename));

    // initialize the dispatchers
    QList<IDispatcher *> dispatcherList;
    dispatcherList.append(inProcessDispatcher);
    dispatcherList.append(joynrDispatcher);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = new LocalDiscoveryAggregator(
                *dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher),
                systemServicesSettings
    );
    QString systemServicesDomain = systemServicesSettings.getDomain();
    QString routingProviderParticipantId = systemServicesSettings.getCcRoutingProviderParticipantId();

    DiscoveryQos routingProviderDiscoveryQos;
    routingProviderDiscoveryQos.setCacheMaxAge(1000);
    routingProviderDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    routingProviderDiscoveryQos.addCustomParameter("fixedParticipantId", routingProviderParticipantId);
    routingProviderDiscoveryQos.setDiscoveryTimeout(50);

    auto routingProxyBuilder = getProxyBuilder<joynr::system::RoutingProxy>(systemServicesDomain);
    auto routingProxy = routingProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(routingProviderDiscoveryQos)
            ->build();
    messageRouter->setParentRouter(routingProxy, ccMessagingAddress, routingProviderParticipantId);
    delete routingProxyBuilder;

    // setup discovery
    QString discoveryProviderParticipantId = systemServicesSettings.getCcDiscoveryProviderParticipantId();
    DiscoveryQos discoveryProviderDiscoveryQos;
    discoveryProviderDiscoveryQos.setCacheMaxAge(1000);
    discoveryProviderDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryProviderDiscoveryQos.addCustomParameter("fixedParticipantId", discoveryProviderParticipantId);
    discoveryProviderDiscoveryQos.setDiscoveryTimeout(1000);

    ProxyBuilder<joynr::system::DiscoveryProxy>* discoveryProxyBuilder =
            getProxyBuilder<joynr::system::DiscoveryProxy>(systemServicesDomain);
    discoveryProxy->setDiscoveryProxy(
                discoveryProxyBuilder
                ->setRuntimeQos(MessagingQos(5000))
                ->setCached(false)
                ->setDiscoveryQos(discoveryProviderDiscoveryQos)
                ->build()
    );
    capabilitiesRegistrar = new CapabilitiesRegistrar(
                dispatcherList,
                *discoveryProxy,
                libjoynrMessagingAddress,
                participantIdStorage,
                dispatcherAddress,
                messageRouter
    );
}

void LibJoynrRuntime::unregisterCapability(QString participantId){
    assert(capabilitiesRegistrar);
    capabilitiesRegistrar->remove(participantId);
}

void LibJoynrRuntime::setRuntimeExecutor(JoynrRuntimeExecutor *runtimeExecutor)
{
    this->runtimeExecutor = runtimeExecutor;
}

LibJoynrRuntime *LibJoynrRuntime::create(JoynrRuntimeExecutor *runtimeExecutor) {
    LibJoynrRuntime *runtime = runtimeExecutor->getRuntime();
    runtime->setRuntimeExecutor(runtimeExecutor);
    return runtime;
}


} // namespace joynr
