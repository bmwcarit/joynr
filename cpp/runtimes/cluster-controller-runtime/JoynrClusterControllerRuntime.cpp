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
#include "joynr/JoynrClusterControllerRuntime.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include <boost/algorithm/string/predicate.hpp>

#include "joynr/BrokerUrl.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/CcMessageRouter.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Dispatcher.h"
#include "joynr/HttpMulticastAddressCalculator.h"
#include "joynr/IDispatcher.h"
#include "joynr/ITransportMessageReceiver.h"
#include "joynr/ITransportMessageSender.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/InProcessAddress.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/MessageSender.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/ProxyFactory.h"
#include "joynr/PublicationManager.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/AccessControlListEditorProvider.h"
#include "joynr/infrastructure/DacTypes/ControlEntry.h"
#include "joynr/infrastructure/DacTypes/MasterAccessControlEntry.h"
#include "joynr/infrastructure/DacTypes/OwnerAccessControlEntry.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/infrastructure/GlobalDomainAccessControllerProxy.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/DiscoveryInProcessConnector.h"
#include "joynr/system/DiscoveryProvider.h"
#include "joynr/system/DiscoveryRequestCaller.h"
#include "joynr/system/MessageNotificationProvider.h"
#include "joynr/system/RoutingProvider.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttProtocol.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "libjoynr/joynr-messaging/DummyPlatformSecurityManager.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynrclustercontroller/access-control/AccessController.h"
#include "libjoynrclustercontroller/access-control/AccessControlListEditor.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"
#include "libjoynrclustercontroller/capabilities-client/CapabilitiesClient.h"
#include "libjoynrclustercontroller/http-communication-manager/HttpMessagingSkeleton.h"
#include "libjoynrclustercontroller/http-communication-manager/HttpReceiver.h"
#include "libjoynrclustercontroller/http-communication-manager/HttpSender.h"
#include "libjoynrclustercontroller/messaging/MessagingPropertiesPersistence.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/HttpMessagingStubFactory.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStubFactory.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"
#include "libjoynrclustercontroller/mqtt/MqttMessagingSkeleton.h"
#include "libjoynrclustercontroller/mqtt/MqttReceiver.h"
#include "libjoynrclustercontroller/mqtt/MqttSender.h"
#include "libjoynrclustercontroller/mqtt/MqttTransportStatus.h"
#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeletonNonTLS.h"
#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeletonTLS.h"

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
#include "libjoynr/dbus/DbusMessagingStubFactory.h"
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

namespace joynr
{

INIT_LOGGER(JoynrClusterControllerRuntime);

static const std::string ACC_ENTRIES_FILE = "CCAccessControl.entries";

JoynrClusterControllerRuntime::JoynrClusterControllerRuntime(
        std::unique_ptr<Settings> settings,
        std::shared_ptr<ITransportMessageReceiver> httpMessageReceiver,
        std::shared_ptr<ITransportMessageSender> httpMessageSender,
        std::shared_ptr<ITransportMessageReceiver> mqttMessageReceiver,
        std::shared_ptr<ITransportMessageSender> mqttMessageSender)
        : JoynrRuntime(*settings),
          joynrDispatcher(),
          inProcessDispatcher(),
          subscriptionManager(nullptr),
          messageSender(nullptr),
          localCapabilitiesDirectory(nullptr),
          libJoynrMessagingSkeleton(nullptr),
          httpMessageReceiver(httpMessageReceiver),
          httpMessageSender(httpMessageSender),
          httpMessagingSkeleton(nullptr),
          mosquittoConnection(nullptr),
          mqttMessageReceiver(mqttMessageReceiver),
          mqttMessageSender(mqttMessageSender),
          mqttMessagingSkeleton(nullptr),
          dispatcherList(),
          inProcessPublicationSender(nullptr),
          settings(std::move(settings)),
          libjoynrSettings(*(this->settings)),
          localDomainAccessController(nullptr),
          clusterControllerSettings(*(this->settings)),
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
          dbusSettings(nullptr),
          ccDbusMessageRouterAdapter(nullptr),
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
          wsSettings(*(this->settings)),
          wsCcMessagingSkeleton(nullptr),
          wsTLSCcMessagingSkeleton(nullptr),
          httpMessagingIsRunning(false),
          mqttMessagingIsRunning(false),
          doMqttMessaging(false),
          doHttpMessaging(false),
          wsMessagingStubFactory(),
          multicastMessagingSkeletonDirectory(
                  std::make_shared<MulticastMessagingSkeletonDirectory>()),
          ccMessageRouter(nullptr),
          aclEditor(nullptr),
          lifetimeSemaphore(0),
          accessController(nullptr)
{
    initializeAllDependencies();
}

std::shared_ptr<JoynrClusterControllerRuntime> JoynrClusterControllerRuntime::create(
        std::size_t argc,
        char* argv[])
{
    // Object that holds all the settings
    auto settings = std::make_unique<Settings>();

    // Discovery entry file name
    std::string discoveryEntriesFile;

    // Walk the argument list and
    //  - merge all the settings files into the settings object
    //  - read in input file name to inject discovery entries
    for (std::size_t i = 1; i < argc; ++i) {

        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            // exit immediately if only --version was asked
            return 0;
        } else if (std::strcmp(argv[i], "-d") == 0) {
            if (++i < argc) {
                discoveryEntriesFile = argv[i];
            } else {
                return nullptr;
            }
            break;
        }

        const std::string settingsFileName(argv[i]);

        // Read the settings file
        JOYNR_LOG_INFO(logger, "Loading settings file: {}", settingsFileName);
        Settings currentSettings(settingsFileName);

        // Check for errors
        if (!currentSettings.isLoaded()) {
            JOYNR_LOG_FATAL(
                    logger, "Provided settings file {} could not be loaded.", settingsFileName);
            return nullptr;
        }

        // Merge
        Settings::merge(currentSettings, *settings, true);
    }
    return create(std::move(settings), discoveryEntriesFile);
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

