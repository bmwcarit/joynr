/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "JoynrClusterControllerRuntime.h"

#include <cassert>
#include <cstdint>
#include <chrono>
#include <functional>

#include <QCoreApplication>
#include <QThread>

#include "joynr/Dispatcher.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "cluster-controller/http-communication-manager/HttpReceiver.h"
#include "cluster-controller/http-communication-manager/HttpSender.h"
#include "joynr/MessagingSettings.h"
#include "cluster-controller/capabilities-client/CapabilitiesClient.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/PublicationManager.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/LocalChannelUrlDirectory.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "cluster-controller/messaging/joynr-messaging/JoynrMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "websocket/WebSocketCcMessagingSkeleton.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "libjoynr/joynr-messaging/DummyPlatformSecurityManager.h"
#include "joynr/Settings.h"
#include "joynr/LibjoynrSettings.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/BounceProxyUrl.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessageReceiver.h"
#include "joynr/IMessageSender.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/InProcessAddress.h"
#include "joynr/MessageRouter.h"
#include "joynr/MessagingQos.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/ProxyFactory.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/ChannelUrlDirectoryProxy.h"
#include "joynr/system/DiscoveryProvider.h"
#include "joynr/system/RoutingProvider.h"

#include "joynr/system/RoutingTypes/WebSocketAddress.h"

#include "joynr/system/DiscoveryRequestCaller.h"
#include "joynr/system/DiscoveryInProcessConnector.h"

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
#include "libjoynr/dbus/DbusMessagingStubFactory.h"
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

