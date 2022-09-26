/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2020 BMW Car IT GmbH
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

#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "joynr/AbstractGlobalMessagingSkeleton.h"
#include "joynr/BrokerUrl.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/CapabilitiesStorage.h"
#include "joynr/CcMessageRouter.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Dispatcher.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessageSender.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/IPlatformSecurityManager.h"
#include "joynr/IProxyBuilder.h"
#include "joynr/IProxyBuilderBase.h"
#include "joynr/ITransportMessageReceiver.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/JoynrClusterControllerMqttConnectionData.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/LocalCapabilitiesDirectoryStore.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/MessageQueue.h"
#include "joynr/MessageSender.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MqttMessagingSkeleton.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/MqttReceiver.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/ProxyFactory.h"
#include "joynr/PublicationManager.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/TaskSequencer.h"
#include "joynr/Url.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/AccessControlListEditorProvider.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/infrastructure/IGlobalCapabilitiesDirectory.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/DiscoveryJoynrMessagingConnector.h"
#include "joynr/system/DiscoveryProvider.h"
#include "joynr/system/MessageNotificationProvider.h"
#include "joynr/system/ProviderReregistrationControllerProvider.h"
#include "joynr/system/RoutingProvider.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/MqttProtocol.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketProtocol.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/Version.h"

#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "libjoynr/joynr-messaging/DummyPlatformSecurityManager.h"
#include "libjoynr/uds/UdsMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"

#include "libjoynrclustercontroller/ClusterControllerCallContext.h"
#include "libjoynrclustercontroller/ClusterControllerCallContextStorage.h"
#include "libjoynrclustercontroller/access-control/AccessControlListEditor.h"
#include "libjoynrclustercontroller/access-control/AccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"
#include "libjoynrclustercontroller/capabilities-directory/GlobalCapabilitiesDirectoryClient.h"
#include "libjoynrclustercontroller/messaging/MessagingPropertiesPersistence.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStubFactory.h"
#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"
#include "libjoynrclustercontroller/mqtt/MqttSender.h"
#include "libjoynrclustercontroller/mqtt/MqttTransportStatus.h"
#include "libjoynrclustercontroller/uds/UdsCcMessagingSkeleton.h"
#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeleton.h"
#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeletonNonTLS.h"
#include "libjoynrclustercontroller/websocket/WebSocketCcMessagingSkeletonTLS.h"

namespace ptree = boost::property_tree;

namespace joynr
{

class IKeychain;
class ITransportStatus;

JoynrClusterControllerRuntime::JoynrClusterControllerRuntime(
        std::unique_ptr<Settings> settings,
        std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError,
        std::shared_ptr<IKeychain> keyChain,
        MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory,
        int64_t removeStaleDelayMs)
        : JoynrRuntimeImpl(*settings, std::move(onFatalRuntimeError), std::move(keyChain)),
          _joynrDispatcher(),
          _subscriptionManager(),
          _messageSender(),
          _localCapabilitiesDirectory(nullptr),
          _libJoynrMessagingSkeleton(nullptr),
          _mqttMessagingSkeletonFactory(std::move(mqttMessagingSkeletonFactory)),
          _settings(std::move(settings)),
          _libjoynrSettings(*(this->_settings)),
          _localDomainAccessController(nullptr),
          _clusterControllerSettings(*(this->_settings)),
          _udsSettings(*(this->_settings)),
          _udsCcMessagingSkeleton(nullptr),
          _wsSettings(*(this->_settings)),
          _wsCcMessagingSkeleton(nullptr),
          _wsTLSCcMessagingSkeleton(nullptr),
          _mqttMessagingIsRunning(false),
          _doMqttMessaging(false),
          _wsMessagingStubFactory(),
          _multicastMessagingSkeletonDirectory(
                  std::make_shared<MulticastMessagingSkeletonDirectory>()),
          _ccMessageRouter(nullptr),
          _aclEditor(nullptr),
          _lifetimeSemaphore(0),
          _accessController(nullptr),
          _localCapabilitiesDirectoryStore(std::make_shared<LocalCapabilitiesDirectoryStore>()),
          _routingProviderParticipantId(),
          _discoveryProviderParticipantId(),
          _providerReregistrationControllerParticipantId(
                  "providerReregistrationController_participantId"),
          _messageNotificationProviderParticipantId(),
          _accessControlListEditorProviderParticipantId(),
          _isShuttingDown(false),
          _dummyGlobalAddress(),
          _clusterControllerStartDateMs(TimePoint::now().toMilliseconds()),
          _removeStaleDelay(removeStaleDelayMs),
          _removeStaleTimer(_singleThreadedIOService->getIOService())
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
        Settings currentSettings;
        try {
            Settings::merge(Settings(settingsFileName), currentSettings, true);
        } catch (const ptree::ini_parser_error& e) {
            JOYNR_LOG_FATAL(
                    logger(), "Can not read properties from {} {}", settingsFileName, e.message());
        }

