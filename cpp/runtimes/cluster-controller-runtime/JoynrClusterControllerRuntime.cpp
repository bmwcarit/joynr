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
#include <boost/filesystem.hpp>

#include "joynr/BrokerUrl.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/CcMessageRouter.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Dispatcher.h"
#include "joynr/HttpMulticastAddressCalculator.h"
#include "joynr/IDispatcher.h"
#include "joynr/IKeychain.h"
#include "joynr/IMqttMessagingSkeleton.h"
#include "joynr/ITransportMessageReceiver.h"
#include "joynr/ITransportMessageSender.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/JoynrClusterControllerMqttConnectionData.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/MessageSender.h"
#include "joynr/MessageQueue.h"
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
#include "joynr/system/DiscoveryJoynrMessagingConnector.h"
#include "joynr/system/DiscoveryProvider.h"
#include "joynr/system/ProviderReregistrationControllerProvider.h"
#include "joynr/system/MessageNotificationProvider.h"
#include "joynr/system/RoutingProvider.h"
#include "joynr/system/RoutingTypes/Address.h"
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
#include "libjoynrclustercontroller/capabilities-client/GlobalCapabilitiesDirectoryClient.h"
#include "libjoynrclustercontroller/http-communication-manager/HttpMessagingSkeleton.h"
#include "libjoynrclustercontroller/http-communication-manager/HttpReceiver.h"
#include "libjoynrclustercontroller/http-communication-manager/HttpSender.h"
#include "libjoynrclustercontroller/messaging/MessagingPropertiesPersistence.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/HttpMessagingStubFactory.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStubFactory.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"
#include "joynr/MqttMessagingSkeleton.h"
#include "joynr/MqttReceiver.h"
#include "libjoynrclustercontroller/mqtt/MqttSender.h"
#include "libjoynrclustercontroller/mqtt/MqttTransportStatus.h"
#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeletonNonTLS.h"
#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeletonTLS.h"
#include "libjoynrclustercontroller/ClusterControllerCallContextStorage.h"
#include "libjoynrclustercontroller/ClusterControllerCallContext.h"

namespace joynr
{

JoynrClusterControllerRuntime::JoynrClusterControllerRuntime(
        std::unique_ptr<Settings> settings,
        std::shared_ptr<IKeychain> keyChain,
        MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory,
        std::shared_ptr<ITransportMessageReceiver> httpMessageReceiver,
        std::shared_ptr<ITransportMessageSender> httpMessageSender,
        std::vector<std::shared_ptr<JoynrClusterControllerMqttConnectionData>>
                mqttConnectionDataVector)
        : JoynrRuntimeImpl(*settings, std::move(keyChain)),
          joynrDispatcher(),
          subscriptionManager(),
          messageSender(),
          localCapabilitiesDirectory(nullptr),
          libJoynrMessagingSkeleton(nullptr),
          httpMessageReceiver(httpMessageReceiver),
          httpMessageSender(httpMessageSender),
          httpMessagingSkeleton(nullptr),
          mqttMessagingSkeletonFactory(std::move(mqttMessagingSkeletonFactory)),
          mqttConnectionDataVector(mqttConnectionDataVector),
          dispatcherList(),
          settings(std::move(settings)),
          libjoynrSettings(*(this->settings)),
          localDomainAccessController(nullptr),
          clusterControllerSettings(*(this->settings)),
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
          accessController(nullptr),
          routingProviderParticipantId(),
          discoveryProviderParticipantId(),
          providerReregistrationControllerParticipantId(
                  "providerReregistrationController_participantId"),
          messageNotificationProviderParticipantId(),
          accessControlListEditorProviderParticipantId(),
          isShuttingDown(false),
          dummyGlobalAddress()
{
}

std::shared_ptr<JoynrClusterControllerRuntime> JoynrClusterControllerRuntime::create(
        std::size_t argc,
        char* argv[],
        std::shared_ptr<IKeychain> keyChain,
        MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory)
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
        JOYNR_LOG_INFO(logger(), "Loading settings file: {}", settingsFileName);
        Settings currentSettings(settingsFileName);

