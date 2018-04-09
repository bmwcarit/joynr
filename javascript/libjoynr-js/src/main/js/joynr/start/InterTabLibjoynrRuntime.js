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
const CapabilitiesRegistrar = require("../capabilities/CapabilitiesRegistrar");
const ParticipantIdStorage = require("../capabilities/ParticipantIdStorage");
const RequestReplyManager = require("../dispatching/RequestReplyManager");
const PublicationManager = require("../dispatching/subscription/PublicationManager");
const SubscriptionManager = require("../dispatching/subscription/SubscriptionManager");
const Dispatcher = require("../dispatching/Dispatcher");
const JoynrException = require("../exceptions/JoynrException");
const PlatformSecurityManager = require("../security/PlatformSecurityManagerNode");
const WebMessagingStub = require("../messaging/webmessaging/WebMessagingStub");
const WebMessagingSkeleton = require("../messaging/webmessaging/WebMessagingSkeleton");
const BrowserMessagingStubFactory = require("../messaging/browser/BrowserMessagingStubFactory");
const BrowserMessagingSkeleton = require("../messaging/browser/BrowserMessagingSkeleton");
const BrowserMulticastAddressCalculator = require("../messaging/browser/BrowserMulticastAddressCalculator");
const MessagingSkeletonFactory = require("../messaging/MessagingSkeletonFactory");
const MessagingStubFactory = require("../messaging/MessagingStubFactory");
const MessageRouter = require("../messaging/routing/MessageRouter");
const MessageQueue = require("../messaging/routing/MessageQueue");
const BrowserAddress = require("../../generated/joynr/system/RoutingTypes/BrowserAddress");
const InProcessMessagingStubFactory = require("../messaging/inprocess/InProcessMessagingStubFactory");
const InProcessMessagingSkeleton = require("../messaging/inprocess/InProcessMessagingSkeleton");
const InProcessMessagingStub = require("../messaging/inprocess/InProcessMessagingStub");
const InProcessAddress = require("../messaging/inprocess/InProcessAddress");
const InProcessStub = require("../util/InProcessStub");
const InProcessSkeleton = require("../util/InProcessSkeleton");
const MessagingQos = require("../messaging/MessagingQos");
const DiscoveryQos = require("../proxy/DiscoveryQos");
const DiscoveryProxy = require("../../generated/joynr/system/DiscoveryProxy");
const RoutingProxy = require("../../generated/joynr/system/RoutingProxy");
const TypeRegistrySingleton = require("../types/TypeRegistrySingleton");
const DiscoveryScope = require("../../generated/joynr/types/DiscoveryScope");
const DiscoveryEntryWithMetaInfo = require("../../generated/joynr/types/DiscoveryEntryWithMetaInfo");
const Util = require("../util/UtilInternal");
const CapabilitiesUtil = require("../util/CapabilitiesUtil");
const WebWorkerMessagingAppender = require("../system/WebWorkerMessagingAppender");
const uuid = require("../../lib/uuid-annotated");
const loggingManager = require("../system/LoggingManager");
const defaultSettings = require("./settings/defaultSettings");
const defaultInterTabSettings = require("./settings/defaultInterTabSettings");
const defaultLibjoynrSettings = require("./settings/defaultLibjoynrSettings");
const LocalStorage = require("../../global/LocalStorageNode");
const JoynrStates = {
    SHUTDOWN: "shut down",
    STARTING: "starting",
    STARTED: "started",
    SHUTTINGDOWN: "shutting down"
};

const TWO_DAYS_IN_MS = 172800000;
const CC_WINDOWID = "ClusterController";

/**
 * The InterTabLibjoynrRuntime is the version of the libjoynr-js runtime that
 * communicates with a cluster controller over inter tab communication
 *
 * @name InterTabLibjoynrRuntime
 * @constructor
 *
 * @param provisioning
 */
