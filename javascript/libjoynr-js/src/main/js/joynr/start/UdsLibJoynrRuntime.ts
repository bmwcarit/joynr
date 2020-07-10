/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import JoynrRuntime from "./JoynrRuntime";
import { ShutdownSettings, UdsLibJoynrProvisioning } from "./interface/Provisioning";
import UdsClient from "../messaging/uds/UdsClient";
import JoynrMessage from "../messaging/JoynrMessage";
import InProcessMessagingSkeleton from "../messaging/inprocess/InProcessMessagingSkeleton";
import InProcessMessagingStub from "../messaging/inprocess/InProcessMessagingStub";
import * as DiagnosticTags from "../system/DiagnosticTags";
import RoutingProxy from "../../generated/joynr/system/RoutingProxy";
import DiscoveryQos from "../proxy/DiscoveryQos";
import DiscoveryScope from "../../generated/joynr/types/DiscoveryScope";
import DiscoveryProxy from "../../generated/joynr/system/DiscoveryProxy";
import MessageReplyToAddressCalculator from "../messaging/MessageReplyToAddressCalculator";
import DiscoveryEntryWithMetaInfo, {
    DiscoveryEntryWithMetaInfoMembers
} from "../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import defaultLibjoynrSettings from "./settings/defaultLibjoynrSettings";
import nanoid from "nanoid";
//import InProcessAddress from "../messaging/inprocess/InProcessAddress";
//import InProcessMessagingStubFactory from "../messaging/inprocess/InProcessMessagingStubFactory";
import MessagingStubFactory from "../messaging/MessagingStubFactory";
import MessagingQos from "../messaging/MessagingQos";
import UdsAddress from "../../generated/joynr/system/RoutingTypes/UdsAddress";
import UdsClientAddress from "../../generated/joynr/system/RoutingTypes/UdsClientAddress";
import LocalDiscoveryAggregator = require("../capabilities/discovery/LocalDiscoveryAggregator");
import UdsMulticastAddressCalculator from "../messaging/uds/UdsMulticastAddressCalculator";
import JoynrStates = require("./JoynrStates");
import loggingManager from "../system/LoggingManager";

const log = loggingManager.getLogger("joynr.start.UdsLibJoynrRuntime");

/**
 * The UdsLibJoynrRuntime is the version of the libjoynr-js runtime that communicates with
 * a cluster controller via a unix domain socket (UDS). The cluster controller is the UDS Server, and the
 * libjoynr connects to it with a single socket for all communication.
 *
 * @name UdsLibJoynrRuntime
 * @constructor
 */
class UdsLibJoynrRuntime extends JoynrRuntime<UdsLibJoynrProvisioning> {
    private udsClient!: UdsClient;
    private messageReplyToAddressCalculator: MessageReplyToAddressCalculator;

    public constructor() {
        super();
        this.messageReplyToAddressCalculator = new MessageReplyToAddressCalculator({});
    }

    /**
     * Starts up the libjoynr instance
     *
     * @param {UdsLibJoynrProvisioning} provisioning
     * @returns an A+ promise object, reporting when libjoynr startup is
     *          completed or has failed
     * @throws {Error} if libjoynr is not in SHUTDOWN state
     */
    public async start(provisioning: UdsLibJoynrProvisioning): Promise<void> {
        super.start(provisioning);

        const udsProvisioning = provisioning.uds || {};
        udsProvisioning.socketPath = udsProvisioning.socketPath || "/var/run/joynr/cluster-controller.sock";
        udsProvisioning.clientId = udsProvisioning.clientId || nanoid();
        udsProvisioning.connectSleepTimeMs = udsProvisioning.connectSleepTimeMs || 500;

        // routing table is still required for addMulticastReceiver and removeMulticastReceiver
        const initialRoutingTable: Record<string, any> = {};
        let untypedCapabilities: DiscoveryEntryWithMetaInfoMembers[] = this.provisioning.capabilities || [];
        const defaultCapabilities = defaultLibjoynrSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);