namespace joynr
{

INIT_LOGGER(JoynrClusterControllerRuntime);

JoynrClusterControllerRuntime::JoynrClusterControllerRuntime(QCoreApplication* app,
                                                             Settings* settings,
                                                             IMessageReceiver* messageReceiver,
                                                             IMessageSender* messageSender)
        : JoynrRuntime(*settings),
          joynrDispatcher(nullptr),
          inProcessDispatcher(nullptr),
          ccDispatcher(nullptr),
          subscriptionManager(nullptr),
          joynrMessagingSendSkeleton(nullptr),
          joynrMessageSender(nullptr),
          app(app),
          capabilitiesClient(nullptr),
          localCapabilitiesDirectory(nullptr),
          channelUrlDirectory(),
          cache(),
          channelUrlDirectoryProxy(nullptr),
          libJoynrMessagingSkeleton(nullptr),
          messageReceiver(messageReceiver),
          messageSender(messageSender),
          dispatcherList(),
          inProcessConnectorFactory(nullptr),
          inProcessPublicationSender(nullptr),
          joynrMessagingConnectorFactory(nullptr),
          connectorFactory(nullptr),
          settings(settings),
          messagingSettings(nullptr),
          libjoynrSettings(nullptr),
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
          dbusSettings(nullptr),
          ccDbusMessageRouterAdapter(nullptr),
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
          wsSettings(*settings),
          wsCcMessagingSkeleton(nullptr),
          securityManager(nullptr),
          messagingIsRunning(false)
{
    initializeAllDependencies();
}

void JoynrClusterControllerRuntime::initializeAllDependencies()
{
    /**
      * libjoynr side skeleton & dispatcher
      * This needs some preparation of libjoynr and clustercontroller parts.
      */
    messagingSettings = new MessagingSettings(*settings);
    messagingSettings->printSettings();
    libjoynrSettings = new LibjoynrSettings(*settings);
    libjoynrSettings->printSettings();
    wsSettings.printSettings();

    // Initialise security manager
    securityManager = new DummyPlatformSecurityManager();

    // CAREFUL: the factory creates an old style dispatcher, not the new one!

    inProcessDispatcher = new InProcessDispatcher();
    /* CC */
    // create the messaging stub factory
    MessagingStubFactory* messagingStubFactory = new MessagingStubFactory();
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(new DbusMessagingStubFactory());
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(new InProcessMessagingStubFactory());
    // init message router
    messageRouter = std::shared_ptr<MessageRouter>(
            new MessageRouter(messagingStubFactory, securityManager));
    // provision global capabilities directory
    std::shared_ptr<joynr::system::RoutingTypes::Address> globalCapabilitiesDirectoryAddress(
            new system::RoutingTypes::ChannelAddress(
                    messagingSettings->getCapabilitiesDirectoryChannelId()));
    messageRouter->addProvisionedNextHop(messagingSettings->getCapabilitiesDirectoryParticipantId(),
                                         globalCapabilitiesDirectoryAddress);
    // provision channel url directory
    std::shared_ptr<joynr::system::RoutingTypes::Address> globalChannelUrlDirectoryAddress(
            new system::RoutingTypes::ChannelAddress(
                    messagingSettings->getChannelUrlDirectoryChannelId()));
    messageRouter->addProvisionedNextHop(messagingSettings->getChannelUrlDirectoryParticipantId(),
                                         globalChannelUrlDirectoryAddress);

    // setup CC WebSocket interface
    WebSocketMessagingStubFactory* wsMessagingStubFactory = new WebSocketMessagingStubFactory();
    messagingStubFactory->registerStubFactory(wsMessagingStubFactory);
    system::RoutingTypes::WebSocketAddress wsAddress =
            wsSettings.createClusterControllerMessagingAddress();

    wsCcMessagingSkeleton =
            new WebSocketCcMessagingSkeleton(*messageRouter, *wsMessagingStubFactory, wsAddress);

    /* LibJoynr */
    assert(messageRouter);
    joynrMessageSender = new JoynrMessageSender(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender);
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    /* CC */
    // TODO: libjoynrmessagingskeleton now uses the Dispatcher, should it use the
    // InprocessDispatcher?
    libJoynrMessagingSkeleton = std::shared_ptr<InProcessMessagingSkeleton>(
            new InProcessLibJoynrMessagingSkeleton(joynrDispatcher));
    // EndpointAddress to messagingStub is transmitted when a provider is registered
    // messagingStubFactory->registerInProcessMessagingSkeleton(libJoynrMessagingSkeleton);

    /**
      * ClusterController side
      *
      */
    if (!messageReceiver) {
        JOYNR_LOG_INFO(
                logger,
                "The message receiver supplied is NULL, creating the default MessageReceiver");
        messageReceiver = std::shared_ptr<IMessageReceiver>(
                new HttpReceiver(*messagingSettings, messageRouter));
    }

    std::string channelId = messageReceiver->getReceiveChannelId();

    // create message sender
    if (!messageSender) {
        JOYNR_LOG_INFO(
                logger, "The message sender supplied is NULL, creating the default MessageSender");
        messageSender = std::shared_ptr<IMessageSender>(new HttpSender(
                messagingSettings->getBounceProxyUrl(),
                std::chrono::milliseconds(messagingSettings->getSendMsgMaxTtl()),
                std::chrono::milliseconds(messagingSettings->getSendMsgRetryInterval())));
    }
    messagingStubFactory->registerStubFactory(
            new JoynrMessagingStubFactory(messageSender, messageReceiver->getReceiveChannelId()));

    // joynrMessagingSendSkeleton = new DummyClusterControllerMessagingSkeleton(messageRouter);
    // ccDispatcher = DispatcherFactory::createDispatcherInSameThread(messagingSettings);

    // we currently have to use the fake client, because JAVA side is not yet working for
    // CapabilitiesServer.
    bool usingRealCapabilitiesClient =
            /*when switching this to true, turn on the UUID in systemintegrationtests again*/ true;
    capabilitiesClient = new CapabilitiesClient(channelId); // ownership of this is not transferred
    // try using the real capabilitiesClient again:
    // capabilitiesClient = new CapabilitiesClient(channelId);// ownership of this is not
    // transferred

    localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
            *messagingSettings, capabilitiesClient, *messageRouter);
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    dbusSettings = new DbusSettings(*settings);
    dbusSettings->printSettings();
    // register dbus skeletons for capabilities and messaging interfaces
    std::string ccMessagingAddress(dbusSettings->createClusterControllerMessagingAddressString());
    ccDbusMessageRouterAdapter = new DBusMessageRouterAdapter(*messageRouter, ccMessagingAddress);
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

