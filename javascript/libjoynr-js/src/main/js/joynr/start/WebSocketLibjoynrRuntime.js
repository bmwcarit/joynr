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

const log = loggingManager.getLogger("joynr.start.WebSocketLibjoynrRuntime");

/**
 * The WebSocketLibjoynrRuntime is the version of the libjoynr-js runtime that communicates with
 * a cluster controller via a WebSocket. The cluster controller is the WebSocket Server, and the
 * libjoynr connects to it with a single websocket for all communication.
 *
 * @name WebSocketLibjoynrRuntime
 * @constructor
 * @param {Object} provisioning
 */
class WebSocketLibjoynrRuntime {
    constructor(provisioning) {
        this.shutdown = this.shutdown.bind(this);
        this._signingCallback = this._signingCallback.bind(this);

        /**
         * @name WebSocketLibjoynrRuntime#typeRegistry
         * @type TypeRegistry
         */
        this.typeRegistry = TypeRegistrySingleton.getInstance();

        /**
         * @name WebSocketLibjoynrRuntime#registration
         * @type CapabilitiesRegistrar
         */
        this.registration = null;

        /**
         * @name WebSocketLibjoynrRuntime#providerBuilder
         * @type ProviderBuilder
         */
        this.providerBuilder = null;

        /**
         * @name WebSocketLibjoynrRuntime#proxyBuilder
         * @type ProxyBuilder
         */
        this.proxyBuilder = null;

        /**
         * @name WebSocketLibjoynrRuntime#participantIdStorage
         * @type ParticipantIdStorage
         */
        this.participantIdStorage = null;

        /**
         * @name WebSocketLibjoynrRuntime#logging
         * @type LoggingManager
         */
        this.logging = loggingManager;

        this._joynrState = JoynrStates.SHUTDOWN;
        this._provisioning = provisioning;
        this._webSocketMessagingSkeleton = null;
        this._arbitrator = null;
        this._messageRouter = null;
        this._requestReplyManager = null;
        this._publicationManager = null;
        this._subscriptionManager = null;
        this._dispatcher = null;
        this._bufferedOwnerId = null;

        if (UtilInternal.checkNullUndefined(provisioning.ccAddress)) {
            throw new Error("ccAddress not set in provisioning.ccAddress");
        }

        this._joynrState = JoynrStates.SHUTDOWN;

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
    }

    _signingCallback() {
        return this._bufferedOwnerId;
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
    start() {
        let persistency;
        const keychain = this._provisioning.keychain;

        if (this._joynrState !== JoynrStates.SHUTDOWN) {
            throw new Error(`Cannot start libjoynr because it's currently "${this._joynrState}"`);
        }
        this._joynrState = JoynrStates.STARTING;

        if (!this._provisioning) {
            throw new Error("Constructor has been invoked without provisioning");
        }

        if (this._provisioning.logging) {
            this.logging.configure(this._provisioning.logging);
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

            this._bufferedOwnerId = Buffer.from(keychain.ownerId);
            JoynrMessage.setSigningCallback(this._signingCallback);

            keychain.checkServerIdentity = function(server) {
                if (this._provisioning.ccAddress.host === server) {
                    return undefined;
                } else {
                    throw new Error(
                        `message from unknown host: ${server} on accepted host is cc: ${
                            this._provisioning.ccAddress.host
                        }.`
                    );
                }
            };
        }

        const persistencyProvisioning = UtilInternal.extend(
            {},
            defaultLibjoynrSettings.persistencySettings,
            this._provisioning.persistency
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

        const initialRoutingTable = {};
        let untypedCapabilities = this._provisioning.capabilities || [];
        const defaultCapabilities = defaultLibjoynrSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);

        const ccAddress = new WebSocketAddress({
            protocol: this._provisioning.ccAddress.protocol || defaultWebSocketSettings.protocol,
            host: this._provisioning.ccAddress.host,
            port: this._provisioning.ccAddress.port,
            path: this._provisioning.ccAddress.path || defaultWebSocketSettings.path
        });

        const typedCapabilities = [];
        for (let i = 0; i < untypedCapabilities.length; i++) {
            const capability = new DiscoveryEntryWithMetaInfo(untypedCapabilities[i]);
            initialRoutingTable[capability.participantId] = ccAddress;
            typedCapabilities.push(capability);
        }

        const messageQueueSettings = {};
        if (
            this._provisioning.messaging !== undefined &&
            this._provisioning.messaging.maxQueueSizeInKBytes !== undefined
        ) {
            messageQueueSettings.maxQueueSizeInKBytes = this._provisioning.messaging.maxQueueSizeInKBytes;
        }

        const localAddress = new WebSocketClientAddress({
            id: uuid()
        });

        const sharedWebSocket = new SharedWebSocket({
            remoteAddress: ccAddress,
            localAddress,
            provisioning: this._provisioning.websocket || {},
            keychain
        });

        this._webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
            sharedWebSocket,
            mainTransport: true
        });