        const ccAddress = new UdsAddress({
            path: udsProvisioning.socketPath
        });

        // provisioned capabilities for Arbitrator
        const typedCapabilities = [];
        for (let i = 0; i < untypedCapabilities.length; i++) {
            const capability = new DiscoveryEntryWithMetaInfo(untypedCapabilities[i]);
            initialRoutingTable[capability.participantId] = ccAddress;
            typedCapabilities.push(capability);
        }

        const localAddress = new UdsClientAddress({
            id: udsProvisioning.clientId
        });

        // TODO
        // MessagingStubFactory is only required for the constructor of MessageRouter.
        // It is used there for routing which is not required in UdsLibJoynrRuntime.
        // This will be refactored and optimized later.
        const messagingStubFactories: Record<string, any> = {};
        //messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        const messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        // TODO
        // MulticastAddressCalculator is only required for the constructor of MessageRouter.
        // It is used there for routing which is not required in UdsLibJoynrRuntime.
        // This will be refactored and optimized later.
        const messageRouterSettings = {
            initialRoutingTable,
            messagingStubFactory,
            incomingAddress: localAddress,
            parentMessageRouterAddress: ccAddress,
            joynrInstanceId: nanoid(),
            multicastAddressCalculator: new UdsMulticastAddressCalculator({
                globalAddress: ccAddress
            })
        };

        const localDiscoveryAggregator = new LocalDiscoveryAggregator();

        await super.initializePersistency(provisioning);

        super.createMessageRouter(provisioning, messageRouterSettings);

        // InProcessMessagingSkeleton and InProcessMessagingStub used for direct handling of outgoing messages
        // from the dispatcher to the UDS client
        const clusterControllerMessagingSkeleton = new InProcessMessagingSkeleton();
        const clusterControllerMessagingStub = new InProcessMessagingStub(clusterControllerMessagingSkeleton);

        super.initializeComponents(
            provisioning,
            messageRouterSettings.joynrInstanceId,
            localDiscoveryAggregator,
            clusterControllerMessagingStub,
            typedCapabilities
        );

        // Forward incoming messages directly to Dispatcher.receive() (no routing)
        const onUdsClientMessageCallback = (joynrMessage: JoynrMessage): void => {
            log.debug(`<<< INCOMING <<< message with ID ${joynrMessage.msgId}`);
            this.dispatcher.receive(joynrMessage);
        };

        this.udsClient = new UdsClient({
            socketPath: udsProvisioning.socketPath,
            clientId: udsProvisioning.clientId,
            connectSleepTimeMs: udsProvisioning.connectSleepTimeMs,
            onMessageCallback: onUdsClientMessageCallback
        });

        // Forward all outgoing messages directly to udsClient.send() (no routing)
        const onMessageSend = (joynrMessage: JoynrMessage): Promise<void> => {
            if (!joynrMessage.isLocalMessage) {
                try {
                    this.messageReplyToAddressCalculator.setReplyTo(joynrMessage);
                } catch (error) {
                    log.warn(
                        `replyTo address could not be set: ${error}. Dropping message.`,
                        DiagnosticTags.forJoynrMessage(joynrMessage)
                    );
                    return Promise.resolve();
                }
            }
            return this.udsClient.send(joynrMessage);
        };

        clusterControllerMessagingSkeleton.registerListener(onMessageSend);

        const internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

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
                return this.messageRouter.getReplyToAddressFromRoutingProxy();
            })
            .catch((error: any) => {
                throw new Error(`Failed to initialize replyToAddress: ${error}`);
            })
            .then(address => {
                this.messageReplyToAddressCalculator.setReplyToAddress(address);
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
                this.publicationManager.restore();
                log.debug("UdsLibJoynrRuntime initialized");
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
        this.udsClient.enableShutdownMode();
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
        if (this.udsClient) {
            this.udsClient.enableShutdownMode();
        }
        await super.shutdown(settings);
        if (this.udsClient) {
            this.udsClient.shutdown();
        }
    }
}

export = UdsLibJoynrRuntime;
