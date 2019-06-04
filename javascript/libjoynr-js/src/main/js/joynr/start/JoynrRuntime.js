/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
const PlatformSecurityManager = require("../security/PlatformSecurityManagerNode");
const MessagingSkeletonFactory = require("../messaging/MessagingSkeletonFactory");
const MessageRouter = require("../messaging/routing/MessageRouter");
const MessageQueue = require("../messaging/routing/MessageQueue");
const InProcessMessagingSkeleton = require("../messaging/inprocess/InProcessMessagingSkeleton");
const InProcessMessagingStub = require("../messaging/inprocess/InProcessMessagingStub");
const InProcessAddress = require("../messaging/inprocess/InProcessAddress");
const InProcessStub = require("../util/InProcessStub");
const MessagingQos = require("../messaging/MessagingQos");
const DiscoveryQos = require("../proxy/DiscoveryQos");
const TypeRegistrySingleton = require("../types/TypeRegistrySingleton");
const nanoid = require("nanoid");
const loggingManager = require("../system/LoggingManager");
const defaultLibjoynrSettings = require("./settings/defaultLibjoynrSettings");
const LocalStorage = require("../../global/LocalStorageNode");
const MemoryStorage = require("../../global/MemoryStorage");
const JoynrStates = {
    SHUTDOWN: "shut down",
    STARTING: "starting",
    STARTED: "started",
    SHUTTINGDOWN: "shutting down"
};

const log = loggingManager.getLogger("joynr.start.JoynrRuntime");

class JoynrRuntime {
    constructor() {
        // all methods apart from start need to be bound here.
        this.shutdown = this.shutdown.bind(this);
        this.terminateAllSubscriptions = this.terminateAllSubscriptions.bind(this);

        /**
         * @name JoynrRuntime#typeRegistry
         * @type TypeRegistry
         */
        this.typeRegistry = TypeRegistrySingleton.getInstance();

        /**
         * @name JoynrRuntime#registration
         * @type CapabilitiesRegistrar
         */
        this.registration = null;

        /**
         * @name JoynrRuntime#providerBuilder
         * @type ProviderBuilder
         */
        this.providerBuilder = null;

        /**
         * @name JoynrRuntime#proxyBuilder
         * @type ProxyBuilder
         */
        this.proxyBuilder = null;

        /**
         * @name JoynrRuntime#participantIdStorage
         * @type ParticipantIdStorage
         */
        this.participantIdStorage = null;

        /**
         * @name JoynrRuntime#logging
         * @type LoggingManager
         */
        this.logging = loggingManager;

        this._joynrState = JoynrStates.SHUTDOWN;
        this._webSocketMessagingSkeleton = null;
        this._arbitrator = null;
        this._messageRouter = null;
        this._requestReplyManager = null;
        this._publicationManager = null;
        this._subscriptionManager = null;
        this._dispatcher = null;
        this._messagingSkeletons = {};

        this._joynrState = JoynrStates.SHUTDOWN;
    }

    _initializePersistency(provisioning) {
        const persistencyProvisioning = Object.assign(
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
            this._persistency = new LocalStorage({
                clearPersistency: persistencyProvisioning.clearPersistency,
                location: persistencyProvisioning.location
            });
            persistencyPromise = this._persistency.init();
        } else {
            persistencyPromise = Promise.resolve();
        }

        this._persistencyConfig = {
            routingTable: persistencyProvisioning.routingTable ? this._persistency : undefined,
            capabilities: persistencyProvisioning.capabilities ? this._persistency : new MemoryStorage(),
            publications: persistencyProvisioning.publications ? this._persistency : undefined
        };

        return persistencyPromise;
    }

    _initializeComponents(provisioning, messageRouterSettings, typedCapabilities) {
        const messageQueueSettings = {};
        if (provisioning.messaging !== undefined && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
            messageQueueSettings.maxQueueSizeInKBytes = provisioning.messaging.maxQueueSizeInKBytes;
        }

        const messagingSkeletonFactory = new MessagingSkeletonFactory();

        messageRouterSettings.persistency = this._persistencyConfig.routingTable;
        messageRouterSettings.messagingSkeletonFactory = messagingSkeletonFactory;
        messageRouterSettings.messageQueue = new MessageQueue(messageQueueSettings);

        this._messageRouter = new MessageRouter(messageRouterSettings);

        // link up clustercontroller messaging to dispatcher
        const messageRouterSkeleton = new InProcessMessagingSkeleton();
        const messageRouterStub = new InProcessMessagingStub(messageRouterSkeleton);

        // clustercontroller messaging handled by the messageRouter
        messageRouterSkeleton.registerListener(this._messageRouter.route);
        const ttlUpLiftMs =
            provisioning.messaging && provisioning.messaging.TTL_UPLIFT ? provisioning.messaging.TTL_UPLIFT : undefined;
        this._dispatcher = new Dispatcher(messageRouterStub, new PlatformSecurityManager(), ttlUpLiftMs);

        const libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
        libjoynrMessagingSkeleton.registerListener(this._dispatcher.receive);

        this._messagingSkeletons[InProcessAddress._typeName] = libjoynrMessagingSkeleton;
        messagingSkeletonFactory.setSkeletons(this._messagingSkeletons);

        this._requestReplyManager = new RequestReplyManager(this._dispatcher);
        this._subscriptionManager = new SubscriptionManager(this._dispatcher);
        this._publicationManager = new PublicationManager(
            this._dispatcher,
            this._persistencyConfig.publications,
            messageRouterSettings.joynrInstanceId
        );

        this._dispatcher.registerRequestReplyManager(this._requestReplyManager);
        this._dispatcher.registerSubscriptionManager(this._subscriptionManager);
        this._dispatcher.registerPublicationManager(this._publicationManager);
        this._dispatcher.registerMessageRouter(this._messageRouter);

        this.participantIdStorage = new ParticipantIdStorage(this._persistencyConfig.capabilities, nanoid);
        this._discovery = new InProcessStub();

        this.registration = new CapabilitiesRegistrar({
            discoveryStub: this._discovery,
            messageRouter: this._messageRouter,
            requestReplyManager: this._requestReplyManager,
            publicationManager: this._publicationManager,
            libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton),
            participantIdStorage: this.participantIdStorage
        });

