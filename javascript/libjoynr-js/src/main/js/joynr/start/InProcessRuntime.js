/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
        "joynr/start/InProcessRuntime",
        [
            "global/Promise",
            "joynr/capabilities/arbitration/Arbitrator",
            "joynr/provider/ProviderBuilder",
            "joynr/proxy/ProxyBuilder",
            "joynr/types/CapabilityInformation",
            "joynr/capabilities/CapabilitiesRegistrar",
            "joynr/capabilities/ParticipantIdStorage",
            "joynr/capabilities/discovery/CapabilityDiscovery",
            "joynr/capabilities/CapabilitiesStore",
            "joynr/messaging/routing/LocalChannelUrlDirectory",
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/subscription/PublicationManager",
            "joynr/dispatching/subscription/SubscriptionManager",
            "joynr/dispatching/Dispatcher",
            "joynr/security/PlatformSecurityManager",
            "joynr/messaging/channel/ChannelMessagingSender",
            "joynr/messaging/channel/ChannelMessagingStubFactory",
            "joynr/messaging/channel/ChannelMessagingSkeleton",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/messaging/MessagingStubFactory",
            "joynr/messaging/routing/MessageRouter",
            "joynr/messaging/routing/MessageQueue",
            "joynr/messaging/CommunicationModule",
            "joynr/util/InProcessSkeleton",
            "joynr/util/InProcessStub",
            "joynr/messaging/inprocess/InProcessMessagingStubFactory",
            "joynr/messaging/inprocess/InProcessMessagingSkeleton",
            "joynr/messaging/inprocess/InProcessMessagingStub",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/messaging/channel/LongPollingChannelMessageReceiver",
            "joynr/messaging/MessagingQos",
            "joynr/proxy/DiscoveryQos",
            "joynr/types/DiscoveryScope",
            "joynr/infrastructure/ChannelUrlDirectoryProxy",
            "joynr/types/ChannelUrlInformation",
            "joynr/types/TypeRegistrySingleton",
            "joynr/util/UtilInternal",
            "joynr/util/CapabilitiesUtil",
            "joynr/system/DistributedLoggingAppenderConstructorFactory",
            "joynr/system/WebWorkerMessagingAppender",
            "joynr/system/LoggingManager",
            "uuid",
            "joynr/system/LoggerFactory",
            "joynr/start/settings/defaultSettings",
            "joynr/start/settings/defaultLibjoynrSettings",
            "joynr/start/settings/defaultClusterControllerSettings",
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
                CapabilityDiscovery,
                CapabilitiesStore,
                LocalChannelUrlDirectory,
                RequestReplyManager,
                PublicationManager,
                SubscriptionManager,
                Dispatcher,
                PlatformSecurityManager,
                ChannelMessagingSender,
                ChannelMessagingStubFactory,
                ChannelMessagingSkeleton,
                ChannelAddress,
                MessagingStubFactory,
                MessageRouter,
                MessageQueue,
                CommunicationModule,
                InProcessSkeleton,
                InProcessStub,
                InProcessMessagingStubFactory,
                InProcessMessagingSkeleton,
                InProcessMessagingStub,
                InProcessAddress,
                LongPollingChannelMessageReceiver,
                MessagingQos,
                DiscoveryQos,
                DiscoveryScope,
                ChannelUrlDirectoryProxy,
                ChannelUrlInformation,
                TypeRegistrySingleton,
                Util,
                CapabilitiesUtil,
                DistributedLoggingAppenderConstructorFactory,
                WebWorkerMessagingAppender,
                LoggingManager,
                uuid,
                LoggerFactory,
                defaultSettings,
                defaultLibjoynrSettings,
                defaultClusterControllerSettings,
                LocalStorage) {
            var JoynrStates = {
                SHUTDOWN : "shut down",
                STARTING : "starting",
                STARTED : "started",
                SHUTTINGDOWN : "shutting down"
            };

            var TWO_DAYS_IN_MS = 172800000;

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
                var loggingManager;
                var initialRoutingTable;
                var untypedCapabilities;
                var typedCapabilities;
                var channelUrlDirectoryStub;
                var localChannelUrlDirectory;
                var channelMessagingSender;
                var messagingStubFactory;
                var messageRouter;
                var communicationModule;
                var longPollingMessageReceiver;
                var libjoynrMessagingSkeleton;
                var clusterControllerMessagingSkeleton;
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

                // this is required at load time of libjoynr
                typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

                /**
                 * @name InProcessRuntime#typeRegistry
                 * @type TypeRegistry
                 */
                Object.defineProperty(this, "typeRegistry", {
                    get : function() {
                        return typeRegistry;
                    },
                    enumerable : true
                });

                /**
                 * @name InProcessRuntime#registration
                 * @type CapabilitiesRegistrar
                 */
                Object.defineProperty(this, "registration", {
                    get : function() {
                        return capabilitiesRegistrar;
                    },
                    enumerable : true
                });

                /**
                 * @name InProcessRuntime#capabilities
                 * @type CapabilitiesRegistrar
                 * @deprecated capabilities will be removed by 01.01.2017. please use registration instead
                 */
                Object.defineProperty(this, "capabilities", {
                    get : function() {
                        return capabilitiesRegistrar;
                    },
                    enumerable : true
                });

                /**
                 * @name InProcessRuntime#participantIdStorage
                 * @type ParticipantIdStorage
                 */
                Object.defineProperty(this, "participantIdStorage", {
                    get : function() {
                        return participantIdStorage;
                    },
                    enumerable : true
                });

                /**
                 * @name InProcessRuntime#providerBuilder
                 * @type ProviderBuilder
                 */
                Object.defineProperty(this, "providerBuilder", {
                    get : function() {
                        return providerBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InProcessRuntime#proxyBuilder
                 * @type ProxyBuilder
                 */
                Object.defineProperty(this, "proxyBuilder", {
                    get : function() {
                        return proxyBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InProcessRuntime#logger
                 * @type LoggingManager
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
                 * @name InProcessRuntime#start
                 * @function
                 * @returns {Object} an A+ promise object, reporting when libjoynr startup is
                 *          then({InProcessRuntime}, {Error})-ed
                 * @throws {Error}
                 *             if libjoynr is not in SHUTDOWN state
                 */
                this.start =
                        function start() {
                            var i,j;

                            if (joynrState !== JoynrStates.SHUTDOWN) {
                                throw new Error("Cannot start libjoynr because it's currently \""
                                    + joynrState
                                    + "\"");
                            }
                            joynrState = JoynrStates.STARTING;

                            if (!provisioning) {
                                throw new Error(
                                        "Constructor has not been invoked with provisioned data");
                            }

                            // initialize Logger with external logging configuration or default
                            // values
                            var logLevel, logLayout, appenderNames, appenderName;

                            log = LoggerFactory.getLogger("joynr.start.InProcessRuntime");

                            persistency = new LocalStorage();

                            if (Util.checkNullUndefined(provisioning.bounceProxyUrl)) {
                                throw new Error(
                                        "bounce proxy URL not set in provisioning.bounceProxyUrl");
                            }
                            if (Util.checkNullUndefined(provisioning.bounceProxyBaseUrl)) {
                                throw new Error(
                                        "bounce proxy base URL not set in provisioning.bounceProxyBaseUrl");
                            }

                            initialRoutingTable = {};
                            bounceProxyBaseUrl = provisioning.bounceProxyBaseUrl;

                            channelId =
                                    provisioning.channelId
                                        || persistency.getItem("joynr.channels.channelId.1")
                                        || "chjs_"
                                        + uuid();
                            persistency.setItem("joynr.channels.channelId.1", channelId);

                            untypedCapabilities = provisioning.capabilities || [];
                            var defaultClusterControllerCapabilities = defaultClusterControllerSettings.capabilities || [];

                            untypedCapabilities = untypedCapabilities.concat(defaultClusterControllerCapabilities);

                            typedCapabilities = [];
                            for (i = 0; i < untypedCapabilities.length; i++) {
                                var capability =
                                        new CapabilityInformation(untypedCapabilities[i]);
                                if (capability.channelId) {
                                    initialRoutingTable[capability.participantId] =
                                        new ChannelAddress({
                                            channelId : capability.channelId
                                        });
                                }
                                typedCapabilities.push(capability);
                            }

                            var channelUrlDirectoryStub = new InProcessStub();

                            /**
                             * Types all provisioned channelUrls using ChannelUrlInformation
                             *
                             * @param {Object}
                             *            channelUrls the Array of ChannelUrls
                             * @param {Array}
                             *            channelUrls.CHANNELID the ChannelUrls
                             * @param {String}
                             *            channelUrls.CHANNELID.array the ChannelUrl
                             * @returns the same structure, but with joynr typed
                             *          ChannelUrlInformation objects
                             */
                            function typeChannelUrls(channelUrls) {
                                channelUrls = channelUrls || {};
                                var channelId, typedChannelUrls = {};
                                for (channelId in channelUrls) {
                                    if (channelUrls.hasOwnProperty(channelId)) {
                                        typedChannelUrls[channelId] = new ChannelUrlInformation({
                                            urls : channelUrls[channelId]
                                        });
                                    }
                                }
                                return typedChannelUrls;
                            }

                            var mergedChannelUrls = provisioning.channelUrls || {};
                            mergedChannelUrls[defaultClusterControllerSettings.discoveryChannel] =
                                mergedChannelUrls[defaultClusterControllerSettings.discoveryChannel] ||
                                defaultClusterControllerSettings.getDefaultDiscoveryChannelUrls(
                                        provisioning.bounceProxyBaseUrl);
                            localChannelUrlDirectory = new LocalChannelUrlDirectory({
                                channelUrlDirectoryProxy : channelUrlDirectoryStub,
                                provisionedChannelUrls : typeChannelUrls(mergedChannelUrls)
                            });

                            communicationModule = new CommunicationModule();

                            channelMessagingSender = new ChannelMessagingSender({
                                channelUrlDirectory : localChannelUrlDirectory,
                                communicationModule : communicationModule,
                                channelQos : provisioning.channelQos
                            });

                            messageQueueSettings = {};
                            if (provisioning.messaging !== undefined
                                && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
                                messageQueueSettings.maxQueueSizeInKBytes =
                                        provisioning.messaging.maxQueueSizeInKBytes;
                            }
                            messagingStubFactory = new MessagingStubFactory({
                                messagingStubFactories : {
                                    InProcessAddress : new InProcessMessagingStubFactory(),
                                    ChannelAddress : new ChannelMessagingStubFactory({
                                        myChannelId : channelId,
                                        channelMessagingSender : channelMessagingSender
                                    })
                                }
                            });
                            messageRouter = new MessageRouter({
                                initialRoutingTable : initialRoutingTable,
                                persistency : persistency,
                                joynrInstanceId : channelId,
                                typeRegistry : typeRegistry,
                                messagingStubFactory : messagingStubFactory,
                                messageQueue : new MessageQueue(messageQueueSettings)
                            });

                            longPollingMessageReceiver = new LongPollingChannelMessageReceiver({
                                persistency : persistency,
                                bounceProxyUrl : bounceProxyBaseUrl + "/bounceproxy/",
                                communicationModule : communicationModule,
                                channelQos: provisioning.channelQos
                            });

                            // link up clustercontroller messaging to channel
                            clusterControllerChannelMessagingSkeleton =
                                    new ChannelMessagingSkeleton({
                                        messageRouter : messageRouter
                                    });
                            // clusterControllerChannelMessagingSkeleton
                            // .registerListener(messageRouter.receive);

                            var longPollingPromise = longPollingMessageReceiver.create(channelId).then(
                                    function(channelUrl) {
                                        longPollingMessageReceiver
                                                .start(clusterControllerChannelMessagingSkeleton.receiveMessage);
                                        return channelUrl;
                                    });

                            // link up clustercontroller messaging to dispatcher
                            clusterControllerMessagingSkeleton = new InProcessMessagingSkeleton();
                            clusterControllerMessagingStub =
                                    new InProcessMessagingStub(clusterControllerMessagingSkeleton);

                            // clustercontroller messaging handled by the messageRouter
                            clusterControllerMessagingSkeleton
                                    .registerListener(messageRouter.route);

                            dispatcher =
                                    new Dispatcher(
                                            clusterControllerMessagingStub,
                                            new PlatformSecurityManager());

                            libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
                            libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

                            requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
                            subscriptionManager = new SubscriptionManager(dispatcher);
                            publicationManager =
                                    new PublicationManager(dispatcher, persistency, channelId);

                            dispatcher.registerRequestReplyManager(requestReplyManager);
                            dispatcher.registerSubscriptionManager(subscriptionManager);
                            dispatcher.registerPublicationManager(publicationManager);

                            localCapabilitiesStore = new CapabilitiesStore(CapabilitiesUtil.toDiscoveryEntries(defaultLibjoynrSettings.capabilities || []));
                            globalCapabilitiesCache = new CapabilitiesStore(typedCapabilities);

                            participantIdStorage = new ParticipantIdStorage(persistency, uuid);

                            discoveryStub = new InProcessStub();
                            capabilitiesRegistrar =
                                Object.freeze(new CapabilitiesRegistrar({
                                    discoveryStub : discoveryStub,
                                    messageRouter : messageRouter,
                                    requestReplyManager : requestReplyManager,
                                    publicationManager : publicationManager,
                                    libjoynrMessagingAddress : new InProcessAddress(
                                            libjoynrMessagingSkeleton),
                                    participantIdStorage : participantIdStorage,
                                    localChannelId : channelId,
                                    loggingManager : loggingManager
                                }));

                            arbitrator = new Arbitrator(discoveryStub);

                            proxyBuilder =
                                Object.freeze(new ProxyBuilder({
                                    arbitrator : arbitrator,
                                    requestReplyManager : requestReplyManager,
                                    subscriptionManager : subscriptionManager,
                                    publicationManager : publicationManager
                                }, {
                                    messageRouter : messageRouter,
                                    libjoynrMessagingAddress : new InProcessAddress(
                                            libjoynrMessagingSkeleton),
                                            loggingManager : loggingManager
                                }));

                            var internalMessagingQos =
                                new MessagingQos(provisioning.internalMessagingQos);

                            var defaultProxyBuildSettings = {
                                domain : "io.joynr",
                                messagingQos : internalMessagingQos,
                                discoveryQos : new DiscoveryQos(
                                        {
                                            discoveryScope : DiscoveryScope.GLOBAL_ONLY,
                                            cacheMaxAge : Util.getMaxLongValue()
                                        })
                            };

                            capabilityDiscovery =
                                    new CapabilityDiscovery(
                                            localCapabilitiesStore,
                                            globalCapabilitiesCache,
                                            messageRouter,
                                            proxyBuilder,
                                            channelId,
                                            defaultProxyBuildSettings.domain);

                            discoveryStub.setSkeleton(new InProcessSkeleton(capabilityDiscovery));

                            providerBuilder = Object.freeze(new ProviderBuilder());

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

                            var channelUrlPromise = (function() {
                                    return proxyBuilder.build(ChannelUrlDirectoryProxy, defaultProxyBuildSettings)
                                            .then(function(newChannelUrlDirectoryProxy) {
                                                channelUrlDirectoryStub
                                                        .setSkeleton(new InProcessSkeleton(
                                                                newChannelUrlDirectoryProxy));
                                                return;
                                            }).catch(function(error) {
                                                var errorString =
                                                        "Failed to create channel url directory proxy: "
                                                            + error;
                                                log.error(errorString);
                                                throw new Error(errorString);
                                            });
                            }());

                            // if commmunication is there (longPollingPromise) and ChannelUrlProxy
                            // exists => register ChannelUrl
                            var channelUrlRegisteredPromise = Promise.all([longPollingPromise, channelUrlPromise])
                                        .then(function(longPollParams) {
                                            var channelUrl = longPollParams[0];
                                            channelMessagingSender.start();
                                            return localChannelUrlDirectory.registerChannelUrls(
                                                        {
                                                            channelId : channelId,
                                                            channelUrlInformation : new ChannelUrlInformation(
                                                                    {
                                                                        urls : [ channelUrl
                                                                        ]
                                                                    })
                                                        })
                                                .catch(function(error) {
                                                    throw new Error(
                                                            "could not register ChannelUrl "
                                                                + channelUrl
                                                                + " for ChannelId "
                                                                + channelId
                                                                + ": "
                                                                + error);
                                                });
                                        });

                            // when everything's ready we can trigger the app
                            return channelUrlPromise.then(function() {
                                    joynrState = JoynrStates.STARTED;
                                    publicationManager.restore();
                                    log.debug("joynr initialized");
                                    return;
                                }).catch(function(error) {
                                    log.error("error starting up joynr: " + error);
                                    throw error;
                                });
                        };

                /**
                 * Shuts down libjoynr
                 *
                 * @name InProcessRuntime#shutdown
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

                            // unregister channel @ ChannelUrlDir
                            var channelUrlUnregisterPromise = localChannelUrlDirectory.unregisterChannelUrls(
                                    {
                                        channelId : channelId
                                    }).catch(function(error) {
                                        var errorString = "error unregistering ChannelId "
                                                    + channelId + ": " + error;
                                        log.error(errorString);
                                        throw new Error(errorString);
                                    });

                            var longPollingReceiverStopFunction = function() {
                                return longPollingMessageReceiver.clear(channelId)
                                .then(function() {
                                    // stop LongPolling
                                    longPollingMessageReceiver.stop();
                                    return;
                                }).catch(function(error) {
                                    var errorString = "error clearing long poll channel: "
                                        + error;
                                    log.error(errorString);
                                    // stop LongPolling
                                    longPollingMessageReceiver.stop();
                                    throw new Error(errorString);
                                });
                            };

                            // unregister channel @ BounceProxy
                            var longPollingPromise = channelUrlUnregisterPromise
                                .then(longPollingReceiverStopFunction)
                                .catch(longPollingReceiverStopFunction);

                            joynrState = JoynrStates.SHUTDOWN;
                            log.debug("joynr shut down");
                            return Promise.resolve();
                        };

                // make every instance immutable
                return Object.freeze(this);
            }

            return InProcessRuntime;
        });
