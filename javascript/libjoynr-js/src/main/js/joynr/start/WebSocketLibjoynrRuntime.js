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
const SharedWebSocket = require("../messaging/websocket/SharedWebSocket");
const WebSocketMessagingSkeleton = require("../messaging/websocket/WebSocketMessagingSkeleton");
const WebSocketMessagingStubFactory = require("../messaging/websocket/WebSocketMessagingStubFactory");
const WebSocketMulticastAddressCalculator = require("../messaging/websocket/WebSocketMulticastAddressCalculator");
const MessagingSkeletonFactory = require("../messaging/MessagingSkeletonFactory");
const MessagingStubFactory = require("../messaging/MessagingStubFactory");
const MessageRouter = require("../messaging/routing/MessageRouter");
const MessageQueue = require("../messaging/routing/MessageQueue");
const WebSocketAddress = require("../../generated/joynr/system/RoutingTypes/WebSocketAddress");
const WebSocketClientAddress = require("../../generated/joynr/system/RoutingTypes/WebSocketClientAddress");
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
const UtilInternal = require("../util/UtilInternal");
const uuid = require("uuid/v4");
const loggingManager = require("../system/LoggingManager");
const defaultWebSocketSettings = require("./settings/defaultWebSocketSettings");
const defaultLibjoynrSettings = require("./settings/defaultLibjoynrSettings");
const LocalStorage = require("../../global/LocalStorageNode");
const MemoryStorage = require("../../global/MemoryStorage");
const JoynrMessage = require("../../joynr/messaging/JoynrMessage");
const JoynrStates = {
    SHUTDOWN: "shut down",
    STARTING: "starting",
    STARTED: "started",
    SHUTTINGDOWN: "shutting down"
};

/**
 * The WebSocketLibjoynrRuntime is the version of the libjoynr-js runtime that communicates with
 * a cluster controller via a WebSocket. The cluster controller is the WebSocket Server, and the
 * libjoynr connects to it with a single websocket for all communication.
 *
 * @name WebSocketLibjoynrRuntime
 * @constructor
 * @param {Object} provisioning
 */