function InterTabLibjoynrRuntime(provisioning) {
    let ccAddress;
    let initialRoutingTable;
    let untypedCapabilities;
    let typedCapabilities;
    let messagingSkeletonFactory;
    let messagingStubFactory;
    let messageRouter;
    let libjoynrMessagingSkeleton;
    let dispatcher;
    let typeRegistry;
    let requestReplyManager;
    let subscriptionManager;
    let publicationManager;
    let participantIdStorage;
    let arbitrator;
    let providerBuilder;
    let proxyBuilder;
    let capabilitiesRegistrar;
    let discovery;
    let messageQueueSettings;
    let webMessagingStub;
    let webMessagingSkeleton;
    let browserMessagingSkeleton;
    let messageRouterSkeleton;
    let messageRouterStub;
    let persistency;
    let libjoynrInterTabAddress;

    // this is required at load time of libjoynr
    typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

    /**
     * @name InterTabLibjoynrRuntime#typeRegistry
     * @type TypeRegistry
     */
    Object.defineProperty(this, "typeRegistry", {
        get() {
            return typeRegistry;
        },
        enumerable: true
    });

    /**
     * @name InterTabLibjoynrRuntime#registration
     * @type CapabilitiesRegistrar
     */
    Object.defineProperty(this, "registration", {
        get() {
            return capabilitiesRegistrar;
        },
        enumerable: true
    });

    /**
     * @name InterTabLibjoynrRuntime#providerBuilder
     * @type ProviderBuilder
     */
    Object.defineProperty(this, "providerBuilder", {
        get() {
            return providerBuilder;
        },
        enumerable: true
    });

    /**
     * @name InterTabLibjoynrRuntime#proxyBuilder
     * @type ProxyBuilder
     */
    Object.defineProperty(this, "proxyBuilder", {
        get() {
            return proxyBuilder;
        },
        enumerable: true
    });

    /**
     * @name InterTabLibjoynrRuntime#participantIdStorage
     * @type ParticipantIdStorage
     */
    Object.defineProperty(this, "participantIdStorage", {
        get() {
            return participantIdStorage;
        },
        enumerable: true
    });

    /**
     * @name InterTabLibjoynrRuntime#logging
     * @type LoggingManager
     */
    Object.defineProperty(this, "logging", {
        get() {
            return loggingManager;
        },
        enumerable: true
    });

    let log, relativeTtl;

    if (provisioning.logging && provisioning.logging.ttl) {
        relativeTtl = provisioning.logging.ttl;
    } else {
        relativeTtl = TWO_DAYS_IN_MS;
    }

    const loggingMessagingQos = new MessagingQos({
        ttl: relativeTtl
    });

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
     * @name InterTabLibjoynrRuntime#start
     * @function
     * @returns {Object} an A+ promise object, reporting when libjoynr startup is
     *          completed or has failed
     * @throws {Error}
     *             if libjoynr is not in SHUTDOWN state
     */
    this.start = function start() {
        let i, j;
        ccAddress = new BrowserAddress({
            windowId: CC_WINDOWID
        });

        if (joynrState !== JoynrStates.SHUTDOWN) {
            throw new Error("Cannot start libjoynr because it's currently \"" + joynrState + '"');
        }
        joynrState = JoynrStates.STARTING;

        if (!provisioning) {
            throw new Error("Constructor has been invoked without provisioning");
        }

        if (provisioning.logging) {
            loggingManager.configure(provisioning.logging);
        }

        log = loggingManager.getLogger("joynr.start.InterTabLibjoynrRuntime");

        const persistencyProvisioning = provisioning.persistency || {};
        persistency = new LocalStorage({
            clearPersistency: persistencyProvisioning.clearPersistency,
            location: persistencyProvisioning.location
        });

        if (Util.checkNullUndefined(provisioning.parentWindow)) {
            log.debug(
                'provisioning.parentWindow not set. Use default setting "' +
                    defaultInterTabSettings.parentWindow +
                    '" instead'
            );
        }

        if (Util.checkNullUndefined(provisioning.windowId)) {
            throw new Error("windowId not set in provisioning.windowId");
        }

        libjoynrInterTabAddress = new BrowserAddress({
            windowId: provisioning.windowId
        });
        initialRoutingTable = {};
        untypedCapabilities = provisioning.capabilities || [];
        const defaultCapabilities = defaultLibjoynrSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);

        typedCapabilities = [];
        for (i = 0; i < untypedCapabilities.length; i++) {
            const capability = new DiscoveryEntryWithMetaInfo(untypedCapabilities[i]);
            initialRoutingTable[capability.participantId] = ccAddress;
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

        messagingSkeletonFactory = new MessagingSkeletonFactory();

        const messagingStubFactories = {};
        /*jslint nomen: true */
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        messagingStubFactories[BrowserAddress._typeName] = new BrowserMessagingStubFactory({
            webMessagingStub
        });
        /*jslint nomen: false */
        messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        messageRouter = new MessageRouter({
            initialRoutingTable,
            persistency,
            typeRegistry,
            joynrInstanceId: provisioning.windowId,
            messagingSkeletonFactory,
            messagingStubFactory,
            messageQueue: new MessageQueue(messageQueueSettings),
            parentMessageRouterAddress: ccAddress,
            multicastAddressCalculator: new BrowserMulticastAddressCalculator({
                globalAddress: ccAddress
            }),
            incomingAddress: libjoynrInterTabAddress
        });
        browserMessagingSkeleton.registerListener(messageRouter.route);

        // link up clustercontroller messaging to dispatcher
        messageRouterSkeleton = new InProcessMessagingSkeleton();
        messageRouterStub = new InProcessMessagingStub(messageRouterSkeleton);

        // clustercontroller messaging handled by the messageRouter
        messageRouterSkeleton.registerListener(messageRouter.route);
        const ttlUpLiftMs =
            provisioning.messaging && provisioning.messaging.TTL_UPLIFT ? provisioning.messaging.TTL_UPLIFT : undefined;
        dispatcher = new Dispatcher(messageRouterStub, new PlatformSecurityManager(), ttlUpLiftMs);

        libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
        libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

        const messagingSkeletons = {};
        /*jslint nomen: true */
        messagingSkeletons[InProcessAddress._typeName] = libjoynrMessagingSkeleton;
        messagingSkeletons[BrowserAddress._typeName] = browserMessagingSkeleton;
        /*jslint nomen: false */
        messagingSkeletonFactory.setSkeletons(messagingSkeletons);

        requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
        subscriptionManager = new SubscriptionManager(dispatcher);
        publicationManager = new PublicationManager(dispatcher, persistency, "joynrInstanceId"); //TODO: set joynrInstanceId

        dispatcher.registerRequestReplyManager(requestReplyManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerMessageRouter(messageRouter);

        participantIdStorage = new ParticipantIdStorage(persistency, uuid);
        discovery = new InProcessStub();

        capabilitiesRegistrar = Object.freeze(
            new CapabilitiesRegistrar({
                discoveryStub: discovery,
                messageRouter,
                requestReplyManager,
                publicationManager,
                libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton),
                participantIdStorage
            })
        );

        arbitrator = new Arbitrator(discovery, typedCapabilities);

        providerBuilder = Object.freeze(new ProviderBuilder());

        proxyBuilder = Object.freeze(
            new ProxyBuilder(
                {
                    arbitrator,
                    typeRegistry,
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

        const getDiscoveryProxy = function getDiscoveryProxy(ttl) {
            return proxyBuilder
                .build(DiscoveryProxy, {
                    domain: "io.joynr",
                    messagingQos: {
                        ttl
                    },
                    discoveryQos: new DiscoveryQos({
                        discoveryScope: DiscoveryScope.LOCAL_ONLY
                    }),
                    staticArbitration: true
                })
                .catch(error => {
                    throw new Error("Failed to create discovery proxy: " + error);
                });
        };

        const TTL_30DAYS_IN_MS = 30 * 24 * 60 * 60 * 1000;
        discovery.setSkeleton(
            new InProcessSkeleton({
                lookup: function lookup(domains, interfaceName, discoveryQos) {
                    return getDiscoveryProxy(discoveryQos.discoveryTimeoutMs).then(newDiscoveryProxy => {
                        return newDiscoveryProxy
                            .lookup({
                                domains,
                                interfaceName,
                                discoveryQos
                            })
                            .then(opArgs => {
                                return opArgs.result;
                            });
                    });
                },
                add: function add(discoveryEntry) {
                    return getDiscoveryProxy(TTL_30DAYS_IN_MS).then(newDiscoveryProxy => {
                        return newDiscoveryProxy.add({
                            discoveryEntry
                        });
                    });
                },
                remove: function remove(participantId) {
                    return getDiscoveryProxy(TTL_30DAYS_IN_MS).then(newDiscoveryProxy => {
                        return newDiscoveryProxy.remove({
                            participantId
                        });
                    });
                }
            })
        );

        const internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

        const routingProxyPromise = proxyBuilder
            .build(RoutingProxy, {
                domain: "io.joynr",
                messagingQos: internalMessagingQos,
                discoveryQos: new DiscoveryQos({
                    discoveryScope: DiscoveryScope.LOCAL_ONLY
                }),
                staticArbitration: true
            })
            .catch(error => {
                throw new Error(
                    "Failed to create routing proxy: " +
                        error +
                        (error instanceof JoynrException ? " " + error.detailMessage : "")
                );
            })
            .then(newRoutingProxy => {
                messageRouter.setRoutingProxy(newRoutingProxy);
                return newRoutingProxy;
            });

        // when everything's ready we can trigger the app
        return routingProxyPromise
            .then(() => {
                joynrState = JoynrStates.STARTED;
                publicationManager.restore();
                log.debug("libjoynr initialized");
                return;
            })
            .catch(error => {
                log.error("error starting up joynr: " + error);
                throw error;
            });
    };

    /**
     * Shuts down libjoynr
     *
     * @name InterTabLibjoynrRuntime#shutdown
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

        if (webMessagingSkeleton !== undefined) {
            webMessagingSkeleton.shutdown();
        }

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

        return Promise.all([] /* TODO: insert promises here */).then(() => {
            joynrState = JoynrStates.SHUTDOWN;
            log.debug("joynr shut down");
            return;
        });
    };

    // make every instance immutable
    return Object.freeze(this);
}

module.exports = InterTabLibjoynrRuntime;
