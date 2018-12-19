/*jslint es5: true, node: true */

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
var Promise = require("../../global/Promise");
var Arbitrator = require("../capabilities/arbitration/Arbitrator");
var ProviderBuilder = require("../provider/ProviderBuilder");
var ProxyBuilder = require("../proxy/ProxyBuilder");
var GlobalDiscoveryEntry = require("../../joynr/types/GlobalDiscoveryEntry");
var CapabilitiesRegistrar = require("../capabilities/CapabilitiesRegistrar");
var ParticipantIdStorage = require("../capabilities/ParticipantIdStorage");
var CapabilityDiscovery = require("../capabilities/discovery/CapabilityDiscovery");
var CapabilitiesStore = require("../capabilities/CapabilitiesStore");
var RequestReplyManager = require("../dispatching/RequestReplyManager");
var PublicationManager = require("../dispatching/subscription/PublicationManager");
var SubscriptionManager = require("../dispatching/subscription/SubscriptionManager");
var Dispatcher = require("../dispatching/Dispatcher");
var PlatformSecurityManager = require("../security/PlatformSecurityManagerNode");
var ChannelMessagingSender = require("../messaging/channel/ChannelMessagingSender");
var ChannelMessagingStubFactory = require("../messaging/channel/ChannelMessagingStubFactory");
var ChannelMessagingSkeleton = require("../messaging/channel/ChannelMessagingSkeleton");
var ChannelAddress = require("../system/RoutingTypes/ChannelAddress");
var MqttMessagingStubFactory = require("../messaging/mqtt/MqttMessagingStubFactory");
var MqttMessagingSkeleton = require("../messaging/mqtt/MqttMessagingSkeleton");
var MqttAddress = require("../system/RoutingTypes/MqttAddress");
var SharedMqttClient = require("../messaging/mqtt/SharedMqttClient");
var MqttMulticastAddressCalculator = require("../messaging/mqtt/MqttMulticastAddressCalculator");
var MessagingSkeletonFactory = require("../messaging/MessagingSkeletonFactory");
var MessagingStubFactory = require("../messaging/MessagingStubFactory");
var MessageRouter = require("../messaging/routing/MessageRouter");
var MessageQueue = require("../messaging/routing/MessageQueue");
var CommunicationModule = require("../messaging/CommunicationModule");
var InProcessSkeleton = require("../util/InProcessSkeleton");
var InProcessStub = require("../util/InProcessStub");
var InProcessMessagingStubFactory = require("../messaging/inprocess/InProcessMessagingStubFactory");
var InProcessMessagingSkeleton = require("../messaging/inprocess/InProcessMessagingSkeleton");
var InProcessMessagingStub = require("../messaging/inprocess/InProcessMessagingStub");
var InProcessAddress = require("../messaging/inprocess/InProcessAddress");
var LongPollingChannelMessageReceiver = require("../messaging/channel/LongPollingChannelMessageReceiver");
var MessagingQos = require("../messaging/MessagingQos");
var DiscoveryQos = require("../proxy/DiscoveryQos");
var DiscoveryScope = require("../../joynr/types/DiscoveryScope");
var TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
var Util = require("../util/UtilInternal");
var CapabilitiesUtil = require("../util/CapabilitiesUtil");
var WebWorkerMessagingAppender = require("../system/WebWorkerMessagingAppender");
var loggingManager = require("../system/LoggingManager");
var uuid = require("../../lib/uuid-annotated");
var defaultSettings = require("./settings/defaultSettings");
var defaultLibjoynrSettings = require("./settings/defaultLibjoynrSettings");
var defaultClusterControllerSettings = require("./settings/defaultClusterControllerSettings");
var Typing = require("../util/Typing");
var LongTimer = require("../util/LongTimer");
var LocalStorage = require("../../global/LocalStorageNode");
var JoynrStates = {
    SHUTDOWN: "shut down",
    STARTING: "starting",
    STARTED: "started",
    SHUTTINGDOWN: "shutting down"
};