function WebSocketLibjoynrRuntime(provisioning) {
    let log;
    let initialRoutingTable;
    let untypedCapabilities;
    let typedCapabilities;
    let messagingSkeletonFactory;
    let messagingStubFactory;
    let messageRouter;
    let libjoynrMessagingSkeleton;
    let dispatcher;
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
    let sharedWebSocket;
    let webSocketMessagingSkeleton;
    let messageRouterSkeleton;
    let messageRouterStub;
    let persistency;
    let localAddress;
    const keychain = provisioning.keychain;
    /*eslint-disable prefer-const*/
    let internalShutdown;
    /*eslint-enable prefer-const*/
    let bufferedOwnerId;

    // this is required at load time of libjoynr
    const typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

    /**
     * @name WebSocketLibjoynrRuntime#typeRegistry
     * @type TypeRegistry
     */
    Object.defineProperty(this, "typeRegistry", {
        get() {
            return typeRegistry;
        },
        enumerable: true
    });

    /**
     * @name WebSocketLibjoynrRuntime#registration
     * @type CapabilitiesRegistrar
     */
    Object.defineProperty(this, "registration", {
        get() {
            return capabilitiesRegistrar;
        },
        enumerable: true
    });

    /**
     * @name WebSocketLibjoynrRuntime#providerBuilder
     * @type ProviderBuilder
     */
    Object.defineProperty(this, "providerBuilder", {
        get() {
            return providerBuilder;
        },
        enumerable: true
    });

    /**
     * @name WebSocketLibjoynrRuntime#proxyBuilder
     * @type ProxyBuilder
     */
    Object.defineProperty(this, "proxyBuilder", {
        get() {
            return proxyBuilder;
        },
        enumerable: true
    });

    /**
     * @name WebSocketLibjoynrRuntime#participantIdStorage
     * @type ParticipantIdStorage
     */
    Object.defineProperty(this, "participantIdStorage", {
        get() {
            return participantIdStorage;
        },
        enumerable: true
    });

    /**
     * @name WebSocketLibjoynrRuntime#logging
     * @type LoggingManager
     */
    Object.defineProperty(this, "logging", {
        get() {
            return loggingManager;
        },
        enumerable: true
    });

    if (UtilInternal.checkNullUndefined(provisioning.ccAddress)) {
        throw new Error("ccAddress not set in provisioning.ccAddress");
    }

    const ccAddress = new WebSocketAddress({
        protocol: provisioning.ccAddress.protocol || defaultWebSocketSettings.protocol,
        host: provisioning.ccAddress.host,
        port: provisioning.ccAddress.port,
        path: provisioning.ccAddress.path || defaultWebSocketSettings.path
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

    function signingCallback() {
        return bufferedOwnerId;
    }

    if (keychain) {
        if (UtilInternal.checkNullUndefined(keychain.tlsCert)) {
            throw new Error("tlsCert not set in keychain.tlsCert");
        }
        if (UtilInternal.checkNullUndefined(keychain.tlsKey)) {
            throw new Error("tlsKey not set in keychain.tlsKey");
        }
        if (UtilInternal.checkNullUndefined(keychain.tlsCa)) {
            throw new Error("tlsCa not set in keychain.tlsCa");
        }
        if (UtilInternal.checkNullUndefined(keychain.ownerId)) {
            throw new Error("ownerId not set in keychain.ownerId");
        }

        bufferedOwnerId = Buffer.from(keychain.ownerId);
        JoynrMessage.setSigningCallback(signingCallback);
    }

    /**
     * Starts up the libjoynr instance
     *
     * @name WebSocketLibjoynrRuntime#start
     * @function
     * @returns {Object} an A+ promise object, reporting when libjoynr startup is
     *          completed or has failed
     * @throws {Error}
     *             if libjoynr is not in SHUTDOWN state
     */
    this.start = function start() {
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

        log = loggingManager.getLogger("joynr.start.WebSocketLibjoynrRuntime");

        const persistencyProvisioning = UtilInternal.extend(
            {},
            defaultLibjoynrSettings.persistencySettings,
            provisioning.persistency
        );

        let persistencyPromise;
        if (
            persistencyProvisioning.routingTable ||
            persistencyProvisioning.capabilities ||
            persistencyProvisioning.publications
        ) {
            persistency = new LocalStorage({
                clearPersistency: persistencyProvisioning.clearPersistency,
                location: persistencyProvisioning.location
            });
            persistencyPromise = persistency.init();
        } else {
            persistencyPromise = Promise.resolve();
        }

        const routingTablePersistency = persistencyProvisioning.routingTable ? persistency : undefined;
        const capabilitiesPersistency = persistencyProvisioning.capabilities ? persistency : new MemoryStorage();
        const publicationsPersistency = persistencyProvisioning.publications ? persistency : undefined;

        initialRoutingTable = {};
        untypedCapabilities = provisioning.capabilities || [];
        const defaultCapabilities = defaultLibjoynrSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);

        typedCapabilities = [];
        for (let i = 0; i < untypedCapabilities.length; i++) {
            const capability = new DiscoveryEntryWithMetaInfo(untypedCapabilities[i]);
            initialRoutingTable[capability.participantId] = ccAddress;
            typedCapabilities.push(capability);
        }

        messageQueueSettings = {};
        if (provisioning.messaging !== undefined && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
            messageQueueSettings.maxQueueSizeInKBytes = provisioning.messaging.maxQueueSizeInKBytes;
        }

        localAddress = new WebSocketClientAddress({
            id: uuid()
        });

        sharedWebSocket = new SharedWebSocket({
            remoteAddress: ccAddress,
            localAddress,
            provisioning: provisioning.websocket || {},
            keychain
        });

        webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
            sharedWebSocket,
            mainTransport: true
        });

        messagingSkeletonFactory = new MessagingSkeletonFactory();

        const messagingStubFactories = {};
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        messagingStubFactories[WebSocketAddress._typeName] = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket
        });

        messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        messageRouter = new MessageRouter({
            initialRoutingTable,
            persistency: routingTablePersistency,
            joynrInstanceId: uuid(),
            messagingSkeletonFactory,
            messagingStubFactory,
            messageQueue: new MessageQueue(messageQueueSettings),
            multicastAddressCalculator: new WebSocketMulticastAddressCalculator({
                globalAddress: ccAddress
            }),
            parentMessageRouterAddress: ccAddress,
            incomingAddress: localAddress
        });

        webSocketMessagingSkeleton.registerListener(messageRouter.route);

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
        messagingSkeletons[InProcessAddress._typeName] = libjoynrMessagingSkeleton;
        messagingSkeletons[WebSocketAddress._typeName] = webSocketMessagingSkeleton;
        messagingSkeletonFactory.setSkeletons(messagingSkeletons);

        requestReplyManager = new RequestReplyManager(dispatcher);
        subscriptionManager = new SubscriptionManager(dispatcher);
        publicationManager = new PublicationManager(dispatcher, publicationsPersistency, "joynrInstanceId"); //TODO: create joynrInstanceId

        dispatcher.registerRequestReplyManager(requestReplyManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerMessageRouter(messageRouter);

        participantIdStorage = new ParticipantIdStorage(capabilitiesPersistency, uuid);
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

        /*
         * if no internalMessagingQos is provided, extend the default ttl by 10 seconds in order
         * to allow the cluster controller to handle timeout for global discovery requests and
         * send back the response to discoveryProxy
         */
        if (provisioning.internalMessagingQos === undefined || provisioning.internalMessagingQos === null) {
            provisioning.internalMessagingQos = {};
            provisioning.internalMessagingQos.ttl = MessagingQos.DEFAULT_TTL + 10000;
        }

        const internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

        function buildDiscoveryProxyOnSuccess(newDiscoveryProxy) {
            discovery.setSkeleton(
                new InProcessSkeleton({
                    lookup: function lookup(domains, interfaceName, discoveryQos) {
                        return newDiscoveryProxy
                            .lookup({
                                domains,
                                interfaceName,
                                discoveryQos
                            })
                            .then(opArgs => {
                                return opArgs.result;
                            });
                    },
                    add: function add(discoveryEntry, awaitGlobalRegistration) {
                        return newDiscoveryProxy.add({
                            discoveryEntry,
                            awaitGlobalRegistration
                        });
                    },
                    remove: function remove(participantId) {
                        return newDiscoveryProxy.remove({
                            participantId
                        });
                    }
                })
            );
            return;
        }

        function buildDiscoveryProxyOnError(error) {
            throw new Error(`Failed to create discovery proxy: ${error}`);
        }

        function buildRoutingProxyOnError(error) {
            throw new Error(
                `Failed to create routing proxy: ${error}${
                    error instanceof JoynrException ? ` ${error.detailMessage}` : ""
                }`
            );
        }

        function buildRoutingProxyOnSuccess(newRoutingProxy) {
            return messageRouter.setRoutingProxy(newRoutingProxy);
        }

        const discoveryProxyPromise = proxyBuilder
            .build(DiscoveryProxy, {
                domain: "io.joynr",
                messagingQos: internalMessagingQos,
                discoveryQos: new DiscoveryQos({
                    discoveryScope: DiscoveryScope.LOCAL_ONLY
                }),
                staticArbitration: true
            })
            .then(buildDiscoveryProxyOnSuccess)
            .catch(buildDiscoveryProxyOnError);

        const routingProxyPromise = proxyBuilder
            .build(RoutingProxy, {
                domain: "io.joynr",
                messagingQos: internalMessagingQos,
                discoveryQos: new DiscoveryQos({
                    discoveryScope: DiscoveryScope.LOCAL_ONLY
                }),
                staticArbitration: true
            })
            .then(buildRoutingProxyOnSuccess)
            .catch(buildRoutingProxyOnError);

        function startOnSuccess() {
            joynrState = JoynrStates.STARTED;
            publicationManager.restore();
            log.debug("joynr web socket initialized");
            return;
        }

        function startOnFailure(error) {
            log.error(`error starting up joynr: ${error}`);

            internalShutdown();

            throw error;
        }

        // when everything's ready we can trigger the app
        return Promise.all([discoveryProxyPromise, routingProxyPromise, persistencyPromise])
            .then(startOnSuccess)
            .catch(startOnFailure);
    };

    /**
     * Shuts down libjoynr
     *
     * @name WebSocketLibjoynrRuntime#shutdown
     * @function
     * @throws {Error}
     *             if libjoynr is not in the STARTED state
     */
    internalShutdown = function shutdown(settings) {
        if (joynrState !== JoynrStates.STARTED && joynrState !== JoynrStates.STARTING) {
            throw new Error(`Cannot shutdown libjoynr because it's currently "${joynrState}"`);
        }
        joynrState = JoynrStates.SHUTTINGDOWN;

        settings = settings || {};

        const shutdownSettings = UtilInternal.extend(
            {},
            defaultLibjoynrSettings.shutdownSettings,
            provisioning.shutdownSettings,
            settings
        );

        if (shutdownSettings.clearSubscriptionsEnabled) {
            subscriptionManager.terminateSubscriptions(shutdownSettings.clearSubscriptionsTimeoutMs);
        }

        if (webSocketMessagingSkeleton !== undefined) {
            webSocketMessagingSkeleton.shutdown();
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

        const persistencyPromise = persistency !== undefined ? persistency.shutdown() : Promise.resolve();

        joynrState = JoynrStates.SHUTDOWN;
        log.debug("joynr shut down");
        return persistencyPromise;
    };

    this.shutdown = internalShutdown;

    // make every instance immutable
    return Object.freeze(this);
}

module.exports = WebSocketLibjoynrRuntime;
