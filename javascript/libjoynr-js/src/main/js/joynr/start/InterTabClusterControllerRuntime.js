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
var Promise = require('../../global/Promise');
var Arbitrator = require('../capabilities/arbitration/Arbitrator');
var ProviderBuilder = require('../provider/ProviderBuilder');
var ProxyBuilder = require('../proxy/ProxyBuilder');
var GlobalDiscoveryEntry = require('../types/GlobalDiscoveryEntry');
var CapabilitiesRegistrar = require('../capabilities/CapabilitiesRegistrar');
var ParticipantIdStorage = require('../capabilities/ParticipantIdStorage');
var CapabilityDiscovery = require('../capabilities/discovery/CapabilityDiscovery');
var CapabilitiesStore = require('../capabilities/CapabilitiesStore');
var RequestReplyManager = require('../dispatching/RequestReplyManager');
var PublicationManager = require('../dispatching/subscription/PublicationManager');
var SubscriptionManager = require('../dispatching/subscription/SubscriptionManager');
var Dispatcher = require('../dispatching/Dispatcher');
var PlatformSecurityManager = require('../security/PlatformSecurityManagerNode');
var ChannelMessagingSender = require('../messaging/channel/ChannelMessagingSender');
var ChannelMessagingStubFactory = require('../messaging/channel/ChannelMessagingStubFactory');
var ChannelMessagingSkeleton = require('../messaging/channel/ChannelMessagingSkeleton');
var MqttMessagingStubFactory = require('../messaging/mqtt/MqttMessagingStubFactory');
var MqttMessagingSkeleton = require('../messaging/mqtt/MqttMessagingSkeleton');
var MqttAddress = require('../system/RoutingTypes/MqttAddress');
var SharedMqttClient = require('../messaging/mqtt/SharedMqttClient');
var MqttMulticastAddressCalculator = require('../messaging/mqtt/MqttMulticastAddressCalculator');
var MessagingSkeletonFactory = require('../messaging/MessagingSkeletonFactory');
var MessagingStubFactory = require('../messaging/MessagingStubFactory');
var MessageRouter = require('../messaging/routing/MessageRouter');
var MessageQueue = require('../messaging/routing/MessageQueue');
var CommunicationModule = require('../messaging/CommunicationModule');
var InProcessSkeleton = require('../util/InProcessSkeleton');
var InProcessStub = require('../util/InProcessStub');
var ChannelAddress = require('../system/RoutingTypes/ChannelAddress');
var InProcessMessagingStubFactory = require('../messaging/inprocess/InProcessMessagingStubFactory');
var InProcessMessagingSkeleton = require('../messaging/inprocess/InProcessMessagingSkeleton');
var InProcessMessagingStub = require('../messaging/inprocess/InProcessMessagingStub');
var InProcessAddress = require('../messaging/inprocess/InProcessAddress');
var BrowserMessagingStubFactory = require('../messaging/browser/BrowserMessagingStubFactory');
var BrowserMessagingSkeleton = require('../messaging/browser/BrowserMessagingSkeleton');
var BrowserAddress = require('../system/RoutingTypes/BrowserAddress');
var WebMessagingStub = require('../messaging/webmessaging/WebMessagingStub');
var WebMessagingSkeleton = require('../messaging/webmessaging/WebMessagingSkeleton');
var LongPollingChannelMessageReceiver = require('../messaging/channel/LongPollingChannelMessageReceiver');
var MessagingQos = require('../messaging/MessagingQos');
var DiscoveryQos = require('../proxy/DiscoveryQos');
var ProviderQos = require('../types/ProviderQos');
var ProviderScope = require('../types/ProviderScope');
var DiscoveryScope = require('../types/DiscoveryScope');
var DiscoveryProvider = require('../system/DiscoveryProvider');
var RoutingProvider = require('../system/RoutingProvider');
var TypeRegistrySingleton = require('../types/TypeRegistrySingleton');
var UtilInternal = require('../util/UtilInternal');
var DistributedLoggingAppenderConstructorFactory = require('../system/DistributedLoggingAppenderConstructorFactory');
var DistributedLoggingAppender = require('../system/DistributedLoggingAppender');
var WebWorkerMessagingAppender = require('../system/WebWorkerMessagingAppender');
var uuid = require('../../lib/uuid-annotated');
var LoggingManager = require('../system/LoggingManager');
var LoggerFactory = require('../system/LoggerFactory');
var defaultSettings = require('./settings/defaultSettings');
var defaultInterTabSettings = require('./settings/defaultInterTabSettings');
var defaultClusterControllerSettings = require('./settings/defaultClusterControllerSettings');
var Typing = require('../util/Typing');
var LongTimer = require('../util/LongTimer');
var LocalStorage = require('../../global/LocalStorageNode');
module.exports = (function (Promise, Arbitrator, ProviderBuilder, ProxyBuilder, GlobalDiscoveryEntry, CapabilitiesRegistrar, ParticipantIdStorage, CapabilityDiscovery, CapabilitiesStore, RequestReplyManager, PublicationManager, SubscriptionManager, Dispatcher, PlatformSecurityManager, ChannelMessagingSender, ChannelMessagingStubFactory, ChannelMessagingSkeleton, MqttMessagingStubFactory, MqttMessagingSkeleton, MqttAddress, SharedMqttClient, MqttMulticastAddressCalculator, MessagingSkeletonFactory, MessagingStubFactory, MessageRouter, MessageQueue, CommunicationModule, InProcessSkeleton, InProcessStub, ChannelAddress, InProcessMessagingStubFactory, InProcessMessagingSkeleton, InProcessMessagingStub, InProcessAddress, BrowserMessagingStubFactory, BrowserMessagingSkeleton, BrowserAddress, WebMessagingStub, WebMessagingSkeleton, LongPollingChannelMessageReceiver, MessagingQos, DiscoveryQos, ProviderQos, ProviderScope, DiscoveryScope, DiscoveryProvider, RoutingProvider, TypeRegistrySingleton, Util, DistributedLoggingAppenderConstructorFactory, DistributedLoggingAppender, WebWorkerMessagingAppender, uuid, LoggingManager, LoggerFactory, defaultSettings, defaultInterTabSettings, defaultClusterControllerSettings, Typing, LongTimer, LocalStorage) {
            var JoynrStates = {
                SHUTDOWN : "shut down",
                STARTING : "starting",
                STARTED : "started",
                SHUTTINGDOWN : "shutting down"
            };

            var TWO_DAYS_IN_MS = 172800000;
            var clusterControllerSettings;

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
                var loggingManager;
                var initialRoutingTable;
                var untypedCapabilities;
                var typedCapabilities;
                var channelMessagingSender;
                var channelMessagingStubFactory;
                var messagingSkeletonFactory;
                var messagingStubFactory;
                var webMessagingStub;
                var webMessagingSkeleton;
                var browserMessagingSkeleton;
                var messageRouter;
                var communicationModule;
                var longPollingMessageReceiver;
                var libjoynrMessagingSkeleton;
                var clusterControllerMessagingSkeleton;
                var mqttMessagingSkeleton;
                var clusterControllerChannelMessagingSkeleton;
                var clusterControllerMessagingStub;
                var dispatcher;
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
                var providerQos;
                var discoveryProvider;
                var routingProvider;
                var registerDiscoveryProviderPromise;
                var registerRoutingProviderPromise;
                var persistency;
                var longPollingCreatePromise;
                var freshnessIntervalId;

                // this is required at load time of libjoynr
                typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

                /**
                 * @name InterTabClusterControllerRuntime#typeRegistry
                 * @type TypeRegistry
                 */
                Object.defineProperty(this, "typeRegistry", {
                    get : function() {
                        return typeRegistry;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#registration
                 * @type CapabilitiesRegistrar
                 */
                Object.defineProperty(this, "registration", {
                    get : function() {
                        return capabilitiesRegistrar;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#participantIdStorage
                 * @type ParticipantIdStorage
                 */
                Object.defineProperty(this, "participantIdStorage", {
                    get : function() {
                        return participantIdStorage;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#providerBuilder
                 * @type ProviderBuilder
                 */
                Object.defineProperty(this, "providerBuilder", {
                    get : function() {
                        return providerBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#proxyBuilder
                 * @type ProxyBuilder
                 */
                Object.defineProperty(this, "proxyBuilder", {
                    get : function() {
                        return proxyBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#logging
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
                 * @name InterTabClusterControllerRuntime#start
                 * @function
                 * @returns {Object} an A+ promise object, reporting when libjoynr startup is completed or failed
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

                            log =
                                    LoggerFactory
                                            .getLogger("joynr.start.InterTabClusterControllerRuntime");

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
                            if (Util.checkNullUndefined(provisioning.parentWindow)) {
                                log.debug("provisioning.parentWindow not set. Use default setting \"" + defaultInterTabSettings.parentWindow + "\" instead");
                            }

                            initialRoutingTable = {};
                            bounceProxyBaseUrl = provisioning.bounceProxyBaseUrl;

                            channelId =
                                    provisioning.channelId
                                        || persistency.getItem("joynr.channels.channelId.1")
                                        || "chjs_"
                                        + uuid();
                            persistency.setItem("joynr.channels.channelId.1", channelId);

                            clusterControllerSettings = defaultClusterControllerSettings({
                                bounceProxyBaseUrl: provisioning.bounceProxyBaseUrl,
                                brokerUri: provisioning.brokerUri
                            });
                            untypedCapabilities = provisioning.capabilities || [];
                            var defaultCapabilities = clusterControllerSettings.capabilities || [];

                            untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);
                            /*jslint nomen: true */// allow use of _typeName once
                            //typeRegistry.addType(new ChannelAddress()._typeName, ChannelAddress, false);
                            typeRegistry.addType(new MqttAddress()._typeName, MqttAddress, false);
                            /*jslint nomen: false */
                            typedCapabilities = [];
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

                            //channelMessagingSender = new ChannelMessagingSender({
                            //    communicationModule : communicationModule,
                            //    channelQos : provisioning.channelQos
                            //});

                            messageQueueSettings = {};
                            if (provisioning.messaging !== undefined
                                && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
                                messageQueueSettings.maxQueueSizeInKBytes =
                                        provisioning.messaging.maxQueueSizeInKBytes;
                            }

                            webMessagingStub = new WebMessagingStub({
                                // parent window variable for communication
                                window : provisioning.parentWindow || defaultInterTabSettings.parentWindow,
                                // target origin of the parent window
                                origin : provisioning.parentOrigin || defaultInterTabSettings.parentOrigin
                            });

                            webMessagingSkeleton = new WebMessagingSkeleton({
                                window : provisioning.window || defaultInterTabSettings.window
                            });

                            browserMessagingSkeleton = new BrowserMessagingSkeleton({
                                webMessagingSkeleton : webMessagingSkeleton
                            });

                            //channelMessagingStubFactory = new ChannelMessagingStubFactory({
                            //    channelMessagingSender : channelMessagingSender
                            //});

                            var globalClusterControllerAddress = new MqttAddress({
                                brokerUri : provisioning.brokerUri,
                                topic : channelId
                            });
                            var serializedGlobalClusterControllerAddress = JSON.stringify(globalClusterControllerAddress);

                            var mqttClient = new SharedMqttClient({
                                address: globalClusterControllerAddress,
                                provisioning : provisioning.mqtt || {}
                            });

                            messagingSkeletonFactory = new MessagingSkeletonFactory();

                            var messagingStubFactories = {};
                            /*jslint nomen: true */
                            messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
                            messagingStubFactories[BrowserAddress._typeName] = new BrowserMessagingStubFactory({
                                webMessagingStub : webMessagingStub
                            });
                            //messagingStubFactories[ChannelAddress._typeName] = channelMessagingStubFactory;
                            messagingStubFactories[MqttAddress._typeName] = new MqttMessagingStubFactory({
                                client : mqttClient,
                                address : globalClusterControllerAddress
                            });
                            /*jslint nomen: false */

                            messagingStubFactory = new MessagingStubFactory({
                                messagingStubFactories :messagingStubFactories
                            });

                            messageRouter = new MessageRouter({
                                initialRoutingTable : initialRoutingTable,
                                persistency : persistency,
                                typeRegistry : typeRegistry,
                                joynrInstanceId : channelId,
                                messagingSkeletonFactory : messagingSkeletonFactory,
                                messagingStubFactory : messagingStubFactory,
                                multicastAddressCalculator : new MqttMulticastAddressCalculator({
                                    globalAddress : globalClusterControllerAddress
                                }),
                                messageQueue : new MessageQueue(messageQueueSettings)
                            });
                            messageRouter.setReplyToAddress(serializedGlobalClusterControllerAddress);
                            browserMessagingSkeleton.registerListener(messageRouter.route);

                            //longPollingMessageReceiver = new LongPollingChannelMessageReceiver({
                            //    persistency : persistency,
                            //    bounceProxyUrl : bounceProxyBaseUrl + "/bounceproxy/",
                            //    communicationModule : communicationModule,
                            //    channelQos: provisioning.channelQos
                            //});

                            //// link up clustercontroller messaging to channel
                            //clusterControllerChannelMessagingSkeleton =
                            //        new ChannelMessagingSkeleton({
                            //            messageRouter : messageRouter
                            //        });
                            // clusterControllerChannelMessagingSkeleton.registerListener(messageRouter.route);

                            mqttMessagingSkeleton = new MqttMessagingSkeleton({
                                address: globalClusterControllerAddress,
                                client : mqttClient,
                                messageRouter : messageRouter
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
                                BrowserAddress : browserMessagingSkeleton,
                                //ChannelAddress : clusterControllerChannelMessagingSkeleton,
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

                            localCapabilitiesStore = new CapabilitiesStore();
                            globalCapabilitiesCache = new CapabilitiesStore(typedCapabilities);

                            participantIdStorage = new ParticipantIdStorage(persistency, uuid);

                            discoveryStub =
                                new InProcessStub();

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

                            providerQos = new ProviderQos({
                                customParameters : [],
                                priority : Date.now(),
                                scope : ProviderScope.LOCAL
                            });

                            discoveryProvider =
                                    providerBuilder.build(
                                            DiscoveryProvider,
                                            {
                                                add : function(opArgs) {
                                                    /*FIXME remove discoveryEntry transformation,
                                                     * once the deserialization of enums works correctly
                                                     */
                                                    if (typeof opArgs.discoveryEntry.qos.scope === "string") {
                                                        opArgs.discoveryEntry.qos.scope = ProviderScope[opArgs.discoveryEntry.qos.scope];
                                                    }
                                                    return capabilityDiscovery
                                                            .add(opArgs.discoveryEntry);
                                                },
                                                lookup : function(opArgs) {
                                                    /*FIXME remove discoveryQos transformation,
                                                     * once the deserialization of enums works correctly
                                                     */
                                                    if (typeof opArgs.discoveryQos.discoveryScope === "string") {
                                                        opArgs.discoveryQos.discoveryScope = DiscoveryScope[opArgs.discoveryQos.discoveryScope];
                                                    }
                                                    return capabilityDiscovery.lookup(
                                                            opArgs.domains,
                                                            opArgs.interfaceName,
                                                            opArgs.discoveryQos).then(function(caps){
                                                                return {
                                                                    result : caps
                                                                };
                                                            });
                                                },
                                                remove : function(opArgs) {
                                                    return capabilityDiscovery
                                                            .remove(opArgs.participantId);
                                                }
                                            });
                            registerDiscoveryProviderPromise =
                                    capabilitiesRegistrar.registerProvider(
                                            "io.joynr",
                                            discoveryProvider,
                                            providerQos);

                            routingProvider =
                                    providerBuilder.build(RoutingProvider, {
                                        globalAddress : {
                                            get : function() {
                                                return Promise.resolve(serializedGlobalClusterControllerAddress);
                                            }
                                        },
                                        replyToAddress : {
                                            get : function() {
                                                return Promise.resolve(serializedGlobalClusterControllerAddress);
                                            }
                                        },
                                        addNextHop : function(opArgs) {
                                            var address;
                                            if (opArgs.channelAddress !== undefined) {
                                                address = opArgs.channelAddress;
                                            } else if (opArgs.commonApiDbusAddress !== undefined) {
                                                address = opArgs.commonApiDbusAddress;
                                            } else if (opArgs.browserAddress !== undefined) {
                                                address = opArgs.browserAddress;
                                            } else if (opArgs.webSocketAddress !== undefined) {
                                                address = opArgs.webSocketAddress;
                                            }
                                            if (address !== undefined) {
                                                return messageRouter.addNextHop(
                                                    opArgs.participantId,
                                                    address,
                                                    opArgs.isGloballyVisible);
                                            }
                                            return Promise.reject(new Error("RoutingProvider.addNextHop failed, because address " +
                                                    "could not be found in the operation arguments " + JSON.stringify(opArgs)));
                                        },
                                        resolveNextHop : function(opArgs) {
                                            return messageRouter.resolveNextHop(opArgs.participantId)
                                                    .then(function(address) {
                                                        var isResolved = address !== undefined;
                                                        return {
                                                            resolved: isResolved
                                                        };
                                                    }).catch(function(error) {
                                                        return false;
                                                    });
                                        },
                                        removeNextHop : function(opArgs) {
                                            return messageRouter.removeNextHop(opArgs.participantId);
                                        },
                                        addMulticastReceiver : messageRouter.addMulticastReceiver,
                                        removeMulticastReceiver : messageRouter.removeMulticastReceiver
                                    });
                            registerRoutingProviderPromise =
                                    capabilitiesRegistrar.registerProvider(
                                            "io.joynr",
                                            routingProvider,
                                            providerQos);

                            // when everything's ready we can resolve the promise
                            return Promise.all(
                                    [
                                        registerDiscoveryProviderPromise,
                                        registerRoutingProviderPromise
                                    ]).then(function() {
                                joynrState = JoynrStates.STARTED;
                                publicationManager.restore();
                                log.debug("joynr cluster controller initialized");
                                return;
                            }).catch(function(error) {
                                log.error("error starting up joynr: " + error);
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

                            if (loggingManager !== undefined) {
                                loggingManager.shutdown();
                            }

                            log.debug("joynr cluster controller shut down");
                            joynrState = JoynrStates.SHUTDOWN;
                            return Promise.resolve();
                };

                // make every instance immutable
                return Object.freeze(this);
            }

            return InterTabClusterControllerRuntime;

}(Promise, Arbitrator, ProviderBuilder, ProxyBuilder, GlobalDiscoveryEntry, CapabilitiesRegistrar, ParticipantIdStorage, CapabilityDiscovery, CapabilitiesStore, RequestReplyManager, PublicationManager, SubscriptionManager, Dispatcher, PlatformSecurityManager, ChannelMessagingSender, ChannelMessagingStubFactory, ChannelMessagingSkeleton, MqttMessagingStubFactory, MqttMessagingSkeleton, MqttAddress, SharedMqttClient, MqttMulticastAddressCalculator, MessagingSkeletonFactory, MessagingStubFactory, MessageRouter, MessageQueue, CommunicationModule, InProcessSkeleton, InProcessStub, ChannelAddress, InProcessMessagingStubFactory, InProcessMessagingSkeleton, InProcessMessagingStub, InProcessAddress, BrowserMessagingStubFactory, BrowserMessagingSkeleton, BrowserAddress, WebMessagingStub, WebMessagingSkeleton, LongPollingChannelMessageReceiver, MessagingQos, DiscoveryQos, ProviderQos, ProviderScope, DiscoveryScope, DiscoveryProvider, RoutingProvider, TypeRegistrySingleton, UtilInternal, DistributedLoggingAppenderConstructorFactory, DistributedLoggingAppender, WebWorkerMessagingAppender, uuid, LoggingManager, LoggerFactory, defaultSettings, defaultInterTabSettings, defaultClusterControllerSettings, Typing, LongTimer, LocalStorage));