        // Check for errors
        if (!currentSettings.isLoaded()) {
            return nullptr;
        }

        // Merge
        Settings::merge(currentSettings, *settings, true);
    }
    return create(std::move(settings),
                  JoynrRuntime::defaultFatalRuntimeErrorHandler,
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
    fillAvailableGbidsVector();
    /*
     * Even if MQTT messaging is not enabled, the corresponding data vector entries need to be
     * accessible
     * _mqttConnectionDataVector can be preinitialized by derived class.
     */
    while (_mqttConnectionDataVector.size() < _availableGbids.size()) {
        _mqttConnectionDataVector.push_back(
                std::make_shared<JoynrClusterControllerMqttConnectionData>());
    }
    _mqttConnectionDataVector.resize(_availableGbids.size()); // Sizes must be equal!

    const BrokerUrl defaultBrokerUrl = _messagingSettings.getBrokerUrl();

    // Check broker url contains valid broker protocol
    const Url url = defaultBrokerUrl.getBrokerBaseUrl();
    std::string brokerProtocol = url.getProtocol();

    std::transform(brokerProtocol.begin(), brokerProtocol.end(), brokerProtocol.begin(), ::toupper);

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator;

    if (brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTT) ||
        brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTTS) ||
        brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::TCP)) {
        addressCalculator = std::make_unique<joynr::MqttMulticastAddressCalculator>(
                _clusterControllerSettings.getMqttMulticastTopicPrefix(), _availableGbids);
        _doMqttMessaging = true;
    } else {
        JOYNR_LOG_FATAL(logger(), "invalid broker protocol in broker-url: {}", brokerProtocol);
        throw exceptions::JoynrRuntimeException(
                "Exception in JoynrRuntime: invalid broker protocol in broker-url: " +
                brokerProtocol);
    }

    std::string capabilitiesDirectoryChannelId =
            _messagingSettings.getCapabilitiesDirectoryChannelId();
    std::string capabilitiesDirectoryParticipantId =
            _messagingSettings.getCapabilitiesDirectoryParticipantId();

    // Initialise security manager
    std::unique_ptr<IPlatformSecurityManager> securityManager =
            std::make_unique<DummyPlatformSecurityManager>();

    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
    messagingStubFactory->registerStubFactory(std::make_shared<InProcessMessagingStubFactory>());

    MessagingPropertiesPersistence persist(
            _messagingSettings.getMessagingPropertiesPersistenceFilename());
    const std::string clusterControllerId = persist.getChannelId();

    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;

    _clusterControllerSettings.printSettings();
    _libjoynrSettings.printSettings();
    _wsSettings.printSettings();
    _udsSettings.printSettings();

    if (_doMqttMessaging) {
        const std::string ccMqttClientIdPrefix = _clusterControllerSettings.getMqttClientIdPrefix();
        const std::string mqttClientId = ccMqttClientIdPrefix + clusterControllerId;

        const std::chrono::seconds mqttReconnectDelayTimeSeconds =
                _messagingSettings.getMqttReconnectDelayTimeSeconds();

        const std::chrono::seconds mqttReconnectMaxDelayTimeSeconds =
                _messagingSettings.getMqttReconnectMaxDelayTimeSeconds();
        const bool isMqttExponentialBackoffEnabled =
                _messagingSettings.getMqttExponentialBackoffEnabled();
        std::chrono::seconds mqttKeepAliveTimeSeconds(0);
        BrokerUrl brokerUrl = defaultBrokerUrl;

        // default brokerIndex = 0
        for (std::uint8_t brokerIndex = 0; brokerIndex < _mqttConnectionDataVector.size();
             brokerIndex++) {
            if (brokerIndex == 0) {
                // default broker settings
                mqttKeepAliveTimeSeconds = _messagingSettings.getMqttKeepAliveTimeSeconds();
                brokerUrl = defaultBrokerUrl;
            } else {
                mqttKeepAliveTimeSeconds =
                        _messagingSettings.getAdditionalBackendMqttKeepAliveTimeSeconds(
                                brokerIndex - 1);
                brokerUrl = _messagingSettings.getAdditionalBackendBrokerUrl(brokerIndex - 1);
            }

            try {
                const auto& mosquittoConnection =
                        std::make_shared<MosquittoConnection>(_clusterControllerSettings,
                                                              brokerUrl,
                                                              mqttKeepAliveTimeSeconds,
                                                              mqttReconnectDelayTimeSeconds,
                                                              mqttReconnectMaxDelayTimeSeconds,
                                                              isMqttExponentialBackoffEnabled,
                                                              mqttClientId,
                                                              _availableGbids[brokerIndex]);

                const auto& connectionData = _mqttConnectionDataVector[brokerIndex];

                const auto& mqttMessageReceiver = connectionData->getMqttMessageReceiver();

                if (!mqttMessageReceiver || !connectionData->getMqttMessageSender()) {
                    JOYNR_LOG_TRACE(logger(),
                                    "{}: The mqtt message receiver supplied is NULL, creating "
                                    "MosquittoConnection ",
                                    brokerIndex);
                    connectionData->setMosquittoConnection(mosquittoConnection);

                    auto mqttTransportStatus = std::make_shared<MqttTransportStatus>(
                            mosquittoConnection, _availableGbids[brokerIndex]);
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
                            _messagingSettings,
                            clusterControllerId,
                            _availableGbids[brokerIndex],
                            _clusterControllerSettings.getMqttUnicastTopicPrefix()));
                }
            } catch (const exceptions::JoynrRuntimeException& e) {
                JOYNR_LOG_ERROR(logger(),
                                "{}: Creating mosquittoConnection failed. Error: {}",
                                brokerIndex,
                                e.getMessage());

                _doMqttMessaging = false;
                break;
            }
        }
    }

    const std::string globalClusterControllerAddress =
            getSerializedGlobalClusterControllerAddress();

    std::unique_ptr<MessageQueue<std::string>> messageQueue =
            std::make_unique<MessageQueue<std::string>>(
                    _clusterControllerSettings.getMessageQueueLimit(),
                    _clusterControllerSettings.getPerParticipantIdMessageQueueLimit(),
                    _clusterControllerSettings.getMessageQueueLimitBytes());
    std::unique_ptr<MessageQueue<std::shared_ptr<ITransportStatus>>> transportStatusQueue =
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>(
                    _clusterControllerSettings.getTransportNotAvailableQueueLimit(),
                    0,
                    _clusterControllerSettings.getTransportNotAvailableQueueLimitBytes());
    // init message router
    _ccMessageRouter = std::make_shared<CcMessageRouter>(
            _messagingSettings,
            _clusterControllerSettings,
            messagingStubFactory,
            _multicastMessagingSkeletonDirectory,
            std::move(securityManager),
            _singleThreadedIOService->getIOService(),
            std::move(addressCalculator),
            globalClusterControllerAddress,
            _systemServicesSettings.getCcMessageNotificationProviderParticipantId(),
            std::move(transportStatuses),
            std::move(messageQueue),
            std::move(transportStatusQueue),
            getGlobalClusterControllerAddress(),
            _availableGbids);

    _ccMessageRouter->init();

    // provision global capabilities directory
    bool isGloballyVisible = true;
    if (boost::starts_with(capabilitiesDirectoryChannelId, "{")) {
        try {
            using system::RoutingTypes::MqttAddress;
            auto globalCapabilitiesDirectoryAddress = std::make_shared<MqttAddress>();
            joynr::serializer::deserializeFromJson(
                    *globalCapabilitiesDirectoryAddress, capabilitiesDirectoryChannelId);
            _ccMessageRouter->addProvisionedNextHop(capabilitiesDirectoryParticipantId,
                                                    std::move(globalCapabilitiesDirectoryAddress),
                                                    isGloballyVisible);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger(),
                            "could not deserialize MqttAddress from {} - error: {}",
                            capabilitiesDirectoryChannelId,
                            e.what());
        }
    }

    if (_clusterControllerSettings.isWebSocketEnabled()) {
        // setup CC WebSocket interface
        _wsMessagingStubFactory = std::make_shared<WebSocketMessagingStubFactory>();
        _wsMessagingStubFactory->registerOnMessagingStubClosedCallback(
                [messagingStubFactory,
                 ccMessageRouterWeakPtr = joynr::util::as_weak_ptr(_ccMessageRouter)](
                        const std::shared_ptr<const joynr::system::RoutingTypes::Address>&
                                destinationAddress) {
                    if (auto ccMessageRouterSharedPtr = ccMessageRouterWeakPtr.lock()) {
                        ccMessageRouterSharedPtr->removeRoutingEntries(destinationAddress);
                    }
                    messagingStubFactory->remove(destinationAddress);
                });

        messagingStubFactory->registerStubFactory(_wsMessagingStubFactory);
    }

    if (_clusterControllerSettings.isUdsEnabled()) {
        // setup CC Uds interface
        _udsMessagingStubFactory = std::make_unique<UdsMessagingStubFactory>();
        _udsMessagingStubFactory->registerOnMessagingStubClosedCallback(
                [messagingStubFactory,
                 ccMessageRouterWeakPtr = joynr::util::as_weak_ptr(_ccMessageRouter)](
                        const std::shared_ptr<const joynr::system::RoutingTypes::Address>&
                                destinationAddress) {
                    if (auto ccMessageRouterSharedPtr = ccMessageRouterWeakPtr.lock()) {
                        ccMessageRouterSharedPtr->removeRoutingEntries(destinationAddress);
                    }
                    messagingStubFactory->remove(destinationAddress);
                });

        messagingStubFactory->registerStubFactory(_udsMessagingStubFactory);
    }

    /* LibJoynr */
    _messageSender = std::make_shared<MessageSender>(
            _ccMessageRouter, _keyChain, _messagingSettings.getTtlUpliftMs());
    _joynrDispatcher =
            std::make_shared<Dispatcher>(_messageSender, _singleThreadedIOService->getIOService());
    _messageSender->registerDispatcher(_joynrDispatcher);
    _messageSender->setReplyToAddress(globalClusterControllerAddress);
    _ccMessageRouter->setMessageSender(_messageSender);

    /* CC */
    _libJoynrMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(_joynrDispatcher);

    /**
     * ClusterController side MQTT
     *
     */
    if (_doMqttMessaging) {
        // create MqttMessagingSkeletonFactory only once and use it to create multiple skeletons
        if (!_mqttMessagingSkeletonFactory) {
            _mqttMessagingSkeletonFactory = [](std::weak_ptr<IMessageRouter> messageRouter,
                                               std::shared_ptr<MqttReceiver> mqttReceiver,
                                               const std::string& multicastTopicPrefix,
                                               const std::string& gbid,
                                               std::uint64_t ttlUplift = 0) {
                return std::make_shared<MqttMessagingSkeleton>(
                        messageRouter, mqttReceiver, multicastTopicPrefix, gbid, ttlUplift);
            };
        }

        for (std::uint8_t brokerIndex = 0; brokerIndex < _mqttConnectionDataVector.size();
             brokerIndex++) {
            const auto& connectionData = _mqttConnectionDataVector[brokerIndex];
            const auto& mqttMessagingSkeleton = _mqttMessagingSkeletonFactory(
                    _ccMessageRouter,
                    std::static_pointer_cast<MqttReceiver>(
                            connectionData->getMqttMessageReceiver()),
                    _clusterControllerSettings.getMqttMulticastTopicPrefix(),
                    _availableGbids[brokerIndex],
                    _messagingSettings.getTtlUpliftMs());

            connectionData->getMqttMessageReceiver()->registerReceiveCallback(
                    [mqttMessagingSkeletonWeakPtr = joynr::util::as_weak_ptr(
                             mqttMessagingSkeleton)](smrf::ByteVector&& msg) {
                        if (auto mqttMessagingSkeletonSharedPtr =
                                    mqttMessagingSkeletonWeakPtr.lock()) {
                            mqttMessagingSkeletonSharedPtr->onMessageReceived(std::move(msg));
                        }
                    });
            _multicastMessagingSkeletonDirectory
                    ->registerSkeleton<system::RoutingTypes::MqttAddress>(
                            std::move(mqttMessagingSkeleton), _availableGbids[brokerIndex]);

            // create message sender
            if (!connectionData->getMqttMessageSender()) {
                JOYNR_LOG_DEBUG(logger(),
                                "Connection {}. The mqtt message sender supplied is NULL, "
                                "creating the default "
                                "mqtt MessageSender",
                                brokerIndex);

                const auto& mqttMessageSender =
                        std::make_shared<MqttSender>(connectionData->getMosquittoConnection());
                connectionData->setMqttMessageSender(std::move(mqttMessageSender));
            }

            messagingStubFactory->registerStubFactory(std::make_shared<MqttMessagingStubFactory>(
                    connectionData->getMqttMessageSender(), _availableGbids[brokerIndex]));
        }
    }

    /**
     * libJoynr side
     *
     */
    _publicationManager =
            std::make_shared<PublicationManager>(_singleThreadedIOService->getIOService(),
                                                 _messageSender,
                                                 _messagingSettings.getTtlUpliftMs());
    _subscriptionManager = std::make_shared<SubscriptionManager>(
            _singleThreadedIOService->getIOService(), _ccMessageRouter);

    _dispatcherAddress = std::make_shared<InProcessMessagingAddress>(_libJoynrMessagingSkeleton);

    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(_messageSender, _subscriptionManager);

    _proxyFactory = std::make_unique<ProxyFactory>(std::move(joynrMessagingConnectorFactory));

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = _libjoynrSettings.getParticipantIdsPersistenceFilename();
    _participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    auto provisionedDiscoveryEntries = getProvisionedEntries();
    _discoveryProxy = std::make_shared<LocalDiscoveryAggregator>(provisionedDiscoveryEntries);

    std::unique_ptr<TaskSequencer<void>> taskSequencer = std::make_unique<TaskSequencer<void>>(
            std::chrono::milliseconds(MessagingQos().getTtl()));
    _globalCapabilitiesDirectoryClient = std::make_shared<GlobalCapabilitiesDirectoryClient>(
            _clusterControllerSettings, std::move(taskSequencer));
    _localCapabilitiesDirectory = std::make_shared<LocalCapabilitiesDirectory>(
            _clusterControllerSettings,
            _globalCapabilitiesDirectoryClient,
            _localCapabilitiesDirectoryStore,
            globalClusterControllerAddress,
            _ccMessageRouter,
            _singleThreadedIOService->getIOService(),
            clusterControllerId,
            _availableGbids,
            _messagingSettings.getDiscoveryEntryExpiryIntervalMs());
    _localCapabilitiesDirectory->init();
    _localCapabilitiesDirectory->loadPersistedFile();
    // importPersistedLocalCapabilitiesDirectory();

    std::string discoveryProviderParticipantId(
            _systemServicesSettings.getCcDiscoveryProviderParticipantId());

    MessagingQos messagingQos;
    messagingQos.setCompress(
            _clusterControllerSettings.isGlobalCapabilitiesDirectoryCompressedMessagesEnabled());

    {
        auto provisionedProviderDiscoveryEntry =
                provisionedDiscoveryEntries
                        .find(_systemServicesSettings.getCcDiscoveryProviderParticipantId())
                        ->second;
        using joynr::system::DiscoveryJoynrMessagingConnector;
        auto discoveryJoynrMessagingConnector = std::make_unique<DiscoveryJoynrMessagingConnector>(
                _messageSender,
                _subscriptionManager,
                std::string(),
                discoveryProviderParticipantId,
                messagingQos,
                provisionedProviderDiscoveryEntry);
        _discoveryProxy->setDiscoveryProxy(std::move(discoveryJoynrMessagingConnector));
    }
    _capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
            _joynrDispatcher,
            _discoveryProxy,
            _participantIdStorage,
            _dispatcherAddress,
            _ccMessageRouter,
            _messagingSettings.getDiscoveryEntryExpiryIntervalMs(),
            _publicationManager,
            globalClusterControllerAddress);

    _joynrDispatcher->registerPublicationManager(_publicationManager);
    _joynrDispatcher->registerSubscriptionManager(_subscriptionManager);

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
            "fixedParticipantId", _messagingSettings.getCapabilitiesDirectoryParticipantId());

    auto capabilitiesProxyBuilder =
            createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                    _messagingSettings.getDiscoveryDirectoriesDomain());
    capabilitiesProxyBuilder->setDiscoveryQos(discoveryQos);

    capabilitiesProxyBuilder->setMessagingQos(messagingQos);

    _globalCapabilitiesDirectoryClient->setProxy(capabilitiesProxyBuilder->build());

    // Do this after local capabilities directory and message router have been initialized.
    enableAccessController(provisionedDiscoveryEntries);

    registerInternalSystemServiceProviders();
}