        // Check for errors
        if (!currentSettings.isLoaded()) {
            JOYNR_LOG_FATAL(
                    logger(), "Provided settings file {} could not be loaded.", settingsFileName);
            return nullptr;
        }

        // Merge
        Settings::merge(currentSettings, *settings, true);
    }
    return create(std::move(settings),
                  discoveryEntriesFile,
                  std::move(keyChain),
                  std::move(mqttMessagingSkeletonFactory));
}

void JoynrClusterControllerRuntime::init()
{
    /**
      * libjoynr side skeleton & dispatcher
      * This needs some preparation of libjoynr and clustercontroller parts.
      */
    messagingSettings.printSettings();
    libjoynrSettings.printSettings();
    wsSettings.printSettings();

    fillBackendsStruct(messagingSettings);

    if (mqttConnectionDataVector.empty()) {
        // Create entries for MqttConnection(s)
        for (std::uint8_t i = 0; i < availableGbids.size(); i++) {
            mqttConnectionDataVector.push_back(
                    std::make_shared<JoynrClusterControllerMqttConnectionData>());
        }
    } else {
        assert(availableGbids.size() == mqttConnectionDataVector.size());
    }

    const BrokerUrl defaultBrokerUrl = messagingSettings.getBrokerUrl();

    // If the BrokerUrl is a mqtt url, MQTT is used instead of HTTP
    const Url url = defaultBrokerUrl.getBrokerChannelsBaseUrl();
    std::string brokerProtocol = url.getProtocol();

    std::transform(brokerProtocol.begin(), brokerProtocol.end(), brokerProtocol.begin(), ::toupper);

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator;

    if (brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTT) ||
        brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTTS) ||
        brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::TCP)) {
        JOYNR_LOG_DEBUG(logger(), "MQTT-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
                "joynrdefaultgbid", "");
        addressCalculator = std::make_unique<joynr::MqttMulticastAddressCalculator>(
                globalAddress, clusterControllerSettings.getMqttMulticastTopicPrefix());
        doMqttMessaging = true;
    } else if (brokerProtocol == "HTTP" || brokerProtocol == "HTTPS") {
        JOYNR_LOG_DEBUG(logger(), "HTTP-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                defaultBrokerUrl.toString(), "");
        addressCalculator = std::make_unique<joynr::HttpMulticastAddressCalculator>(globalAddress);
        doHttpMessaging = true;
    } else {
        JOYNR_LOG_FATAL(logger(), "invalid broker protocol in broker-url: {}", brokerProtocol);
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

    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
    messagingStubFactory->registerStubFactory(std::make_shared<InProcessMessagingStubFactory>());

    const bool httpMessageReceiverSupplied = httpMessageReceiver != nullptr;

    MessagingPropertiesPersistence persist(
            messagingSettings.getMessagingPropertiesPersistenceFilename());
    const std::string clusterControllerId = persist.getChannelId();

    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;

    if (doHttpMessaging) {
        if (!httpMessageReceiverSupplied) {
            JOYNR_LOG_DEBUG(logger(),
                            "The http message receiver supplied is NULL, creating the default "
                            "http MessageReceiver");

            const std::string receiverId = persist.getReceiverId();
            httpMessageReceiver = std::make_shared<HttpReceiver>(
                    messagingSettings, clusterControllerId, receiverId);

            assert(httpMessageReceiver != nullptr);
        }
    }

    if (doMqttMessaging) {
        const std::string ccMqttClientIdPrefix = clusterControllerSettings.getMqttClientIdPrefix();
        const std::string mqttCliendId = ccMqttClientIdPrefix + clusterControllerId;

        const std::chrono::seconds mqttReconnectDelayTimeSeconds =
                messagingSettings.getMqttReconnectDelayTimeSeconds();

        const std::chrono::seconds mqttReconnectMaxDelayTimeSeconds =
                messagingSettings.getMqttReconnectMaxDelayTimeSeconds();
        const bool isMqttExponentialBackoffEnabled =
                messagingSettings.getMqttExponentialBackoffEnabled();
        std::chrono::seconds mqttKeepAliveTimeSeconds(0);
        BrokerUrl brokerUrl("");

        // default brokerIndex = 0
        for (std::uint8_t brokerIndex = 0; brokerIndex < mqttConnectionDataVector.size();
             brokerIndex++) {
            if (mqttConnectionDataVector.size() == 1) {
                // we have only the default broker
                mqttKeepAliveTimeSeconds = messagingSettings.getMqttKeepAliveTimeSeconds();
                brokerUrl = defaultBrokerUrl;
            } else {
                mqttKeepAliveTimeSeconds =
                        messagingSettings.getAdditionalBackendMqttKeepAliveTimeSeconds(brokerIndex);
                brokerUrl = messagingSettings.getAdditionalBackendBrokerUrl(brokerIndex);
            }

            try {
                const auto& mosquittoConnection =
                        std::make_shared<MosquittoConnection>(clusterControllerSettings,
                                                              brokerUrl,
                                                              mqttKeepAliveTimeSeconds,
                                                              mqttReconnectDelayTimeSeconds,
                                                              mqttReconnectMaxDelayTimeSeconds,
                                                              isMqttExponentialBackoffEnabled,
                                                              mqttCliendId);

                const auto& connectionData = mqttConnectionDataVector[brokerIndex];

                const auto& mqttMessageReceiver = connectionData->getMqttMessageReceiver();

                if (!mqttMessageReceiver || !connectionData->getMqttMessageSender()) {
                    JOYNR_LOG_TRACE(logger(),
                                    "{}: The mqtt message receiver supplied is NULL, creating "
                                    "MosquittoConnection ",
                                    brokerIndex);
                    connectionData->setMosquittoConnection(mosquittoConnection);

                    auto mqttTransportStatus =
                            std::make_unique<MqttTransportStatus>(mosquittoConnection);
                    transportStatuses.emplace_back(std::move(mqttTransportStatus));
                }
                if (!mqttMessageReceiver) {
                    JOYNR_LOG_TRACE(
                            logger(),
                            "{}: The mqtt message receiver supplied is NULL, creating the default "
                            "mqtt MessageReceiver",
                            brokerIndex);

                    connectionData->setMqttMessageReceiver(std::make_shared<MqttReceiver>(
                            mosquittoConnection,
                            messagingSettings,
                            clusterControllerId,
                            availableGbids[brokerIndex],
                            clusterControllerSettings.getMqttUnicastTopicPrefix()));
                    assert(connectionData->getMqttMessageReceiver() != nullptr);
                }
            } catch (const exceptions::JoynrRuntimeException& e) {
                JOYNR_LOG_ERROR(logger(),
                                "{}: Creating mosquittoConnection failed. Error: {}",
                                brokerIndex,
                                e.getMessage());

                doMqttMessaging = false;
                break;
            }
        }
    }

    const std::string globalClusterControllerAddress =
            getSerializedGlobalClusterControllerAddress();

    std::unique_ptr<MessageQueue<std::string>> messageQueue =
            std::make_unique<MessageQueue<std::string>>(
                    clusterControllerSettings.getMessageQueueLimit(),
                    clusterControllerSettings.getPerParticipantIdMessageQueueLimit(),
                    clusterControllerSettings.getMessageQueueLimitBytes());
    std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> transportStatusQueue =
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>(
                    clusterControllerSettings.getTransportNotAvailableQueueLimit(),
                    0,
                    clusterControllerSettings.getTransportNotAvailableQueueLimitBytes());
    // init message router
    ccMessageRouter = std::make_shared<CcMessageRouter>(
            messagingSettings,
            clusterControllerSettings,
            messagingStubFactory,
            multicastMessagingSkeletonDirectory,
            std::move(securityManager),
            singleThreadIOService->getIOService(),
            std::move(addressCalculator),
            globalClusterControllerAddress,
            systemServicesSettings.getCcMessageNotificationProviderParticipantId(),
            libjoynrSettings.isMessageRouterPersistencyEnabled(),
            std::move(transportStatuses),
            std::move(messageQueue),
            std::move(transportStatusQueue),
            getGlobalClusterControllerAddress());

    ccMessageRouter->init();
    if (libjoynrSettings.isMessageRouterPersistencyEnabled()) {
        ccMessageRouter->loadRoutingTable(libjoynrSettings.getMessageRouterPersistenceFilename());
    }
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
            JOYNR_LOG_FATAL(logger(),
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

    messagingStubFactory->registerStubFactory(wsMessagingStubFactory);

    /* LibJoynr */
    assert(ccMessageRouter);
    messageSender = std::make_shared<MessageSender>(
            ccMessageRouter, keyChain, messagingSettings.getTtlUpliftMs());
    joynrDispatcher =
            std::make_shared<Dispatcher>(messageSender, singleThreadIOService->getIOService());
    messageSender->registerDispatcher(joynrDispatcher);
    messageSender->setReplyToAddress(globalClusterControllerAddress);
    ccMessageRouter->setMessageSender(messageSender);

    /* CC */
    libJoynrMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(joynrDispatcher);

    /**
      * ClusterController side HTTP
      *
      */
    if (doHttpMessaging) {
        if (!httpMessageReceiverSupplied) {
            httpMessagingSkeleton = std::make_shared<HttpMessagingSkeleton>(ccMessageRouter);
            auto httpMessagingSkeletonCopyForCapturing = httpMessagingSkeleton;
            httpMessageReceiver
                    ->registerReceiveCallback([httpMessagingSkeleton =
                                                       httpMessagingSkeletonCopyForCapturing](
                            smrf::ByteVector &&
                            msg) { httpMessagingSkeleton->onMessageReceived(std::move(msg)); });
        }

        // create http message sender
        if (!httpMessageSender) {
            JOYNR_LOG_DEBUG(logger(),
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
        // create MqttMessagingSkeletonFactory only once and use it to create multiple skeletons
        if (!mqttMessagingSkeletonFactory) {
            mqttMessagingSkeletonFactory = [](std::weak_ptr<IMessageRouter> messageRouter,
                                              std::shared_ptr<MqttReceiver> mqttReceiver,
                                              const std::string& multicastTopicPrefix,
                                              const std::string& gbid,
                                              std::uint64_t ttlUplift = 0) {
                return std::make_shared<MqttMessagingSkeleton>(
                        messageRouter, mqttReceiver, multicastTopicPrefix, gbid, ttlUplift);
            };
        }

        for (std::uint8_t brokerIndex = 0; brokerIndex < mqttConnectionDataVector.size();
             brokerIndex++) {
            const auto& connectionData = mqttConnectionDataVector[brokerIndex];
            const auto& mqttMessagingSkeleton = mqttMessagingSkeletonFactory(
                    ccMessageRouter,
                    std::static_pointer_cast<MqttReceiver>(
                            connectionData->getMqttMessageReceiver()),
                    clusterControllerSettings.getMqttMulticastTopicPrefix(),
                    availableGbids[brokerIndex],
                    messagingSettings.getTtlUpliftMs());

            connectionData->getMqttMessageReceiver()->registerReceiveCallback(
                    [mqttMessagingSkeleton](smrf::ByteVector&& msg) {
                        mqttMessagingSkeleton->onMessageReceived(std::move(msg));
                    });
            multicastMessagingSkeletonDirectory
                    ->registerSkeleton<system::RoutingTypes::MqttAddress>(
                            std::move(mqttMessagingSkeleton), availableGbids[brokerIndex]);

            // create message sender
            if (!connectionData->getMqttMessageSender()) {
                JOYNR_LOG_DEBUG(logger(),
                                "Connection {}. The mqtt message sender supplied is NULL, "
                                "creating the default "
                                "mqtt MessageSender",
                                brokerIndex);

                const auto& mqttMessageSender = std::make_shared<MqttSender>(
                        connectionData->getMosquittoConnection(), messagingSettings);
                connectionData->setMqttMessageSender(std::move(mqttMessageSender));
            }

            messagingStubFactory->registerStubFactory(std::make_shared<MqttMessagingStubFactory>(
                    connectionData->getMqttMessageSender(), availableGbids[brokerIndex]));
        }
    }

    /**
      * libJoynr side
      *
      */
    publicationManager = std::make_shared<PublicationManager>(
            singleThreadIOService->getIOService(),
            messageSender,
            libjoynrSettings.isSubscriptionPersistencyEnabled(),
            messagingSettings.getTtlUpliftMs());
    publicationManager->loadSavedAttributeSubscriptionRequestsMap(
            libjoynrSettings.getSubscriptionRequestPersistenceFilename());
    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(
            libjoynrSettings.getBroadcastSubscriptionRequestPersistenceFilename());

    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadIOService->getIOService(), ccMessageRouter);

    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(libJoynrMessagingSkeleton);

    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(messageSender, subscriptionManager);

    proxyFactory = std::make_unique<ProxyFactory>(std::move(joynrMessagingConnectorFactory));

    dispatcherList.push_back(joynrDispatcher);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings.getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    auto provisionedDiscoveryEntries = getProvisionedEntries();
    discoveryProxy = std::make_shared<LocalDiscoveryAggregator>(provisionedDiscoveryEntries);

    auto globalCapabilitiesDirectoryClient =
            std::make_shared<GlobalCapabilitiesDirectoryClient>(clusterControllerSettings);
    localCapabilitiesDirectory =
            std::make_shared<LocalCapabilitiesDirectory>(clusterControllerSettings,
                                                         globalCapabilitiesDirectoryClient,
                                                         globalClusterControllerAddress,
                                                         ccMessageRouter,
                                                         singleThreadIOService->getIOService(),
                                                         clusterControllerId,
                                                         availableGbids);
    localCapabilitiesDirectory->init();
    localCapabilitiesDirectory->loadPersistedFile();
    // importPersistedLocalCapabilitiesDirectory();

    std::string discoveryProviderParticipantId(
            systemServicesSettings.getCcDiscoveryProviderParticipantId());

    MessagingQos messagingQos;
    messagingQos.setCompress(
            clusterControllerSettings.isGlobalCapabilitiesDirectoryCompressedMessagesEnabled());

    {
        auto provisionedProviderDiscoveryEntry =
                provisionedDiscoveryEntries
                        .find(systemServicesSettings.getCcDiscoveryProviderParticipantId())
                        ->second;
        using joynr::system::DiscoveryJoynrMessagingConnector;
        auto discoveryJoynrMessagingConnector = std::make_unique<DiscoveryJoynrMessagingConnector>(
                messageSender,
                subscriptionManager,
                std::string(),
                discoveryProviderParticipantId,
                messagingQos,
                provisionedProviderDiscoveryEntry);
        discoveryProxy->setDiscoveryProxy(std::move(discoveryJoynrMessagingConnector));
    }
    capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
            dispatcherList,
            discoveryProxy,
            participantIdStorage,
            dispatcherAddress,
            ccMessageRouter,
            messagingSettings.getDiscoveryEntryExpiryIntervalMs(),
            publicationManager,
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

    auto capabilitiesProxyBuilder =
            createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                    messagingSettings.getDiscoveryDirectoriesDomain());
    capabilitiesProxyBuilder->setDiscoveryQos(discoveryQos);

    capabilitiesProxyBuilder->setMessagingQos(messagingQos);

    globalCapabilitiesDirectoryClient->setProxy(capabilitiesProxyBuilder->build(), messagingQos);

    // Do this after local capabilities directory and message router have been initialized.
    enableAccessController(provisionedDiscoveryEntries);

    registerInternalSystemServiceProviders();
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

    auto provisionedDiscoveryEntries = JoynrRuntimeImpl::getProvisionedEntries();
    // setting up the provisioned values for GlobalCapabilitiesDirectoryClient
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

    JOYNR_LOG_INFO(logger(),
                   "Access control was enabled attempting to load entries from {}.",
                   clusterControllerSettings.getAclEntriesDirectory());

    auto localDomainAccessStore = std::make_shared<joynr::LocalDomainAccessStore>(
            clusterControllerSettings.getLocalDomainAccessStorePersistenceFilename());

    namespace fs = boost::filesystem;

    fs::path aclEntriesPath(clusterControllerSettings.getAclEntriesDirectory());

    if (fs::is_directory(aclEntriesPath)) {
        for (const auto& entry : fs::directory_iterator(aclEntriesPath)) {
            if (fs::is_regular_file(entry.path())) {
                const std::string aclPath = entry.path().string();
                localDomainAccessStore->mergeDomainAccessStore(LocalDomainAccessStore(aclPath));
                JOYNR_LOG_INFO(logger(), "Loading ACL/RCL templates from {}", aclPath);
            }
        }
    } else {
        JOYNR_LOG_ERROR(
                logger(), "Access control directory: {} does not exist.", aclEntriesPath.string());
    }

    localDomainAccessController = std::make_shared<joynr::LocalDomainAccessController>(
            localDomainAccessStore, clusterControllerSettings.getUseOnlyLDAS());

    if (!clusterControllerSettings.getUseOnlyLDAS()) {
        auto proxyGlobalDomainAccessController = createGlobalDomainAccessControllerProxy();
        localDomainAccessController->setGlobalDomainAccessControllerProxy(
                std::move(proxyGlobalDomainAccessController));
    }

    accessController = std::make_shared<joynr::AccessController>(
            localCapabilitiesDirectory, localDomainAccessController, availableGbids);

    // whitelist provisioned entries into access controller
    for (const auto& entry : provisionedEntries) {
        accessController->addParticipantToWhitelist(entry.second.getParticipantId());
    }

    ccMessageRouter->setAccessController(std::move(util::as_weak_ptr(accessController)));

    aclEditor = std::make_shared<AccessControlListEditor>(localDomainAccessStore,
                                                          localDomainAccessController,
                                                          clusterControllerSettings.aclAudit());

    // Set accessController also in LocalCapabilitiesDirectory
    localCapabilitiesDirectory->setAccessController(std::move(util::as_weak_ptr(accessController)));

    // Log entries
    localDomainAccessStore->logContent();
}

std::shared_ptr<infrastructure::GlobalDomainAccessControllerProxy> JoynrClusterControllerRuntime::
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
        JOYNR_LOG_ERROR(logger(),
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

void JoynrClusterControllerRuntime::registerInternalSystemServiceProviders()
{
    ClusterControllerCallContext clusterControllerCallContext;

    clusterControllerCallContext.setIsValid(true);
    clusterControllerCallContext.setIsInternalProviderRegistration(true);

    ClusterControllerCallContextStorage::set(std::move(clusterControllerCallContext));

    discoveryProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::DiscoveryProvider>(localCapabilitiesDirectory),
            systemServicesSettings.getCcDiscoveryProviderParticipantId());
    routingProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::RoutingProvider>(ccMessageRouter),
            systemServicesSettings.getCcRoutingProviderParticipantId());
    providerReregistrationControllerParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::ProviderReregistrationControllerProvider>(
                    localCapabilitiesDirectory),
            providerReregistrationControllerParticipantId);
    messageNotificationProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::MessageNotificationProvider>(
                    ccMessageRouter->getMessageNotificationProvider()),
            systemServicesSettings.getCcMessageNotificationProviderParticipantId());

    if (clusterControllerSettings.enableAccessController()) {
        accessControlListEditorProviderParticipantId = registerInternalSystemServiceProvider(
                std::dynamic_pointer_cast<joynr::infrastructure::AccessControlListEditorProvider>(
                        aclEditor),
                systemServicesSettings.getCcAccessControlListEditorProviderParticipantId());
    }

    ClusterControllerCallContextStorage::invalidate();
}

