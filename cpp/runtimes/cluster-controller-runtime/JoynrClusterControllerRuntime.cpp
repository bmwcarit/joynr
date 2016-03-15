/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <QCoreApplication>
#include <QThread>
#include <boost/algorithm/string/predicate.hpp>
#include <cassert>

#include "mosquittopp.h"

#include "JoynrClusterControllerRuntime.h"

#include <cassert>
#include <cstdint>
#include <chrono>
#include <functional>

#include <QCoreApplication>

#include "joynr/Dispatcher.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "cluster-controller/http-communication-manager/HttpReceiver.h"
#include "cluster-controller/http-communication-manager/HttpSender.h"
#include "cluster-controller/http-communication-manager/HttpMessagingSkeleton.h"
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
#include "joynr/JsonSerializer.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/LocalChannelUrlDirectory.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "cluster-controller/messaging/joynr-messaging/HttpMessagingStubFactory.h"
#include "cluster-controller/messaging/joynr-messaging/MqttMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "websocket/WebSocketCcMessagingSkeleton.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "libjoynr/joynr-messaging/DummyPlatformSecurityManager.h"
#include "joynr/Settings.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/BrokerUrl.h"
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

#include "joynr/system/RoutingTypes/MqttProtocol.h"
#include "cluster-controller/mqtt/MqttReceiver.h"
#include "cluster-controller/mqtt/MqttSender.h"
#include "cluster-controller/mqtt/MqttMessagingSkeleton.h"

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
                                                             IMessageReceiver* httpMessageReceiver,
                                                             IMessageSender* httpMessageSender,
                                                             IMessageReceiver* mqttMessageReceiver,
                                                             IMessageSender* mqttMessageSender)

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
          httpMessagingSkeleton(nullptr),
          mqttMessagingSkeleton(nullptr),
          httpMessageReceiver(httpMessageReceiver),
          httpMessageSender(httpMessageSender),
          mqttMessageReceiver(mqttMessageReceiver),
          mqttMessageSender(mqttMessageSender),
          dispatcherList(),
          inProcessConnectorFactory(nullptr),
          inProcessPublicationSender(nullptr),
          joynrMessagingConnectorFactory(nullptr),
          connectorFactory(nullptr),
          settings(settings),
          libjoynrSettings(*settings),
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
          dbusSettings(nullptr),
          ccDbusMessageRouterAdapter(nullptr),
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
          wsSettings(*settings),
          wsCcMessagingSkeleton(nullptr),
          httpMessagingIsRunning(false),
          mqttMessagingIsRunning(false),
          doMqttMessaging(false),
          doHttpMessaging(false),
          mqttSettings()
{
    initializeAllDependencies();
}

