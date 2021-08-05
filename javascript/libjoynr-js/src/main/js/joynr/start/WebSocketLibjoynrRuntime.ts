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
import * as WebSocketProtocol from "../../generated/joynr/system/RoutingTypes/WebSocketProtocol";
import SharedWebSocket from "../messaging/websocket/SharedWebSocket";
import WebSocketMessagingSkeleton from "../messaging/websocket/WebSocketMessagingSkeleton";
import WebSocketMessagingStubFactory from "../messaging/websocket/WebSocketMessagingStubFactory";
import WebSocketMulticastAddressCalculator from "../messaging/websocket/WebSocketMulticastAddressCalculator";
import MessagingStubFactory from "../messaging/MessagingStubFactory";
import WebSocketAddress from "../../generated/joynr/system/RoutingTypes/WebSocketAddress";
import WebSocketClientAddress from "../../generated/joynr/system/RoutingTypes/WebSocketClientAddress";
import InProcessMessagingStubFactory from "../messaging/inprocess/InProcessMessagingStubFactory";
import InProcessAddress from "../messaging/inprocess/InProcessAddress";
import MessagingQos from "../messaging/MessagingQos";
import DiscoveryQos from "../proxy/DiscoveryQos";
import DiscoveryProxy from "../../generated/joynr/system/DiscoveryProxy";
import RoutingProxy from "../../generated/joynr/system/RoutingProxy";
import DiscoveryScope from "../../generated/joynr/types/DiscoveryScope";
import DiscoveryEntryWithMetaInfo, {
    DiscoveryEntryWithMetaInfoMembers
} from "../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import * as UtilInternal from "../util/UtilInternal";
import nanoid from "nanoid";
import loggingManager from "../system/LoggingManager";
import { ShutdownSettings, WebSocketLibjoynrProvisioning } from "./interface/Provisioning";
import defaultWebSocketSettings from "./settings/defaultWebSocketSettings";
import defaultLibjoynrSettings from "./settings/defaultLibjoynrSettings";
import JoynrMessage from "../../joynr/messaging/JoynrMessage";
import JoynrRuntime from "./JoynrRuntime";
import JoynrStates = require("./JoynrStates");
import LocalDiscoveryAggregator = require("../capabilities/discovery/LocalDiscoveryAggregator");
import InProcessMessagingSkeleton from "../messaging/inprocess/InProcessMessagingSkeleton";
import InProcessMessagingStub from "../messaging/inprocess/InProcessMessagingStub";
import JoynrRuntimeException from "../exceptions/JoynrRuntimeException";

const log = loggingManager.getLogger("joynr.start.WebSocketLibjoynrRuntime");

/**
 * The WebSocketLibjoynrRuntime is the version of the libjoynr-js runtime that communicates with
 * a cluster controller via a WebSocket. The cluster controller is the WebSocket Server, and the
 * libjoynr connects to it with a single websocket for all communication.
 *
 * @name WebSocketLibjoynrRuntime
 * @constructor
 */
class WebSocketLibjoynrRuntime extends JoynrRuntime<WebSocketLibjoynrProvisioning> {
    private sharedWebSocket!: SharedWebSocket;
    private webSocketMessagingSkeleton!: WebSocketMessagingSkeleton;
    private bufferedOwnerId!: Buffer;
    public constructor(onFatalRuntimeError: (error: JoynrRuntimeException) => void) {
        super(onFatalRuntimeError);
        this.signingCallback = this.signingCallback.bind(this);
    }

    private signingCallback(): Buffer {
        return this.bufferedOwnerId;
    }

    /**
     * Starts up the libjoynr instance
     *
     * @param {WebSocketLibjoynrProvisioning} provisioning
     * @returns an A+ promise object, reporting when libjoynr startup is
     *          completed or has failed
     * @throws {Error} if libjoynr is not in SHUTDOWN state
     */
    public async start(provisioning: WebSocketLibjoynrProvisioning): Promise<void> {
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

            this.bufferedOwnerId = Buffer.from(keychain.ownerId);
            JoynrMessage.setSigningCallback(this.signingCallback);

            (keychain as any).checkServerIdentity = function(server: string) {
                if (provisioning.ccAddress.host === server) {
                    return undefined;
                } else {
                    throw new Error(
                        `message from unknown host: ${server} on accepted host is cc: ${provisioning.ccAddress.host}.`
                    );
                }
            };
        }