    const BrokerUrl brokerUrl = messagingSettings.getBrokerUrl();
    assert(brokerUrl.getBrokerChannelsBaseUrl().isValid());

    // If the BrokerUrl is a mqtt url, MQTT is used instead of HTTP
    const Url url = brokerUrl.getBrokerChannelsBaseUrl();
    std::string brokerProtocol = url.getProtocol();

    std::transform(brokerProtocol.begin(), brokerProtocol.end(), brokerProtocol.begin(), ::toupper);

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator;

    if (brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTT) ||
        brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTTS) ||
        brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::TCP)) {
        JOYNR_LOG_DEBUG(logger, "MQTT-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
                brokerUrl.toString(), "");
        addressCalculator = std::make_unique<joynr::MqttMulticastAddressCalculator>(
                globalAddress, clusterControllerSettings.getMqttMulticastTopicPrefix());
        doMqttMessaging = true;
    } else if (brokerProtocol == "HTTP" || brokerProtocol == "HTTPS") {
        JOYNR_LOG_DEBUG(logger, "HTTP-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                brokerUrl.toString(), "");
        addressCalculator = std::make_unique<joynr::HttpMulticastAddressCalculator>(globalAddress);
        doHttpMessaging = true;
    } else {
        JOYNR_LOG_FATAL(logger, "invalid broker protocol in broker-url: {}", brokerProtocol);
        throw exceptions::JoynrRuntimeException(
                "Exception in JoynrRuntime: invalid broker protocol in broker-url: " +
                brokerProtocol);
    }

    std::string capabilitiesDirectoryChannelId =
            messagingSettings.getCapabilitiesDirectoryChannelId();
    std::string capabilitiesDirectoryParticipantId =
            messagingSettings.getCapabilitiesDirectoryParticipantId();

    // Initialise security manager
    std::unique_ptr<IPlatformSecurityManager> securityManager =
            std::make_unique<DummyPlatformSecurityManager>();

    // CAREFUL: the factory creates an old style dispatcher, not the new one!
    inProcessDispatcher =
            std::make_shared<InProcessDispatcher>(singleThreadIOService->getIOService());
    /* CC */
    // create the messaging stub factory
    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(std::make_shared<DbusMessagingStubFactory>());
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(std::make_shared<InProcessMessagingStubFactory>());

    const bool httpMessageReceiverSupplied = httpMessageReceiver != nullptr;

    std::string httpSerializedGlobalClusterControllerAddress;
    std::string mqttSerializedGlobalClusterControllerAddress;

    MessagingPropertiesPersistence persist(
            messagingSettings.getMessagingPropertiesPersistenceFilename());
    std::string clusterControllerId = persist.getChannelId();
    std::string receiverId = persist.getReceiverId();
    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;

    if (doHttpMessaging) {
        if (!httpMessageReceiverSupplied) {
            JOYNR_LOG_DEBUG(logger,
                            "The http message receiver supplied is NULL, creating the default "
                            "http MessageReceiver");

            httpMessageReceiver = std::make_shared<HttpReceiver>(
                    messagingSettings, clusterControllerId, receiverId);

            assert(httpMessageReceiver != nullptr);
        }

        httpSerializedGlobalClusterControllerAddress =
                httpMessageReceiver->getGlobalClusterControllerAddress();
    }

    if (doMqttMessaging) {
        if (!mqttMessageReceiver || !mqttMessageSender) {
            std::string ccMqttClientIdPrefix = clusterControllerSettings.getMqttClientIdPrefix();
            std::string mqttCliendId = ccMqttClientIdPrefix + receiverId;

            mosquittoConnection = std::make_shared<MosquittoConnection>(
                    messagingSettings, clusterControllerSettings, mqttCliendId);

            auto mqttTransportStatus = std::make_unique<MqttTransportStatus>(mosquittoConnection);
            transportStatuses.emplace_back(std::move(mqttTransportStatus));
        }
        if (!mqttMessageReceiver) {
            JOYNR_LOG_DEBUG(logger,
                            "The mqtt message receiver supplied is NULL, creating the default "
                            "mqtt MessageReceiver");

            mqttMessageReceiver = std::make_shared<MqttReceiver>(
                    mosquittoConnection,
                    messagingSettings,
                    clusterControllerId,
                    clusterControllerSettings.getMqttUnicastTopicPrefix());

            assert(mqttMessageReceiver != nullptr);
        }

        mqttSerializedGlobalClusterControllerAddress =
                mqttMessageReceiver->getGlobalClusterControllerAddress();
    }

    const std::string globalClusterControllerAddress =
            doMqttMessaging ? mqttSerializedGlobalClusterControllerAddress
                            : httpSerializedGlobalClusterControllerAddress;

    // init message router
    ccMessageRouter = std::make_shared<CcMessageRouter>(messagingStubFactory,
                                                        multicastMessagingSkeletonDirectory,
                                                        std::move(securityManager),
                                                        singleThreadIOService->getIOService(),
                                                        std::move(addressCalculator),
                                                        globalClusterControllerAddress,
                                                        std::move(transportStatuses));
    ccMessageRouter->loadRoutingTable(libjoynrSettings.getMessageRouterPersistenceFilename());
    ccMessageRouter->loadMulticastReceiverDirectory(
            clusterControllerSettings.getMulticastReceiverDirectoryPersistenceFilename());

    // provision global capabilities directory
    bool isGloballyVisible = true;
    if (boost::starts_with(capabilitiesDirectoryChannelId, "{")) {
        try {
            using system::RoutingTypes::MqttAddress;
            auto globalCapabilitiesDirectoryAddress = std::make_shared<MqttAddress>();
            joynr::serializer::deserializeFromJson(
                    *globalCapabilitiesDirectoryAddress, capabilitiesDirectoryChannelId);
            ccMessageRouter->addProvisionedNextHop(capabilitiesDirectoryParticipantId,
                                                   std::move(globalCapabilitiesDirectoryAddress),
                                                   isGloballyVisible);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "could not deserialize MqttAddress from {} - error: {}",
                            capabilitiesDirectoryChannelId,
                            e.what());
        }
    } else {
        auto globalCapabilitiesDirectoryAddress =
                std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                        messagingSettings.getCapabilitiesDirectoryUrl() +
                                capabilitiesDirectoryChannelId + "/",
                        capabilitiesDirectoryChannelId);
        ccMessageRouter->addProvisionedNextHop(capabilitiesDirectoryParticipantId,
                                               std::move(globalCapabilitiesDirectoryAddress),
                                               isGloballyVisible);
    }

    // setup CC WebSocket interface
    wsMessagingStubFactory = std::make_shared<WebSocketMessagingStubFactory>();
    wsMessagingStubFactory->registerOnMessagingStubClosedCallback([messagingStubFactory](
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress) {
        messagingStubFactory->remove(destinationAddress);
    });

    createWsCCMessagingSkeletons();

    messagingStubFactory->registerStubFactory(wsMessagingStubFactory);

    /* LibJoynr */
    assert(ccMessageRouter);
    messageSender =
            std::make_shared<MessageSender>(ccMessageRouter, messagingSettings.getTtlUpliftMs());
    joynrDispatcher =
            std::make_shared<Dispatcher>(messageSender, singleThreadIOService->getIOService());
    messageSender->registerDispatcher(joynrDispatcher);
    messageSender->setReplyToAddress(globalClusterControllerAddress);

    /* CC */
    // TODO: libjoynrmessagingskeleton now uses the Dispatcher, should it use the
    // InprocessDispatcher?
    libJoynrMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(joynrDispatcher);
    // EndpointAddress to messagingStub is transmitted when a provider is registered
    // messagingStubFactory->registerInProcessMessagingSkeleton(libJoynrMessagingSkeleton);

    /**
      * ClusterController side HTTP
      *
      */
    if (doHttpMessaging) {
        if (!httpMessageReceiverSupplied) {
            httpMessagingSkeleton = std::make_shared<HttpMessagingSkeleton>(*ccMessageRouter);
            httpMessageReceiver->registerReceiveCallback([&](smrf::ByteVector&& msg) {
                httpMessagingSkeleton->onMessageReceived(std::move(msg));
            });
        }

        // create http message sender
        if (!httpMessageSender) {
            JOYNR_LOG_DEBUG(logger,
                            "The http message sender supplied is NULL, creating the default "
                            "http MessageSender");

            httpMessageSender = std::make_shared<HttpSender>(
                    messagingSettings.getBrokerUrl(),
                    std::chrono::milliseconds(messagingSettings.getSendMsgMaxTtl()),
                    std::chrono::milliseconds(messagingSettings.getSendMsgRetryInterval()));
        }

        messagingStubFactory->registerStubFactory(
                std::make_shared<HttpMessagingStubFactory>(httpMessageSender));
    }

    /**
      * ClusterController side MQTT
      *
      */
    if (doMqttMessaging) {
        if (!mqttMessagingIsRunning) {
            mqttMessagingSkeleton = std::make_shared<MqttMessagingSkeleton>(
                    *ccMessageRouter,
                    std::static_pointer_cast<MqttReceiver>(mqttMessageReceiver),
                    clusterControllerSettings.getMqttMulticastTopicPrefix(),
                    messagingSettings.getTtlUpliftMs());
            mqttMessageReceiver->registerReceiveCallback([&](smrf::ByteVector&& msg) {
                mqttMessagingSkeleton->onMessageReceived(std::move(msg));
            });
            multicastMessagingSkeletonDirectory
                    ->registerSkeleton<system::RoutingTypes::MqttAddress>(mqttMessagingSkeleton);
        }

        // create message sender
        if (!mqttMessageSender) {
            JOYNR_LOG_DEBUG(logger,
                            "The mqtt message sender supplied is NULL, creating the default "
                            "mqtt MessageSender");

            mqttMessageSender = std::make_shared<MqttSender>(mosquittoConnection);
        }

        messagingStubFactory->registerStubFactory(
                std::make_shared<MqttMessagingStubFactory>(mqttMessageSender));
    }

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    dbusSettings = new DbusSettings(*settings);
    dbusSettings->printSettings();
    // register dbus skeletons for capabilities and messaging interfaces
    std::string ccMessagingAddress(dbusSettings->createClusterControllerMessagingAddressString());
    ccDbusMessageRouterAdapter = new DBusMessageRouterAdapter(*ccMessageRouter, ccMessagingAddress);
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

    /**
      * libJoynr side
      *
      */
    publicationManager = new PublicationManager(singleThreadIOService->getIOService(),
                                                messageSender.get(),
                                                messagingSettings.getTtlUpliftMs());
    publicationManager->loadSavedAttributeSubscriptionRequestsMap(
            libjoynrSettings.getSubscriptionRequestPersistenceFilename());
    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(
            libjoynrSettings.getBroadcastSubscriptionRequestPersistenceFilename());

    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadIOService->getIOService(), ccMessageRouter);
    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);

    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(libJoynrMessagingSkeleton);
    // subscriptionManager = new SubscriptionManager(...)
    auto inProcessConnectorFactory = std::make_unique<InProcessConnectorFactory>(
            subscriptionManager.get(),
            publicationManager,
            inProcessPublicationSender,
            std::dynamic_pointer_cast<IRequestCallerDirectory>(inProcessDispatcher));
    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(messageSender, subscriptionManager);

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            std::move(inProcessConnectorFactory), std::move(joynrMessagingConnectorFactory));
    proxyFactory = std::make_unique<ProxyFactory>(std::move(connectorFactory));

    dispatcherList.push_back(joynrDispatcher);
    dispatcherList.push_back(inProcessDispatcher);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings.getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    auto provisionedDiscoveryEntries = getProvisionedEntries();
    discoveryProxy = std::make_unique<LocalDiscoveryAggregator>(provisionedDiscoveryEntries);
    requestCallerDirectory =
            std::dynamic_pointer_cast<IRequestCallerDirectory>(inProcessDispatcher);

    std::shared_ptr<ICapabilitiesClient> capabilitiesClient =
            std::make_shared<CapabilitiesClient>();
    localCapabilitiesDirectory =
            std::make_shared<LocalCapabilitiesDirectory>(clusterControllerSettings,
                                                         capabilitiesClient,
                                                         globalClusterControllerAddress,
                                                         *ccMessageRouter,
                                                         singleThreadIOService->getIOService(),
                                                         clusterControllerId);
    localCapabilitiesDirectory->loadPersistedFile();
    // importPersistedLocalCapabilitiesDirectory();

    std::string discoveryProviderParticipantId(
            systemServicesSettings.getCcDiscoveryProviderParticipantId());
    auto discoveryRequestCaller =
            std::make_shared<joynr::system::DiscoveryRequestCaller>(localCapabilitiesDirectory);
    auto discoveryProviderAddress = std::make_shared<InProcessAddress>(discoveryRequestCaller);

    {
        using joynr::system::DiscoveryInProcessConnector;
        auto discoveryInProcessConnector = std::make_unique<DiscoveryInProcessConnector>(
                subscriptionManager.get(),
                publicationManager,
                inProcessPublicationSender,
                std::make_shared<DummyPlatformSecurityManager>(),
                std::string(), // can be ignored
                discoveryProviderParticipantId,
                discoveryProviderAddress);
        discoveryProxy->setDiscoveryProxy(std::move(discoveryInProcessConnector));
    }
    capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
            dispatcherList,
            *discoveryProxy,
            participantIdStorage,
            dispatcherAddress,
            ccMessageRouter,
            messagingSettings.getDiscoveryEntryExpiryIntervalMs(),
            *publicationManager,
            globalClusterControllerAddress);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    // ******************************************************************************************
    // WARNING: Latent dependency in place!
    //
    // ProxyBuilder performs a discovery this is why discoveryProxy->setDiscoveryProxy must be
    // called before any createProxyBuilder().
    //
    // ******************************************************************************************
    DiscoveryQos discoveryQos(10000);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.addCustomParameter(
            "fixedParticipantId", messagingSettings.getCapabilitiesDirectoryParticipantId());

    std::shared_ptr<ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
            capabilitiesProxyBuilder =
                    createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                            messagingSettings.getDiscoveryDirectoriesDomain());
    capabilitiesProxyBuilder->setDiscoveryQos(discoveryQos);

    capabilitiesClient->setProxyBuilder(std::move(capabilitiesProxyBuilder));