void JoynrClusterControllerRuntime::unregisterInternalSystemServiceProvider(
        const std::string& participantId)
{
    localCapabilitiesDirectory->remove(participantId, nullptr, nullptr);
    for (std::shared_ptr<IDispatcher> currentDispatcher : dispatcherList) {
        currentDispatcher->removeRequestCaller(participantId);
    }
}
void JoynrClusterControllerRuntime::unregisterInternalSystemServiceProviders()
{
    if (!accessControlListEditorProviderParticipantId.empty()) {
        unregisterInternalSystemServiceProvider(accessControlListEditorProviderParticipantId);
    }

    unregisterInternalSystemServiceProvider(messageNotificationProviderParticipantId);
    unregisterInternalSystemServiceProvider(providerReregistrationControllerParticipantId);
    unregisterInternalSystemServiceProvider(routingProviderParticipantId);
    unregisterInternalSystemServiceProvider(discoveryProviderParticipantId);
}

void JoynrClusterControllerRuntime::startLocalCommunication()
{
    if (clusterControllerSettings.isWsTLSPortSet()) {
        std::string certificateAuthorityPemFilename =
                wsSettings.getCertificateAuthorityPemFilename();
        std::string certificatePemFilename = wsSettings.getCertificatePemFilename();
        std::string privateKeyPemFilename = wsSettings.getPrivateKeyPemFilename();

        if (checkAndLogCryptoFileExistence(certificateAuthorityPemFilename,
                                           certificatePemFilename,
                                           privateKeyPemFilename,
                                           logger())) {
            JOYNR_LOG_INFO(logger(), "Using TLS connection");

            system::RoutingTypes::WebSocketAddress wsAddress(
                    system::RoutingTypes::WebSocketProtocol::WSS,
                    "localhost",
                    clusterControllerSettings.getWsTLSPort(),
                    "");

            bool useEncryptedTls = wsSettings.getEncryptedTlsUsage();

            wsTLSCcMessagingSkeleton = std::make_shared<WebSocketCcMessagingSkeletonTLS>(
                    singleThreadIOService->getIOService(),
                    ccMessageRouter,
                    wsMessagingStubFactory,
                    wsAddress,
                    certificateAuthorityPemFilename,
                    certificatePemFilename,
                    privateKeyPemFilename,
                    useEncryptedTls);
            wsTLSCcMessagingSkeleton->init();
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
        wsCcMessagingSkeleton->init();
    }
}

JoynrClusterControllerRuntime::~JoynrClusterControllerRuntime()
{
    JOYNR_LOG_TRACE(logger(), "entering ~JoynrClusterControllerRuntime");
    assert(isShuttingDown);
    JOYNR_LOG_TRACE(logger(), "leaving ~JoynrClusterControllerRuntime");
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
    if (doMqttMessaging && !mqttMessagingIsRunning) {
        for (const auto& connectionData : mqttConnectionDataVector) {
            connectionData->getMosquittoConnection()->start();
        }
        mqttMessagingIsRunning = true;
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
    if (doMqttMessaging && mqttMessagingIsRunning) {
        for (const auto& connecton : mqttConnectionDataVector) {
            connecton->getMosquittoConnection()->stop();
        }
        mqttMessagingIsRunning = false;
    }
}

void JoynrClusterControllerRuntime::shutdown()
{
    assert(!isShuttingDown);
    isShuttingDown = true;
    std::lock_guard<std::mutex> lock(proxyBuildersMutex);
    for (auto proxyBuilder : proxyBuilders) {
        proxyBuilder->stop();
        proxyBuilder.reset();
    }
    proxyBuilders.clear();

    if (wsCcMessagingSkeleton) {
        wsCcMessagingSkeleton->shutdown();
    }
    if (wsTLSCcMessagingSkeleton) {
        wsTLSCcMessagingSkeleton->shutdown();
    }

    unregisterInternalSystemServiceProviders();

    if (ccMessageRouter) {
        ccMessageRouter->shutdown();
    }
    if (publicationManager) {
        publicationManager->shutdown();
    }
    if (subscriptionManager) {
        subscriptionManager->shutdown();
    }
    if (localCapabilitiesDirectory) {
        localCapabilitiesDirectory->shutdown();
    }

    stop(true);

    if (multicastMessagingSkeletonDirectory) {
        multicastMessagingSkeletonDirectory
                ->unregisterSkeletons<system::RoutingTypes::MqttAddress>();
    }

    if (joynrDispatcher != nullptr) {
        joynrDispatcher->shutdown();
        joynrDispatcher.reset();
    }
}

void JoynrClusterControllerRuntime::shutdownClusterController()
{
    JOYNR_LOG_TRACE(logger(), "Shutdown Cluster Controller");
    lifetimeSemaphore.notify();
}

void JoynrClusterControllerRuntime::runForever()
{
    lifetimeSemaphore.wait();
    shutdown();
}

std::shared_ptr<JoynrClusterControllerRuntime> JoynrClusterControllerRuntime::create(
        std::unique_ptr<Settings> settings,
        const std::string& discoveryEntriesFile,
        std::shared_ptr<IKeychain> keyChain,
        MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory)
{
    auto runtime = std::make_shared<JoynrClusterControllerRuntime>(
            std::move(settings), std::move(keyChain), std::move(mqttMessagingSkeletonFactory));
    runtime->init();

    assert(runtime->localCapabilitiesDirectory);
    runtime->localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(discoveryEntriesFile);
    runtime->start();

    return runtime;
}

void JoynrClusterControllerRuntime::start()
{
    singleThreadIOService->start();
    startLocalCommunication();
    startExternalCommunication();
}

void JoynrClusterControllerRuntime::stop(bool deleteHttpChannel)
{
    if (deleteHttpChannel) {
        deleteChannel();
    }

    stopExternalCommunication();

    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    if (singleThreadIOService) {
        singleThreadIOService->stop();
    }
}

void JoynrClusterControllerRuntime::deleteChannel()
{
    if (doHttpMessaging) {
        httpMessageReceiver->tryToDeleteChannel();
    }
    // Nothing to do for MQTT
}

std::string JoynrClusterControllerRuntime::getSerializedGlobalClusterControllerAddress() const
{
    const auto defaultConnectionData = mqttConnectionDataVector[0];
    if (doMqttMessaging) {
        return defaultConnectionData->getMqttMessageReceiver()
                ->getSerializedGlobalClusterControllerAddress();
    }
    if (doHttpMessaging) {
        return httpMessageReceiver->getSerializedGlobalClusterControllerAddress();
    }
    JOYNR_LOG_ERROR(logger(),
                    "Cannot obtain globalClusterControllerAddress, as doMqttMessaging is: {} and "
                    "doHttpMessaging is: {}",
                    doMqttMessaging,
                    doHttpMessaging);
    // in order to at least allow local communication in case global transport
    // is not correctly configured, a dummy address must be provided since
    // otherwise LibJoynrRuntime cannot be started
    return "global-transport-not-available";
}

void JoynrClusterControllerRuntime::fillBackendsStruct(const MessagingSettings& messagingSettings)
{
    availableGbids.emplace_back(messagingSettings.getGbid());
    gbidToBrokerUrlMapping.emplace(messagingSettings.getGbid(), messagingSettings.getBrokerUrl());

    std::uint8_t additionalBackends = messagingSettings.getAdditionalBackendsCount();
    for (std::uint8_t index = 0; index < additionalBackends; index++) {
        availableGbids.emplace_back(messagingSettings.getAdditionalBackendGbid(index));
        gbidToBrokerUrlMapping.emplace(messagingSettings.getAdditionalBackendGbid(index),
                                       messagingSettings.getAdditionalBackendBrokerUrl(index));
    }
}

const system::RoutingTypes::Address& JoynrClusterControllerRuntime::
        getGlobalClusterControllerAddress() const
{
    const auto defaultConnectionData = mqttConnectionDataVector[0];
    if (doMqttMessaging) {
        return defaultConnectionData->getMqttMessageReceiver()->getGlobalClusterControllerAddress();
    }
    if (doHttpMessaging) {
        return httpMessageReceiver->getGlobalClusterControllerAddress();
    }
    JOYNR_LOG_ERROR(logger(),
                    "Cannot obtain globalClusterControllerAddress, as doMqttMessaging is: {} and "
                    "doHttpMessaging is: {}",
                    doMqttMessaging,
                    doHttpMessaging);
    // in order to at least allow local communication in case global transport
    // is not correctly configured, a dummy address must be provided since
    // otherwise LibJoynrRuntime cannot be started
    return dummyGlobalAddress;
}
} // namespace joynr