        // typedCapabilites can be undefined in case of InProcessRuntime
        this._arbitrator = new Arbitrator(this._discovery, typedCapabilities);

        this.providerBuilder = new ProviderBuilder({
            typeRegistry: this.typeRegistry
        });

        this.proxyBuilder = new ProxyBuilder(
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
    }

    /**
     * Starts up the libjoynr instance
     *
     * @name JoynrRuntime#start
     * @function
     * @returns {Object} an A+ promise object, reporting when libjoynr startup is
     *          completed or has failed
     * @throws {Error} if libjoynr is not in SHUTDOWN state
     */
    start(provisioning) {
        if (this._joynrState !== JoynrStates.SHUTDOWN) {
            throw new Error(`Cannot start libjoynr because it's currently "${this._joynrState}"`);
        }
        this._joynrState = JoynrStates.STARTING;

        if (!provisioning) {
            throw new Error("Constructor has been invoked without provisioning");
        }

        if (provisioning.logging) {
            this.logging.configure(provisioning.logging);
        }

        this._shutdownSettings = provisioning.shutdownSettings;
        this._provisioning = provisioning;

        if (provisioning.discoveryQos) {
            const discoveryQos = provisioning.discoveryQos;

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

    /**
     *  Sends subscriptionStop messages for all active subscriptions.
     *
     *  @param timeout {number} optional timeout defaulting to 0 = no timeout
     *  @returns {Promise} - resolved after all SubscriptionStop messages are sent.
     *  - rejected in case of any issues or timeout occurs.
     */
    terminateAllSubscriptions(timeout = 0) {
        return this._subscriptionManager.terminateSubscriptions(timeout);
    }

    /**
     * Shuts down libjoynr
     * @param settings.clearSubscriptionsTimeoutMs {number} time in ms till clearSubscriptionsPromise will be rejected
     *  if it's not resolved yet
     * @param settings.clearSubscriptionsEnabled {boolean} clear all subscriptions before shutting down.
     *  Set this to false in process.exit handler as this is not synchronous.
     *
     * @returns {Promise} - resolved after successful shutdown
     * - rejected in case of any issues
     */
    async shutdown(settings) {
        if (this._joynrState !== JoynrStates.STARTED && this._joynrState !== JoynrStates.STARTING) {
            throw new Error(`Cannot shutdown libjoynr because it's currently "${this._joynrState}"`);
        }
        this._joynrState = JoynrStates.SHUTTINGDOWN;

        settings = settings || {};

        const shutdownSettings = Object.assign(
            {},
            defaultLibjoynrSettings.shutdownSettings,
            this._shutdownSettings,
            settings
        );

        if (shutdownSettings.clearSubscriptionsEnabled) {
            await this._subscriptionManager
                .terminateSubscriptions(shutdownSettings.clearSubscriptionsTimeoutMs)
                .catch(e => {
                    log.error(`could not shutdown joynr in time due to ${e}`);
                });
        }

        if (this.registration !== null) {
            this.registration.shutdown();
        }

        if (this._arbitrator !== null) {
            this._arbitrator.shutdown();
        }

        if (this._messageRouter !== null) {
            this._messageRouter.shutdown();
        }

        if (this._requestReplyManager !== null) {
            this._requestReplyManager.shutdown();
        }

        if (this._publicationManager !== null) {
            this._publicationManager.shutdown();
        }

        if (this._subscriptionManager !== null) {
            this._subscriptionManager.shutdown();
        }

        if (this._dispatcher !== null) {
            this._dispatcher.shutdown();
        }

        if (this.typeRegistry !== null) {
            this.typeRegistry.shutdown();
        }

        const persistencyPromise = this._persistency !== undefined ? this._persistency.shutdown() : Promise.resolve();

        this._joynrState = JoynrStates.SHUTDOWN;
        log.debug("joynr shut down");
        return persistencyPromise;
    }
}

module.exports = JoynrRuntime;