#ifdef JOYNR_ENABLE_ACCESS_CONTROL
    // Do this after local capabilities directory and message router have been initialized.
    enableAccessController(provisionedDiscoveryEntries);
#endif // JOYNR_ENABLE_ACCESS_CONTROL
}

std::shared_ptr<IMessageRouter> JoynrClusterControllerRuntime::getMessageRouter()
{
    return ccMessageRouter;
}

std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> JoynrClusterControllerRuntime::
        getProvisionedEntries() const
{
    std::int64_t lastSeenDateMs = 0;
    std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    std::string defaultPublicKeyId("");

    auto provisionedDiscoveryEntries = JoynrRuntime::getProvisionedEntries();
    // setting up the provisioned values for GlobalCapabilitiesClient
    // The GlobalCapabilitiesServer is also provisioned in MessageRouter
    types::ProviderQos capabilityProviderQos;
    capabilityProviderQos.setPriority(1);
    types::Version capabilityProviderVersion(
            infrastructure::IGlobalCapabilitiesDirectory::MAJOR_VERSION,
            infrastructure::IGlobalCapabilitiesDirectory::MINOR_VERSION);
    provisionedDiscoveryEntries.insert(
            std::make_pair(messagingSettings.getCapabilitiesDirectoryParticipantId(),
                           types::DiscoveryEntryWithMetaInfo(
                                   capabilityProviderVersion,
                                   messagingSettings.getDiscoveryDirectoriesDomain(),
                                   infrastructure::IGlobalCapabilitiesDirectory::INTERFACE_NAME(),
                                   messagingSettings.getCapabilitiesDirectoryParticipantId(),
                                   capabilityProviderQos,
                                   lastSeenDateMs,
                                   expiryDateMs,
                                   defaultPublicKeyId,
                                   false)));

    types::Version gDACProviderVersion(
            infrastructure::IGlobalDomainAccessController::MAJOR_VERSION,
            infrastructure::IGlobalDomainAccessController::MINOR_VERSION);
    provisionedDiscoveryEntries.insert(std::make_pair(
            clusterControllerSettings.getGlobalDomainAccessControlParticipantId(),
            types::DiscoveryEntryWithMetaInfo(
                    gDACProviderVersion,
                    messagingSettings.getDiscoveryDirectoriesDomain(),
                    infrastructure::IGlobalDomainAccessController::INTERFACE_NAME(),
                    clusterControllerSettings.getGlobalDomainAccessControlParticipantId(),
                    capabilityProviderQos,
                    lastSeenDateMs,
                    expiryDateMs,
                    defaultPublicKeyId,
                    false)));

    return provisionedDiscoveryEntries;
}

