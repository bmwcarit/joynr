/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define(
        "joynr/start/InterTabLibjoynrRuntime",
        [
            "global/Promise",
            "joynr/capabilities/arbitration/Arbitrator",
            "joynr/provider/ProviderBuilder",
            "joynr/proxy/ProxyBuilder",
            "joynr/types/CapabilityInformation",
            "joynr/capabilities/CapabilitiesRegistrar",
            "joynr/capabilities/ParticipantIdStorage",
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/subscription/PublicationManager",
            "joynr/dispatching/subscription/SubscriptionManager",
            "joynr/dispatching/Dispatcher",
            "joynr/security/PlatformSecurityManager",
            "joynr/messaging/webmessaging/WebMessagingStub",
            "joynr/messaging/webmessaging/WebMessagingSkeleton",
            "joynr/messaging/browser/BrowserMessagingStubFactory",
            "joynr/messaging/browser/BrowserMessagingSkeleton",
            "joynr/messaging/MessagingStubFactory",
            "joynr/messaging/routing/MessageRouter",
            "joynr/messaging/routing/MessageQueue",
            "joynr/system/RoutingTypes/BrowserAddress",
            "joynr/messaging/inprocess/InProcessMessagingStubFactory",
            "joynr/messaging/inprocess/InProcessMessagingSkeleton",
            "joynr/messaging/inprocess/InProcessMessagingStub",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/util/InProcessStub",
            "joynr/util/InProcessSkeleton",
            "joynr/messaging/MessagingQos",
            "joynr/proxy/DiscoveryQos",
            "joynr/system/DiscoveryProxy",
            "joynr/system/RoutingProxy",
            "joynr/types/TypeRegistrySingleton",
            "joynr/types/DiscoveryQos",
            "joynr/types/DiscoveryScope",
            "joynr/types/DiscoveryEntry",
            "joynr/util/UtilInternal",
            "joynr/system/DistributedLoggingAppenderConstructorFactory",
            "joynr/system/DistributedLoggingAppender",
            "joynr/system/WebWorkerMessagingAppender",
            "uuid",
            "joynr/system/LoggingManager",
            "joynr/system/LoggerFactory",
            "global/LocalStorage"
        ],
        function(
                Promise,
                Arbitrator,
                ProviderBuilder,
                ProxyBuilder,
                CapabilityInformation,
                CapabilitiesRegistrar,
                ParticipantIdStorage,
                RequestReplyManager,
                PublicationManager,
                SubscriptionManager,
                Dispatcher,
                PlatformSecurityManager,
                WebMessagingStub,
                WebMessagingSkeleton,
                BrowserMessagingStubFactory,
                BrowserMessagingSkeleton,
                MessagingStubFactory,
                MessageRouter,
                MessageQueue,
                BrowserAddress,
                InProcessMessagingStubFactory,
                InProcessMessagingSkeleton,
                InProcessMessagingStub,
                InProcessAddress,
                InProcessStub,
                InProcessSkeleton,
                MessagingQos,
                DiscoveryQos,
                DiscoveryProxy,
                RoutingProxy,
                TypeRegistrySingleton,
                DiscoveryQosGenerated,
                DiscoveryScope,
                DiscoveryEntry,
                Util,
                DistributedLoggingAppenderConstructorFactory,
                DistributedLoggingAppender,
                WebWorkerMessagingAppender,
                uuid,
                LoggingManager,
                LoggerFactory,
                LocalStorage) {
            var JoynrStates = {
                SHUTDOWN : "shut down",
                STARTING : "starting",
                STARTED : "started",
                SHUTTINGDOWN : "shutting down"
            };

            var TWO_DAYS_IN_MS = 172800000;
            var CC_WINDOWID = "ClusterController";

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
                var loggingManager;
                var ccAddress;
                var initialRoutingTable;
                var untypedCapabilities;
                var typedCapabilities;
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
                var webMessagingStub;
                var webMessagingSkeleton;
                var browserMessagingSkeleton;
                var messageRouterSkeleton;
                var messageRouterStub;
                var persistency;
                var libjoynrInterTabAddress;

                // this is required at load time of libjoynr
                typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

                /**
                 * @name InterTabLibjoynrRuntime#typeRegistry
                 * @type TypeRegistry
                 * @field
                 */
                Object.defineProperty(this, "typeRegistry", {
                    get : function() {
                        return typeRegistry;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabLibjoynrRuntime#capabilities
                 * @type CapabilitiesRegistrar
                 * @field
                 */
                Object.defineProperty(this, "capabilities", {
                    get : function() {
                        return capabilitiesRegistrar;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabLibjoynrRuntime#providerBuilder
                 * @type ProviderBuilder
                 * @field
                 */
                Object.defineProperty(this, "providerBuilder", {
                    get : function() {
                        return providerBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabLibjoynrRuntime#proxyBuilder
                 * @type ProxyBuilder
                 * @field
                 */
                Object.defineProperty(this, "proxyBuilder", {
                    get : function() {
                        return proxyBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabLibjoynrRuntime#participantIdStorage
                 * @type ParticipantIdStorage
                 * @field
                 */
                Object.defineProperty(this, "participantIdStorage", {
                    get : function() {
                        return participantIdStorage;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabLibjoynrRuntime#logging
                 * @type LoggingManager
                 * @field
                 */
                Object.defineProperty(this, "logging", {
                    get : function() {
                        return loggingManager;
                    },
                    enumerable : true
                });

                var log, relativeTtl;

                if (provisioning.logging && provisioning.logging.ttl) {
                    relativeTtl = provisioning.logging.ttl;
                } else {
                    relativeTtl = TWO_DAYS_IN_MS;
                }

                var loggingMessagingQos = new MessagingQos({
                    ttl : Date.now() + relativeTtl
                });
                loggingManager = Object.freeze(new LoggingManager());
                LoggerFactory.init(loggingManager);

                var joynrState = JoynrStates.SHUTDOWN;

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
                this.start =
                        function start() {
                            var i;
                            ccAddress = new BrowserAddress({
                                windowId : CC_WINDOWID
                            });

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
                            loggingManager.registerAppenderClass(
                                    "WebWorker",
                                    WebWorkerMessagingAppender);
                            loggingManager.registerAppenderClass(
                                    "Distributed",
                                    DistributedLoggingAppenderConstructorFactory.build(
                                            proxyBuilder,
                                            loggingMessagingQos));

                            if (provisioning.logging) {
                                loggingManager.configure(provisioning.logging);
                            }

                            log = LoggerFactory.getLogger("joynr.start.InterTabLibjoynrRuntime");

                            persistency = new LocalStorage();

                            if (Util.checkNullUndefined(provisioning.parentWindow)) {
                                throw new Error(
                                        "parent window not set in provisioning.parentWindow, use \"window.opener || window.top\" for example");
                            }

                            if (Util.checkNullUndefined(provisioning.windowId)) {
                                throw new Error("windowId not set in provisioning.windowId");
                            }

                            libjoynrInterTabAddress = new BrowserAddress({
                                windowId : provisioning.windowId
                            });
                            initialRoutingTable = {};
                            untypedCapabilities = provisioning.capabilities || [];
                            typedCapabilities = [];
                            if (untypedCapabilities) {
                                for (i = 0; i < untypedCapabilities.length; i++) {
                                    var capability =
                                            new CapabilityInformation(untypedCapabilities[i]);
                                    initialRoutingTable[capability.participantId] = ccAddress;
                                    typedCapabilities.push(capability);
                                }
                            }

                            messageQueueSettings = {};
                            if (provisioning.messaging !== undefined
                                && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
                                messageQueueSettings.maxQueueSizeInKBytes =
                                        provisioning.messaging.maxQueueSizeInKBytes;
                            }

                            webMessagingStub = new WebMessagingStub({
                                window : provisioning.parentWindow, // parent window variable for
                                // communication, this should be
                                // window.opener || window.top
                                origin : provisioning.parentOrigin
                            // target origin of the parent window, this will be location.origin in
                            // most cases
                            });

                            webMessagingSkeleton = new WebMessagingSkeleton({
                                window : provisioning.window
                            // window variable for communication, this should be window
                            });

                            browserMessagingSkeleton = new BrowserMessagingSkeleton({
                                webMessagingSkeleton : webMessagingSkeleton
                            });

                            messagingStubFactory = new MessagingStubFactory({
                                messagingStubFactories : {
                                    InProcessAddress : new InProcessMessagingStubFactory(),
                                    BrowserAddress : new BrowserMessagingStubFactory({
                                        webMessagingStub : webMessagingStub
                                    })
                                }
                            });
                            messageRouter = new MessageRouter({
                                initialRoutingTable : initialRoutingTable,
                                persistency : persistency,
                                typeRegistry : typeRegistry,
                                joynrInstanceId : provisioning.windowId,
                                messagingStubFactory : messagingStubFactory,
                                messageQueue : new MessageQueue(messageQueueSettings),
                                parentMessageRouterAddress : ccAddress,
                                incomingAddress : libjoynrInterTabAddress
                            });
                            browserMessagingSkeleton.registerListener(messageRouter.route);

                            // link up clustercontroller messaging to dispatcher
                            messageRouterSkeleton = new InProcessMessagingSkeleton();
                            messageRouterStub = new InProcessMessagingStub(messageRouterSkeleton);

                            // clustercontroller messaging handled by the messageRouter
                            messageRouterSkeleton.registerListener(messageRouter.route);
                            dispatcher =
                                    new Dispatcher(messageRouterStub, new PlatformSecurityManager());

                            libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
                            libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

                            requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
                            subscriptionManager = new SubscriptionManager(dispatcher);
                            publicationManager =
                                    new PublicationManager(
                                            dispatcher,
                                            persistency,
                                            "localchannelId"); //TODO: set joynrInstanceId

                            dispatcher.registerRequestReplyManager(requestReplyManager);
                            dispatcher.registerSubscriptionManager(subscriptionManager);
                            dispatcher.registerPublicationManager(publicationManager);

                            participantIdStorage = new ParticipantIdStorage(persistency, uuid);
                            discovery = new InProcessStub();

                            capabilitiesRegistrar =
                                    Object.freeze(new CapabilitiesRegistrar({
                                        discoveryStub : discovery,
                                        messageRouter : messageRouter,
                                        requestReplyManager : requestReplyManager,
                                        publicationManager : publicationManager,
                                        libjoynrMessagingAddress : new InProcessAddress(
                                                libjoynrMessagingSkeleton),
                                        participantIdStorage : participantIdStorage,
                                        localChannelId : "localchannelId", //TODO: set joynrInstanceId
                                        loggingManager : loggingManager
                                    }));

                            arbitrator = new Arbitrator(discovery, typedCapabilities);

                            providerBuilder = Object.freeze(new ProviderBuilder());

                            proxyBuilder =
                                    Object.freeze(new ProxyBuilder({
                                        arbitrator : arbitrator,
                                        typeRegistry : typeRegistry,
                                        requestReplyManager : requestReplyManager,
                                        subscriptionManager : subscriptionManager,
                                        publicationManager : publicationManager
                                    }, {
                                        messageRouter : messageRouter,
                                        libjoynrMessagingAddress : new InProcessAddress(
                                                libjoynrMessagingSkeleton),
                                        loggingManager : loggingManager
                                    }));

                            var getDiscoveryProxy = function getDiscoveryProxy(ttl) {
                                return proxyBuilder.build(DiscoveryProxy, {
                                    domain : "io.joynr",
                                    messagingQos : {
                                        ttl: ttl
                                    },
                                    discoveryQos : new DiscoveryQos({
                                        discoveryScope : DiscoveryScope.LOCAL_ONLY
                                    }),
                                    staticArbitration : true
                                }).catch(function(error) {
                                    throw new Error("Failed to create discovery proxy: "
                                            + error);
                                });
                            };

                            var TTL_30DAYS_IN_MS = 30*24*60*60*1000;
                            discovery.setSkeleton(new InProcessSkeleton({
                                lookup : function lookup(
                                        domain,
                                        interfaceName,
                                        discoveryQos) {
                                    return getDiscoveryProxy(discoveryQos.discoveryTimeout).then(function(newDiscoveryProxy) {
                                        return newDiscoveryProxy.lookup({
                                            domain : domain,
                                            interfaceName : interfaceName,
                                            discoveryQos : discoveryQos
                                        });
                                    });
                                },
                                add : function add(discoveryEntry) {
                                    return getDiscoveryProxy(TTL_30DAYS_IN_MS).then(function(newDiscoveryProxy) {
                                        return newDiscoveryProxy.add({
                                            discoveryEntry : discoveryEntry
                                        });
                                    });
                                },
                                remove : function remove(participantId) {
                                    return getDiscoveryProxy(TTL_30DAYS_IN_MS).then(function(newDiscoveryProxy) {
                                        return newDiscoveryProxy.remove({
                                            participantId : participantId
                                        });
                                    });
                                }
                            }));

                            var internalMessagingQos =
                                new MessagingQos(provisioning.internalMessagingQos);

                            var routingProxyPromise = proxyBuilder.build(RoutingProxy, {
                                    domain : "io.joynr",
                                    messagingQos : internalMessagingQos,
                                    discoveryQos : new DiscoveryQos({
                                        discoveryScope : DiscoveryScope.LOCAL_ONLY
                                    }),
                                    staticArbitration : true
                                }).then(function(newRoutingProxy) {
                                    messageRouter.setRoutingProxy(newRoutingProxy);
                                    return newRoutingProxy;
                                }).catch(function(error) {
                                    throw new Error("Failed to create routing proxy: " + error);
                                });

                            // when everything's ready we can trigger the app
                            return  Promise.all([
                                    routingProxyPromise
                                ]).then(
                            function() {
                                joynrState = JoynrStates.STARTED;
                                publicationManager.restore();
                                log.debug("libjoynr initialized");
                                return;
                            }).catch(function(error) {
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
                this.shutdown =
                        function shutdown() {
                            if (joynrState !== JoynrStates.STARTED) {
                                throw new Error(
                                        "Cannot shutdown libjoynr because it's currently \""
                                            + joynrState
                                            + "\"");
                            }
                            joynrState = JoynrStates.SHUTTINGDOWN;
                            if (webMessagingSkeleton !== undefined) {
                                webMessagingSkeleton.shutdown();
                            }
                            return Promise.all([]/* TODO: insert promises here */).then(function() {
                                joynrState = JoynrStates.SHUTDOWN;
                                log.debug("joynr shut down");
                                return;
                            });
                        };

                // make every instance immutable
                return Object.freeze(this);
            }

            return InterTabLibjoynrRuntime;

        });
