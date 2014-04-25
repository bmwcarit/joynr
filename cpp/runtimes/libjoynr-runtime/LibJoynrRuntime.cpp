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
#include "joynr/Dispatcher.h"
#include "joynr/InProcessDispatcher.h"
#include "common/dbus/DbusMessagingStubAdapter.h"
#include "joynr/system/CommonApiDbusAddress.h"
#include "joynr/DBusMessageRouterAdapter.h"
#include "joynr/PublicationManager.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/JoynrMessageSender.h"
#include "common/dbus/DbusSettings.h"
#include "joynr/MessageRouter.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "libjoynr/dbus/DbusMessagingStubFactory.h"
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
    dbusMessageRouterAdapter(NULL),
    settings(settings),
    libjoynrSettings(NULL),
    dbusSettings(NULL),
    dispatcherMessagingSkeleton(NULL)
{
    initializeAllDependencies();
}

LibJoynrRuntime::~LibJoynrRuntime() {
    delete proxyFactory;
    delete inProcessDispatcher;
    delete capabilitiesRegistrar;
    delete joynrMessageSender;
    delete joynrDispatcher;
    delete dbusMessageRouterAdapter;
    delete libjoynrSettings;
    delete dbusSettings;
    settings->clear();
    settings->deleteLater();
}

void LibJoynrRuntime::initializeAllDependencies() {
    assert(settings);
    libjoynrSettings = new LibjoynrSettings(*settings);
    libjoynrSettings->printSettings();
    dbusSettings = new DbusSettings(*settings);
    dbusSettings->printSettings();

    QString messagingUuid = Util::createUuid().replace("-", "");
    QString libjoynrMessagingDomain("local");
    QString libjoynrMessagingServiceName("io.joynr.libjoynr.Messaging");
    QString libjoynrMessagingId("libjoynr.messaging.participantid_" + messagingUuid);
    QString libjoynrMessagingServiceUrl(libjoynrMessagingDomain + ":" + libjoynrMessagingServiceName + ":" + libjoynrMessagingId);
    QSharedPointer<joynr::system::Address> libjoynrMessagingAddress(new system::CommonApiDbusAddress(libjoynrMessagingDomain, libjoynrMessagingServiceName, libjoynrMessagingId));

    // create messaging stub factory
    MessagingStubFactory* messagingStubFactory = new MessagingStubFactory();
    messagingStubFactory->registerStubFactory(new DbusMessagingStubFactory());
    messagingStubFactory->registerStubFactory(new InProcessMessagingStubFactory());

    // create message router
    messageRouter = QSharedPointer<MessageRouter>(new MessageRouter(messagingStubFactory, libjoynrMessagingAddress));

    // create messaging skeleton using uuid
    dbusMessageRouterAdapter = new DBusMessageRouterAdapter(*messageRouter, libjoynrMessagingServiceUrl);

    joynrMessageSender = new JoynrMessageSender(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender);
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    // create the inprocess skeleton for the dispatcher
    dispatcherMessagingSkeleton = QSharedPointer<InProcessMessagingSkeleton> (new InProcessLibJoynrMessagingSkeleton(joynrDispatcher));
    dispatcherAddress = QSharedPointer<joynr::system::Address>(new InProcessMessagingAddress(dispatcherMessagingSkeleton));



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
    // create connection to parent routing service
    QSharedPointer<joynr::system::Address> ccMessagingAddress(
                new system::CommonApiDbusAddress(dbusSettings->getClusterControllerMessagingDomain(),
                                                 dbusSettings->getClusterControllerMessagingServiceName(),
                                                 dbusSettings->getClusterControllerMessagingParticipantId())
    );
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
    capabilitiesRegistrar->unregisterCapability(participantId);
}

LibJoynrRuntime *LibJoynrRuntime::create(QSettings* settings) {
    LibJoynrRuntime* runtime = new LibJoynrRuntime(settings);
    return runtime;
}

} // namespace joynr