        const messagingSkeletonFactory = new MessagingSkeletonFactory();

        const messagingStubFactories = {};
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        messagingStubFactories[WebSocketAddress._typeName] = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket
        });

        const messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        this._messageRouter = new MessageRouter({
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

        this._webSocketMessagingSkeleton.registerListener(this._messageRouter.route);

        // link up clustercontroller messaging to dispatcher
        const messageRouterSkeleton = new InProcessMessagingSkeleton();
        const messageRouterStub = new InProcessMessagingStub(messageRouterSkeleton);

        // clustercontroller messaging handled by the messageRouter
        messageRouterSkeleton.registerListener(this._messageRouter.route);
        const ttlUpLiftMs =
            this._provisioning.messaging && this._provisioning.messaging.TTL_UPLIFT
                ? this._provisioning.messaging.TTL_UPLIFT
                : undefined;
        this._dispatcher = new Dispatcher(messageRouterStub, new PlatformSecurityManager(), ttlUpLiftMs);

        const libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
        libjoynrMessagingSkeleton.registerListener(this._dispatcher.receive);

        const messagingSkeletons = {};
        messagingSkeletons[InProcessAddress._typeName] = libjoynrMessagingSkeleton;
        messagingSkeletons[WebSocketAddress._typeName] = this._webSocketMessagingSkeleton;
        messagingSkeletonFactory.setSkeletons(messagingSkeletons);

        this._requestReplyManager = new RequestReplyManager(this._dispatcher);
        this._subscriptionManager = new SubscriptionManager(this._dispatcher);
        this._publicationManager = new PublicationManager(this._dispatcher, publicationsPersistency, "joynrInstanceId"); //TODO: create joynrInstanceId

        this._dispatcher.registerRequestReplyManager(this._requestReplyManager);
        this._dispatcher.registerSubscriptionManager(this._subscriptionManager);
        this._dispatcher.registerPublicationManager(this._publicationManager);
        this._dispatcher.registerMessageRouter(this._messageRouter);

        this.participantIdStorage = new ParticipantIdStorage(capabilitiesPersistency, uuid);
        const discovery = new InProcessStub();

        this.registration = Object.freeze(
            new CapabilitiesRegistrar({
                discoveryStub: discovery,
                messageRouter: this._messageRouter,
                requestReplyManager: this._requestReplyManager,
                publicationManager: this._publicationManager,
                libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton),
                participantIdStorage: this.participantIdStorage
            })
        );

        this._arbitrator = new Arbitrator(discovery, typedCapabilities);

        this.providerBuilder = Object.freeze(new ProviderBuilder());

        this.proxyBuilder = Object.freeze(
            new ProxyBuilder(
                {
                    arbitrator: this._arbitrator,
                    typeRegistry: this.typeRegistry,
                    requestReplyManager: this._requestReplyManager,
                    subscriptionManager: this._subscriptionManager,
                    publicationManager: this._publicationManager
                },
                {
                    messageRouter: this._messageRouter,
                    libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton)
                }
            )
        );

        /*
         * if no internalMessagingQos is provided, extend the default ttl by 10 seconds in order
         * to allow the cluster controller to handle timeout for global discovery requests and
         * send back the response to discoveryProxy
         */
        if (this._provisioning.internalMessagingQos === undefined || this._provisioning.internalMessagingQos === null) {
            this._provisioning.internalMessagingQos = {};
            this._provisioning.internalMessagingQos.ttl = MessagingQos.DEFAULT_TTL + 10000;
        }

        const internalMessagingQos = new MessagingQos(this._provisioning.internalMessagingQos);

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

        const internalProxiesPromise = this.proxyBuilder
            .build(RoutingProxy, {
                domain: "io.joynr",
                messagingQos: internalMessagingQos,
                discoveryQos: new DiscoveryQos({
                    discoveryScope: DiscoveryScope.LOCAL_ONLY
                }),
                staticArbitration: true
            })
            .then(newRoutingProxy => {
                return this._messageRouter.setRoutingProxy(newRoutingProxy);
            })
            .catch(buildRoutingProxyOnError)
            .then(() => {
                return this.proxyBuilder.build(DiscoveryProxy, {
                    domain: "io.joynr",
                    messagingQos: internalMessagingQos,
                    discoveryQos: new DiscoveryQos({
                        discoveryScope: DiscoveryScope.LOCAL_ONLY
                    }),
                    staticArbitration: true
                });
            })
            .then(buildDiscoveryProxyOnSuccess)
            .catch(buildDiscoveryProxyOnError);

        // when everything's ready we can trigger the app
        return Promise.all([internalProxiesPromise, persistencyPromise])
            .then(() => {
                this._joynrState = JoynrStates.STARTED;
                this._publicationManager.restore();
                log.debug("joynr web socket initialized");
            })
            .catch(error => {
                log.error(`error starting up joynr: ${error}`);

                this.shutdown();
                throw error;
            });
    }

    /**
     * Shuts down libjoynr
     *
     * @name WebSocketLibjoynrRuntime#shutdown
     * @function
     * @throws {Error}
     *             if libjoynr is not in the STARTED state
     */
    shutdown(settings) {
        if (this._joynrState !== JoynrStates.STARTED && this._joynrState !== JoynrStates.STARTING) {
            throw new Error(`Cannot shutdown libjoynr because it's currently "${this._joynrState}"`);
        }
        this._joynrState = JoynrStates.SHUTTINGDOWN;

        settings = settings || {};

        const shutdownSettings = UtilInternal.extend(
            {},
            defaultLibjoynrSettings.shutdownSettings,
            this._provisioning.shutdownSettings,
            settings
        );

        if (shutdownSettings.clearSubscriptionsEnabled) {
            this._subscriptionManager.terminateSubscriptions(shutdownSettings.clearSubscriptionsTimeoutMs);
        }

        if (this._webSocketMessagingSkeleton !== undefined) {
            this._webSocketMessagingSkeleton.shutdown();
        }

        if (this.registration !== undefined) {
            this.registration.shutdown();
        }

        if (this._arbitrator !== undefined) {
            this._arbitrator.shutdown();
        }

        if (this._messageRouter !== undefined) {
            this._messageRouter.shutdown();
        }

        if (this._requestReplyManager !== undefined) {
            this._requestReplyManager.shutdown();
        }

        if (this._publicationManager !== undefined) {
            this._publicationManager.shutdown();
        }

        if (this._subscriptionManager !== undefined) {
            this._subscriptionManager.shutdown();
        }

        if (this._dispatcher !== undefined) {
            this._dispatcher.shutdown();
        }

        if (this.typeRegistry !== undefined) {
            this.typeRegistry.shutdown();
        }

        const persistencyPromise = this._persistency !== undefined ? this._persistency.shutdown() : Promise.resolve();

        this._joynrState = JoynrStates.SHUTDOWN;
        log.debug("joynr shut down");
        return persistencyPromise;
    }
}

module.exports = WebSocketLibjoynrRuntime;