std::shared_ptr<IMessageRouter> JoynrClusterControllerRuntime::getMessageRouter()
{
    return _ccMessageRouter;
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
            std::make_pair(_messagingSettings.getCapabilitiesDirectoryParticipantId(),
                           types::DiscoveryEntryWithMetaInfo(
                                   capabilityProviderVersion,
                                   _messagingSettings.getDiscoveryDirectoriesDomain(),
                                   infrastructure::IGlobalCapabilitiesDirectory::INTERFACE_NAME(),
                                   _messagingSettings.getCapabilitiesDirectoryParticipantId(),
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
    if (!_clusterControllerSettings.enableAccessController()) {
        return;
    }

    JOYNR_LOG_INFO(logger(),
                   "Access control was enabled attempting to load entries from {}.",
                   _clusterControllerSettings.getAclEntriesDirectory());

    auto localDomainAccessStore = std::make_shared<joynr::LocalDomainAccessStore>(
            _clusterControllerSettings.getLocalDomainAccessStorePersistenceFilename());

    namespace fs = boost::filesystem;

    fs::path aclEntriesPath(_clusterControllerSettings.getAclEntriesDirectory());

    if (fs::is_directory(aclEntriesPath)) {
        for (const auto& entry : fs::directory_iterator(aclEntriesPath)) {
            if (fs::is_regular_file(entry.path())) {
                const std::string aclPath = entry.path().string();
                localDomainAccessStore->mergeDomainAccessStore(LocalDomainAccessStore(aclPath));
            }
        }
    } else {
        JOYNR_LOG_ERROR(
                logger(), "Access control directory: {} does not exist.", aclEntriesPath.string());
    }

    _localDomainAccessController =
            std::make_shared<joynr::LocalDomainAccessController>(localDomainAccessStore);

    _accessController = std::make_shared<joynr::AccessController>(
            _localCapabilitiesDirectory, _localDomainAccessController);

    // whitelist provisioned entries into access controller
    for (const auto& entry : provisionedEntries) {
        _accessController->addParticipantToWhitelist(entry.second.getParticipantId());
    }

    _ccMessageRouter->setAccessController(util::as_weak_ptr(_accessController));

    _aclEditor = std::make_shared<AccessControlListEditor>(localDomainAccessStore,
                                                           _localDomainAccessController,
                                                           _clusterControllerSettings.aclAudit());

    // Set accessController also in LocalCapabilitiesDirectory
    _localCapabilitiesDirectory->setAccessController(util::as_weak_ptr(_accessController));

    // Log entries
    localDomainAccessStore->logContent();
}

void JoynrClusterControllerRuntime::registerInternalSystemServiceProviders()
{
    ClusterControllerCallContext clusterControllerCallContext;

    clusterControllerCallContext.setIsValid(true);
    clusterControllerCallContext.setIsInternalProviderRegistration(true);

    ClusterControllerCallContextStorage::set(std::move(clusterControllerCallContext));

    _discoveryProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::DiscoveryProvider>(
                    _localCapabilitiesDirectory),
            _systemServicesSettings.getCcDiscoveryProviderParticipantId());
    _routingProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::RoutingProvider>(_ccMessageRouter),
            _systemServicesSettings.getCcRoutingProviderParticipantId());
    _providerReregistrationControllerParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::ProviderReregistrationControllerProvider>(
                    _localCapabilitiesDirectory),
            _providerReregistrationControllerParticipantId);
    _messageNotificationProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::MessageNotificationProvider>(
                    _ccMessageRouter->getMessageNotificationProvider()),
            _systemServicesSettings.getCcMessageNotificationProviderParticipantId());

    if (_clusterControllerSettings.enableAccessController()) {
        _accessControlListEditorProviderParticipantId = registerInternalSystemServiceProvider(
                std::dynamic_pointer_cast<joynr::infrastructure::AccessControlListEditorProvider>(
                        _aclEditor),
                _systemServicesSettings.getCcAccessControlListEditorProviderParticipantId());
    }

    ClusterControllerCallContextStorage::invalidate();
}

