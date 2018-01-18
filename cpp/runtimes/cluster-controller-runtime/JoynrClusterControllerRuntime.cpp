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
#include "joynr/ConnectorFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Dispatcher.h"
#include "joynr/HttpMulticastAddressCalculator.h"
#include "joynr/IDispatcher.h"
#include "joynr/IKeychain.h"
#include "joynr/IMqttMessagingSkeleton.h"
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
#include "joynr/system/ProviderReregistrationControllerProvider.h"
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
        std::shared_ptr<ITransportMessageReceiver> mqttMessageReceiver,
        std::shared_ptr<ITransportMessageSender> mqttMessageSender)
        : JoynrRuntime(*settings, std::move(keyChain)),
          joynrDispatcher(),
          inProcessDispatcher(),
          subscriptionManager(),
          messageSender(),
          localCapabilitiesDirectory(nullptr),
          libJoynrMessagingSkeleton(nullptr),
          httpMessageReceiver(httpMessageReceiver),
          httpMessageSender(httpMessageSender),
          httpMessagingSkeleton(nullptr),
          mosquittoConnection(nullptr),
          mqttMessageReceiver(mqttMessageReceiver),
          mqttMessageSender(mqttMessageSender),
          mqttMessagingSkeletonFactory(std::move(mqttMessagingSkeletonFactory)),
          mqttMessagingSkeleton(nullptr),
          dispatcherList(),
          inProcessPublicationSender(),
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
          accessControlListEditorProviderParticipantId()
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
        JOYNR_LOG_DEBUG(logger(), "MQTT-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
                brokerUrl.toString(), "");
        addressCalculator = std::make_unique<joynr::MqttMulticastAddressCalculator>(
                globalAddress, clusterControllerSettings.getMqttMulticastTopicPrefix());
        doMqttMessaging = true;
    } else if (brokerProtocol == "HTTP" || brokerProtocol == "HTTPS") {
        JOYNR_LOG_DEBUG(logger(), "HTTP-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                brokerUrl.toString(), "");
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

    // CAREFUL: the factory creates an old style dispatcher, not the new one!
    inProcessDispatcher =
            std::make_shared<InProcessDispatcher>(singleThreadIOService->getIOService());
    /* CC */
    // create the messaging stub factory
    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
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
            JOYNR_LOG_DEBUG(logger(),
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
            JOYNR_LOG_DEBUG(logger(),
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
            std::move(transportStatuses));
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
        if (!mqttMessagingIsRunning) {
            if (!mqttMessagingSkeletonFactory) {
                mqttMessagingSkeletonFactory = [](std::weak_ptr<IMessageRouter> messageRouter,
                                                  std::shared_ptr<MqttReceiver> mqttReceiver,
                                                  const std::string& multicastTopicPrefix,
                                                  std::uint64_t ttlUplift = 0) {
                    return std::make_shared<MqttMessagingSkeleton>(
                            messageRouter, mqttReceiver, multicastTopicPrefix, ttlUplift);
                };
            }

            mqttMessagingSkeleton = mqttMessagingSkeletonFactory(
                    ccMessageRouter,
                    std::static_pointer_cast<MqttReceiver>(mqttMessageReceiver),
                    clusterControllerSettings.getMqttMulticastTopicPrefix(),
                    messagingSettings.getTtlUpliftMs());

            auto mqttMessagingSkeletonCopyForCapturing = mqttMessagingSkeleton;
            mqttMessageReceiver
                    ->registerReceiveCallback([mqttMessagingSkeleton =
                                                       mqttMessagingSkeletonCopyForCapturing](
                            smrf::ByteVector &&
                            msg) { mqttMessagingSkeleton->onMessageReceived(std::move(msg)); });
            multicastMessagingSkeletonDirectory
                    ->registerSkeleton<system::RoutingTypes::MqttAddress>(mqttMessagingSkeleton);
        }

        // create message sender
        if (!mqttMessageSender) {
            JOYNR_LOG_DEBUG(logger(),
                            "The mqtt message sender supplied is NULL, creating the default "
                            "mqtt MessageSender");

            mqttMessageSender =
                    std::make_shared<MqttSender>(mosquittoConnection, messagingSettings);
        }

        messagingStubFactory->registerStubFactory(
                std::make_shared<MqttMessagingStubFactory>(mqttMessageSender));
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
    inProcessPublicationSender = std::make_shared<InProcessPublicationSender>(subscriptionManager);

    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(libJoynrMessagingSkeleton);
    auto inProcessConnectorFactory = std::make_unique<InProcessConnectorFactory>(
            subscriptionManager,
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
    discoveryProxy = std::make_shared<LocalDiscoveryAggregator>(provisionedDiscoveryEntries);
    requestCallerDirectory =
            std::dynamic_pointer_cast<IRequestCallerDirectory>(inProcessDispatcher);

    std::shared_ptr<ICapabilitiesClient> capabilitiesClient =
            std::make_shared<CapabilitiesClient>();
    localCapabilitiesDirectory =
            std::make_shared<LocalCapabilitiesDirectory>(clusterControllerSettings,
                                                         capabilitiesClient,
                                                         globalClusterControllerAddress,
                                                         ccMessageRouter,
                                                         singleThreadIOService->getIOService(),
                                                         clusterControllerId);
    localCapabilitiesDirectory->init();
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
                subscriptionManager,
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

    std::shared_ptr<ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
            capabilitiesProxyBuilder =
                    createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                            messagingSettings.getDiscoveryDirectoriesDomain());
    capabilitiesProxyBuilder->setDiscoveryQos(discoveryQos);

    capabilitiesClient->setProxyBuilder(std::move(capabilitiesProxyBuilder));

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

    JOYNR_LOG_DEBUG(logger(),
                    "AccessControl was enabled attempting to load entries from {}.",
                    clusterControllerSettings.getAclEntriesDirectory());

    auto localDomainAccessStore = std::make_shared<joynr::LocalDomainAccessStore>(
            clusterControllerSettings.getLocalDomainAccessStorePersistenceFilename());

    namespace fs = boost::filesystem;

    fs::path aclEntriesPath(clusterControllerSettings.getAclEntriesDirectory());

    if (fs::is_directory(aclEntriesPath)) {
        for (const auto& entry : fs::directory_iterator(aclEntriesPath)) {
            if (fs::is_regular_file(entry.path())) {
                localDomainAccessStore->mergeDomainAccessStore(
                        LocalDomainAccessStore(entry.path().string()));
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
            localCapabilitiesDirectory, localDomainAccessController);

    // whitelist provisioned entries into access controller
    for (const auto& entry : provisionedEntries) {
        accessController->addParticipantToWhitelist(entry.second.getParticipantId());
    }

    ccMessageRouter->setAccessController(std::move(util::as_weak_ptr(accessController)));

    aclEditor = std::make_shared<AccessControlListEditor>(std::move(localDomainAccessStore),
                                                          localDomainAccessController,
                                                          clusterControllerSettings.aclAudit());

    // Set accessController also in LocalCapabilitiesDirectory
    localCapabilitiesDirectory->setAccessController(std::move(util::as_weak_ptr(accessController)));
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

    routingProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::RoutingProvider>(ccMessageRouter),
            systemServicesSettings.getCcRoutingProviderParticipantId());
    discoveryProviderParticipantId = registerInternalSystemServiceProvider(
            std::dynamic_pointer_cast<joynr::system::DiscoveryProvider>(localCapabilitiesDirectory),
            systemServicesSettings.getCcDiscoveryProviderParticipantId());
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

void JoynrClusterControllerRuntime::unregisterInternalSystemServiceProviders()
{
    if (!accessControlListEditorProviderParticipantId.empty()) {
        unregisterProvider(accessControlListEditorProviderParticipantId);
    }
    unregisterProvider(messageNotificationProviderParticipantId);
    unregisterProvider(discoveryProviderParticipantId);
    unregisterProvider(providerReregistrationControllerParticipantId);
    unregisterProvider(routingProviderParticipantId);
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
    shutdown();
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
    if (inProcessDispatcher) {
        inProcessDispatcher->shutdown();
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

    stopExternalCommunication();

    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    if (singleThreadIOService) {
        singleThreadIOService->stop();
    }

    if (multicastMessagingSkeletonDirectory) {
        multicastMessagingSkeletonDirectory
                ->unregisterSkeleton<system::RoutingTypes::MqttAddress>();
    }

    if (joynrDispatcher != nullptr) {
        joynrDispatcher->shutdown();
        joynrDispatcher.reset();
    }

    inProcessPublicationSender.reset();
}

void JoynrClusterControllerRuntime::shutdownClusterController()
{
    JOYNR_LOG_TRACE(logger(), "Shutdown Cluster Controller");
    lifetimeSemaphore.notify();
}

void JoynrClusterControllerRuntime::runForever()
{
    lifetimeSemaphore.wait();
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
    startExternalCommunication();
    startLocalCommunication();
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