void JoynrClusterControllerRuntime::enableAccessController(
        const std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo>& provisionedEntries)
{
    if (!clusterControllerSettings.enableAccessController()) {
        return;
    }

    JOYNR_LOG_DEBUG(logger,
                    "AccessControl was enabled attempting to load entries from {}.",
                    ACC_ENTRIES_FILE);

    std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry> accessControlEntries;

    if (joynr::util::fileExists(ACC_ENTRIES_FILE)) {
        try {
            joynr::serializer::deserializeFromJson(
                    accessControlEntries, joynr::util::loadStringFromFile(ACC_ENTRIES_FILE));
        } catch (const std::runtime_error& ex) {
            JOYNR_LOG_ERROR(logger, ex.what());
            accessControlEntries.clear();
        } catch (const std::invalid_argument& ex) {
            JOYNR_LOG_ERROR(logger,
                            "Could not deserialize access control entries from {}: {}",
                            ACC_ENTRIES_FILE,
                            ex.what());
            accessControlEntries.clear();
        }
    } else {
        JOYNR_LOG_INFO(
                logger, "Access control file with entries does not exists.", ACC_ENTRIES_FILE);
    }

    auto localDomainAccessStore = std::make_shared<joynr::LocalDomainAccessStore>(
            clusterControllerSettings.getLocalDomainAccessStorePersistenceFilename());

    // Use update methods to insert deserialized entries in access store
    for (const auto& entry : accessControlEntries) {
        localDomainAccessStore->updateMasterAccessControlEntry(entry);
    }

    localDomainAccessController = std::make_shared<joynr::LocalDomainAccessController>(
            localDomainAccessStore, clusterControllerSettings.getUseOnlyLDAS());

    if (!clusterControllerSettings.getUseOnlyLDAS()) {
        auto proxyGlobalDomainAccessController = createGlobalDomainAccessControllerProxy();
        localDomainAccessController->setGlobalDomainAccessControllerProxy(
                std::move(proxyGlobalDomainAccessController));
    }

    accessController = std::make_shared<joynr::AccessController>(
            localCapabilitiesDirectory, localDomainAccessController);

    // whitelist provisioned entries into access controller
    for (const auto& entry : provisionedEntries) {
        accessController->addParticipantToWhitelist(entry.second.getParticipantId());
    }

    ccMessageRouter->setAccessController(std::move(util::as_weak_ptr(accessController)));

    aclEditor = std::make_shared<AccessControlListEditor>(
            std::move(localDomainAccessStore), localDomainAccessController);

    // Set accessController also in LocalCapabilitiesDirectory
    localCapabilitiesDirectory->setAccessController(std::move(util::as_weak_ptr(accessController)));
}

