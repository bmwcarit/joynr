/*eslint promise/catch-or-return: "off"*/
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
const Promise = require("../../global/Promise");
const Arbitrator = require("../capabilities/arbitration/Arbitrator");
const ProviderBuilder = require("../provider/ProviderBuilder");
const ProxyBuilder = require("../proxy/ProxyBuilder");
const GlobalDiscoveryEntry = require("../../generated/joynr/types/GlobalDiscoveryEntry");
const CapabilitiesRegistrar = require("../capabilities/CapabilitiesRegistrar");
const ParticipantIdStorage = require("../capabilities/ParticipantIdStorage");
const CapabilityDiscovery = require("../capabilities/discovery/CapabilityDiscovery");
const CapabilitiesStore = require("../capabilities/CapabilitiesStore");
const RequestReplyManager = require("../dispatching/RequestReplyManager");
const PublicationManager = require("../dispatching/subscription/PublicationManager");
const SubscriptionManager = require("../dispatching/subscription/SubscriptionManager");
const Dispatcher = require("../dispatching/Dispatcher");
const PlatformSecurityManager = require("../security/PlatformSecurityManagerNode");
const ChannelAddress = require("../../generated/joynr/system/RoutingTypes/ChannelAddress");
const MqttMessagingStubFactory = require("../messaging/mqtt/MqttMessagingStubFactory");
const MqttMessagingSkeleton = require("../messaging/mqtt/MqttMessagingSkeleton");
const MqttAddress = require("../../generated/joynr/system/RoutingTypes/MqttAddress");
const SharedMqttClient = require("../messaging/mqtt/SharedMqttClient");
const MqttMulticastAddressCalculator = require("../messaging/mqtt/MqttMulticastAddressCalculator");
const MessagingSkeletonFactory = require("../messaging/MessagingSkeletonFactory");
const MessagingStubFactory = require("../messaging/MessagingStubFactory");
const MessageRouter = require("../messaging/routing/MessageRouter");
const MessageQueue = require("../messaging/routing/MessageQueue");
const InProcessSkeleton = require("../util/InProcessSkeleton");
const InProcessStub = require("../util/InProcessStub");
const InProcessMessagingStubFactory = require("../messaging/inprocess/InProcessMessagingStubFactory");
const InProcessMessagingSkeleton = require("../messaging/inprocess/InProcessMessagingSkeleton");
const InProcessMessagingStub = require("../messaging/inprocess/InProcessMessagingStub");
const InProcessAddress = require("../messaging/inprocess/InProcessAddress");
const MessagingQos = require("../messaging/MessagingQos");
const DiscoveryQos = require("../proxy/DiscoveryQos");
const DiscoveryScope = require("../../generated/joynr/types/DiscoveryScope");
const TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
const UtilInternal = require("../util/UtilInternal");
const CapabilitiesUtil = require("../util/CapabilitiesUtil");
const loggingManager = require("../system/LoggingManager");
const uuid = require("uuid/v4");
const defaultLibjoynrSettings = require("./settings/defaultLibjoynrSettings");
const defaultClusterControllerSettings = require("./settings/defaultClusterControllerSettings");
const Typing = require("../util/Typing");
const LongTimer = require("../util/LongTimer");
const LocalStorage = require("../../global/LocalStorageNode");
const JoynrStates = {
    SHUTDOWN: "shut down",
    STARTING: "starting",
    STARTED: "started",
    SHUTTINGDOWN: "shutting down"
};

let clusterControllerSettings;

/**
 * The InProcessRuntime is the version of the libjoynr-js runtime that hosts its own
 * cluster controller
 *
 * @name InProcessRuntime
 * @constructor
 *
 * @param provisioning
 */
