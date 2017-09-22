/*jslint es5: true, node: true, node: true */
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
var Promise = require('../../global/Promise');
var WebSocket = require('../../global/WebSocketNode');
var Arbitrator = require('../capabilities/arbitration/Arbitrator');
var ProviderBuilder = require('../provider/ProviderBuilder');
var ProxyBuilder = require('../proxy/ProxyBuilder');
var CapabilitiesRegistrar = require('../capabilities/CapabilitiesRegistrar');
var ParticipantIdStorage = require('../capabilities/ParticipantIdStorage');
var RequestReplyManager = require('../dispatching/RequestReplyManager');
var PublicationManager = require('../dispatching/subscription/PublicationManager');
var SubscriptionManager = require('../dispatching/subscription/SubscriptionManager');
var Dispatcher = require('../dispatching/Dispatcher');
var JoynrException = require('../exceptions/JoynrException');
var PlatformSecurityManager = require('../security/PlatformSecurityManagerNode');
var SharedWebSocket = require('../messaging/websocket/SharedWebSocket');
var WebSocketMessagingSkeleton = require('../messaging/websocket/WebSocketMessagingSkeleton');
var WebSocketMessagingStubFactory = require('../messaging/websocket/WebSocketMessagingStubFactory');
var WebSocketMulticastAddressCalculator = require('../messaging/websocket/WebSocketMulticastAddressCalculator');
var MessagingSkeletonFactory = require('../messaging/MessagingSkeletonFactory');
var MessagingStubFactory = require('../messaging/MessagingStubFactory');
var MessageRouter = require('../messaging/routing/MessageRouter');
var MessageQueue = require('../messaging/routing/MessageQueue');
var WebSocketAddress = require('../system/RoutingTypes/WebSocketAddress');
var WebSocketClientAddress = require('../system/RoutingTypes/WebSocketClientAddress');
var InProcessMessagingStubFactory = require('../messaging/inprocess/InProcessMessagingStubFactory');
var InProcessMessagingSkeleton = require('../messaging/inprocess/InProcessMessagingSkeleton');
var InProcessMessagingStub = require('../messaging/inprocess/InProcessMessagingStub');
var InProcessAddress = require('../messaging/inprocess/InProcessAddress');
var InProcessStub = require('../util/InProcessStub');
var InProcessSkeleton = require('../util/InProcessSkeleton');
var MessagingQos = require('../messaging/MessagingQos');
var DiscoveryQos = require('../proxy/DiscoveryQos');
var DiscoveryProxy = require('../system/DiscoveryProxy');
var RoutingProxy = require('../system/RoutingProxy');
var TypeRegistrySingleton = require('../types/TypeRegistrySingleton');
var DiscoveryScope = require('../types/DiscoveryScope');
var DiscoveryEntry = require('../types/DiscoveryEntry');
var DiscoveryEntryWithMetaInfo = require('../types/DiscoveryEntryWithMetaInfo');
var UtilInternal = require('../util/UtilInternal');
var CapabilitiesUtil = require('../util/CapabilitiesUtil');
var Typing = require('../util/Typing');
var DistributedLoggingAppenderConstructorFactory = require('../system/DistributedLoggingAppenderConstructorFactory');
var DistributedLoggingAppender = require('../system/DistributedLoggingAppender');
var WebWorkerMessagingAppender = require('../system/WebWorkerMessagingAppender');
var uuid = require('../../lib/uuid-annotated');
var LoggingManager = require('../system/LoggingManager');
var LoggerFactory = require('../system/LoggerFactory');
var defaultSettings = require('./settings/defaultSettings');
var defaultWebSocketSettings = require('./settings/defaultWebSocketSettings');
var defaultLibjoynrSettings = require('./settings/defaultLibjoynrSettings');
var LocalStorage = require('../../global/LocalStorageNode');
module.exports = (function (Promise, WebSocket, Arbitrator, ProviderBuilder, ProxyBuilder, CapabilitiesRegistrar, ParticipantIdStorage, RequestReplyManager, PublicationManager, SubscriptionManager, Dispatcher, JoynrException, PlatformSecurityManager, SharedWebSocket, WebSocketMessagingSkeleton, WebSocketMessagingStubFactory, WebSocketMulticastAddressCalculator, MessagingSkeletonFactory, MessagingStubFactory, MessageRouter, MessageQueue, WebSocketAddress, WebSocketClientAddress, InProcessMessagingStubFactory, InProcessMessagingSkeleton, InProcessMessagingStub, InProcessAddress, InProcessStub, InProcessSkeleton, MessagingQos, DiscoveryQos, DiscoveryProxy, RoutingProxy, TypeRegistrySingleton, DiscoveryScope, DiscoveryEntry, DiscoveryEntryWithMetaInfo, Util, CapabilitiesUtil, Typing, DistributedLoggingAppenderConstructorFactory, DistributedLoggingAppender, WebWorkerMessagingAppender, uuid, LoggingManager, LoggerFactory, defaultSettings, defaultWebSocketSettings, defaultLibjoynrSettings, LocalStorage) {
    var JoynrStates = {
        SHUTDOWN : "shut down",
        STARTING : "starting",
        STARTED : "started",
        SHUTTINGDOWN : "shutting down"
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
        var log, loggingManager;
        var initialRoutingTable;
        var untypedCapabilities;
        var typedCapabilities;
        var messagingSkeletonFactory;
        var messagingStubFactory;
        var messageRouter;
        var libjoynrMessagingSkeleton;
        var dispatcher;
        var typeRegistry;
        var requestReplyManager;
        var subscriptionManager;
        var publicationManager;
        var participantIdStorage;
        var arbitrator;
        var providerBuilder;
        var proxyBuilder;
        var capabilitiesRegistrar;
        var discovery;
        var messageQueueSettings;
        var ccAddress;
        var sharedWebSocket;
        var webSocketMessagingSkeleton;
        var messageRouterSkeleton;
        var messageRouterStub;
        var persistency;
        var localAddress;
        var TWO_DAYS_IN_MS = 172800000;

        // this is required at load time of libjoynr
        typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

        /**
         * @name WebSocketLibjoynrRuntime#typeRegistry
         * @type TypeRegistry
         */
        Object.defineProperty(this, "typeRegistry", {
            get : function() {
                return typeRegistry;
            },
            enumerable : true
        });

        /**
         * @name WebSocketLibjoynrRuntime#registration
         * @type CapabilitiesRegistrar
         */
        Object.defineProperty(this, "registration", {
            get : function() {
                return capabilitiesRegistrar;
            },
            enumerable : true
        });

        /**
         * @name WebSocketLibjoynrRuntime#providerBuilder
         * @type ProviderBuilder
         */
        Object.defineProperty(this, "providerBuilder", {
            get : function() {
                return providerBuilder;
            },
            enumerable : true
        });

        /**
         * @name WebSocketLibjoynrRuntime#proxyBuilder
         * @type ProxyBuilder
         */
        Object.defineProperty(this, "proxyBuilder", {
            get : function() {
                return proxyBuilder;
            },
            enumerable : true
        });

        /**
         * @name WebSocketLibjoynrRuntime#participantIdStorage
         * @type ParticipantIdStorage
         */
        Object.defineProperty(this, "participantIdStorage", {
            get : function() {
                return participantIdStorage;
            },
            enumerable : true
        });

        /**
         * @name WebSocketLibjoynrRuntime#logging
         * @type LoggingManager
         */
        Object.defineProperty(this, "logging", {
            get : function() {
                return loggingManager;
            },
            enumerable : true
        });

        var relativeTtl;

        if (provisioning.logging && provisioning.logging.ttl) {
            relativeTtl = provisioning.logging.ttl;
        } else {
            relativeTtl = TWO_DAYS_IN_MS;
        }

        var loggingMessagingQos = new MessagingQos({
            ttl : relativeTtl
        });
        loggingManager = Object.freeze(new LoggingManager());
        LoggerFactory.init(loggingManager);

        if (Util.checkNullUndefined(provisioning.ccAddress)) {
            throw new Error("ccAddress not set in provisioning.ccAddress");
        }

        ccAddress = new WebSocketAddress({
            protocol : provisioning.ccAddress.protocol || defaultWebSocketSettings.protocol,
            host : provisioning.ccAddress.host,
            port : provisioning.ccAddress.port,
            path : provisioning.ccAddress.path || defaultWebSocketSettings.path
        });

        var joynrState = JoynrStates.SHUTDOWN;

        if (provisioning.capabilities && provisioning.capabilities.discoveryQos){
            var discoveryQos = provisioning.capabilities.discoveryQos;

            if (discoveryQos.discoveryExpiryIntervalMs){
                CapabilitiesRegistrar.setDefaultExpiryIntervalMs(discoveryQos.discoveryExpiryIntervalMs);
            }

            var discoveryQosSettings = {};

            if (discoveryQos.discoveryRetryDelayMs){
                discoveryQosSettings.discoveryRetryDelayMs = discoveryQos.discoveryRetryDelayMs;
            }
            if (discoveryQos.discoveryTimeoutMs){
                discoveryQosSettings.discoveryTimeoutMs = discoveryQos.discoveryTimeoutMs;
            }

            DiscoveryQos.setDefaultSettings(discoveryQosSettings);
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
        this.start =
                function start() {
                    var i, j, routingProxyPromise, discoveryProxyPromise;

                    if (joynrState !== JoynrStates.SHUTDOWN) {
                        throw new Error("Cannot start libjoynr because it's currently \""
                            + joynrState
                            + "\"");
                    }
                    joynrState = JoynrStates.STARTING;

                    if (!provisioning) {
                        throw new Error("Constructor has been invoked without provisioning");
                    }

                    // initialize Logger with external logging configuration or default
                    // values
                    loggingManager.registerAppenderClass("WebWorker", WebWorkerMessagingAppender);
                    loggingManager.registerAppenderClass(
                            "Distributed",
                            DistributedLoggingAppenderConstructorFactory.build(
                                    proxyBuilder,
                                    loggingMessagingQos));

                    if (provisioning.logging) {
                        loggingManager.configure(provisioning.logging);
                    }

                    log = LoggerFactory.getLogger("joynr.start.WebSocketLibjoynrRuntime");

                    var persistencyProvisioning = provisioning.persistency || {};
                    persistency = new LocalStorage({
                        clearPersistency : persistencyProvisioning.clearPersistency,
                        location : persistencyProvisioning.location
                    });

                    initialRoutingTable = {};
                    untypedCapabilities = provisioning.capabilities || [];
                    var defaultCapabilities = defaultLibjoynrSettings.capabilities || [];

                    untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);

                    typedCapabilities = [];
                    for (i = 0; i < untypedCapabilities.length; i++) {
                        var capability = new DiscoveryEntryWithMetaInfo(untypedCapabilities[i]);
                        initialRoutingTable[capability.participantId] = ccAddress;
                        typedCapabilities.push(capability);
                    }

                    messageQueueSettings = {};
                    if (provisioning.messaging !== undefined
                        && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
                        messageQueueSettings.maxQueueSizeInKBytes =
                                provisioning.messaging.maxQueueSizeInKBytes;
                    }

                    localAddress = new WebSocketClientAddress({
                        id : uuid()
                    });

                    sharedWebSocket = new SharedWebSocket({
                        remoteAddress : ccAddress,
                        localAddress : localAddress,
                        provisioning : provisioning.websocket || {}
                    });

                    webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
                        sharedWebSocket : sharedWebSocket,
                        mainTransport : true
                    });

                    messagingSkeletonFactory = new MessagingSkeletonFactory();

                    var messagingStubFactories = {};
                    /*jslint nomen: true */
                    messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
                    messagingStubFactories[WebSocketAddress._typeName] = new WebSocketMessagingStubFactory({
                        address : ccAddress,
                        sharedWebSocket : sharedWebSocket
                    });
                    /*jslint nomen: false */

                    messagingStubFactory = new MessagingStubFactory({
                        messagingStubFactories :messagingStubFactories
                    });

                    messageRouter = new MessageRouter({
                        initialRoutingTable : initialRoutingTable,
                        persistency : persistency,
                        typeRegistry : typeRegistry,
                        joynrInstanceId : uuid(),
                        messagingSkeletonFactory : messagingSkeletonFactory,
                        messagingStubFactory : messagingStubFactory,
                        messageQueue : new MessageQueue(messageQueueSettings),
                        multicastAddressCalculator : new WebSocketMulticastAddressCalculator({
                            globalAddress : ccAddress
                        }),
                        parentMessageRouterAddress : ccAddress,
                        incomingAddress : localAddress
                    });
                    webSocketMessagingSkeleton.registerListener(messageRouter.route);

                    // link up clustercontroller messaging to dispatcher
                    messageRouterSkeleton = new InProcessMessagingSkeleton();
                    messageRouterStub = new InProcessMessagingStub(messageRouterSkeleton);

                    // clustercontroller messaging handled by the messageRouter
                    messageRouterSkeleton.registerListener(messageRouter.route);
                    var ttlUpLiftMs = (provisioning.messaging && provisioning.messaging.TTL_UPLIFT) ?
                        provisioning.messaging.TTL_UPLIFT : undefined;
                    dispatcher = new Dispatcher(messageRouterStub, new PlatformSecurityManager(), ttlUpLiftMs);

                    libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
                    libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

                    messagingSkeletonFactory.setSkeletons({
                        InProcessAddress : libjoynrMessagingSkeleton,
                        WebSocketAddress : webSocketMessagingSkeleton
                    });

                    requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
                    subscriptionManager = new SubscriptionManager(dispatcher);
                    publicationManager =
                            new PublicationManager(dispatcher, persistency, "joynrInstanceId");//TODO: create joynrInstanceId

                    dispatcher.registerRequestReplyManager(requestReplyManager);
                    dispatcher.registerSubscriptionManager(subscriptionManager);
                    dispatcher.registerPublicationManager(publicationManager);
                    dispatcher.registerMessageRouter(messageRouter);

                    participantIdStorage = new ParticipantIdStorage(persistency, uuid);
                    discovery = new InProcessStub();

                    capabilitiesRegistrar = Object.freeze(new CapabilitiesRegistrar({
                        discoveryStub : discovery,
                        messageRouter : messageRouter,
                        requestReplyManager : requestReplyManager,
                        publicationManager : publicationManager,
                        libjoynrMessagingAddress : new InProcessAddress(libjoynrMessagingSkeleton),
                        participantIdStorage : participantIdStorage,
                        loggingManager : loggingManager
                    }));

                    arbitrator = new Arbitrator(discovery, typedCapabilities);

                    providerBuilder = Object.freeze(new ProviderBuilder());

                    proxyBuilder = Object.freeze(new ProxyBuilder({
                        arbitrator : arbitrator,
                        typeRegistry : typeRegistry,
                        requestReplyManager : requestReplyManager,
                        subscriptionManager : subscriptionManager,
                        publicationManager : publicationManager
                    }, {
                        messageRouter : messageRouter,
                        libjoynrMessagingAddress : new InProcessAddress(libjoynrMessagingSkeleton),
                        loggingManager : loggingManager
                    }));

                    var internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

                    discoveryProxyPromise = proxyBuilder.build(DiscoveryProxy, {
                            domain : "io.joynr",
                            messagingQos : internalMessagingQos,
                            discoveryQos : new DiscoveryQos({
                                discoveryScope : DiscoveryScope.LOCAL_ONLY
                            }),
                            staticArbitration : true
                        }).then(function(newDiscoveryProxy) {
                            discovery.setSkeleton(new InProcessSkeleton({
                                lookup : function lookup(domains, interfaceName, discoveryQos) {
                                    return newDiscoveryProxy.lookup({
                                        domains : domains,
                                        interfaceName : interfaceName,
                                        discoveryQos : discoveryQos
                                    }).then(function(opArgs){
                                        return opArgs.result;
                                    });
                                },
                                add : function add(discoveryEntry) {
                                    return newDiscoveryProxy.add({
                                        discoveryEntry : discoveryEntry
                                    });
                                },
                                remove : function remove(participantId) {
                                    return newDiscoveryProxy.remove({
                                        participantId : participantId
                                    });
                                }
                            }));
                            return;
                        }).catch(function(error) {
                            throw new Error("Failed to create discovery proxy: " + error);
                        });

                    routingProxyPromise = proxyBuilder.build(RoutingProxy, {
                            domain : "io.joynr",
                            messagingQos : internalMessagingQos,
                            discoveryQos : new DiscoveryQos({
                                discoveryScope : DiscoveryScope.LOCAL_ONLY
                            }),
                            staticArbitration : true
                        }).catch(function(error) {
                            throw new Error("Failed to create routing proxy: "
                                    + error
                                    + (error instanceof JoynrException ? " " + error.detailMessage : ""));
                        }).then(function(newRoutingProxy) {
                            messageRouter.setRoutingProxy(newRoutingProxy);
                            return newRoutingProxy;
                        });

                    // when everything's ready we can trigger the app
                    return Promise.all([
                                        discoveryProxyPromise,
                                        routingProxyPromise
                                    ]).then(function() {
                            joynrState = JoynrStates.STARTED;
                            publicationManager.restore();
                            log.debug("joynr web socket initialized");
                            return;
                        }).catch(function(error) {
                            log.error("error starting up joynr: " + error);
                            throw error;
                        });
                };

        /**
         * Shuts down libjoynr
         *
         * @name WebSocketLibjoynrRuntime#shutdown
         * @function
         * @throws {Error}
         *             if libjoynr is not in the STARTED state
         */
        this.shutdown =
                function shutdown() {
                    if (joynrState !== JoynrStates.STARTED) {
                        throw new Error("Cannot shutdown libjoynr because it's currently \""
                            + joynrState
                            + "\"");
                    }
                    joynrState = JoynrStates.SHUTTINGDOWN;
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

                    if (loggingManager !== undefined) {
                        loggingManager.shutdown();
                    }

                    joynrState = JoynrStates.SHUTDOWN;
                    log.debug("joynr shut down");
                    return Promise.resolve();
                };

        // make every instance immutable
        return Object.freeze(this);
    }

    return WebSocketLibjoynrRuntime;

}(Promise, WebSocket, Arbitrator, ProviderBuilder, ProxyBuilder, CapabilitiesRegistrar, ParticipantIdStorage, RequestReplyManager, PublicationManager, SubscriptionManager, Dispatcher, JoynrException, PlatformSecurityManager, SharedWebSocket, WebSocketMessagingSkeleton, WebSocketMessagingStubFactory, WebSocketMulticastAddressCalculator, MessagingSkeletonFactory, MessagingStubFactory, MessageRouter, MessageQueue, WebSocketAddress, WebSocketClientAddress, InProcessMessagingStubFactory, InProcessMessagingSkeleton, InProcessMessagingStub, InProcessAddress, InProcessStub, InProcessSkeleton, MessagingQos, DiscoveryQos, DiscoveryProxy, RoutingProxy, TypeRegistrySingleton, DiscoveryScope, DiscoveryEntry, DiscoveryEntryWithMetaInfo, UtilInternal, CapabilitiesUtil, Typing, DistributedLoggingAppenderConstructorFactory, DistributedLoggingAppender, WebWorkerMessagingAppender, uuid, LoggingManager, LoggerFactory, defaultSettings, defaultWebSocketSettings, defaultLibjoynrSettings, LocalStorage));