        const initialRoutingTable: Record<string, any> = {};
        let untypedCapabilities: DiscoveryEntryWithMetaInfoMembers[] = this.provisioning.capabilities || [];
        const defaultCapabilities = defaultLibjoynrSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);
        const protocol = provisioning.ccAddress.protocol || defaultWebSocketSettings.protocol;

        const ccAddress = new WebSocketAddress({
            protocol: WebSocketProtocol[protocol.toUpperCase() as "WSS" | "WS"],
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
            id: nanoid()
        });

        this.sharedWebSocket = new SharedWebSocket({
            remoteAddress: ccAddress,
            localAddress,
            provisioning: provisioning.websocket || {},
            keychain
        });

        this.webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
            sharedWebSocket: this.sharedWebSocket,
            mainTransport: true
        });

        const messagingStubFactories: Record<string, any> = {};
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        messagingStubFactories[WebSocketAddress._typeName] = new WebSocketMessagingStubFactory({
            address: ccAddress,
            sharedWebSocket: this.sharedWebSocket
        });

        const messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        const messageRouterSettings = {
            initialRoutingTable,
            messagingStubFactory,
            multicastAddressCalculator: new WebSocketMulticastAddressCalculator({
                globalAddress: ccAddress
            }),
            parentMessageRouterAddress: ccAddress,
            incomingAddress: localAddress
        };

        const localDiscoveryAggregator = new LocalDiscoveryAggregator();

        await super.initializePersistency(provisioning);

        super.createMessageRouter(provisioning, messageRouterSettings);

        const clusterControllerMessagingSkeleton = new InProcessMessagingSkeleton();
        const clusterControllerMessagingStub = new InProcessMessagingStub(clusterControllerMessagingSkeleton);

        clusterControllerMessagingSkeleton.registerListener(this.messageRouter.route);

        super.initializeComponents(
            provisioning,
            localDiscoveryAggregator,
            clusterControllerMessagingStub,
            typedCapabilities
        );

        this.webSocketMessagingSkeleton.registerListener(this.messageRouter.route);

        const internalMessagingQos = new MessagingQos({ ttl: MessagingQos.DEFAULT_TTL + 10000 });

        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        return this.proxyBuilder!.build(RoutingProxy, {
            domain: "io.joynr",
            messagingQos: internalMessagingQos,
            discoveryQos: new DiscoveryQos({
                discoveryScope: DiscoveryScope.LOCAL_ONLY
            }),
            staticArbitration: true
        })
            .then(newRoutingProxy => {
                return this.messageRouter.setRoutingProxy(newRoutingProxy);
            })
            .catch((error: any) => {
                throw new Error(`Failed to create routing proxy: ${error}`);
            })
            .then(() => {
                return this.messageRouter.configureReplyToAddressFromRoutingProxy();
            })
            .catch((error: any) => {
                throw new Error(`Failed to initialize replyToAddress: ${error}`);
            })
            .then(() => {
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                return this.proxyBuilder!.build(DiscoveryProxy, {
                    domain: "io.joynr",
                    messagingQos: internalMessagingQos,
                    discoveryQos: new DiscoveryQos({
                        discoveryScope: DiscoveryScope.LOCAL_ONLY
                    }),
                    staticArbitration: true
                });
            })
            .then((newDiscoveryProxy: DiscoveryProxy) => {
                localDiscoveryAggregator.setDiscoveryProxy(newDiscoveryProxy);
                this.joynrState = JoynrStates.STARTED;
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
     *  @returns - resolved after all SubscriptionStop messages are sent.
     *  - rejected in case of any issues or timeout occurs.
     */
    public terminateAllSubscriptions(timeout = 0): Promise<any> {
        this.sharedWebSocket.enableShutdownMode();
        return super.terminateAllSubscriptions(timeout);
    }

    /**
     * Shuts down libjoynr
     * @param settings.clearSubscriptionsTimeoutMs {number} time in ms till clearSubscriptionsPromise will be rejected
     *  if it's not resolved yet
     * @param settings.clearSubscriptionsEnabled {boolean} clear all subscriptions before shutting down.
     *  Set this to false in process.exit handler as this is not synchronous.
     *
     * @returns - resolved after successful shutdown
     * - rejected in case of any issues
     */
    public async shutdown(settings?: ShutdownSettings): Promise<any> {
        if (this.sharedWebSocket) {
            this.sharedWebSocket.enableShutdownMode();
        }

        await super.shutdown(settings);

        if (this.webSocketMessagingSkeleton !== null) {
            this.webSocketMessagingSkeleton.shutdown();
        }
    }
}

export = WebSocketLibjoynrRuntime;
