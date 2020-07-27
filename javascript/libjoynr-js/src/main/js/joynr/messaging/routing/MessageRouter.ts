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
import * as RoutingProxy from "../../../generated/joynr/system/RoutingProxy";
import BrowserAddress = require("../../../generated/joynr/system/RoutingTypes/BrowserAddress");
import ChannelAddress = require("../../../generated/joynr/system/RoutingTypes/ChannelAddress");
import WebSocketAddress = require("../../../generated/joynr/system/RoutingTypes/WebSocketAddress");
import WebSocketClientAddress = require("../../../generated/joynr/system/RoutingTypes/WebSocketClientAddress");
import MulticastWildcardRegexFactory from "../util/MulticastWildcardRegexFactory";

import * as DiagnosticTags from "../../system/DiagnosticTags";
import LoggingManager from "../../system/LoggingManager";
import InProcessAddress from "../inprocess/InProcessAddress";
import JoynrMessage from "../JoynrMessage";
import MessageReplyToAddressCalculator from "../MessageReplyToAddressCalculator";
import JoynrRuntimeException from "../../exceptions/JoynrRuntimeException";
import * as Typing from "../../util/Typing";
import * as UtilInternal from "../../util/UtilInternal";
import * as JSONSerializer from "../../util/JSONSerializer";
import MessagingStubFactory = require("../MessagingStubFactory");
import LocalStorageNode = require("../../../global/LocalStorageNode");
import MessageQueue = require("./MessageQueue");
import Address = require("../../../generated/joynr/system/RoutingTypes/Address");
import JoynrCompound = require("../../types/JoynrCompound");
import UdsClientAddress from "../../../generated/joynr/system/RoutingTypes/UdsClientAddress";
import UdsAddress from "../../../generated/joynr/system/RoutingTypes/UdsAddress";

const log = LoggingManager.getLogger("joynr/messaging/routing/MessageRouter");

type RoutingTable = Record<string, Address>;
type LocalStorage = LocalStorageNode;
type MulticastSkeletons = Record<string, any>;
/*
    TODO: let WebSocketMulticastAddressCalculator and MqttMulticastAddresscalculator, etc. implement this interface,
    or create an abstract class for it
 */
interface MulticastAddressCalculator {
    calculate: (joynrMessage: JoynrMessage) => Address;
}

namespace MessageRouter {
    export interface MessageRouterSettings {
        routingTable?: RoutingTable;
        messagingStubFactory: MessagingStubFactory;
        incomingAddress?: Address;
        parentMessageRouterAddress?: Address;
        persistency: LocalStorage;
        messageQueue: MessageQueue;
        joynrInstanceId: string;
        initialRoutingTable?: RoutingTable;
        multicastAddressCalculator: MulticastAddressCalculator;
        multicastSkeletons: MulticastSkeletons;
    }
}

class MessageRouter {
    private settings: MessageRouter.MessageRouterSettings;

