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
const JoynrException = require("../exceptions/JoynrException");
const SharedWebSocket = require("../messaging/websocket/SharedWebSocket");
const WebSocketMessagingSkeleton = require("../messaging/websocket/WebSocketMessagingSkeleton");
const WebSocketMessagingStubFactory = require("../messaging/websocket/WebSocketMessagingStubFactory");
const WebSocketMulticastAddressCalculator = require("../messaging/websocket/WebSocketMulticastAddressCalculator");
const MessagingStubFactory = require("../messaging/MessagingStubFactory");
const WebSocketAddress = require("../../generated/joynr/system/RoutingTypes/WebSocketAddress");
const WebSocketClientAddress = require("../../generated/joynr/system/RoutingTypes/WebSocketClientAddress");
const InProcessMessagingStubFactory = require("../messaging/inprocess/InProcessMessagingStubFactory");
const InProcessAddress = require("../messaging/inprocess/InProcessAddress");
const InProcessSkeleton = require("../util/InProcessSkeleton");
const MessagingQos = require("../messaging/MessagingQos");
const DiscoveryQos = require("../proxy/DiscoveryQos");
const DiscoveryProxy = require("../../generated/joynr/system/DiscoveryProxy");
const RoutingProxy = require("../../generated/joynr/system/RoutingProxy");
const DiscoveryScope = require("../../generated/joynr/types/DiscoveryScope");
const DiscoveryEntryWithMetaInfo = require("../../generated/joynr/types/DiscoveryEntryWithMetaInfo");
const UtilInternal = require("../util/UtilInternal");
const uuid = require("uuid/v4");
const loggingManager = require("../system/LoggingManager");
const defaultWebSocketSettings = require("./settings/defaultWebSocketSettings");
const defaultLibjoynrSettings = require("./settings/defaultLibjoynrSettings");
const JoynrMessage = require("../../joynr/messaging/JoynrMessage");
const JoynrRuntime = require("./JoynrRuntime");

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
class WebSocketLibjoynrRuntime extends JoynrRuntime {
    constructor() {
        super();
        this._signingCallback = this._signingCallback.bind(this);
        this._bufferedOwnerId = null;
        this._webSocketMessagingSkeleton = null;
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
    async start(provisioning) {
        super.start(provisioning);

        if (UtilInternal.checkNullUndefined(provisioning.ccAddress)) {
            throw new Error("ccAddress not set in provisioning.ccAddress");
        }

        const keychain = provisioning.keychain;

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
                if (provisioning.ccAddress.host === server) {
                    return undefined;
                } else {
                    throw new Error(
                        `message from unknown host: ${server} on accepted host is cc: ${provisioning.ccAddress.host}.`
                    );
                }
            };
        }

        const initialRoutingTable = {};
        let untypedCapabilities = this._provisioning.capabilities || [];
        const defaultCapabilities = defaultLibjoynrSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);

        const ccAddress = new WebSocketAddress({
            protocol: provisioning.ccAddress.protocol || defaultWebSocketSettings.protocol,
            host: provisioning.ccAddress.host,
            port: provisioning.ccAddress.port,
            path: provisioning.ccAddress.path || defaultWebSocketSettings.path
        });

        const typedCapabilities = [];
        for (let i = 0; i < untypedCapabilities.length; i++) {
            const capability = new DiscoveryEntryWithMetaInfo(untypedCapabilities[i]);
            initialRoutingTable[capability.participantId] = ccAddress;
            typedCapabilities.push(capability);
        }

        const localAddress = new WebSocketClientAddress({
            id: uuid()
        });

        this._sharedWebSocket = new SharedWebSocket({
            remoteAddress: ccAddress,
            localAddress,
            provisioning: provisioning.websocket || {},
            keychain
        });

        this._webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
            sharedWebSocket: this._sharedWebSocket,
            mainTransport: true
        });

        const messagingStubFactories = {};
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        messagingStubFactories[WebSocketAddress._typeName] = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket: this._sharedWebSocket
        });

        const messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        const messageRouterSettings = {
            initialRoutingTable,
            joynrInstanceId: uuid(),
            messagingStubFactory,
            multicastAddressCalculator: new WebSocketMulticastAddressCalculator({
                globalAddress: ccAddress
            }),
            parentMessageRouterAddress: ccAddress,
            incomingAddress: localAddress
        };

        this._messagingSkeletons[WebSocketAddress._typeName] = this._webSocketMessagingSkeleton;

        await super._initializePersistency(provisioning);
        super._initializeComponents(provisioning, messageRouterSettings, typedCapabilities);

        this._webSocketMessagingSkeleton.registerListener(this._messageRouter.route);

        const internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

        const discovery = this._discovery;
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

        return this.proxyBuilder
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
            .catch(buildDiscoveryProxyOnError)
            .then(() => {
                this._joynrState = JoynrStates.STARTED;
                this._publicationManager.restore();
                log.debug("joynr web socket initialized");
            })
            .catch(async error => {
                log.error(`error starting up joynr: ${error}`);

                await this.shutdown();
                throw error;
            });
    }

    /**
     *  Sends subscriptionStop messages for all active subscriptions.
     *
     *  @param timeout {number} optional timeout defaulting to 0 = no timeout
     *  @returns {Promise}
     *  - resolved after all SubscriptionStop messages are sent.
     *  - rejected in case of any issues or timeout occurs.
     */
    terminateAllSubscriptions(timeout = 0) {
        this._sharedWebSocket.enableShutdownMode();
        return super.terminateSubscriptions(timeout);
    }

    /**
     * Shuts down libjoynr
     * @param settings.clearSubscriptionsTimeoutMs {number} time in ms till clearSubscriptionsPromise will be rejected
     *  if it's not resolved yet
     * @param settings.clearSubscriptionsEnabled {boolean} clear all subscriptions before shutting down.
     *  Set this to false in process.exit handler as this is not synchronous.
     *
     * @returns {Promise}
     * - resolved after successful shutdown
     * - rejected in case of any issues
     */
    async shutdown(settings) {
        if (this._sharedWebSocket) {
            this._sharedWebSocket.enableShutdownMode();
        }

        await super.shutdown(settings);

        if (this._webSocketMessagingSkeleton !== null) {
            this._webSocketMessagingSkeleton.shutdown();
        }
    }
}

module.exports = WebSocketLibjoynrRuntime;
