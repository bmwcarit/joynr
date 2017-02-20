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
            "joynr/types/GlobalDiscoveryEntry",
            "joynr/capabilities/CapabilitiesRegistrar",
            "joynr/capabilities/ParticipantIdStorage",
            "joynr/capabilities/discovery/CapabilityDiscovery",
            "joynr/capabilities/CapabilitiesStore",
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/subscription/PublicationManager",
            "joynr/dispatching/subscription/SubscriptionManager",
            "joynr/dispatching/Dispatcher",
            "joynr/security/PlatformSecurityManager",
            "joynr/messaging/channel/ChannelMessagingSender",
            "joynr/messaging/channel/ChannelMessagingStubFactory",
            "joynr/messaging/channel/ChannelMessagingSkeleton",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/messaging/mqtt/MqttMessagingStubFactory",
            "joynr/messaging/mqtt/MqttMessagingSkeleton",
            "joynr/system/RoutingTypes/MqttAddress",
            "joynr/messaging/MessageReplyToAddressCalculator",
            "joynr/messaging/mqtt/SharedMqttClient",
            "joynr/messaging/mqtt/MqttMulticastAddressCalculator",
            "joynr/messaging/MessagingSkeletonFactory",
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
            "joynr/util/Typing",
            "joynr/util/LongTimer",
            "global/LocalStorage"
        ],
        function(
                Promise,
                Arbitrator,
                ProviderBuilder,
                ProxyBuilder,
                GlobalDiscoveryEntry,
                CapabilitiesRegistrar,
                ParticipantIdStorage,
                CapabilityDiscovery,
                CapabilitiesStore,
                RequestReplyManager,
                PublicationManager,
                SubscriptionManager,
                Dispatcher,
                PlatformSecurityManager,
                ChannelMessagingSender,
                ChannelMessagingStubFactory,
                ChannelMessagingSkeleton,
                ChannelAddress,
                MqttMessagingStubFactory,
                MqttMessagingSkeleton,
                MqttAddress,
                MessageReplyToAddressCalculator,
                SharedMqttClient,
                MqttMulticastAddressCalculator,
                MessagingSkeletonFactory,
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
                Typing,
                LongTimer,
                LocalStorage) {
            var JoynrStates = {
                SHUTDOWN : "shut down",
                STARTING : "starting",
                STARTED : "started",
                SHUTTINGDOWN : "shutting down"
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
                var loggingManager;
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
                    ttl : relativeTtl
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

                            var persistencyProvisioning = provisioning.persistency || {};
                            persistency = new LocalStorage({
                                clearPersistency : persistencyProvisioning.clearPersistency,
                                location : persistencyProvisioning.location
                            });

                            if (Util.checkNullUndefined(provisioning.bounceProxyUrl)) {
                                throw new Error(
                                        "bounce proxy URL not set in provisioning.bounceProxyUrl");
                            }
                            if (Util.checkNullUndefined(provisioning.bounceProxyBaseUrl)) {
                                throw new Error(
                                        "bounce proxy base URL not set in provisioning.bounceProxyBaseUrl");
                            }
                            if (Util.checkNullUndefined(provisioning.brokerUri)) {
                                throw new Error(
                                        "broker URI not set in provisioning.brokerUri");
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
                            clusterControllerSettings = defaultClusterControllerSettings({
                                bounceProxyBaseUrl: provisioning.bounceProxyBaseUrl
                            });
                            var defaultClusterControllerCapabilities = clusterControllerSettings.capabilities || [];

                            untypedCapabilities = untypedCapabilities.concat(defaultClusterControllerCapabilities);
                            /*jslint nomen: true */// allow use of _typeName once
                            typeRegistry.addType(new ChannelAddress()._typeName, ChannelAddress, false);
                            /*jslint nomen: false */
                            typedCapabilities = [];
                            var errorMessage;
                            for (i = 0; i < untypedCapabilities.length; i++) {
                                var capability =
                                        new GlobalDiscoveryEntry(untypedCapabilities[i]);
                                if (!capability.address) {
                                    throw new Error("provisioned capability is missing address: " + JSON.stringify(capability));
                                }
                                initialRoutingTable[capability.participantId] = Typing.augmentTypes(JSON.parse(capability.address), typeRegistry);
                                typedCapabilities.push(capability);
                            }

                            communicationModule = new CommunicationModule();

                            channelMessagingSender = new ChannelMessagingSender({
                                communicationModule : communicationModule,
                                channelQos : provisioning.channelQos
                            });

                            messageQueueSettings = {};
                            if (provisioning.messaging !== undefined
                                && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
                                messageQueueSettings.maxQueueSizeInKBytes =
                                        provisioning.messaging.maxQueueSizeInKBytes;
                            }

                            var channelMessageReplyToAddressCalculator = new MessageReplyToAddressCalculator({
                                //replyToAddress is provided later
                            });

                            channelMessagingStubFactory = new ChannelMessagingStubFactory({
                                myChannelId : channelId,
                                channelMessagingSender : channelMessagingSender,
                                messageReplyToAddressCalculator : channelMessageReplyToAddressCalculator
                            });

                            var globalClusterControllerAddress = new MqttAddress({
                                brokerUri : provisioning.brokerUri,
                                topic : channelId
                            });

                            var mqttMessageReplyToAddressCalculator = new MessageReplyToAddressCalculator({
                                replyToAddress : globalClusterControllerAddress
                            });

                            var mqttClient = new SharedMqttClient({
                                address: globalClusterControllerAddress,
                                provisioning : provisioning.mqtt || {}
                            });

                            messagingSkeletonFactory = new MessagingSkeletonFactory();

                            var messagingStubFactories = {};
                            /*jslint nomen: true */
                            messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
                            messagingStubFactories[ChannelAddress._typeName] = channelMessagingStubFactory;
                            messagingStubFactories[MqttAddress._typeName] = new MqttMessagingStubFactory({
                                client : mqttClient,
                                address : globalClusterControllerAddress,
                                messageReplyToAddressCalculator : mqttMessageReplyToAddressCalculator
                            });
                            /*jslint nomen: false */

                            messagingStubFactory = new MessagingStubFactory({
                                messagingStubFactories :messagingStubFactories
                            });

                            messageRouter = new MessageRouter({
                                initialRoutingTable : initialRoutingTable,
                                persistency : persistency,
                                joynrInstanceId : channelId,
                                typeRegistry : typeRegistry,
                                messagingStubFactory : messagingStubFactory,
                                messagingSkeletonFactory : messagingSkeletonFactory,
                                multicastAddressCalculator : new MqttMulticastAddressCalculator({
                                    globalAddress : globalClusterControllerAddress
                                }),
                                messageQueue : new MessageQueue(messageQueueSettings)
                            });
                            messageRouter.setGlobalClusterControllerAddress(globalClusterControllerAddress);

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

                            mqttMessagingSkeleton = new MqttMessagingSkeleton({
                                address: globalClusterControllerAddress,
                                client : mqttClient,
                                messageRouter : messageRouter
                            });

                            longPollingCreatePromise = longPollingMessageReceiver.create(channelId).then(
                                    function(channelUrl) {
                                        var channelAddress = new ChannelAddress({
                                            channelId: channelId,
                                            messagingEndpointUrl: channelUrl
                                        });

                                        mqttClient.onConnected().then(function() {
                                            capabilityDiscovery.globalAddressReady(globalClusterControllerAddress);
                                            channelMessageReplyToAddressCalculator.setReplyToAddress(channelAddress);
                                            channelMessagingStubFactory.globalAddressReady(channelAddress);
                                            return null;
                                        });

                                        longPollingMessageReceiver
                                                .start(clusterControllerChannelMessagingSkeleton.receiveMessage);
                                        channelMessagingSender.start();
                                        return null;
                                    });

                            // link up clustercontroller messaging to dispatcher
                            clusterControllerMessagingSkeleton = new InProcessMessagingSkeleton();
                            clusterControllerMessagingStub =
                                    new InProcessMessagingStub(clusterControllerMessagingSkeleton);

                            // clustercontroller messaging handled by the messageRouter
                            clusterControllerMessagingSkeleton
                                    .registerListener(messageRouter.route);

                            var ttlUpLiftMs = (provisioning.messaging && provisioning.messaging.TTL_UPLIFT) ?
                                    provisioning.messaging.TTL_UPLIFT : undefined;
                            dispatcher =
                                    new Dispatcher(
                                            clusterControllerMessagingStub,
                                            new PlatformSecurityManager(),
                                            ttlUpLiftMs);

                            libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
                            libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

                            messagingSkeletonFactory.setSkeletons({
                                InProcessAddress : libjoynrMessagingSkeleton,
                                ChannelAddress : clusterControllerChannelMessagingSkeleton,
                                MqttAddress : mqttMessagingSkeleton
                            });
                            requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
                            subscriptionManager = new SubscriptionManager(dispatcher);
                            publicationManager =
                                    new PublicationManager(dispatcher, persistency, channelId);

                            dispatcher.registerRequestReplyManager(requestReplyManager);
                            dispatcher.registerSubscriptionManager(subscriptionManager);
                            dispatcher.registerPublicationManager(publicationManager);
                            dispatcher.registerMessageRouter(messageRouter);

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
                                            cacheMaxAgeMs : Util.getMaxLongValue()
                                        })
                            };

                            capabilityDiscovery =
                                    new CapabilityDiscovery(
                                            localCapabilitiesStore,
                                            globalCapabilitiesCache,
                                            messageRouter,
                                            proxyBuilder,
                                            defaultProxyBuildSettings.domain);

                            discoveryStub.setSkeleton(new InProcessSkeleton(capabilityDiscovery));

                            var period = provisioning.capabilitiesFreshnessUpdateIntervalMs || 3600000; // default: 1 hour
                            freshnessIntervalId = LongTimer.setInterval(function() {
                                capabilityDiscovery.touch(channelId, period).catch(function(error) {
                                    log.error("error sending freshness update: " + error);
                                });
                                return null;
                            }, period);

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
                this.shutdown =
                        function shutdown() {
                            if (joynrState !== JoynrStates.STARTED) {
                                throw new Error(
                                        "Cannot shutdown libjoynr because it's currently \""
                                            + joynrState
                                            + "\"");
                            }
                            joynrState = JoynrStates.SHUTTINGDOWN;

                            LongTimer.clearInterval(freshnessIntervalId);

                            longPollingCreatePromise.then(function() {
                                return longPollingMessageReceiver.clear(channelId)
                                .then(function() {
                                    // stop LongPolling
                                    longPollingMessageReceiver.stop();
                                }).catch(function(error) {
                                    var errorString = "error clearing long poll channel: "
                                        + error;
                                    log.error(errorString);
                                    // stop LongPolling
                                    longPollingMessageReceiver.stop();
                                    throw new Error(errorString);
                                });
                            });

                            if (mqttMessagingSkeleton !== undefined) {
                                mqttMessagingSkeleton.shutdown();
                            }

                            if (channelMessagingSender !== undefined) {
                                channelMessagingSender.shutdown();
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

            return InProcessRuntime;
        });
