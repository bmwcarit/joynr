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
#include "libjoynr/dbus/DbusCapabilitiesStubAdapter.h"
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
    messagingEndpointDirectory(new Directory<QString, joynr::system::Address>(QString("JoynrClusterControllerRuntime-MessagingEndpointDirectory"))),
    systemServiceSettings(NULL),
    dispatcherMessagingSkeleton(NULL)
{
    initializeAllDependencies();
}

LibJoynrRuntime::~LibJoynrRuntime() {
    delete proxyFactory;
    delete joynrCapabilitiesSendStub;
    delete inProcessDispatcher;
    delete capabilitiesRegistrar;
    delete joynrMessageSender;
    delete joynrDispatcher;
    delete dbusMessageRouterAdapter;
    delete libjoynrSettings;
    delete dbusSettings;
    delete messagingEndpointDirectory;
    delete systemServiceSettings;
    settings->clear();
    settings->deleteLater();
}

void LibJoynrRuntime::initializeAllDependencies() {
    assert(settings);
    libjoynrSettings = new LibjoynrSettings(*settings);
    libjoynrSettings->printSettings();
    dbusSettings = new DbusSettings(*settings);
    dbusSettings->printSettings();
    systemServiceSettings = new SystemServicesSettings(*settings);
    systemServiceSettings->printSettings();

    // create capabilities send stub
    QString ccCapabilitiesAddress(dbusSettings->createClusterControllerCapabilitiesAddressString());
    joynrCapabilitiesSendStub = new DbusCapabilitiesStubAdapter(ccCapabilitiesAddress);

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
    messageRouter = QSharedPointer<MessageRouter>(new MessageRouter(messagingEndpointDirectory, messagingStubFactory, libjoynrMessagingAddress));

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
    inProcessConnectorFactory = new InProcessConnectorFactory(subscriptionManager, publicationManager, inProcessPublicationSender);
    joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    connectorFactory = new ConnectorFactory(inProcessConnectorFactory, joynrMessagingConnectorFactory);

    proxyFactory = new ProxyFactory(joynrCapabilitiesSendStub, libjoynrMessagingAddress, connectorFactory, NULL);

    capabilitiesAggregator = QSharedPointer<CapabilitiesAggregator>(new CapabilitiesAggregator(joynrCapabilitiesSendStub, dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher)));

    // Set up the persistence file for storing provider participant ids
    QString persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage = QSharedPointer<ParticipantIdStorage>(new ParticipantIdStorage(persistenceFilename));

    // initialize the dispatchers
    QList<IDispatcher *> dispatcherList;
    dispatcherList.append(inProcessDispatcher);
    dispatcherList.append(joynrDispatcher);

    capabilitiesRegistrar = new CapabilitiesRegistrar(dispatcherList, capabilitiesAggregator, libjoynrMessagingAddress, participantIdStorage, dispatcherAddress, messageRouter);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    // create connection to parent routing service
    QSharedPointer<joynr::system::Address> parentAddress(
                new system::CommonApiDbusAddress(dbusSettings->getClusterControllerMessagingDomain(),
                                                 dbusSettings->getClusterControllerMessagingServiceName(),
                                                 dbusSettings->getClusterControllerMessagingParticipantId())
    );
    QString routingDomain = systemServiceSettings->getDomain();
    QString routingProviderParticipantId = systemServiceSettings->getCcRoutingProviderParticipantId();

    DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(1000);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.addCustomParameter("fixedParticipantId", routingProviderParticipantId);
    discoveryQos.setDiscoveryTimeout(50);

    auto routingProxyBuilder = this->getProxyBuilder<joynr::system::RoutingProxy>(routingDomain);
    auto routingProxy = routingProxyBuilder->setRuntimeQos(MessagingQos(5000))
                            ->setCached(false)
                            ->setDiscoveryQos(discoveryQos)
                            ->build();
    messageRouter->setParentRouter(routingProxy,  parentAddress, routingProviderParticipantId);
    delete routingProxyBuilder;
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
