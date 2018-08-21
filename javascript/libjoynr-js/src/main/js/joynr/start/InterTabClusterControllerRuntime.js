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
const BrowserMessagingStubFactory = require("../messaging/browser/BrowserMessagingStubFactory");
const BrowserMessagingSkeleton = require("../messaging/browser/BrowserMessagingSkeleton");
const BrowserAddress = require("../../generated/joynr/system/RoutingTypes/BrowserAddress");
const WebMessagingStub = require("../messaging/webmessaging/WebMessagingStub");
const WebMessagingSkeleton = require("../messaging/webmessaging/WebMessagingSkeleton");
const MessagingQos = require("../messaging/MessagingQos");
const DiscoveryQos = require("../proxy/DiscoveryQos");
const ProviderQos = require("../../generated/joynr/types/ProviderQos");
const ProviderScope = require("../../generated/joynr/types/ProviderScope");
const DiscoveryScope = require("../../generated/joynr/types/DiscoveryScope");
const DiscoveryProvider = require("../../generated/joynr/system/DiscoveryProvider");
const RoutingProvider = require("../../generated/joynr/system/RoutingProvider");
const TypeRegistrySingleton = require("../types/TypeRegistrySingleton");
const UtilInternal = require("../util/UtilInternal");
const uuid = require("uuid/v4");
const loggingManager = require("../system/LoggingManager");
const defaultInterTabSettings = require("./settings/defaultInterTabSettings");
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
 * The InterTabClusterControllerRuntime is the version of the libjoynr-js runtime that
 * hosts its own cluster controller
 *
 * @name InterTabClusterControllerRuntime
 * @constructor
 *
 * @param provisioning
 */