    private started: boolean = true;
    private messagesWithoutReplyTo: JoynrMessage[];
    private messageReplyToAddressCalculator: MessageReplyToAddressCalculator;
    private replyToAddress!: string;
    private multicastReceiversRegistry: any = {};
    private multicastSkeletons: MulticastSkeletons;
    private multicastAddressCalculator: MulticastAddressCalculator;
    private parentMessageRouterAddress?: Address;
    private incomingAddress?: Address;
    private persistency: LocalStorage;
    private id: string;
    private routingTable: RoutingTable;
    private queuedRemoveMulticastReceiverCalls: any;
    private queuedAddMulticastReceiverCalls: any;
    private queuedRemoveNextHopCalls: any;
    private queuedAddNextHopCalls: any;
    private messagingStub: any;
    private routingProxy!: RoutingProxy;
    private multicastWildcardRegexFactory: MulticastWildcardRegexFactory;
    /**
     * Message Router receives a message and forwards it to the correct endpoint, as looked up in the {@link RoutingTable} @constructor
     *
     * @param settings the settings object holding dependencies
     * @param settings.routingTable
     * @param settings.messagingStubFactory
     * @param settings.incomingAddress
     * @param settings.parentMessageRouterAddress
     * @param settings.persistency - LocalStorage or another object implementing the same interface
     * @param settings.messageQueue
     * @param settings.joynrInstanceId
     * @param settings.initialRoutingTable
     * @param settings.multicastAddressCalculator
     * @param settings.multicastSkeletons
     *
     * @classdesc The <code>MessageRouter</code> is a joynr internal interface. The Message
     * Router receives messages from Message Receivers, and forwards them along using to the
     * appropriate Address, either <code>{@link ChannelAddress}</code> for messages being
     * sent to a joynr channel via HTTP, <code>{@link BrowserAddress}</code> for messages
     * going to applications running within a seperate browser tab, or
     * <code>{@link InProcessAddress}</code> for messages going to a dispatcher running
     * within the same JavaScript scope as the MessageRouter.
     *
     * MessageRouter is part of the cluster controller, and is used for
     * internal messaging only.
     */
    public constructor(settings: MessageRouter.MessageRouterSettings) {
        this.multicastWildcardRegexFactory = new MulticastWildcardRegexFactory();
        this.queuedAddNextHopCalls = [];
        this.queuedRemoveNextHopCalls = [];
        this.queuedAddMulticastReceiverCalls = [];
        this.queuedRemoveMulticastReceiverCalls = [];
        this.routingTable = settings.initialRoutingTable || {};
        this.id = settings.joynrInstanceId;
        this.persistency = settings.persistency;
        this.incomingAddress = settings.incomingAddress;
        this.parentMessageRouterAddress = settings.parentMessageRouterAddress;
        this.multicastAddressCalculator = settings.multicastAddressCalculator;
        this.multicastSkeletons = settings.multicastSkeletons;
        this.messageReplyToAddressCalculator = new MessageReplyToAddressCalculator({});
        this.messagesWithoutReplyTo = [];

        if (settings.parentMessageRouterAddress !== undefined && settings.incomingAddress === undefined) {
            throw new Error("incoming address is undefined");
        }
        if (settings.messagingStubFactory === undefined) {
            throw new Error("messaging stub factory is undefined");
        }
        if (settings.messageQueue === undefined) {
            throw new Error("messageQueue is undefined");
        }

        if (!this.persistency) {
            this.getAddressFromPersistency = UtilInternal.emptyFunction as any;
        }

        this.route = this.route.bind(this);

        this.settings = settings;
    }

    private isReady(): boolean {
        return this.started;
    }

    private addRoutingProxyToParentRoutingTable(): Promise<void> {
        if (this.routingProxy && this.routingProxy.proxyParticipantId !== undefined) {
            // isGloballyVisible is false because the routing provider is local
            const isGloballyVisible = false;
            return this.addNextHopToParentRoutingTable(this.routingProxy.proxyParticipantId, isGloballyVisible).catch(
                error => {
                    if (!this.isReady()) {
                        //in this case, the error is expected, e.g. during shut down
                        log.debug(
                            `Adding routingProxy.proxyParticipantId ${
                                this.routingProxy.proxyParticipantId
                            }failed while the message router is not ready. Error: ${error.message}`
                        );
                        return;
                    }
                    throw new Error(error);
                }
            );
        }
        return Promise.resolve();
    }

    private processQueuedRoutingProxyCalls(): void {
        let hopIndex, receiverIndex, queuedCall, length;

        length = this.queuedAddNextHopCalls.length;
        for (hopIndex = 0; hopIndex < length; hopIndex++) {
            queuedCall = this.queuedAddNextHopCalls[hopIndex];
            if (queuedCall.participantId !== this.routingProxy.proxyParticipantId) {
                this.addNextHopToParentRoutingTable(queuedCall.participantId, queuedCall.isGloballyVisible)
                    .then(queuedCall.resolve)
                    .catch(queuedCall.reject);
            }
        }
        length = this.queuedRemoveNextHopCalls.length;
        for (hopIndex = 0; hopIndex < length; hopIndex++) {
            queuedCall = this.queuedRemoveNextHopCalls[hopIndex];
            this.removeNextHop(queuedCall.participantId)
                .then(queuedCall.resolve)
                .catch(queuedCall.reject);
        }
        length = this.queuedAddMulticastReceiverCalls.length;
        for (receiverIndex = 0; receiverIndex < length; receiverIndex++) {
            queuedCall = this.queuedAddMulticastReceiverCalls[receiverIndex];
            this.routingProxy
                .addMulticastReceiver(queuedCall.parameters)
                .then(queuedCall.resolve)
                .catch(queuedCall.reject);
        }
        length = this.queuedRemoveMulticastReceiverCalls.length;
        for (receiverIndex = 0; receiverIndex < length; receiverIndex++) {
            queuedCall = this.queuedRemoveMulticastReceiverCalls[receiverIndex];
            this.routingProxy
                .removeMulticastReceiver(queuedCall.parameters)
                .then(queuedCall.resolve)
                .catch(queuedCall.reject);
        }
        this.queuedAddNextHopCalls = undefined;
        this.queuedRemoveNextHopCalls = undefined;
        this.queuedAddMulticastReceiverCalls = undefined;
        this.queuedRemoveMulticastReceiverCalls = undefined;
    }