void JoynrClusterControllerRuntime::unregisterInternalSystemServiceProvider(
        const std::string& participantId)
{
    if (_localCapabilitiesDirectory) {
        _localCapabilitiesDirectory->remove(participantId, nullptr, nullptr);
    }
    if (_joynrDispatcher) {
        _joynrDispatcher->removeRequestCaller(participantId);
    }
}
void JoynrClusterControllerRuntime::unregisterInternalSystemServiceProviders()
{
    if (!_accessControlListEditorProviderParticipantId.empty()) {
        unregisterInternalSystemServiceProvider(_accessControlListEditorProviderParticipantId);
    }

    unregisterInternalSystemServiceProvider(_messageNotificationProviderParticipantId);
    unregisterInternalSystemServiceProvider(_providerReregistrationControllerParticipantId);
    unregisterInternalSystemServiceProvider(_routingProviderParticipantId);
    unregisterInternalSystemServiceProvider(_discoveryProviderParticipantId);
}

void JoynrClusterControllerRuntime::startLocalCommunication()
{
    if (_clusterControllerSettings.isWebSocketEnabled()) {
        if (_clusterControllerSettings.isWsTLSPortSet()) {
            std::string certificateAuthorityPemFilename =
                    _wsSettings.getCertificateAuthorityPemFilename();
            std::string certificatePemFilename = _wsSettings.getCertificatePemFilename();
            std::string privateKeyPemFilename = _wsSettings.getPrivateKeyPemFilename();

            if (checkAndLogCryptoFileExistence(certificateAuthorityPemFilename,
                                               certificatePemFilename,
                                               privateKeyPemFilename,
                                               logger())) {
                JOYNR_LOG_INFO(logger(), "Using TLS connection");

                system::RoutingTypes::WebSocketAddress wsAddress(
                        system::RoutingTypes::WebSocketProtocol::WSS,
                        "localhost",
                        _clusterControllerSettings.getWsTLSPort(),
                        "");

                _wsTLSCcMessagingSkeleton = std::make_shared<WebSocketCcMessagingSkeletonTLS>(
                        _singleThreadedIOService->getIOService(),
                        _ccMessageRouter,
                        _wsMessagingStubFactory,
                        wsAddress,
                        certificateAuthorityPemFilename,
                        certificatePemFilename,
                        privateKeyPemFilename);
                _wsTLSCcMessagingSkeleton->init();
            }
        }

        if (_clusterControllerSettings.isWsPortSet()) {
            system::RoutingTypes::WebSocketAddress wsAddress(
                    system::RoutingTypes::WebSocketProtocol::WS,
                    "localhost",
                    _clusterControllerSettings.getWsPort(),
                    "");

            _wsCcMessagingSkeleton = std::make_shared<WebSocketCcMessagingSkeletonNonTLS>(
                    _singleThreadedIOService->getIOService(),
                    _ccMessageRouter,
                    _wsMessagingStubFactory,
                    wsAddress);
            _wsCcMessagingSkeleton->init();
        }
    }
    if (_udsMessagingStubFactory) {
        //_udsMessagingStubFactory only created if UDS is enabled
        _udsCcMessagingSkeleton = std::make_unique<UdsCcMessagingSkeleton>(_ccMessageRouter);

        _udsServer =
                std::make_unique<UdsServer>(_udsSettings); // Stops implicitly old server if exists
        _udsServer->setConnectCallback([this](const system::RoutingTypes::UdsClientAddress& address,
                                              std::unique_ptr<IUdsSender> sender) {
            _udsMessagingStubFactory->addClient(address, std::move(sender));
        });
        _udsServer->setDisconnectCallback(
                [this](const system::RoutingTypes::UdsClientAddress& address) {
                    _udsMessagingStubFactory->onMessagingStubClosed(address);
                });
        _udsServer->setReceiveCallback([this](const system::RoutingTypes::UdsClientAddress&,
                                              smrf::ByteVector&& newMessage,
                                              const std::string& creator) {
            _udsCcMessagingSkeleton->onMessageReceived(std::move(newMessage), creator);
        });
        _udsServer->start();
    }
}