void JoynrClusterControllerRuntime::initializeAllDependencies()
{
    /**
      * libjoynr side skeleton & dispatcher
      * This needs some preparation of libjoynr and clustercontroller parts.
      */
    messagingSettings.printSettings();
    libjoynrSettings.printSettings();
    wsSettings.printSettings();

    // Initialise security manager
    std::unique_ptr<IPlatformSecurityManager> securityManager =
            std::make_unique<DummyPlatformSecurityManager>();

    // CAREFUL: the factory creates an old style dispatcher, not the new one!
    inProcessDispatcher = new InProcessDispatcher();
    /* CC */
    // create the messaging stub factory
    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(std::make_unique<DbusMessagingStubFactory>());
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());
    // init message router
    messageRouter =
            std::make_shared<MessageRouter>(messagingStubFactory, std::move(securityManager));

    const BrokerUrl brokerUrl = messagingSettings.getBrokerUrl();
    assert(brokerUrl.getBrokerChannelsBaseUrl().isValid());
    const BrokerUrl bounceProxyUrl = messagingSettings.getBounceProxyUrl();
    assert(bounceProxyUrl.getBrokerChannelsBaseUrl().isValid());

    // If the BrokerUrl is a mqtt url, MQTT is used instead of HTTP
    const Url url = brokerUrl.getBrokerChannelsBaseUrl();
    std::string brokerProtocol = url.getProtocol();
    std::string bounceproxyProtocol = bounceProxyUrl.getBrokerChannelsBaseUrl().getProtocol();

    std::transform(brokerProtocol.begin(), brokerProtocol.end(), brokerProtocol.begin(), ::toupper);
    std::transform(bounceproxyProtocol.begin(),
                   bounceproxyProtocol.end(),
                   bounceproxyProtocol.begin(),
                   ::toupper);

    if (brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTT)) {
        JOYNR_LOG_INFO(logger, "MQTT-Messaging");
        doMqttMessaging = true;
    } else {
        JOYNR_LOG_INFO(logger, "HTTP-Messaging");
        doHttpMessaging = true;
    }

    if (!doHttpMessaging && boost::starts_with(bounceproxyProtocol, "HTTP")) {
        JOYNR_LOG_INFO(logger, "HTTP-Messaging");
        doHttpMessaging = true;
    }

    std::string capabilitiesDirectoryChannelId =
            messagingSettings.getCapabilitiesDirectoryChannelId();
    std::string capabilitiesDirectoryParticipantId =
            messagingSettings.getCapabilitiesDirectoryParticipantId();
    std::string channelUrlDirectoryChannelId = messagingSettings.getChannelUrlDirectoryChannelId();
    std::string channelUrlDirectoryParticipantId =
            messagingSettings.getChannelUrlDirectoryParticipantId();

    // provision global capabilities directory
    if (boost::starts_with(capabilitiesDirectoryChannelId, "{")) {
        try {
            using system::RoutingTypes::MqttAddress;
            auto globalCapabilitiesDirectoryAddress = std::make_shared<MqttAddress>(
                    JsonSerializer::deserialize<MqttAddress>(capabilitiesDirectoryChannelId));
            messageRouter->addProvisionedNextHop(
                    capabilitiesDirectoryParticipantId, globalCapabilitiesDirectoryAddress);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "could not deserialize MqttAddress from {} - error: {}",
                            capabilitiesDirectoryChannelId,
                            e.what());
        }

    } else {
        std::shared_ptr<joynr::system::RoutingTypes::Address> globalCapabilitiesDirectoryAddress(
                new system::RoutingTypes::ChannelAddress(capabilitiesDirectoryChannelId));
        messageRouter->addProvisionedNextHop(
                capabilitiesDirectoryParticipantId, globalCapabilitiesDirectoryAddress);
    }

    // provision channel url directory
    if (boost::starts_with(channelUrlDirectoryChannelId, "{")) {

        try {
            using system::RoutingTypes::MqttAddress;
            auto globalChannelUrlDirectoryAddress = std::make_shared<MqttAddress>(
                    JsonSerializer::deserialize<MqttAddress>(channelUrlDirectoryChannelId));
            messageRouter->addProvisionedNextHop(
                    channelUrlDirectoryParticipantId, globalChannelUrlDirectoryAddress);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "could not deserialize MqttAddress from {} - error: {}",
                            capabilitiesDirectoryChannelId,
                            e.what());
        }
    } else {
        std::shared_ptr<joynr::system::RoutingTypes::Address> globalChannelUrlDirectoryAddress(
                new system::RoutingTypes::ChannelAddress(channelUrlDirectoryChannelId));
        messageRouter->addProvisionedNextHop(
                channelUrlDirectoryParticipantId, globalChannelUrlDirectoryAddress);
    }

    // setup CC WebSocket interface
    auto wsMessagingStubFactory = std::make_unique<WebSocketMessagingStubFactory>();
    system::RoutingTypes::WebSocketAddress wsAddress =
            wsSettings.createClusterControllerMessagingAddress();
    wsCcMessagingSkeleton =
            new WebSocketCcMessagingSkeleton(*messageRouter, *wsMessagingStubFactory, wsAddress);
    messagingStubFactory->registerStubFactory(std::move(wsMessagingStubFactory));

    /* LibJoynr */
    assert(messageRouter);
    joynrMessageSender = new JoynrMessageSender(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender);
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    /* CC */
    // TODO: libjoynrmessagingskeleton now uses the Dispatcher, should it use the
    // InprocessDispatcher?
    libJoynrMessagingSkeleton =
            std::make_shared<InProcessLibJoynrMessagingSkeleton>(joynrDispatcher);
    // EndpointAddress to messagingStub is transmitted when a provider is registered
    // messagingStubFactory->registerInProcessMessagingSkeleton(libJoynrMessagingSkeleton);

    std::string httpChannelId;
    std::string mqttChannelId;

    /**
      * ClusterController side HTTP
      *
      */

    if (doHttpMessaging) {

        if (!httpMessageReceiver) {
            JOYNR_LOG_INFO(logger,
                           "The http message receiver supplied is NULL, creating the default "
                           "http MessageReceiver");

            httpMessageReceiver = std::make_shared<HttpReceiver>(messagingSettings);

            assert(httpMessageReceiver != nullptr);

            httpMessagingSkeleton = std::make_shared<HttpMessagingSkeleton>(*messageRouter);
            httpMessageReceiver->registerReceiveCallback([&](const std::string& msg) {
                httpMessagingSkeleton->onTextMessageReceived(msg);
            });
        }

        httpChannelId = httpMessageReceiver->getReceiveChannelId();

        // create http message sender
        if (!httpMessageSender) {
            JOYNR_LOG_INFO(logger,
                           "The http message sender supplied is NULL, creating the default "
                           "http MessageSender");

            httpMessageSender = std::make_shared<HttpSender>(
                    messagingSettings.getBounceProxyUrl(),
                    std::chrono::milliseconds(messagingSettings.getSendMsgMaxTtl()),
                    std::chrono::milliseconds(messagingSettings.getSendMsgRetryInterval()));
        }

        messagingStubFactory->registerStubFactory(
                std::make_unique<HttpMessagingStubFactory>(httpMessageSender, httpChannelId));
    }

    /**
      * ClusterController side MQTT
      *
      */

    if (doMqttMessaging) {

        if (!mqttMessageReceiver && !mqttMessageSender) {
            mosqpp::lib_init();
        }

        if (!mqttMessageReceiver) {
            JOYNR_LOG_INFO(logger,
                           "The mqtt message receiver supplied is NULL, creating the default "
                           "mqtt MessageReceiver");

            mqttMessageReceiver = std::make_shared<MqttReceiver>(messagingSettings);

            assert(mqttMessageReceiver != nullptr);
        }

        if (!mqttMessagingIsRunning) {
            mqttMessagingSkeleton = std::make_shared<MqttMessagingSkeleton>(*messageRouter);
            mqttMessageReceiver->registerReceiveCallback([&](const std::string& msg) {
                mqttMessagingSkeleton->onTextMessageReceived(msg);
            });
        }

        mqttChannelId = mqttMessageReceiver->getReceiveChannelId();

        // create message sender
        if (!mqttMessageSender) {
            JOYNR_LOG_INFO(logger,
                           "The mqtt message sender supplied is NULL, creating the default "
                           "mqtt MessageSender");

            mqttMessageSender = std::make_shared<MqttSender>(messagingSettings.getBrokerUrl());

            mqttMessageSender->registerReceiveQueueStartedCallback(
                    [&](void) { mqttMessageReceiver->waitForReceiveQueueStarted(); });
        }

        messagingStubFactory->registerStubFactory(
                std::make_unique<MqttMessagingStubFactory>(mqttMessageSender, mqttChannelId));
    }

    // joynrMessagingSendSkeleton = new DummyClusterControllerMessagingSkeleton(messageRouter);
    // ccDispatcher = DispatcherFactory::createDispatcherInSameThread(messagingSettings);

    // we currently have to use the fake client, because JAVA side is not yet working for
    // CapabilitiesServer.
    bool usingRealCapabilitiesClient =
            /*when switching this to true, turn on the UUID in systemintegrationtests again*/ true;
    if (doMqttMessaging) {
        capabilitiesClient =
                new CapabilitiesClient(mqttChannelId); // ownership of this is not transferred
    } else {
        capabilitiesClient =
                new CapabilitiesClient(httpChannelId); // ownership of this is not transferred
    }
    // try using the real capabilitiesClient again:
    // capabilitiesClient = new CapabilitiesClient(channelId);// ownership of this is not
    // transferred

    localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
            messagingSettings, capabilitiesClient, *messageRouter);
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

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            inProcessConnectorFactory, joynrMessagingConnectorFactory);
    proxyFactory = new ProxyFactory(libjoynrMessagingAddress, std::move(connectorFactory), &cache);

    dispatcherList.push_back(joynrDispatcher);
    dispatcherList.push_back(inProcessDispatcher);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings.getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    dispatcherAddress = libjoynrMessagingAddress;
    discoveryProxy = std::make_unique<LocalDiscoveryAggregator>(
            *dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher), systemServicesSettings);

    std::string discoveryProviderParticipantId(
            systemServicesSettings.getCcDiscoveryProviderParticipantId());
    std::shared_ptr<RequestCaller> discoveryRequestCaller(
            new joynr::system::DiscoveryRequestCaller(localCapabilitiesDirectory));
    std::shared_ptr<InProcessAddress> discoveryProviderAddress(
            new InProcessAddress(discoveryRequestCaller));

    {
        using joynr::system::DiscoveryInProcessConnector;
        auto discoveryInProcessConnector =
                std::make_unique<DiscoveryInProcessConnector>(subscriptionManager,
                                                              publicationManager,
                                                              inProcessPublicationSender,
                                                              std::string(), // can be ignored
                                                              discoveryProviderParticipantId,
                                                              discoveryProviderAddress);
        discoveryProxy->setDiscoveryProxy(std::move(discoveryInProcessConnector));
    }
    capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(dispatcherList,
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
    std::int64_t discoveryMessagesTtl = messagingSettings.getDiscoveryMessagesTtl();

    if (usingRealCapabilitiesClient) {
        ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>* capabilitiesProxyBuilder =
                createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                        messagingSettings.getDiscoveryDirectoriesDomain());
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
                    messagingSettings.getDiscoveryDirectoriesDomain());

    DiscoveryQos discoveryQos(10000);
    discoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY); // actually only one provider
                                                                  // should be available
    channelUrlDirectoryProxy = std::shared_ptr<infrastructure::ChannelUrlDirectoryProxy>(
            channelUrlDirectoryProxyBuilder->setMessagingQos(MessagingQos(discoveryMessagesTtl))
                    ->setCached(true)
                    ->setDiscoveryQos(discoveryQos)
                    ->build());

    channelUrlDirectory =
            std::make_shared<LocalChannelUrlDirectory>(messagingSettings, channelUrlDirectoryProxy);

    if (doHttpMessaging) {
        httpMessageReceiver->init(channelUrlDirectory);
        httpMessageSender->init(channelUrlDirectory, messagingSettings);
    }

    if (doMqttMessaging) {
        mqttMessageReceiver->init(channelUrlDirectory);
        mqttMessageSender->init(channelUrlDirectory, messagingSettings);
    }
}