std::unique_ptr<infrastructure::GlobalDomainAccessControllerProxy> JoynrClusterControllerRuntime::
        createGlobalDomainAccessControllerProxy()
{
    // Provision global domain access controller in MessageRouter
    auto globalDomainAccessControlAddress =
            std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    try {
        joynr::serializer::deserializeFromJson(
                *globalDomainAccessControlAddress,
                clusterControllerSettings.getGlobalDomainAccessControlAddress());
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger,
                        "Cannot deserialize global domain access controller address. Reason: {}.",
                        ex.what());
    }

    bool isGloballyVisible = true;
    ccMessageRouter->addProvisionedNextHop(
            clusterControllerSettings.getGlobalDomainAccessControlParticipantId(),
            std::move(globalDomainAccessControlAddress),
            isGloballyVisible);

    // create GlobalDomainAccessController proxy
    std::shared_ptr<ProxyBuilder<infrastructure::GlobalDomainAccessControllerProxy>>
            globalDomainAccessControllerProxyBuilder =
                    createProxyBuilder<infrastructure::GlobalDomainAccessControllerProxy>(
                            messagingSettings.getDiscoveryDirectoriesDomain());

    DiscoveryQos discoveryQos(10000);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.addCustomParameter(
            "fixedParticipantId",
            clusterControllerSettings.getGlobalDomainAccessControlParticipantId());

    return globalDomainAccessControllerProxyBuilder->setDiscoveryQos(discoveryQos)->build();
}