JoynrClusterControllerRuntime::~JoynrClusterControllerRuntime()
{
    JOYNR_LOG_TRACE(logger(), "entering ~JoynrClusterControllerRuntime");
    shutdown();
    JOYNR_LOG_TRACE(logger(), "leaving ~JoynrClusterControllerRuntime");
}

void JoynrClusterControllerRuntime::startExternalCommunication()
{
    if (_doMqttMessaging && !_mqttMessagingIsRunning) {
        for (const auto& connectionData : _mqttConnectionDataVector) {
            connectionData->getMosquittoConnection()->start();
        }
        _mqttMessagingIsRunning = true;
    }
}

void JoynrClusterControllerRuntime::stopExternalCommunication()
{
    // joynrDispatcher->stopExternalCommunication();
    if (_doMqttMessaging && _mqttMessagingIsRunning) {
        for (const auto& connecton : _mqttConnectionDataVector) {
            connecton->getMosquittoConnection()->stop();
        }
        _mqttMessagingIsRunning = false;
    }
}

void JoynrClusterControllerRuntime::shutdown()
{
    std::lock_guard<std::mutex> lock(_proxyBuildersMutex);
    if (_isShuttingDown) {
        return;
    }
    _isShuttingDown = true;
    for (auto proxyBuilder : _proxyBuilders) {
        proxyBuilder->stop();
        proxyBuilder.reset();
    }
    _proxyBuilders.clear();

    if (_wsCcMessagingSkeleton) {
        _wsCcMessagingSkeleton->shutdown();
    }
    if (_wsTLSCcMessagingSkeleton) {
        _wsTLSCcMessagingSkeleton->shutdown();
    }

    _udsServer.reset();

    unregisterInternalSystemServiceProviders();

    if (_ccMessageRouter) {
        _ccMessageRouter->shutdown();
    }
    if (_publicationManager) {
        _publicationManager->shutdown();
    }
    if (_subscriptionManager) {
        _subscriptionManager->shutdown();
    }
    if (_globalCapabilitiesDirectoryClient) {
        _globalCapabilitiesDirectoryClient->shutdown();
    }
    if (_localCapabilitiesDirectory) {
        _localCapabilitiesDirectory->shutdown();
    }

    stop();

    if (_multicastMessagingSkeletonDirectory) {
        _multicastMessagingSkeletonDirectory
                ->unregisterSkeletons<system::RoutingTypes::MqttAddress>();
    }

    if (_joynrDispatcher) {
        _joynrDispatcher->shutdown();
        _joynrDispatcher.reset();
    }

    _removeStaleTimer.cancel();
}