function InProcessRuntime(provisioning) {
    let initialRoutingTable;
    let untypedCapabilities;
    let typedCapabilities;
    let messagingSkeletonFactory;
    let messagingStubFactory;
    let messageRouter;
    let libjoynrMessagingSkeleton;
    let clusterControllerMessagingSkeleton;
    let mqttMessagingSkeleton;
    let clusterControllerMessagingStub, dispatcher;
    let requestReplyManager;
    let subscriptionManager;
    let publicationManager;
    let participantIdStorage;
    let capabilityDiscovery;
    let arbitrator;
    let channelId;
    let providerBuilder;
    let proxyBuilder;
    let capabilitiesRegistrar;
    let localCapabilitiesStore;
    let globalCapabilitiesCache;
    let discoveryStub;
    let messageQueueSettings;
    let persistency;
    let freshnessIntervalId;

    // this is required at load time of libjoynr
    const typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

    /**
     * @name InProcessRuntime#typeRegistry
     * @type TypeRegistry
     */
    Object.defineProperty(this, "typeRegistry", {
        get() {
            return typeRegistry;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#registration
     * @type CapabilitiesRegistrar
     */
    Object.defineProperty(this, "registration", {
        get() {
            return capabilitiesRegistrar;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#participantIdStorage
     * @type ParticipantIdStorage
     */
    Object.defineProperty(this, "participantIdStorage", {
        get() {
            return participantIdStorage;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#providerBuilder
     * @type ProviderBuilder
     */
    Object.defineProperty(this, "providerBuilder", {
        get() {
            return providerBuilder;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#proxyBuilder
     * @type ProxyBuilder
     */
    Object.defineProperty(this, "proxyBuilder", {
        get() {
            return proxyBuilder;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#logger
     * @type LoggingManager
     */
    Object.defineProperty(this, "logging", {
        get() {
            return loggingManager;
        },
        enumerable: true
    });

    let log;

    let joynrState = JoynrStates.SHUTDOWN;

    if (provisioning.capabilities && provisioning.capabilities.discoveryQos) {
        const discoveryQos = provisioning.capabilities.discoveryQos;

        if (discoveryQos.discoveryExpiryIntervalMs) {
            CapabilitiesRegistrar.setDefaultExpiryIntervalMs(discoveryQos.discoveryExpiryIntervalMs);
        }

        const discoveryQosSettings = {};

        if (discoveryQos.discoveryRetryDelayMs) {
            discoveryQosSettings.discoveryRetryDelayMs = discoveryQos.discoveryRetryDelayMs;
        }
        if (discoveryQos.discoveryTimeoutMs) {
            discoveryQosSettings.discoveryTimeoutMs = discoveryQos.discoveryTimeoutMs;
        }

        DiscoveryQos.setDefaultSettings(discoveryQosSettings);
    }

    /**
     * Starts up the libjoynr instance
     *
     * @name InProcessRuntime#start
     * @function
     * @returns {Object} an A+ promise object, reporting when libjoynr startup is
     *          then({InProcessRuntime}, {Error})-ed
     * @throws {Error}
     *             if libjoynr is not in SHUTDOWN state
     */
    this.start = function start() {
        let i;

        if (joynrState !== JoynrStates.SHUTDOWN) {
            throw new Error("Cannot start libjoynr because it's currently \"" + joynrState + '"');
        }
        joynrState = JoynrStates.STARTING;

        if (!provisioning) {
            throw new Error("Constructor has not been invoked with provisioned data");
        }

        log = loggingManager.getLogger("joynr.start.InProcessRuntime");

        const persistencyProvisioning = provisioning.persistency || {};
        persistency = new LocalStorage({
            clearPersistency: persistencyProvisioning.clearPersistency,
            location: persistencyProvisioning.location
        });

        if (UtilInternal.checkNullUndefined(provisioning.bounceProxyUrl)) {
            throw new Error("bounce proxy URL not set in provisioning.bounceProxyUrl");
        }
        if (UtilInternal.checkNullUndefined(provisioning.bounceProxyBaseUrl)) {
            throw new Error("bounce proxy base URL not set in provisioning.bounceProxyBaseUrl");
        }
        if (UtilInternal.checkNullUndefined(provisioning.brokerUri)) {
            throw new Error("broker URI not set in provisioning.brokerUri");
        }

        initialRoutingTable = {};

        channelId = provisioning.channelId || persistency.getItem("joynr.channels.channelId.1") || "chjs_" + uuid();
        persistency.setItem("joynr.channels.channelId.1", channelId);

        untypedCapabilities = provisioning.capabilities || [];
        clusterControllerSettings = defaultClusterControllerSettings({
            bounceProxyBaseUrl: provisioning.bounceProxyBaseUrl,
            brokerUri: provisioning.brokerUri
        });
        const defaultClusterControllerCapabilities = clusterControllerSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultClusterControllerCapabilities);
        // allow use of _typeName once
        typeRegistry.addType(new ChannelAddress()._typeName, ChannelAddress, false);
        typeRegistry.addType(new MqttAddress()._typeName, MqttAddress, false);
        typedCapabilities = [];
        for (i = 0; i < untypedCapabilities.length; i++) {
            const capability = new GlobalDiscoveryEntry(untypedCapabilities[i]);
            if (!capability.address) {
                throw new Error("provisioned capability is missing address: " + JSON.stringify(capability));
            }
            initialRoutingTable[capability.participantId] = Typing.augmentTypes(
                JSON.parse(capability.address),
                typeRegistry
            );
            typedCapabilities.push(capability);
        }

        messageQueueSettings = {};
        if (provisioning.messaging !== undefined && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
            messageQueueSettings.maxQueueSizeInKBytes = provisioning.messaging.maxQueueSizeInKBytes;
        }

        const globalClusterControllerAddress = new MqttAddress({
            brokerUri: provisioning.brokerUri,
            topic: channelId
        });
        const serializedGlobalClusterControllerAddress = JSON.stringify(globalClusterControllerAddress);

        const mqttClient = new SharedMqttClient({
            address: globalClusterControllerAddress,
            provisioning: provisioning.mqtt || {}
        });

        messagingSkeletonFactory = new MessagingSkeletonFactory();

        const messagingStubFactories = {};
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        //messagingStubFactories[ChannelAddress._typeName] = channelMessagingStubFactory;
        messagingStubFactories[MqttAddress._typeName] = new MqttMessagingStubFactory({
            client: mqttClient,
            address: globalClusterControllerAddress
        });

        messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        messageRouter = new MessageRouter({
            initialRoutingTable,
            persistency,
            joynrInstanceId: channelId,
            typeRegistry,
            messagingStubFactory,
            messagingSkeletonFactory,
            multicastAddressCalculator: new MqttMulticastAddressCalculator({
                globalAddress: globalClusterControllerAddress
            }),
            messageQueue: new MessageQueue(messageQueueSettings)
        });
        messageRouter.setReplyToAddress(serializedGlobalClusterControllerAddress);

        mqttMessagingSkeleton = new MqttMessagingSkeleton({
            address: globalClusterControllerAddress,
            client: mqttClient,
            messageRouter
        });

        mqttClient.onConnected().then(() => {
            capabilityDiscovery.globalAddressReady(globalClusterControllerAddress);
            //channelMessagingStubFactory.globalAddressReady(channelAddress);
            return null;
        });

        // link up clustercontroller messaging to dispatcher
        clusterControllerMessagingSkeleton = new InProcessMessagingSkeleton();
        clusterControllerMessagingStub = new InProcessMessagingStub(clusterControllerMessagingSkeleton);

        // clustercontroller messaging handled by the messageRouter
        clusterControllerMessagingSkeleton.registerListener(messageRouter.route);

        const ttlUpLiftMs =
            provisioning.messaging && provisioning.messaging.TTL_UPLIFT ? provisioning.messaging.TTL_UPLIFT : undefined;
        dispatcher = new Dispatcher(clusterControllerMessagingStub, new PlatformSecurityManager(), ttlUpLiftMs);

        libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
        libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

        const messagingSkeletons = {};
        messagingSkeletons[InProcessAddress._typeName] = libjoynrMessagingSkeleton;
        messagingSkeletons[MqttAddress._typeName] = mqttMessagingSkeleton;
        messagingSkeletonFactory.setSkeletons(messagingSkeletons);

        requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
        subscriptionManager = new SubscriptionManager(dispatcher);
        publicationManager = new PublicationManager(dispatcher, persistency, channelId);

        dispatcher.registerRequestReplyManager(requestReplyManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerMessageRouter(messageRouter);

        localCapabilitiesStore = new CapabilitiesStore(
            CapabilitiesUtil.toDiscoveryEntries(defaultLibjoynrSettings.capabilities || [])
        );
        globalCapabilitiesCache = new CapabilitiesStore(typedCapabilities);

        participantIdStorage = new ParticipantIdStorage(persistency, uuid);

        discoveryStub = new InProcessStub();
        capabilitiesRegistrar = Object.freeze(
            new CapabilitiesRegistrar({
                discoveryStub,
                messageRouter,
                requestReplyManager,
                publicationManager,
                libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton),
                participantIdStorage
            })
        );

        arbitrator = new Arbitrator(discoveryStub);

        proxyBuilder = Object.freeze(
            new ProxyBuilder(
                {
                    arbitrator,
                    requestReplyManager,
                    subscriptionManager,
                    publicationManager
                },
                {
                    messageRouter,
                    libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton)
                }
            )
        );

        const internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

        const defaultProxyBuildSettings = {
            domain: "io.joynr",
            messagingQos: internalMessagingQos,
            discoveryQos: new DiscoveryQos({
                discoveryScope: DiscoveryScope.GLOBAL_ONLY,
                cacheMaxAgeMs: UtilInternal.getMaxLongValue()
            })
        };

        capabilityDiscovery = new CapabilityDiscovery(
            localCapabilitiesStore,
            globalCapabilitiesCache,
            messageRouter,
            proxyBuilder,
            defaultProxyBuildSettings.domain
        );

        discoveryStub.setSkeleton(new InProcessSkeleton(capabilityDiscovery));

        const period = provisioning.capabilitiesFreshnessUpdateIntervalMs || 3600000; // default: 1 hour
        freshnessIntervalId = LongTimer.setInterval(() => {
            capabilityDiscovery.touch(channelId, period).catch(error => {
                log.error("error sending freshness update: " + error);
            });
            return null;
        }, period);

        providerBuilder = Object.freeze(new ProviderBuilder());

        if (provisioning.logging) {
            loggingManager.configure(provisioning.logging);
        }

        joynrState = JoynrStates.STARTED;
        publicationManager.restore();
        log.debug("joynr initialized");
        return Promise.resolve();
    };

    /**
     * Shuts down libjoynr
     *
     * @name InProcessRuntime#shutdown
     * @function
     * @throws {Error}
     *             if libjoynr is not in the STARTED state
     */
    this.shutdown = function shutdown(settings) {
        if (joynrState !== JoynrStates.STARTED) {
            throw new Error("Cannot shutdown libjoynr because it's currently \"" + joynrState + '"');
        }
        joynrState = JoynrStates.SHUTTINGDOWN;

        const shutdownProvisioning = provisioning.shutdownSettings || {};
        settings = settings || {};
        if (settings.clearSubscriptionsEnabled || shutdownProvisioning.clearSubscriptionsEnabled) {
            const clearSubscriptionTimeoutMs =
                settings.clearSubscriptionsTimeoutMs || shutdownProvisioning.clearSubscriptionsTimeoutMs || 1000;
            subscriptionManager.terminateSubscriptions(clearSubscriptionTimeoutMs);
        }

        LongTimer.clearInterval(freshnessIntervalId);

        //longPollingCreatePromise.then(function() {
        //    return longPollingMessageReceiver.clear(channelId)
        //    .then(function() {
        //        // stop LongPolling
        //        longPollingMessageReceiver.stop();
        //    }).catch(function(error) {
        //        var errorString = "error clearing long poll channel: "
        //            + error;
        //        log.error(errorString);
        //        // stop LongPolling
        //        longPollingMessageReceiver.stop();
        //        throw new Error(errorString);
        //    });
        //});

        if (mqttMessagingSkeleton !== undefined) {
            mqttMessagingSkeleton.shutdown();
        }

        //if (channelMessagingSender !== undefined) {
        //    channelMessagingSender.shutdown();
        //}

        if (capabilitiesRegistrar !== undefined) {
            capabilitiesRegistrar.shutdown();
        }

        if (arbitrator !== undefined) {
            arbitrator.shutdown();
        }

        if (messageRouter !== undefined) {
            messageRouter.shutdown();
        }

        if (requestReplyManager !== undefined) {
            requestReplyManager.shutdown();
        }

        if (publicationManager !== undefined) {
            publicationManager.shutdown();
        }

        if (subscriptionManager !== undefined) {
            subscriptionManager.shutdown();
        }

        if (dispatcher !== undefined) {
            dispatcher.shutdown();
        }

        if (typeRegistry !== undefined) {
            typeRegistry.shutdown();
        }

        joynrState = JoynrStates.SHUTDOWN;
        log.debug("joynr shut down");
        return Promise.resolve();
    };

    // make every instance immutable
    return Object.freeze(this);
}

module.exports = InProcessRuntime;