void JoynrClusterControllerRuntime::createWsCCMessagingSkeletons()
{
    if (clusterControllerSettings.isWsTLSPortSet()) {
        std::string certificateAuthorityPemFilename =
                wsSettings.getCertificateAuthorityPemFilename();
        std::string certificatePemFilename = wsSettings.getCertificatePemFilename();
        std::string privateKeyPemFilename = wsSettings.getPrivateKeyPemFilename();

        if (checkAndLogCryptoFileExistence(certificateAuthorityPemFilename,
                                           certificatePemFilename,
                                           privateKeyPemFilename,
                                           logger)) {
            JOYNR_LOG_INFO(logger, "Using TLS connection");

            system::RoutingTypes::WebSocketAddress wsAddress(
                    system::RoutingTypes::WebSocketProtocol::WSS,
                    "localhost",
                    clusterControllerSettings.getWsTLSPort(),
                    "");

            wsTLSCcMessagingSkeleton = std::make_shared<WebSocketCcMessagingSkeletonTLS>(
                    singleThreadIOService->getIOService(),
                    ccMessageRouter,
                    wsMessagingStubFactory,
                    wsAddress,
                    certificateAuthorityPemFilename,
                    certificatePemFilename,
                    privateKeyPemFilename);
        }
    }

    if (clusterControllerSettings.isWsPortSet()) {
        system::RoutingTypes::WebSocketAddress wsAddress(
                system::RoutingTypes::WebSocketProtocol::WS,
                "localhost",
                clusterControllerSettings.getWsPort(),
                "");

        wsCcMessagingSkeleton = std::make_shared<WebSocketCcMessagingSkeletonNonTLS>(
                singleThreadIOService->getIOService(),
                ccMessageRouter,
                wsMessagingStubFactory,
                wsAddress);
    }
}