function InterTabClusterControllerRuntime(provisioning) {
    let initialRoutingTable;
    let untypedCapabilities;
    let typedCapabilities;
    let messagingSkeletonFactory;
    let messagingStubFactory;
    let webMessagingStub;
    let webMessagingSkeleton;
    let browserMessagingSkeleton;
    let messageRouter;
    let libjoynrMessagingSkeleton;
    let clusterControllerMessagingSkeleton;
    let mqttMessagingSkeleton;
    let clusterControllerMessagingStub;
    let dispatcher;
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
    let providerQos;
    let discoveryProvider;
    let routingProvider;
    let registerDiscoveryProviderPromise;
    let registerRoutingProviderPromise;
    let persistency;
    let freshnessIntervalId;

    // this is required at load time of libjoynr
    const typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

    /**
     * @name InterTabClusterControllerRuntime#typeRegistry
     * @type TypeRegistry
     */
    Object.defineProperty(this, "typeRegistry", {
        get() {
            return typeRegistry;
        },
        enumerable: true
    });

    /**
     * @name InterTabClusterControllerRuntime#registration
     * @type CapabilitiesRegistrar
     */
    Object.defineProperty(this, "registration", {
        get() {
            return capabilitiesRegistrar;
        },
        enumerable: true
    });

    /**
     * @name InterTabClusterControllerRuntime#participantIdStorage
     * @type ParticipantIdStorage
     */
    Object.defineProperty(this, "participantIdStorage", {
        get() {
            return participantIdStorage;
        },
        enumerable: true
    });

    /**
     * @name InterTabClusterControllerRuntime#providerBuilder
     * @type ProviderBuilder
     */
    Object.defineProperty(this, "providerBuilder", {
        get() {
            return providerBuilder;
        },
        enumerable: true
    });

    /**
     * @name InterTabClusterControllerRuntime#proxyBuilder
     * @type ProxyBuilder
     */
    Object.defineProperty(this, "proxyBuilder", {
        get() {
            return proxyBuilder;
        },
        enumerable: true
    });

    /**
     * @name InterTabClusterControllerRuntime#logging
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
     * @name InterTabClusterControllerRuntime#start
     * @function
     * @returns {Object} an A+ promise object, reporting when libjoynr startup is completed or failed
     * @throws {Error}
     *             if libjoynr is not in SHUTDOWN state
     */
    this.start = function start() {
        let i;

        if (joynrState !== JoynrStates.SHUTDOWN) {
            throw new Error(`Cannot start libjoynr because it's currently "${joynrState}"`);
        }
        joynrState = JoynrStates.STARTING;

        if (!provisioning) {
            throw new Error("Constructor has been invoked without provisioning");
        }

        if (provisioning.logging) {
            loggingManager.configure(provisioning.logging);
        }

        log = loggingManager.getLogger("joynr.start.InterTabClusterControllerRuntime");

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
        if (UtilInternal.checkNullUndefined(provisioning.parentWindow)) {
            log.debug(
                `provisioning.parentWindow not set. Use default setting "${
                    defaultInterTabSettings.parentWindow
                }" instead`
            );
        }

        initialRoutingTable = {};

        channelId = provisioning.channelId || persistency.getItem("joynr.channels.channelId.1") || `chjs_${uuid()}`;
        persistency.setItem("joynr.channels.channelId.1", channelId);

        clusterControllerSettings = defaultClusterControllerSettings({
            bounceProxyBaseUrl: provisioning.bounceProxyBaseUrl,
            brokerUri: provisioning.brokerUri
        });
        untypedCapabilities = provisioning.capabilities || [];
        const defaultCapabilities = clusterControllerSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);
        // allow use of _typeName once
        //typeRegistry.addType(new ChannelAddress()._typeName, ChannelAddress, false);
        typeRegistry.addType(new MqttAddress()._typeName, MqttAddress, false);
        typedCapabilities = [];
        for (i = 0; i < untypedCapabilities.length; i++) {
            const capability = new GlobalDiscoveryEntry(untypedCapabilities[i]);
            if (!capability.address) {
                throw new Error(`provisioned capability is missing address: ${JSON.stringify(capability)}`);
            }
            initialRoutingTable[capability.participantId] = Typing.augmentTypes(JSON.parse(capability.address));
            typedCapabilities.push(capability);
        }

        messageQueueSettings = {};
        if (provisioning.messaging !== undefined && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
            messageQueueSettings.maxQueueSizeInKBytes = provisioning.messaging.maxQueueSizeInKBytes;
        }

        webMessagingStub = new WebMessagingStub({
            // parent window variable for communication
            window: provisioning.parentWindow || defaultInterTabSettings.parentWindow,
            // target origin of the parent window
            origin: provisioning.parentOrigin || defaultInterTabSettings.parentOrigin
        });

        webMessagingSkeleton = new WebMessagingSkeleton({
            window: provisioning.window || defaultInterTabSettings.window
        });

        browserMessagingSkeleton = new BrowserMessagingSkeleton({
            webMessagingSkeleton
        });

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
        messagingStubFactories[BrowserAddress._typeName] = new BrowserMessagingStubFactory({
            webMessagingStub
        });
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
            typeRegistry,
            joynrInstanceId: channelId,
            messagingSkeletonFactory,
            messagingStubFactory,
            multicastAddressCalculator: new MqttMulticastAddressCalculator({
                globalAddress: globalClusterControllerAddress
            }),
            messageQueue: new MessageQueue(messageQueueSettings)
        });
        messageRouter.setReplyToAddress(serializedGlobalClusterControllerAddress);
        browserMessagingSkeleton.registerListener(messageRouter.route);

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

        messagingSkeletonFactory.setSkeletons({
            InProcessAddress: libjoynrMessagingSkeleton,
            BrowserAddress: browserMessagingSkeleton,
            //ChannelAddress : clusterControllerChannelMessagingSkeleton,
            MqttAddress: mqttMessagingSkeleton
        });

        requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
        subscriptionManager = new SubscriptionManager(dispatcher);
        publicationManager = new PublicationManager(dispatcher, persistency, channelId);

        dispatcher.registerRequestReplyManager(requestReplyManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerMessageRouter(messageRouter);

        localCapabilitiesStore = new CapabilitiesStore();
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
                log.error(`error sending freshness update: ${error}`);
            });
            return null;
        }, period);

        providerBuilder = Object.freeze(new ProviderBuilder());

        providerQos = new ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: ProviderScope.LOCAL
        });

        discoveryProvider = providerBuilder.build(DiscoveryProvider, {
            add(opArgs) {
                /*FIXME remove discoveryEntry transformation,
                                                     * once the deserialization of enums works correctly
                                                     */
                if (typeof opArgs.discoveryEntry.qos.scope === "string") {
                    opArgs.discoveryEntry.qos.scope = ProviderScope[opArgs.discoveryEntry.qos.scope];
                }
                return capabilityDiscovery.add(opArgs.discoveryEntry);
            },
            lookup(opArgs) {
                /*FIXME remove discoveryQos transformation,
                                                     * once the deserialization of enums works correctly
                                                     */
                if (typeof opArgs.discoveryQos.discoveryScope === "string") {
                    opArgs.discoveryQos.discoveryScope = DiscoveryScope[opArgs.discoveryQos.discoveryScope];
                }
                return capabilityDiscovery
                    .lookup(opArgs.domains, opArgs.interfaceName, opArgs.discoveryQos)
                    .then(caps => {
                        return {
                            result: caps
                        };
                    });
            },
            remove(opArgs) {
                return capabilityDiscovery.remove(opArgs.participantId);
            }
        });
        registerDiscoveryProviderPromise = capabilitiesRegistrar.registerProvider(
            "io.joynr",
            discoveryProvider,
            providerQos
        );

        routingProvider = providerBuilder.build(RoutingProvider, {
            globalAddress: {
                get() {
                    return Promise.resolve(serializedGlobalClusterControllerAddress);
                }
            },
            replyToAddress: {
                get() {
                    return Promise.resolve(serializedGlobalClusterControllerAddress);
                }
            },
            addNextHop(opArgs) {
                let address;
                if (opArgs.channelAddress !== undefined) {
                    address = opArgs.channelAddress;
                } else if (opArgs.browserAddress !== undefined) {
                    address = opArgs.browserAddress;
                } else if (opArgs.webSocketAddress !== undefined) {
                    address = opArgs.webSocketAddress;
                }
                if (address !== undefined) {
                    return messageRouter.addNextHop(opArgs.participantId, address, opArgs.isGloballyVisible);
                }
                return Promise.reject(
                    new Error(
                        `${"RoutingProvider.addNextHop failed, because address " +
                            "could not be found in the operation arguments "}${JSON.stringify(opArgs)}`
                    )
                );
            },
            resolveNextHop(opArgs) {
                return messageRouter
                    .resolveNextHop(opArgs.participantId)
                    .then(address => {
                        const isResolved = address !== undefined;
                        return {
                            resolved: isResolved
                        };
                    })
                    .catch(() => {
                        return false;
                    });
            },
            removeNextHop(opArgs) {
                return messageRouter.removeNextHop(opArgs.participantId);
            },
            addMulticastReceiver: messageRouter.addMulticastReceiver,
            removeMulticastReceiver: messageRouter.removeMulticastReceiver
        });
        registerRoutingProviderPromise = capabilitiesRegistrar.registerProvider(
            "io.joynr",
            routingProvider,
            providerQos
        );

        // when everything's ready we can resolve the promise
        return Promise.all([registerDiscoveryProviderPromise, registerRoutingProviderPromise])
            .then(() => {
                joynrState = JoynrStates.STARTED;
                publicationManager.restore();
                log.debug("joynr cluster controller initialized");
                return;
            })
            .catch(error => {
                log.error(`error starting up joynr: ${error}`);
                throw error;
            });
    };

    /**
     * Shuts down libjoynr
     *
     * @name InterTabClusterControllerRuntime#shutdown
     * @function
     * @throws {Error}
     *             if libjoynr is not in the STARTED state
     */
    this.shutdown = function shutdown(settings) {
        if (joynrState !== JoynrStates.STARTED) {
            throw new Error(`Cannot shutdown libjoynr because it's currently "${joynrState}"`);
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
        //    longPollingMessageReceiver.clear(channelId)
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

        log.debug("joynr cluster controller shut down");
        joynrState = JoynrStates.SHUTDOWN;
        return Promise.resolve();
    };

    // make every instance immutable
    return Object.freeze(this);
}

module.exports = InterTabClusterControllerRuntime;