void JoynrClusterControllerRuntime::shutdownClusterController()
{
    JOYNR_LOG_TRACE(logger(), "Shutdown Cluster Controller");
    _lifetimeSemaphore.notify();
}

void JoynrClusterControllerRuntime::runForever()
{
    _lifetimeSemaphore.wait();
    shutdown();
}

std::shared_ptr<JoynrClusterControllerRuntime> JoynrClusterControllerRuntime::create(
        std::unique_ptr<Settings> settings,
        std::function<void(const exceptions::JoynrRuntimeException&)> onFatalRuntimeError,
        const std::string& discoveryEntriesFile,
        std::shared_ptr<IKeychain> keyChain,
        MqttMessagingSkeletonFactory mqttMessagingSkeletonFactory)
{
    auto runtime = std::shared_ptr<JoynrClusterControllerRuntime>(
            new JoynrClusterControllerRuntime(std::move(settings),
                                              std::move(onFatalRuntimeError),
                                              std::move(keyChain),
                                              std::move(mqttMessagingSkeletonFactory)));
    runtime->init();

    if (runtime->_localCapabilitiesDirectory) {
        runtime->_localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(
                discoveryEntriesFile);
        runtime->start();
    } else {
        static const exceptions::JoynrRuntimeException fatalError(
                "Initialization of runtime failed to create local capabilities directory.");
        if (onFatalRuntimeError) {
            onFatalRuntimeError(fatalError);
        }
        JOYNR_LOG_ERROR(logger(), fatalError.getMessage());
        runtime->shutdown();
    }
    return runtime;
}