void JoynrClusterControllerRuntime::registerRoutingProvider()
{
    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::system::RoutingProvider> routingProvider(ccMessageRouter);
    std::string interfaceName(routingProvider->getInterfaceName());
    std::string participantId(systemServicesSettings.getCcRoutingProviderParticipantId());

    // provision the participant ID for the routing provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    joynr::types::ProviderQos routingProviderQos;
    routingProviderQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    routingProviderQos.setPriority(1);
    routingProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    routingProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider(domain, routingProvider, routingProviderQos);
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
    discoveryProviderQos.setPriority(1);
    discoveryProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    discoveryProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider(domain, discoveryProvider, discoveryProviderQos);
}

void JoynrClusterControllerRuntime::registerMessageNotificationProvider()
{
    std::shared_ptr<joynr::system::MessageNotificationProvider> messageNotificationProvider =
            ccMessageRouter->getMessageNotificationProvider();
    std::string domain(systemServicesSettings.getDomain());
    std::string interfaceName(messageNotificationProvider->getInterfaceName());
    std::string participantId(
            systemServicesSettings.getCcMessageNotificationProviderParticipantId());

    // provision the participant ID for the message notification provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    joynr::types::ProviderQos messageNotificationProviderQos;
    messageNotificationProviderQos.setPriority(1);
    messageNotificationProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    messageNotificationProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider(domain, messageNotificationProvider, messageNotificationProviderQos);
}