    /**
     * This method is called when no address can be found in the local routing table.
     *
     * It tries to resolve the next hop from the persistency and parent router.
     */
    private resolveNextHopInternal(participantId: string): Promise<Address> {
        const address = this.getAddressFromPersistency(participantId);

        if (address === undefined && this.routingProxy !== undefined) {
            return this.routingProxy
                .resolveNextHop({
                    participantId
                })
                .then(opArgs => {
                    if (opArgs.resolved) {
                        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                        this.routingTable[participantId] = this.parentMessageRouterAddress!;
                        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                        return this.parentMessageRouterAddress!;
                    }
                    throw new Error(
                        `nextHop cannot be resolved, as participant with id ${participantId} is not reachable by parent routing table`
                    );
                });
        }
        return Promise.resolve(address as Address);
    }

    private containsAddress(array: Address[], address: Address): boolean {
        //each address class provides an equals method, e.g. InProcessAddress
        if (array === undefined) {
            return false;
        }
        for (let j = 0; j < array.length; j++) {
            if ((array[j] as JoynrCompound).equals(address)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Get the address to which the passed in message should be sent to.
     * This is a multicast address calculated from the header content of the message.
     *
     * @param message the message for which we want to find an address to send it to.
     * @return the address to send the message to. Will not be null, because if an address can't be determined an exception is thrown.
     */
    private getAddressesForMulticast(joynrMessage: JoynrMessage): Address[] {
        const result = [];
        let address;
        if (!joynrMessage.isReceivedFromGlobal) {
            address = this.multicastAddressCalculator.calculate(joynrMessage);
            if (address !== undefined) {
                result.push(address);
            }
        }

        let multicastIdPattern, receivers;
        for (multicastIdPattern in this.multicastReceiversRegistry) {
            if (this.multicastReceiversRegistry.hasOwnProperty(multicastIdPattern)) {
                if (joynrMessage.to.match(new RegExp(multicastIdPattern)) !== null) {
                    receivers = this.multicastReceiversRegistry[multicastIdPattern];
                    if (receivers !== undefined) {
                        for (let i = 0; i < receivers.length; i++) {
                            address = this.routingTable[receivers[i]];
                            if (address !== undefined && !this.containsAddress(result, address)) {
                                result.push(address);
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    private routeInternalTransmitOnError(error: Error): void {
        //error while transmitting message
        log.debug(`Error while transmitting message: ${error}`);
        //TODO queue message and retry later
    }

    /**
     * Helper function to route a message once the address is known
     */
    private routeInternal(address: Address, joynrMessage: JoynrMessage): Promise<void> {
        let errorMsg;
        // Error: The participant is not registered yet.
        // remote provider participants are registered by capabilitiesDirectory on lookup
        // local providers are registered by capabilitiesDirectory on register
        // replyCallers are registered when they are created

        if (!joynrMessage.isLocalMessage) {
            try {
                this.messageReplyToAddressCalculator.setReplyTo(joynrMessage);
            } catch (error) {
                this.messagesWithoutReplyTo.push(joynrMessage);
                errorMsg = `replyTo address could not be set: ${error}. Queuing message.`;
                log.warn(errorMsg, DiagnosticTags.forJoynrMessage(joynrMessage));
                return Promise.resolve();
            }
        }

        this.messagingStub = this.settings.messagingStubFactory.createMessagingStub(address);
        if (this.messagingStub === undefined) {
            errorMsg = `No message receiver found for participantId: ${joynrMessage.to} queuing message.`;
            log.info(errorMsg, DiagnosticTags.forJoynrMessage(joynrMessage));
            // TODO queue message and retry later
            return Promise.resolve();
        }
        return this.messagingStub.transmit(joynrMessage).catch(this.routeInternalTransmitOnError);
    }

    private registerGlobalRoutingEntryIfRequired(joynrMessage: JoynrMessage): void {
        if (!joynrMessage.isReceivedFromGlobal) {
            return;
        }

        const type = joynrMessage.type;
        if (
            type === JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST ||
            type === JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
            type === JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST ||
            type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST
        ) {
            try {
                const replyToAddress = joynrMessage.replyChannelId;
                if (!UtilInternal.checkNullUndefined(replyToAddress)) {
                    // because the message is received via global transport, isGloballyVisible must be true
                    const isGloballyVisible = true;
                    this.addNextHop(
                        joynrMessage.from,
                        Typing.augmentTypes(JSON.parse(replyToAddress)),
                        isGloballyVisible
                    );
                }
            } catch (e) {
                log.error(`could not register global Routing Entry: ${e}`);
            }
        }
    }

    private resolveNextHopAndRoute(participantId: string, joynrMessage: JoynrMessage): Promise<void> {
        const address = this.getAddressFromPersistency(participantId);

        if (address === undefined) {
            if (this.routingProxy !== undefined) {
                return this.routingProxy
                    .resolveNextHop({
                        participantId
                    })
                    .then(opArgs => {
                        if (opArgs.resolved && this.parentMessageRouterAddress !== undefined) {
                            this.routingTable[participantId] = this.parentMessageRouterAddress;
                            return this.routeInternal(this.parentMessageRouterAddress, joynrMessage);
                        }
                        throw new Error(
                            `nextHop cannot be resolved, as participant with id ${participantId} is not reachable by parent routing table`
                        );
                    })
                    .catch((e: Error) => {
                        log.error(e.message);
                    });
            }
            if (
                joynrMessage.type === JoynrMessage.JOYNRMESSAGE_TYPE_REPLY ||
                joynrMessage.type === JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY ||
                joynrMessage.type === JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION
            ) {
                const errorMsg = `Received message for unknown proxy. Dropping the message. ID: ${joynrMessage.msgId}`;
                const now = Date.now();
                log.warn(`${errorMsg}, expiryDate: ${joynrMessage.expiryDate}, now: ${now}`);
                return Promise.resolve();
            }

            log.warn(
                `No message receiver found for participantId: ${joynrMessage.to}. Queuing message.`,
                DiagnosticTags.forJoynrMessage(joynrMessage)
            );

            // message is queued until the participant is registered
            this.settings.messageQueue.putMessage(joynrMessage);
            return Promise.resolve();
        }
        return this.routeInternal(address, joynrMessage);
    }

    private getAddressFromPersistency(participantId: string): Address | undefined {
        try {
            const addressString = this.persistency.getItem(this.getStorageKey(participantId));
            if (addressString === undefined || addressString === null || addressString === "{}") {
                this.persistency.removeItem(this.getStorageKey(participantId));
            } else {
                const address = Typing.augmentTypes(JSON.parse(addressString));
                this.routingTable[participantId] = address;
                return address;
            }
        } catch (error) {
            log.error(`Failed to get address from persisted routing entries for participant ${participantId}`);
        }
    }

    /**
     * @param newAddress - the address to be used as replyTo  address
     *
     * @returns void
     */
    public setReplyToAddress(newAddress: string): void {
        this.replyToAddress = newAddress;
        this.messageReplyToAddressCalculator.setReplyToAddress(this.replyToAddress);
        this.messagesWithoutReplyTo.forEach(this.route);
    }

    /**
     * @param participantId
     *
     * @returns the storage key
     */
    public getStorageKey(participantId: string): string {
        return `${this.id}_${participantId}`;
    }

    /**
     * @param participantId
     *
     * @returns promise
     */
    public removeNextHop(participantId: string): Promise<any> {
        if (!this.isReady()) {
            log.debug("removeNextHop: ignore call as message router is already shut down");
            return Promise.reject(new Error("message router is already shut down"));
        }

        delete this.routingTable[participantId];
        if (this.persistency) {
            this.persistency.removeItem(this.getStorageKey(participantId));
        }

        if (this.routingProxy !== undefined) {
            return this.routingProxy.removeNextHop({
                participantId
            });
        }
        if (this.parentMessageRouterAddress !== undefined) {
            const deferred = UtilInternal.createDeferred();
            this.queuedRemoveNextHopCalls.push({
                participantId,
                resolve: deferred.resolve,
                reject: deferred.reject
            });
            return deferred.promise;
        }
        return Promise.resolve();
    }

    /**
     * @param participantId
     * @param isGloballyVisible
     *
     * @returns promise
     */
    private addNextHopToParentRoutingTable(participantId: string, isGloballyVisible: boolean): Promise<any> {
        if (this.incomingAddress instanceof UdsClientAddress) {
            return this.routingProxy.addNextHop({
                participantId,
                udsClientAddress: this.incomingAddress,
                isGloballyVisible
            });
        }
        if (this.incomingAddress instanceof WebSocketClientAddress) {
            return this.routingProxy.addNextHop({
                participantId,
                webSocketClientAddress: this.incomingAddress,
                isGloballyVisible
            });
        }
        if (this.incomingAddress instanceof BrowserAddress) {
            return this.routingProxy.addNextHop({
                participantId,
                browserAddress: this.incomingAddress,
                isGloballyVisible
            });
        }
        if (this.incomingAddress instanceof WebSocketAddress) {
            return this.routingProxy.addNextHop({
                participantId,
                webSocketAddress: this.incomingAddress,
                isGloballyVisible
            });
        }
        if (this.incomingAddress instanceof UdsAddress) {
            return this.routingProxy.addNextHop({
                participantId,
                udsAddress: this.incomingAddress,
                isGloballyVisible
            });
        }
        if (this.incomingAddress instanceof ChannelAddress) {
            return this.routingProxy.addNextHop({
                participantId,
                channelAddress: this.incomingAddress,
                isGloballyVisible
            });
        }

        const errorMsg = `Invalid address type of incomingAddress: ${Typing.getObjectType(this.incomingAddress)}`;
        log.fatal(errorMsg);
        return Promise.reject(new JoynrRuntimeException({ detailMessage: errorMsg }));
    }

    /**
     * @param newRoutingProxy - the routing proxy to be set
     * @returns A+ promise object
     */
    public setRoutingProxy(newRoutingProxy: RoutingProxy): Promise<void> {
        this.routingProxy = newRoutingProxy;

        return this.addRoutingProxyToParentRoutingTable().then(() => this.processQueuedRoutingProxyCalls());
    }

    /**
     * Get replyToAddress from routing proxy
     * @returns A+ promise object
     */
    public getReplyToAddressFromRoutingProxy(): Promise<string> {
        if (!this.routingProxy) {
            return Promise.reject(new Error(`"setRoutingProxy()" has to be called first`));
        }
        return this.routingProxy.replyToAddress.get();
    }

    /**
     * Get replyToAddress via routing proxy to enable global communication in message router.
     * @returns A+ promise object
     */
    public configureReplyToAddressFromRoutingProxy(): Promise<void> {
        return this.getReplyToAddressFromRoutingProxy()
            .then(address => this.setReplyToAddress(address))
            .catch((error: Error) => {
                throw new Error(`Failed to get replyToAddress from parent router: ${error}`);
            });
    }

    /**
     * Looks up an Address for a given participantId (next hop)
     *
     * @param participantId
     * @returns the address of the next hop in the direction of the given
     *          participantId, or undefined if not found
     */
    public resolveNextHop(participantId: string): Promise<Address> {
        if (!this.isReady()) {
            log.debug("resolveNextHop: ignore call as message router is already shut down");
            return Promise.reject(new Error("message router is already shut down"));
        }

        const address = this.routingTable[participantId];

        if (address === undefined) {
            return this.resolveNextHopInternal(participantId);
        }

        return Promise.resolve(address);
    }

    /**
     * @param joynrMessage
     * @returns A+ promise object
     */
    public route(joynrMessage: JoynrMessage): Promise<any> {
        try {
            const now = Date.now();
            if (now > joynrMessage.expiryDate) {
                const errorMsg = `Received expired message. Dropping the message. ID: ${joynrMessage.msgId}`;
                log.warn(`${errorMsg}, expiryDate: ${joynrMessage.expiryDate}, now: ${now}`);
                return Promise.resolve();
            }
            log.debug(`Route message. ID: ${joynrMessage.msgId}, expiryDate: ${joynrMessage.expiryDate}, now: ${now}`);

            this.registerGlobalRoutingEntryIfRequired(joynrMessage);

            if (joynrMessage.type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST) {
                return Promise.all(
                    this.getAddressesForMulticast(joynrMessage).map((address: Address) =>
                        this.routeInternal(address, joynrMessage)
                    )
                );
            }

            const participantId = joynrMessage.to;
            const address = this.routingTable[participantId];
            if (address !== undefined) {
                return this.routeInternal(address, joynrMessage);
            }

            return this.resolveNextHopAndRoute(participantId, joynrMessage);
        } catch (e) {
            log.error(`MessageRouter.route failed: ${e.message}`);
            return Promise.resolve();
        }
    }

    /**
     * Registers the next hop with this specific participant Id
     *
     * @param participantId
     * @param address the address to register
     * @param isGloballyVisible
     * @returns A+ promise object
     */
    public addNextHop(participantId: string, address: Address, isGloballyVisible: boolean): Promise<void> {
        if (!this.isReady()) {
            log.debug("addNextHop: ignore call as message router is already shut down");
            return Promise.reject(new Error("message router is already shut down"));
        }
        // store the address of the participantId persistently
        this.routingTable[participantId] = address;
        const serializedAddress = JSONSerializer.stringify(address);
        let promise;
        if (serializedAddress === undefined || serializedAddress === null || serializedAddress === "{}") {
            log.info(
                `addNextHop: HOP address ${serializedAddress} will not be persisted for participant id: ${participantId}`
            );
        } else if (address._typeName !== InProcessAddress._typeName && this.persistency) {
            // only persist if it's not an InProcessAddress
            this.persistency.setItem(this.getStorageKey(participantId), serializedAddress);
        }

        if (this.routingProxy !== undefined) {
            // register remotely
            promise = this.addNextHopToParentRoutingTable(participantId, isGloballyVisible);
        } else {
            promise = Promise.resolve();
        }
        this.participantRegistered(participantId);
        return promise;
    }

    /**
     * Adds a new receiver for the identified multicasts.
     *
     * @param parameters - object containing parameters
     * @param parameters.multicastId
     * @param parameters.subscriberParticipantId
     * @param parameters.providerParticipantId
     * @returns A+ promise object
     */
    public addMulticastReceiver(parameters: {
        multicastId: string;
        subscriberParticipantId: string;
        providerParticipantId: string;
    }): Promise<void> {
        //1. handle call in local router
        //1.a store receiver in multicastReceiverRegistry
        const multicastIdPattern = this.multicastWildcardRegexFactory.createIdPattern(parameters.multicastId);
        const providerAddress = this.routingTable[parameters.providerParticipantId];

        if (this.multicastReceiversRegistry[multicastIdPattern] === undefined) {
            this.multicastReceiversRegistry[multicastIdPattern] = [];

            //1.b the first receiver for this multicastId -> inform MessagingSkeleton about receiver
            const skeleton = this.multicastSkeletons[providerAddress._typeName];
            if (skeleton !== undefined && skeleton.registerMulticastSubscription !== undefined) {
                skeleton.registerMulticastSubscription(parameters.multicastId);
            }
        }

        this.multicastReceiversRegistry[multicastIdPattern].push(parameters.subscriberParticipantId);

        //2. forward call to parent router (if available)
        if (
            this.parentMessageRouterAddress === undefined ||
            providerAddress === undefined ||
            providerAddress instanceof InProcessAddress
        ) {
            return Promise.resolve();
        }
        if (this.routingProxy !== undefined) {
            return this.routingProxy.addMulticastReceiver(parameters);
        }

        const deferred = UtilInternal.createDeferred();
        this.queuedAddMulticastReceiverCalls.push({
            parameters,
            resolve: deferred.resolve,
            reject: deferred.reject
        });

        return deferred.promise;
    }

    /**
     * Removes a receiver for the identified multicasts.
     *
     * @param parameters - object containing parameters
     * @param parameters.multicastId
     * @param parameters.subscriberParticipantId
     * @param parameters.providerParticipantId
     * @returns A+ promise object
     */
    public removeMulticastReceiver(parameters: {
        multicastId: string;
        subscriberParticipantId: string;
        providerParticipantId: string;
    }): Promise<void> {
        //1. handle call in local router
        //1.a remove receiver from multicastReceiverRegistry
        const multicastIdPattern = this.multicastWildcardRegexFactory.createIdPattern(parameters.multicastId);
        const providerAddress = this.routingTable[parameters.providerParticipantId];
        if (this.multicastReceiversRegistry[multicastIdPattern] !== undefined) {
            const receivers = this.multicastReceiversRegistry[multicastIdPattern];
            for (let i = 0; i < receivers.length; i++) {
                if (receivers[i] === parameters.subscriberParticipantId) {
                    receivers.splice(i, 1);
                    break;
                }
            }
            if (receivers.length === 0) {
                delete this.multicastReceiversRegistry[multicastIdPattern];

                //1.b no receiver anymore for this multicastId -> inform MessagingSkeleton about removed receiver
                const skeleton = this.multicastSkeletons[providerAddress._typeName];
                if (skeleton !== undefined && skeleton.unregisterMulticastSubscription !== undefined) {
                    skeleton.unregisterMulticastSubscription(parameters.multicastId);
                }
            }
        }

        //2. forward call to parent router (if available)
        if (
            this.parentMessageRouterAddress === undefined ||
            providerAddress === undefined ||
            providerAddress instanceof InProcessAddress
        ) {
            return Promise.resolve();
        }
        if (this.routingProxy !== undefined) {
            return this.routingProxy.removeMulticastReceiver(parameters);
        }

        const deferred = UtilInternal.createDeferred();
        this.queuedRemoveMulticastReceiverCalls.push({
            parameters,
            resolve: deferred.resolve,
            reject: deferred.reject
        });

        return deferred.promise;
    }

    /**
     * @param participantId
     *
     * @returns void
     */
    public participantRegistered(participantId: string): void {
        const messageQueue = this.settings.messageQueue.getAndRemoveMessages(participantId);

        if (messageQueue !== undefined) {
            let i = messageQueue.length;
            while (i--) {
                this.route(messageQueue[i]);
            }
        }
    }

    /**
     * Tell the message router that the given participantId is known. The message router
     * checks internally if an address is already present in the routing table. If not,
     * it adds the parentMessageRouterAddress to the routing table for this participantId.
     *
     * @param participantId
     *
     * @returns void
     */
    public setToKnown(participantId: string): void {
        if (!this.isReady()) {
            log.debug("setToKnown: ignore call as message router is already shut down");
            return;
        }

        //if not already set
        if (this.routingTable[participantId] === undefined) {
            if (this.parentMessageRouterAddress !== undefined) {
                this.routingTable[participantId] = this.parentMessageRouterAddress;
            }
        }
    }

    public hasMulticastReceivers(): boolean {
        return Object.keys(this.multicastReceiversRegistry).length > 0;
    }

    /**
     * Shutdown the message router
     */
    public shutdown(): void {
        function rejectCall(call: { reject: Function }): void {
            call.reject(new Error("Message Router has been shut down"));
        }

        if (this.queuedAddNextHopCalls !== undefined) {
            this.queuedAddNextHopCalls.forEach(rejectCall);
            this.queuedAddNextHopCalls = [];
        }
        if (this.queuedRemoveNextHopCalls !== undefined) {
            this.queuedRemoveNextHopCalls.forEach(rejectCall);
            this.queuedRemoveNextHopCalls = [];
        }
        if (this.queuedAddMulticastReceiverCalls !== undefined) {
            this.queuedAddMulticastReceiverCalls.forEach(rejectCall);
            this.queuedAddMulticastReceiverCalls = [];
        }
        if (this.queuedRemoveMulticastReceiverCalls !== undefined) {
            this.queuedRemoveMulticastReceiverCalls.forEach(rejectCall);
            this.queuedRemoveMulticastReceiverCalls = [];
        }
        this.started = false;
        this.settings.messageQueue.shutdown();
    }
}

export = MessageRouter;