    /**
      * libJoynr side
      *
      */
    publicationManager = new PublicationManager();
    subscriptionManager = new SubscriptionManager();
    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);
    std::shared_ptr<joynr::system::RoutingTypes::Address> libjoynrMessagingAddress(
            new InProcessMessagingAddress(libJoynrMessagingSkeleton));
    // subscriptionManager = new SubscriptionManager(...)
    inProcessConnectorFactory = new InProcessConnectorFactory(
            subscriptionManager,
            publicationManager,
            inProcessPublicationSender,
            dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher));
    joynrMessagingConnectorFactory =
            new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    connectorFactory =
            createConnectorFactory(inProcessConnectorFactory, joynrMessagingConnectorFactory);

    proxyFactory = new ProxyFactory(libjoynrMessagingAddress, connectorFactory, &cache);

    dispatcherList.push_back(joynrDispatcher);
    dispatcherList.push_back(inProcessDispatcher);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage =
            std::shared_ptr<ParticipantIdStorage>(new ParticipantIdStorage(persistenceFilename));

    dispatcherAddress = libjoynrMessagingAddress;
    discoveryProxy = new LocalDiscoveryAggregator(
            *dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher), systemServicesSettings);

    std::string discoveryProviderParticipantId(
            systemServicesSettings.getCcDiscoveryProviderParticipantId());
    std::shared_ptr<RequestCaller> discoveryRequestCaller(
            new joynr::system::DiscoveryRequestCaller(localCapabilitiesDirectory));
    std::shared_ptr<InProcessAddress> discoveryProviderAddress(
            new InProcessAddress(discoveryRequestCaller));
    joynr::system::DiscoveryInProcessConnector* discoveryInProcessConnector =
            InProcessConnectorFactoryHelper<joynr::system::IDiscoveryConnector>().create(
                    subscriptionManager,
                    publicationManager,
                    inProcessPublicationSender,
                    std::string(), // can be ignored
                    discoveryProviderParticipantId,
                    discoveryProviderAddress);

    discoveryProxy->setDiscoveryProxy(discoveryInProcessConnector);

    capabilitiesRegistrar = new CapabilitiesRegistrar(dispatcherList,
                                                      *discoveryProxy,
                                                      libjoynrMessagingAddress,
                                                      participantIdStorage,
                                                      dispatcherAddress,
                                                      messageRouter);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    /**
     * Finish initialising Capabilitiesclient by building a Proxy and passing it
     */
    std::int64_t discoveryMessagesTtl = messagingSettings->getDiscoveryMessagesTtl();

    if (usingRealCapabilitiesClient) {
        ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>* capabilitiesProxyBuilder =
                createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                        messagingSettings->getDiscoveryDirectoriesDomain());
        DiscoveryQos discoveryQos(10000);
        discoveryQos.setArbitrationStrategy(
                DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY); // actually only one provider
                                                                      // should be available
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy(
                capabilitiesProxyBuilder->setMessagingQos(MessagingQos(discoveryMessagesTtl))
                        ->setCached(true)
                        ->setDiscoveryQos(discoveryQos)
                        ->build());
        ((CapabilitiesClient*)capabilitiesClient)->init(capabilitiesProxy);
    }

    ProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>* channelUrlDirectoryProxyBuilder =
            createProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>(
                    messagingSettings->getDiscoveryDirectoriesDomain());

    DiscoveryQos discoveryQos(10000);
    discoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY); // actually only one provider
                                                                  // should be available
    channelUrlDirectoryProxy = std::shared_ptr<infrastructure::ChannelUrlDirectoryProxy>(
            channelUrlDirectoryProxyBuilder->setMessagingQos(MessagingQos(discoveryMessagesTtl))
                    ->setCached(true)
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    channelUrlDirectory = std::shared_ptr<ILocalChannelUrlDirectory>(
            new LocalChannelUrlDirectory(*messagingSettings, channelUrlDirectoryProxy));
    messageReceiver->init(channelUrlDirectory);
    messageSender->init(channelUrlDirectory, *messagingSettings);
}