void JoynrClusterControllerRuntime::registerAccessControlListEditorProvider()
{
    if (!clusterControllerSettings.enableAccessController()) {
        return;
    }

    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::infrastructure::AccessControlListEditorProvider> aclEditorProvider(
            aclEditor);
    std::string interfaceName(aclEditorProvider->getInterfaceName());
    std::string participantId(
            systemServicesSettings.getCcAccessControlListEditorProviderParticipantId());

    // provision the participant ID for the AccessControlListEditor provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    joynr::types::ProviderQos aclEditorProviderQos;
    aclEditorProviderQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    aclEditorProviderQos.setPriority(1);
    aclEditorProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    aclEditorProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider(domain, aclEditorProvider, aclEditorProviderQos);
}

JoynrClusterControllerRuntime::~JoynrClusterControllerRuntime()
{
    JOYNR_LOG_TRACE(logger, "entering ~JoynrClusterControllerRuntime");

    inProcessDispatcher->shutdown();

    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    singleThreadIOService->stop();

    stopExternalCommunication();

    multicastMessagingSkeletonDirectory->unregisterSkeleton<system::RoutingTypes::MqttAddress>();

    if (messageSender) {
        messageSender->registerDispatcher(nullptr);
    }

    if (joynrDispatcher != nullptr) {
        joynrDispatcher->shutdown();
        JOYNR_LOG_TRACE(logger, "joynrDispatcher");
        joynrDispatcher.reset();
    }

    inProcessDispatcher.reset();

    delete inProcessPublicationSender;
    inProcessPublicationSender = nullptr;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    delete ccDbusMessageRouterAdapter;
    delete dbusSettings;
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    JOYNR_LOG_TRACE(logger, "leaving ~JoynrClusterControllerRuntime");
}

void JoynrClusterControllerRuntime::startExternalCommunication()
{
    //    assert(joynrDispatcher!=NULL);
    //    joynrDispatcher->startExternalCommunication();
    //    joynrDispatcher->waitForMessaging();
    if (doHttpMessaging) {
        assert(httpMessageReceiver != nullptr);
        if (!httpMessagingIsRunning) {
            httpMessageReceiver->startReceiveQueue();
            httpMessagingIsRunning = true;
        }
    }
    if (doMqttMessaging) {
        if (mosquittoConnection && !mqttMessagingIsRunning) {
            mosquittoConnection->start();
            mqttMessagingIsRunning = true;
        }
    }
}

void JoynrClusterControllerRuntime::stopExternalCommunication()
{
    // joynrDispatcher->stopExternalCommunication();
    if (doHttpMessaging) {
        if (httpMessagingIsRunning) {
            httpMessageReceiver->stopReceiveQueue();
            httpMessagingIsRunning = false;
        }
    }
    if (doMqttMessaging) {
        if (mosquittoConnection && mqttMessagingIsRunning) {
            mosquittoConnection->stop();
            mqttMessagingIsRunning = false;
        }
    }
}

void JoynrClusterControllerRuntime::shutdown()
{
    JOYNR_LOG_TRACE(logger, "Shutdown Cluster Controller");
    lifetimeSemaphore.notify();
}

void JoynrClusterControllerRuntime::runForever()
{
    lifetimeSemaphore.wait();
}

std::shared_ptr<JoynrClusterControllerRuntime> JoynrClusterControllerRuntime::create(
        std::unique_ptr<Settings> settings,
        const std::string& discoveryEntriesFile)
{
    auto runtime = std::make_shared<JoynrClusterControllerRuntime>(std::move(settings));

    assert(runtime->localCapabilitiesDirectory);
    runtime->localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(discoveryEntriesFile);
    runtime->start();

    return runtime;
}

void JoynrClusterControllerRuntime::start()
{
    startExternalCommunication();
    registerRoutingProvider();
    registerDiscoveryProvider();
    registerMessageNotificationProvider();
#ifdef JOYNR_ENABLE_ACCESS_CONTROL
    registerAccessControlListEditorProvider();
#endif // JOYNR_ENABLE_ACCESS_CONTROL
    singleThreadIOService->start();
}

void JoynrClusterControllerRuntime::stop(bool deleteChannel)
{
    if (deleteChannel) {
        this->deleteChannel();
    }
    stopExternalCommunication();
    singleThreadIOService->stop();
}

void JoynrClusterControllerRuntime::deleteChannel()
{
    if (doHttpMessaging) {
        httpMessageReceiver->tryToDeleteChannel();
    }
    // Nothing to do for MQTT
}

} // namespace joynr