void JoynrClusterControllerRuntime::start()
{
    _singleThreadedIOService->start();
    startExternalCommunication();
    startLocalCommunication();
    scheduleRemoveStaleTimer();
}

void JoynrClusterControllerRuntime::scheduleRemoveStaleTimer()
{
    boost::system::error_code timerError = boost::system::error_code();
    _removeStaleTimer.expires_from_now(std::chrono::milliseconds(_removeStaleDelay), timerError);
    if (timerError) {
        JOYNR_LOG_ERROR(logger(),
                        "Error from remove stale timer in cluster controller: {}: {}",
                        timerError.value(),
                        timerError.message());
    } else {
        _removeStaleTimer.async_wait([lcdWeakPtr =
                                              joynr::util::as_weak_ptr(_localCapabilitiesDirectory),
                                      ccStartTimeMs = _clusterControllerStartDateMs](
                                             const boost::system::error_code& localTimerError) {
            if (localTimerError == boost::asio::error::operation_aborted) {
                JOYNR_LOG_ERROR(
                        logger(),
                        "scheduled remove stale call aborted after shutdown, error code from "
                        "remove stale timer: {}",
                        localTimerError.message());
                return;
            } else if (localTimerError) {
                JOYNR_LOG_ERROR(logger(),
                                "send scheduled remove stale aborted because of error from remove "
                                "stale timer: {}",
                                localTimerError.message());
                return;
            }

            if (auto lcdSharedPtr = lcdWeakPtr.lock()) {
                lcdSharedPtr->removeStaleProvidersOfClusterController(ccStartTimeMs);
            }
        });
    }
}