void JoynrClusterControllerRuntime::registerRoutingProvider()
{
    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::system::RoutingProvider> routingProvider(messageRouter);
    std::string interfaceName(routingProvider->getInterfaceName());
    std::string participantId(systemServicesSettings.getCcRoutingProviderParticipantId());

    // provision the participant ID for the routing provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    joynr::types::ProviderQos routingProviderQos;
    routingProviderQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    routingProviderQos.setProviderVersion(1);
    routingProviderQos.setPriority(1);
    routingProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    routingProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider<joynr::system::RoutingProvider>(domain, routingProvider, routingProviderQos);
}

void JoynrClusterControllerRuntime::registerDiscoveryProvider()
{
    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::system::DiscoveryProvider> discoveryProvider(localCapabilitiesDirectory);
    std::string interfaceName(discoveryProvider->getInterfaceName());
    std::string participantId(systemServicesSettings.getCcDiscoveryProviderParticipantId());

    // provision the participant ID for the discovery provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    joynr::types::ProviderQos discoveryProviderQos;
    discoveryProviderQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    discoveryProviderQos.setProviderVersion(1);
    discoveryProviderQos.setPriority(1);
    discoveryProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    discoveryProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider<joynr::system::DiscoveryProvider>(
            domain, discoveryProvider, discoveryProviderQos);
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
    if (doHttpMessaging) {
        assert(httpMessageReceiver != nullptr);
        if (!httpMessagingIsRunning) {
            httpMessageReceiver->startReceiveQueue();
            httpMessagingIsRunning = true;
        }
    }
    if (doMqttMessaging) {
        assert(mqttMessageReceiver != nullptr);
        if (!mqttMessagingIsRunning) {
            mqttMessageReceiver->startReceiveQueue();
            mqttMessagingIsRunning = true;
        }
    }
}

void JoynrClusterControllerRuntime::stopMessaging()
{
    // joynrDispatcher->stopMessaging();
    if (doHttpMessaging) {
        if (httpMessagingIsRunning) {
            httpMessageReceiver->stopReceiveQueue();
            httpMessagingIsRunning = false;
        }
    }
    if (doMqttMessaging) {
        if (mqttMessagingIsRunning) {
            mqttMessageReceiver->stopReceiveQueue();
            mqttMessagingIsRunning = false;
        }
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
    if (doHttpMessaging) {
        httpMessageReceiver->waitForReceiveQueueStarted();
    }
    // Nothing to do for MQTT
}

void JoynrClusterControllerRuntime::deleteChannel()
{
    if (doHttpMessaging) {
        httpMessageReceiver->tryToDeleteChannel();
    }
    // Nothing to do for MQTT
}

} // namespace joynr