var TWO_DAYS_IN_MS = 172800000;
var clusterControllerSettings;

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
    var initialRoutingTable;
    var untypedCapabilities;
    var typedCapabilities;
    var channelMessagingSender;
    var channelMessagingStubFactory;
    var messagingSkeletonFactory;
    var messagingStubFactory;
    var messageRouter;
    var communicationModule;
    var longPollingMessageReceiver;
    var libjoynrMessagingSkeleton;
    var clusterControllerMessagingSkeleton;
    var mqttMessagingSkeleton;
    var clusterControllerChannelMessagingSkeleton;
    var clusterControllerMessagingStub, dispatcher;
    var typeRegistry;
    var requestReplyManager;
    var subscriptionManager;
    var publicationManager;
    var participantIdStorage;
    var capabilityDiscovery;
    var arbitrator;
    var channelId;
    var bounceProxyBaseUrl;
    var providerBuilder;
    var proxyBuilder;
    var capabilitiesRegistrar;
    var localCapabilitiesStore;
    var globalCapabilitiesCache;
    var discoveryStub;
    var messageQueueSettings;
    var persistency;
    var longPollingCreatePromise;
    var freshnessIntervalId;

    // this is required at load time of libjoynr
    typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

    /**
     * @name InProcessRuntime#typeRegistry
     * @type TypeRegistry
     */
    Object.defineProperty(this, "typeRegistry", {
        get: function() {
            return typeRegistry;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#registration
     * @type CapabilitiesRegistrar
     */
    Object.defineProperty(this, "registration", {
        get: function() {
            return capabilitiesRegistrar;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#participantIdStorage
     * @type ParticipantIdStorage
     */
    Object.defineProperty(this, "participantIdStorage", {
        get: function() {
            return participantIdStorage;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#providerBuilder
     * @type ProviderBuilder
     */
    Object.defineProperty(this, "providerBuilder", {
        get: function() {
            return providerBuilder;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#proxyBuilder
     * @type ProxyBuilder
     */
    Object.defineProperty(this, "proxyBuilder", {
        get: function() {
            return proxyBuilder;
        },
        enumerable: true
    });

    /**
     * @name InProcessRuntime#logger
     * @type LoggingManager
     */
    Object.defineProperty(this, "logging", {
        get: function() {
            return loggingManager;
        },
        enumerable: true
    });

    var log, relativeTtl;

    if (provisioning.logging && provisioning.logging.ttl) {
        relativeTtl = provisioning.logging.ttl;
    } else {
        relativeTtl = TWO_DAYS_IN_MS;
    }

    var loggingMessagingQos = new MessagingQos({
        ttl: relativeTtl
    });

    var joynrState = JoynrStates.SHUTDOWN;

    if (provisioning.capabilities && provisioning.capabilities.discoveryQos) {
        var discoveryQos = provisioning.capabilities.discoveryQos;

        if (discoveryQos.discoveryExpiryIntervalMs) {
            CapabilitiesRegistrar.setDefaultExpiryIntervalMs(discoveryQos.discoveryExpiryIntervalMs);
        }

        var discoveryQosSettings = {};

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
        var i, j;

        if (joynrState !== JoynrStates.SHUTDOWN) {
            throw new Error("Cannot start libjoynr because it's currently \"" + joynrState + '"');
        }
        joynrState = JoynrStates.STARTING;

        if (!provisioning) {
            throw new Error("Constructor has not been invoked with provisioned data");
        }

        // initialize Logger with external logging configuration or default
        // values
        var logLevel, logLayout, appenderNames, appenderName;

        log = loggingManager.getLogger("joynr.start.InProcessRuntime");

        var persistencyProvisioning = provisioning.persistency || {};
        persistency = new LocalStorage({
            clearPersistency: persistencyProvisioning.clearPersistency,
            location: persistencyProvisioning.location
        });

        if (Util.checkNullUndefined(provisioning.bounceProxyUrl)) {
            throw new Error("bounce proxy URL not set in provisioning.bounceProxyUrl");
        }
        if (Util.checkNullUndefined(provisioning.bounceProxyBaseUrl)) {
            throw new Error("bounce proxy base URL not set in provisioning.bounceProxyBaseUrl");
        }
        if (Util.checkNullUndefined(provisioning.brokerUri)) {
            throw new Error("broker URI not set in provisioning.brokerUri");
        }

        initialRoutingTable = {};
        bounceProxyBaseUrl = provisioning.bounceProxyBaseUrl;

        channelId = provisioning.channelId || persistency.getItem("joynr.channels.channelId.1") || "chjs_" + uuid();
        persistency.setItem("joynr.channels.channelId.1", channelId);

        untypedCapabilities = provisioning.capabilities || [];
        clusterControllerSettings = defaultClusterControllerSettings({
            bounceProxyBaseUrl: provisioning.bounceProxyBaseUrl,
            brokerUri: provisioning.brokerUri
        });
        var defaultClusterControllerCapabilities = clusterControllerSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultClusterControllerCapabilities);
        // allow use of _typeName once
        /*jslint nomen: true */
        typeRegistry.addType(new ChannelAddress()._typeName, ChannelAddress, false);
        typeRegistry.addType(new MqttAddress()._typeName, MqttAddress, false);
        /*jslint nomen: false */
        typedCapabilities = [];
        var errorMessage;
        for (i = 0; i < untypedCapabilities.length; i++) {
            var capability = new GlobalDiscoveryEntry(untypedCapabilities[i]);
            if (!capability.address) {
                throw new Error("provisioned capability is missing address: " + JSON.stringify(capability));
            }
            initialRoutingTable[capability.participantId] = Typing.augmentTypes(
                JSON.parse(capability.address),
                typeRegistry
            );
            typedCapabilities.push(capability);
        }

        communicationModule = new CommunicationModule();

        //channelMessagingSender = new ChannelMessagingSender({
        //    communicationModule : communicationModule,
        //    channelQos : provisioning.channelQos
        //});

        messageQueueSettings = {};
        if (provisioning.messaging !== undefined && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
            messageQueueSettings.maxQueueSizeInKBytes = provisioning.messaging.maxQueueSizeInKBytes;
        }

        //channelMessagingStubFactory = new ChannelMessagingStubFactory({
        //    myChannelId : channelId,
        //    channelMessagingSender : channelMessagingSender
        //});

        var globalClusterControllerAddress = new MqttAddress({
            brokerUri: provisioning.brokerUri,
            topic: channelId
        });
        var serializedGlobalClusterControllerAddress = JSON.stringify(globalClusterControllerAddress);

        var mqttClient = new SharedMqttClient({
            address: globalClusterControllerAddress,
            provisioning: provisioning.mqtt || {}
        });

        messagingSkeletonFactory = new MessagingSkeletonFactory();

        var messagingStubFactories = {};
        /*jslint nomen: true */
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        //messagingStubFactories[ChannelAddress._typeName] = channelMessagingStubFactory;
        messagingStubFactories[MqttAddress._typeName] = new MqttMessagingStubFactory({
            client: mqttClient,
            address: globalClusterControllerAddress
        });
        /*jslint nomen: false */

        messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories: messagingStubFactories
        });

        messageRouter = new MessageRouter({
            initialRoutingTable: initialRoutingTable,
            persistency: persistency,
            joynrInstanceId: channelId,
            typeRegistry: typeRegistry,
            messagingStubFactory: messagingStubFactory,
            messagingSkeletonFactory: messagingSkeletonFactory,
            multicastAddressCalculator: new MqttMulticastAddressCalculator({
                globalAddress: globalClusterControllerAddress
            }),
            messageQueue: new MessageQueue(messageQueueSettings)
        });
        messageRouter.setReplyToAddress(serializedGlobalClusterControllerAddress);

        //longPollingMessageReceiver = new LongPollingChannelMessageReceiver({
        //    persistency : persistency,
        //    bounceProxyUrl : bounceProxyBaseUrl + "/bounceproxy/",
        //    communicationModule : communicationModule,
        //    channelQos: provisioning.channelQos
        //});

        // link up clustercontroller messaging to channel
        clusterControllerChannelMessagingSkeleton = new ChannelMessagingSkeleton({
            messageRouter: messageRouter
        });

        mqttMessagingSkeleton = new MqttMessagingSkeleton({
            address: globalClusterControllerAddress,
            client: mqttClient,
            messageRouter: messageRouter
        });

        //longPollingCreatePromise = longPollingMessageReceiver.create(channelId).then(
        //        function(channelUrl) {
        //            var channelAddress = new ChannelAddress({
        //                channelId: channelId,
        //                messagingEndpointUrl: channelUrl
        //            });

        //            mqttClient.onConnected().then(function() {
        //                capabilityDiscovery.globalAddressReady(globalClusterControllerAddress);
        //                channelMessagingStubFactory.globalAddressReady(channelAddress);
        //                return null;
        //            });

        //            longPollingMessageReceiver
        //                    .start(clusterControllerChannelMessagingSkeleton.receiveMessage);
        //            channelMessagingSender.start();
        //            return null;
        //        });
        mqttClient.onConnected().then(function() {
            capabilityDiscovery.globalAddressReady(globalClusterControllerAddress);
            //channelMessagingStubFactory.globalAddressReady(channelAddress);
            return null;
        });

        // link up clustercontroller messaging to dispatcher
        clusterControllerMessagingSkeleton = new InProcessMessagingSkeleton();
        clusterControllerMessagingStub = new InProcessMessagingStub(clusterControllerMessagingSkeleton);

        // clustercontroller messaging handled by the messageRouter
        clusterControllerMessagingSkeleton.registerListener(messageRouter.route);

        var ttlUpLiftMs =
            provisioning.messaging && provisioning.messaging.TTL_UPLIFT ? provisioning.messaging.TTL_UPLIFT : undefined;
        dispatcher = new Dispatcher(clusterControllerMessagingStub, new PlatformSecurityManager(), ttlUpLiftMs);

        libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
        libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

        var messagingSkeletons = {};
        /*jslint nomen: true */
        messagingSkeletons[InProcessAddress._typeName] = libjoynrMessagingSkeleton;
        messagingSkeletons[MqttAddress._typeName] = mqttMessagingSkeleton;
        /*jslint nomen: false */
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
                discoveryStub: discoveryStub,
                messageRouter: messageRouter,
                requestReplyManager: requestReplyManager,
                publicationManager: publicationManager,
                libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton),
                participantIdStorage: participantIdStorage
            })
        );

        arbitrator = new Arbitrator(discoveryStub);

        proxyBuilder = Object.freeze(
            new ProxyBuilder(
                {
                    arbitrator: arbitrator,
                    requestReplyManager: requestReplyManager,
                    subscriptionManager: subscriptionManager,
                    publicationManager: publicationManager
                },
                {
                    messageRouter: messageRouter,
                    libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton)
                }
            )
        );

        var internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

        var defaultProxyBuildSettings = {
            domain: "io.joynr",
            messagingQos: internalMessagingQos,
            discoveryQos: new DiscoveryQos({
                discoveryScope: DiscoveryScope.GLOBAL_ONLY,
                cacheMaxAgeMs: Util.getMaxLongValue()
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

        var period = provisioning.capabilitiesFreshnessUpdateIntervalMs || 3600000; // default: 1 hour
        freshnessIntervalId = LongTimer.setInterval(function() {
            capabilityDiscovery.touch(channelId, period).catch(function(error) {
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

        var shutdownProvisioning = provisioning.shutdownSettings || {};
        settings = settings || {};
        if (settings.clearSubscriptionsEnabled || shutdownProvisioning.clearSubscriptionsEnabled) {
            var clearSubscriptionTimeoutMs =
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

    /**
     *  Sends subscriptionStop messages for all active subscriptions.
     *
     *  @param timeout {number} optional timeout defaulting to 0 = no timeout
     *  @returns {Promise}
     *  - resolved after all SubscriptionStop messages are sent.
     *  - rejected in case of any issues or timeout occurs.
     */
    this.terminateAllSubscriptions = function(timeout = 0) {
        return subscriptionManager.terminateSubscriptions(timeout);
    };

    // make every instance immutable
    return Object.freeze(this);
}

module.exports = InProcessRuntime;