ConnectorFactory* JoynrClusterControllerRuntime::createConnectorFactory(
        InProcessConnectorFactory* inProcessConnectorFactory,
        JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory)
{
    return new ConnectorFactory(inProcessConnectorFactory, joynrMessagingConnectorFactory);
}

void JoynrClusterControllerRuntime::registerRoutingProvider()
{
    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::system::RoutingProvider> routingProvider(messageRouter);
    std::string interfaceName(routingProvider->getInterfaceName());
    std::string participantId(systemServicesSettings.getCcRoutingProviderParticipantId());

    // provision the participant ID for the routing provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    registerProvider<joynr::system::RoutingProvider>(domain, routingProvider);
}

void JoynrClusterControllerRuntime::registerDiscoveryProvider()
{
    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::system::DiscoveryProvider> discoveryProvider(localCapabilitiesDirectory);
    std::string interfaceName(discoveryProvider->getInterfaceName());
    std::string participantId(systemServicesSettings.getCcDiscoveryProviderParticipantId());

    // provision the participant ID for the discovery provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    registerProvider<joynr::system::DiscoveryProvider>(domain, discoveryProvider);
}

JoynrClusterControllerRuntime::~JoynrClusterControllerRuntime()
{
    JOYNR_LOG_TRACE(logger, "entering ~JoynrClusterControllerRuntime");
    stopMessaging();

    if (joynrDispatcher != nullptr) {
        JOYNR_LOG_TRACE(logger, "joynrDispatcher");
        // joynrDispatcher->stopMessaging();
        delete joynrDispatcher;
    }

    delete inProcessDispatcher;
    inProcessDispatcher = nullptr;
    delete capabilitiesClient;
    capabilitiesClient = nullptr;

    delete inProcessPublicationSender;
    inProcessPublicationSender = nullptr;
    delete joynrMessageSender;
    delete proxyFactory;
    delete messagingSettings;
    delete libjoynrSettings;
    delete capabilitiesRegistrar;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    delete ccDbusMessageRouterAdapter;
    delete dbusSettings;
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    delete settings;

    JOYNR_LOG_TRACE(logger, "leaving ~JoynrClusterControllerRuntime");
}

void JoynrClusterControllerRuntime::startMessaging()
{
    //    assert(joynrDispatcher!=NULL);
    //    joynrDispatcher->startMessaging();
    //    joynrDispatcher->waitForMessaging();
    assert(messageReceiver != nullptr);
    if (!messagingIsRunning) {
        messageReceiver->startReceiveQueue();
        messagingIsRunning = true;
    }
}

void JoynrClusterControllerRuntime::stopMessaging()
{
    // joynrDispatcher->stopMessaging();
    if (messagingIsRunning) {
        messageReceiver->stopReceiveQueue();
        messagingIsRunning = false;
    }
}

void JoynrClusterControllerRuntime::runForever()
{
    app->exec();
}

JoynrClusterControllerRuntime* JoynrClusterControllerRuntime::create(Settings* settings)
{
    // Only allow one QCoreApplication instance
    static int argc = 0;
    static char* argv[] = {nullptr};
    static QCoreApplication* coreApplication =
            (QCoreApplication::instance() == nullptr) ? new QCoreApplication(argc, argv) : nullptr;

    JoynrClusterControllerRuntime* runtime =
            new JoynrClusterControllerRuntime(coreApplication, settings);
    runtime->start();
    return runtime;
}

void JoynrClusterControllerRuntime::unregisterProvider(const std::string& participantId)
{
    assert(capabilitiesRegistrar);
    capabilitiesRegistrar->remove(participantId);
}

void JoynrClusterControllerRuntime::start()
{
    startMessaging();
    registerRoutingProvider();
    registerDiscoveryProvider();
}

void JoynrClusterControllerRuntime::stop(bool deleteChannel)
{
    if (deleteChannel) {
        this->deleteChannel();
    }
    stopMessaging();
}

void JoynrClusterControllerRuntime::waitForChannelCreation()
{
    messageReceiver->waitForReceiveQueueStarted();
}

void JoynrClusterControllerRuntime::deleteChannel()
{
    messageReceiver->tryToDeleteChannel();
}

} // namespace joynr