void JoynrClusterControllerRuntime::stop()
{
    stopExternalCommunication();

    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    if (_singleThreadedIOService) {
        _singleThreadedIOService->stop();
    }
}

std::string JoynrClusterControllerRuntime::getSerializedGlobalClusterControllerAddress() const
{
    const auto defaultConnectionData = _mqttConnectionDataVector[0];
    if (_doMqttMessaging) {
        return defaultConnectionData->getMqttMessageReceiver()
                ->getSerializedGlobalClusterControllerAddress();
    }
    JOYNR_LOG_ERROR(logger(),
                    "Cannot obtain globalClusterControllerAddress, as doMqttMessaging is: {}",
                    _doMqttMessaging);
    // in order to at least allow local communication in case global transport
    // is not correctly configured, a dummy address must be provided since
    // otherwise LibJoynrRuntime cannot be started
    return "global-transport-not-available";
}

void JoynrClusterControllerRuntime::fillAvailableGbidsVector()
{
    const std::string defaultBackendGbid = _messagingSettings.getGbid();
    _availableGbids.emplace_back(defaultBackendGbid);

    std::uint8_t additionalBackends = _messagingSettings.getAdditionalBackendsCount();

    if (defaultBackendGbid.empty() && additionalBackends) {
        JOYNR_LOG_ERROR(
                logger(), "defaultBackendGbid is empty, MultipleBackend functionality disabled");
        return;
    }

    for (std::uint8_t index = 0; index < additionalBackends; index++) {
        _availableGbids.emplace_back(_messagingSettings.getAdditionalBackendGbid(index));
    }
}

const system::RoutingTypes::Address& JoynrClusterControllerRuntime::
        getGlobalClusterControllerAddress() const
{
    const auto defaultConnectionData = _mqttConnectionDataVector[0];
    if (_doMqttMessaging) {
        return defaultConnectionData->getMqttMessageReceiver()->getGlobalClusterControllerAddress();
    }
    JOYNR_LOG_ERROR(logger(),
                    "Cannot obtain globalClusterControllerAddress, as doMqttMessaging is: {}",
                    _doMqttMessaging);
    // in order to at least allow local communication in case global transport
    // is not correctly configured, a dummy address must be provided since
    // otherwise LibJoynrRuntime cannot be started
    return _dummyGlobalAddress;
}
} // namespace joynr
