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
import * as DiscoveryEntryWithMetaInfo from "../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import * as MessagingQos from "../messaging/MessagingQos";
import * as Request from "./types/Request";

import * as Reply from "./types/Reply";
import * as OneWayRequest from "./types/OneWayRequest";
import BroadcastSubscriptionRequest from "./types/BroadcastSubscriptionRequest";
import MulticastSubscriptionRequest from "./types/MulticastSubscriptionRequest";
import SubscriptionRequest from "./types/SubscriptionRequest";
import SubscriptionReply from "./types/SubscriptionReply";
import SubscriptionStop from "./types/SubscriptionStop";
import * as SubscriptionPublication from "./types/SubscriptionPublication";
import * as MulticastPublication from "./types/MulticastPublication";
import JoynrMessage from "../messaging/JoynrMessage";
import * as MessagingQosEffort from "../messaging/MessagingQosEffort";
import defaultMessagingSettings from "../start/settings/defaultMessagingSettings";
import * as DiagnosticTags from "../system/DiagnosticTags";
import * as UtilInternal from "../util/UtilInternal";
import * as JSONSerializer from "../util/JSONSerializer";
import * as Typing from "../util/Typing";
import SubscriptionQos from "../proxy/SubscriptionQos";
import LoggingManager from "../system/LoggingManager";
import PlatformSecurityManagerNode = require("../security/PlatformSecurityManagerNode");
import InProcessMessagingStub = require("../messaging/inprocess/InProcessMessagingStub");
import MessageRouter = require("../messaging/routing/MessageRouter");
import RequestReplyManager = require("./RequestReplyManager");
import SubscriptionManager = require("./subscription/SubscriptionManager");
import PublicationManager = require("./subscription/PublicationManager");
const log = LoggingManager.getLogger("joynr.dispatching.Dispatcher");

class Dispatcher {
    private ttlUpLiftMs?: number;
    private securityManager: PlatformSecurityManagerNode;
    private clusterControllerMessagingStub: InProcessMessagingStub;
    private messageRouter!: MessageRouter;
    private publicationManager!: PublicationManager;
    private subscriptionManager!: SubscriptionManager;
    private requestReplyManager!: RequestReplyManager;
    /**
     * @constructor
     *
     * @param clusterControllerMessagingStub for sending outgoing joynr messages
     * @param securityManager for setting the creator user ID header
     * @param ttlUpLiftMs
     */
    public constructor(
        clusterControllerMessagingStub: InProcessMessagingStub,
        securityManager: PlatformSecurityManagerNode,
        ttlUpLiftMs?: number
    ) {
        this.clusterControllerMessagingStub = clusterControllerMessagingStub;
        this.securityManager = securityManager;
        this.ttlUpLiftMs = ttlUpLiftMs;
        this.sendSubscriptionReply = this.sendSubscriptionReply.bind(this);
        // bind due to InProcessMessagingSkeleton using receive as callback.
        this.receive = this.receive.bind(this);
    }

    /**
     * @param expiryDate the expiry date in milliseconds
     * @returns the expiry date with TTL_UPLIFT added as time delta
     */
    private upLiftTtl(expiryDate: number): number {
        expiryDate += this.ttlUpLiftMs !== undefined ? this.ttlUpLiftMs : defaultMessagingSettings.TTL_UPLIFT;
        if (expiryDate > UtilInternal.getMaxLongValue()) {
            expiryDate = UtilInternal.getMaxLongValue();
        }

        return expiryDate;
    }

    /**
     * @param subscriptionRequest the subscription request
     * @returns the subscription request with qos.expiry date with TTL_UPLIFT added as time delta
     */
    private upLiftExpiryDateInSubscriptionRequest<T extends SubscriptionRequest>(subscriptionRequest: T): T {
        // if expiryDateMs == SubscriptionQos.NO_EXPIRY_DATE (=0), expiryDateMs must not be changed
        if (subscriptionRequest.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE) {
            subscriptionRequest.qos.expiryDateMs = this.upLiftTtl(subscriptionRequest.qos.expiryDateMs);
        }
        return subscriptionRequest;
    }

    /**
     * @param joynrMessage
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @returns Promise resolved on success
     */
    private sendJoynrMessage(
        joynrMessage: JoynrMessage,
        settings: {
            from: string;
            toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
            messagingQos: MessagingQos;
        }
    ): Promise<void> {
        // set headers
        joynrMessage.creator = this.securityManager.getCurrentProcessUserId();
        joynrMessage.from = settings.from;
        joynrMessage.to = settings.toDiscoveryEntry.participantId;
        joynrMessage.expiryDate = this.upLiftTtl(Date.now() + settings.messagingQos.ttl);
        const effort = settings.messagingQos.effort;
        if (effort !== MessagingQosEffort.NORMAL) {
            joynrMessage.effort = effort.value;
        }
        if (settings.messagingQos.compress) {
            joynrMessage.compress = true;
        }

        joynrMessage.isLocalMessage = settings.toDiscoveryEntry.isLocal;

        if (log.isDebugEnabled()) {
            log.debug(`sendJoynrMessage, message = ${JSON.stringify(joynrMessage)}`);
        }
        // send message
        return this.clusterControllerMessagingStub.transmit(joynrMessage);
    }

    private getJoynrMessageType(subscriptionRequest: SubscriptionRequest): string {
        const type = Typing.getObjectType(subscriptionRequest);
        switch (type) {
            case "BroadcastSubscriptionRequest":
                return JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
            case "MulticastSubscriptionRequest":
                return JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
            default:
                return JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        }
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.to participantId of the receiver
     * @param settings.expiryDate time-to-live
     * @param settings.customHeaders custom headers from request
     * @param settings.messageType
     * @param settings.effort
     * @param settings.compress
     * @param settings.reply the reply to be transmitted. It can either be a Reply or a SubscriptionReply object
     * @returns A+ promise object
     */
    private sendReply(settings: {
        from: string;
        to: string;
        expiryDate: number;
        customHeaders: Record<string, any>;
        messageType: string;
        effort?: MessagingQosEffort.effort;
        compress?: boolean;
        reply: string;
    }): Promise<void> {
        // reply with the result in a JoynrMessage
        const joynrMessage = new JoynrMessage({
            type: settings.messageType,
            payload: settings.reply
        });
        joynrMessage.from = settings.from;
        joynrMessage.to = settings.to;

        joynrMessage.expiryDate = settings.expiryDate;

        // set custom headers
        joynrMessage.setCustomHeaders(settings.customHeaders);

        if (settings.effort && settings.effort !== MessagingQosEffort.NORMAL) {
            joynrMessage.effort = settings.effort.value;
        }

        if (settings.compress) {
            joynrMessage.compress = true;
        }

        if (log.isDebugEnabled()) {
            log.debug(`sendReply, message = ${JSON.stringify(joynrMessage)}`);
        }
        return this.clusterControllerMessagingStub.transmit(joynrMessage);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.to participantId of the receiver
     * @param settings.expiryDate time-to-live
     * @param settings.customHeaders custom headers from request
     * @param settings.messageType
     * @param settings.effort
     * @param settings.compress
     * @param reply
     * @returns A+ promise object
     */
    private sendRequestReply(
        settings: {
            from: string;
            to: string;
            expiryDate: number;
            customHeaders: Record<string, any>;
            effort: MessagingQosEffort.effort;
            compress: boolean;
        },
        reply: Reply.Reply
    ): Promise<void> {
        const toParticipantId = settings.to;
        log.info(
            "replying",
            DiagnosticTags.forReply({
                reply,
                to: toParticipantId,
                from: settings.from
            })
        );
        type Settings = typeof settings & { reply: string; messageType: string };
        (settings as Settings).reply = JSONSerializer.stringifyOptional(reply, reply.error !== undefined);
        (settings as Settings).messageType = JoynrMessage.JOYNRMESSAGE_TYPE_REPLY;
        return this.sendReply(settings as Settings);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.to participantId of the receiver
     * @param settings.expiryDate time-to-live
     * @param settings.customHeaders custom headers from request
     * @param subscriptionReply
     */
    private sendSubscriptionReply(
        settings: {
            from: string;
            to: string;
            expiryDate: number;
            customHeaders: Record<string, any>;
        },
        subscriptionReply: SubscriptionReply
    ): void {
        const toParticipantId = settings.to;
        log.info(
            "replying",
            DiagnosticTags.forSubscriptionReply({
                subscriptionReply,
                to: toParticipantId,
                from: settings.from
            })
        );

        type Settings = typeof settings & { reply: string; messageType: string };
        (settings as Settings).reply = JSONSerializer.stringifyOptional(
            subscriptionReply,
            subscriptionReply.error !== undefined
        );
        (settings as Settings).messageType = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY;
        this.sendReply(settings as Settings);
    }

    private sendPublicationInternal(
        settings: {
            from: string;
            to: string;
            expiryDate: number;
        },
        type: string,
        publication: SubscriptionPublication.SubscriptionPublication
    ): void {
        // create JoynrMessage for the publication
        const publicationMessage = new JoynrMessage({
            type,
            payload: JSONSerializer.stringify(publication)
        });

        // set reply headers
        const toParticipantId = settings.to;
        publicationMessage.from = settings.from;
        publicationMessage.to = toParticipantId;
        publicationMessage.expiryDate = this.upLiftTtl(settings.expiryDate);

        if (log.isDebugEnabled()) {
            log.debug(`sendPublicationInternal, message = ${JSON.stringify(publicationMessage)}`);
        }
        this.clusterControllerMessagingStub.transmit(publicationMessage);
    }

    private createReplySettings(
        joynrMessage: JoynrMessage
    ): {
        from: string;
        to: string;
        expiryDate: number;
        customHeaders: Record<string, any>;
        effort?: MessagingQosEffort.effort;
        compress?: boolean;
    } {
        return {
            from: joynrMessage.to,
            to: joynrMessage.from,
            expiryDate: joynrMessage.expiryDate,
            customHeaders: joynrMessage.getCustomHeaders()
        };
    }

    /**
     * @param newRequestReplyManager handles incoming and outgoing requests and replies
     */
    public registerRequestReplyManager(newRequestReplyManager: RequestReplyManager): void {
        this.requestReplyManager = newRequestReplyManager;
    }

    /**
     * @param newMessageRouter
     */
    public registerMessageRouter(newMessageRouter: MessageRouter): void {
        this.messageRouter = newMessageRouter;
    }

    /**
     * @param newSubscriptionManager sends subscription requests; handles incoming publications
     * and incoming replies to subscription requests
     */
    public registerSubscriptionManager(newSubscriptionManager: SubscriptionManager): void {
        this.subscriptionManager = newSubscriptionManager;
    }

    /**
     * @param newPublicationManager sends publications; handles incoming subscription start and stop requests
     */
    public registerPublicationManager(newPublicationManager: PublicationManager): void {
        this.publicationManager = newPublicationManager;
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @param settings.request
     * @returns A+ promise object
     */
    public sendRequest(settings: {
        from: string;
        toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        messagingQos: MessagingQos;
        request: Request.Request;
    }): Promise<void> {
        // Create a JoynrMessage with the Request
        const requestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSONSerializer.stringify(settings.request)
        });
        if (settings.messagingQos.customHeaders) {
            requestMessage.setCustomHeaders(settings.messagingQos.customHeaders);
        }

        log.info(
            `calling ${settings.request.methodName}.`,
            DiagnosticTags.forRequest({
                request: settings.request,
                to: settings.toDiscoveryEntry.participantId,
                from: settings.from
            })
        );

        return this.sendJoynrMessage(requestMessage, settings);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @param settings.request
     * @returns A+ promise object
     */
    public sendOneWayRequest(settings: {
        from: string;
        toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        messagingQos: MessagingQos;
        request: OneWayRequest.OneWayRequest;
    }): Promise<void> {
        // Create a JoynrMessage with the OneWayRequest
        const oneWayRequestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
            payload: JSONSerializer.stringify(settings.request)
        });
        if (settings.messagingQos.customHeaders) {
            oneWayRequestMessage.setCustomHeaders(settings.messagingQos.customHeaders);
        }
        log.info(
            `calling ${settings.request.methodName}.`,
            DiagnosticTags.forOneWayRequest({
                request: settings.request,
                to: settings.toDiscoveryEntry.participantId,
                from: settings.from
            })
        );

        return this.sendJoynrMessage(oneWayRequestMessage, settings);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @param settings.subscriptionRequest
     * @returns  promise object that is resolved when the request is sent by the messaging stub
     */
    public sendSubscriptionRequest(settings: {
        from: string;
        toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        messagingQos: MessagingQos;
        subscriptionRequest: SubscriptionRequest;
    }): Promise<void> {
        log.info(
            `subscription to ${settings.subscriptionRequest.subscribedToName}`,
            DiagnosticTags.forSubscriptionRequest({
                subscriptionRequest: settings.subscriptionRequest,
                to: settings.toDiscoveryEntry.participantId,
                from: settings.from
            })
        );

        const requestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            payload: JSONSerializer.stringify(settings.subscriptionRequest)
        });

        return this.sendJoynrMessage(requestMessage, settings);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @param settings.subscriptionRequest
     * @returns  promise object that is resolved when the request is sent by the messaging stub
     */
    public sendBroadcastSubscriptionRequest(settings: {
        from: string;
        toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        messagingQos: MessagingQos;
        subscriptionRequest: MulticastSubscriptionRequest | BroadcastSubscriptionRequest;
    }): Promise<void> {
        const type = this.getJoynrMessageType(settings.subscriptionRequest);

        const requestMessage = new JoynrMessage({
            type,
            payload: JSONSerializer.stringify(settings.subscriptionRequest)
        });

        if (type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST) {
            log.info(
                `multicast subscription to ${settings.subscriptionRequest.subscribedToName}`,
                DiagnosticTags.forMulticastSubscriptionRequest({
                    subscriptionRequest: settings.subscriptionRequest,
                    to: settings.toDiscoveryEntry.participantId,
                    from: settings.from
                })
            );
            return this.messageRouter.addMulticastReceiver({
                multicastId: (settings.subscriptionRequest as MulticastSubscriptionRequest).multicastId,
                subscriberParticipantId: settings.from,
                providerParticipantId: settings.toDiscoveryEntry.participantId
            });
        }
        log.info(
            `broadcast subscription to ${settings.subscriptionRequest.subscribedToName}`,
            DiagnosticTags.forBroadcastSubscriptionRequest({
                subscriptionRequest: settings.subscriptionRequest,
                to: settings.toDiscoveryEntry.participantId,
                from: settings.from
            })
        );
        return this.sendJoynrMessage(requestMessage, settings);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.multicastId of the multicast
     * @param settings.subscriptionStop
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @returns A+ promise object
     */
    public sendMulticastSubscriptionStop(settings: {
        from: string;
        toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        multicastId: string;
        subscriptionStop: SubscriptionStop;
        messagingQos: MessagingQos;
    }): Promise<void> {
        this.sendSubscriptionStop(settings);
        return this.messageRouter.removeMulticastReceiver({
            multicastId: settings.multicastId,
            subscriberParticipantId: settings.from,
            providerParticipantId: settings.toDiscoveryEntry.participantId
        });
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.subscriptionStop
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @returns A+ promise object
     */
    public sendSubscriptionStop(settings: {
        from: string;
        toDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        subscriptionStop: SubscriptionStop;
        messagingQos: MessagingQos;
    }): Promise<void> {
        log.info(
            `subscription stop ${settings.subscriptionStop.subscriptionId}`,
            DiagnosticTags.forSubscriptionStop({
                subscriptionId: settings.subscriptionStop.subscriptionId,
                to: settings.toDiscoveryEntry.participantId,
                from: settings.from
            })
        );

        const message = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
            payload: JSONSerializer.stringify(settings.subscriptionStop)
        });
        return this.sendJoynrMessage(message, settings);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.to participantId of the receiver
     * @param settings.expiryDate time-to-live
     * @param publication
     * @param [publication.response]
     * @param [publication.error]
     * @param publication.subscriptionId
     */
    public sendPublication(
        settings: {
            from: string;
            to: string;
            expiryDate: number;
        },
        publication: SubscriptionPublication.SubscriptionPublication
    ): void {
        log.info(
            "publication",
            DiagnosticTags.forPublication({
                publication,
                to: settings.to,
                from: settings.from
            })
        );

        this.sendPublicationInternal(settings, JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION, publication);
    }

    /**
     * @param settings
     * @param settings.from participantId of the sender
     * @param settings.multicastName name of multicast
     * @param {String[]} partitions partitions for this multicast
     * @param settings.expiryDate time-to-live
     * @param publication
     * @param [publication.response]
     * @param [publication.error]
     * @param publication.multicastId
     */
    public sendMulticastPublication(
        settings: {
            from: string;
            multicastName?: string;
            expiryDate: number;
        },
        publication: {
            response?: any;
            error?: any;
            multicastId: string;
        }
    ): void {
        const multicastId = publication.multicastId;
        log.info(
            "publication",
            DiagnosticTags.forMulticastPublication({
                publication,
                from: settings.from
            })
        );

        // Reply with the result in a JoynrMessage
        const publicationMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
            payload: JSONSerializer.stringify(publication)
        });
        publicationMessage.from = settings.from;
        publicationMessage.to = multicastId;
        publicationMessage.expiryDate = this.upLiftTtl(settings.expiryDate);

        if (log.isDebugEnabled()) {
            log.debug(`sendMulticastPublication, message = ${JSON.stringify(publicationMessage)}`);
        }
        this.clusterControllerMessagingStub.transmit(publicationMessage);
    }

    /**
     * receives a new JoynrMessage that has to be routed to one of the managers
     *
     * @param joynrMessage being routed
     */
    public receive(joynrMessage: JoynrMessage): Promise<void> {
        log.debug(`received message with id "${joynrMessage.msgId}"`);
        if (log.isDebugEnabled()) {
            log.debug(`receive, message = ${JSON.stringify(joynrMessage)}`);
        }
        let payload;
        try {
            payload = JSON.parse(joynrMessage.payload);
        } catch (error) {
            log.error(`error parsing joynrMessage: ${error} payload: ${joynrMessage.payload}`);
            return Promise.resolve();
        }

        switch (joynrMessage.type) {
            case JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST:
                try {
                    const request = Request.create(payload);
                    log.info(
                        `received request for ${request.methodName}.`,
                        DiagnosticTags.forRequest({
                            request,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );

                    const handleReplySettings = this.createReplySettings(joynrMessage);

                    const effort = joynrMessage.effort;
                    if (effort && MessagingQosEffort[effort]) {
                        handleReplySettings.effort = MessagingQosEffort[effort];
                    }
                    if (joynrMessage.compress) {
                        handleReplySettings.compress = true;
                    }

                    return this.requestReplyManager.handleRequest(
                        joynrMessage.to,
                        request,
                        (handleReplySettings: any, reply: Reply.Reply) =>
                            this.sendRequestReply(handleReplySettings, reply),
                        handleReplySettings
                    );
                } catch (errorInRequest) {
                    // TODO handle error in handling the request
                    log.error(`error handling request: ${errorInRequest}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_REPLY:
                try {
                    const reply = Reply.create(payload);
                    log.info(
                        "received reply ",
                        DiagnosticTags.forReply({
                            reply,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );
                    this.requestReplyManager.handleReply(reply);
                } catch (errorInReply) {
                    // TODO handle error in handling the reply
                    log.error(`error handling reply: ${errorInReply}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY:
                try {
                    const oneWayRequest = OneWayRequest.create(payload);
                    log.info(
                        `received one way request for ${oneWayRequest.methodName}.`,
                        DiagnosticTags.forOneWayRequest({
                            request: oneWayRequest,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );
                    this.requestReplyManager.handleOneWayRequest(joynrMessage.to, oneWayRequest);
                } catch (errorInOneWayRequest) {
                    log.error(`error handling one way: ${errorInOneWayRequest}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST:
                try {
                    const subscriptionRequest = this.upLiftExpiryDateInSubscriptionRequest(
                        new SubscriptionRequest(payload)
                    );
                    log.info(
                        `received subscription to ${subscriptionRequest.subscribedToName}`,
                        DiagnosticTags.forSubscriptionRequest({
                            subscriptionRequest,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );

                    this.publicationManager.handleSubscriptionRequest(
                        joynrMessage.from,
                        joynrMessage.to,
                        subscriptionRequest,
                        this.sendSubscriptionReply,
                        this.createReplySettings(joynrMessage)
                    );
                } catch (errorInSubscriptionRequest) {
                    // TODO handle error in handling the subscriptionRequest
                    log.error(`error handling subscriptionRequest: ${errorInSubscriptionRequest}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST:
                try {
                    const broadcastSubscriptionRequest = this.upLiftExpiryDateInSubscriptionRequest(
                        new BroadcastSubscriptionRequest(payload)
                    );
                    log.info(
                        `received broadcast subscription to ${broadcastSubscriptionRequest.subscribedToName}`,
                        DiagnosticTags.forBroadcastSubscriptionRequest({
                            subscriptionRequest: broadcastSubscriptionRequest,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );

                    this.publicationManager.handleBroadcastSubscriptionRequest(
                        joynrMessage.from,
                        joynrMessage.to,
                        broadcastSubscriptionRequest,
                        this.sendSubscriptionReply,
                        this.createReplySettings(joynrMessage)
                    );
                } catch (errorInBroadcastSubscriptionRequest) {
                    // TODO handle error in handling the subscriptionRequest
                    log.error(`error handling broadcastSubscriptionRequest: ${errorInBroadcastSubscriptionRequest}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST:
                try {
                    const multicastSubscriptionRequest = this.upLiftExpiryDateInSubscriptionRequest(
                        new MulticastSubscriptionRequest(payload)
                    );
                    log.info(
                        `received broadcast subscription to ${multicastSubscriptionRequest.subscribedToName}`,
                        DiagnosticTags.forMulticastSubscriptionRequest({
                            subscriptionRequest: multicastSubscriptionRequest,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );

                    this.publicationManager.handleMulticastSubscriptionRequest(
                        joynrMessage.from,
                        joynrMessage.to,
                        multicastSubscriptionRequest,
                        this.sendSubscriptionReply,
                        this.createReplySettings(joynrMessage)
                    );
                } catch (errorInMulticastSubscriptionRequest) {
                    // TODO handle error in handling the subscriptionRequest
                    log.error(`error handling multicastSubscriptionRequest: ${errorInMulticastSubscriptionRequest}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY:
                try {
                    const subscriptionReply = new SubscriptionReply(payload);
                    log.info(
                        "received subscription reply",
                        DiagnosticTags.forSubscriptionReply({
                            subscriptionReply,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );
                    this.subscriptionManager.handleSubscriptionReply(subscriptionReply);
                } catch (errorInSubscriptionReply) {
                    // TODO handle error in handling the subscriptionReply
                    log.error(`error handling subscriptionReply: ${errorInSubscriptionReply}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP:
                try {
                    const subscriptionStop = new SubscriptionStop(payload);
                    log.info(
                        `received subscription stop ${subscriptionStop.subscriptionId}`,
                        DiagnosticTags.forSubscriptionStop({
                            subscriptionId: subscriptionStop.subscriptionId,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );
                    this.publicationManager.handleSubscriptionStop(subscriptionStop);
                } catch (errorInSubscriptionStop) {
                    // TODO handle error in handling the subscriptionStop
                    log.error(`error handling subscriptionStop: ${errorInSubscriptionStop}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION:
                try {
                    const subscriptionPublication = SubscriptionPublication.create(payload);
                    log.info(
                        "received publication",
                        DiagnosticTags.forPublication({
                            publication: subscriptionPublication,
                            to: joynrMessage.to,
                            from: joynrMessage.from
                        })
                    );
                    this.subscriptionManager.handlePublication(subscriptionPublication);
                } catch (errorInPublication) {
                    // TODO handle error in handling the publication
                    log.error(`error handling publication: ${errorInPublication}`);
                }
                break;

            case JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST:
                try {
                    const multicastPublication = MulticastPublication.create(payload);
                    log.info(
                        "received publication",
                        DiagnosticTags.forMulticastPublication({
                            publication: multicastPublication,
                            from: joynrMessage.from
                        })
                    );
                    this.subscriptionManager.handleMulticastPublication(multicastPublication);
                } catch (errorInMulticastPublication) {
                    // TODO handle error in handling the multicast publication
                    log.error(`error handling multicast publication: ${errorInMulticastPublication}`);
                }
                break;

            default:
                log.error(
                    `unknown JoynrMessage type : ${joynrMessage.type}. Discarding message: ${JSONSerializer.stringify(
                        joynrMessage
                    )}`
                );
                break;
        }
        return Promise.resolve();
    }

    /**
     * Shutdown the dispatcher
     */
    public shutdown(): void {
        log.debug("Dispatcher shut down");
        /* do nothing, as either the managers on the layer above (RRM, PM, SM) or
         * the message router on the layer below are implementing the
         * correct handling when the runtime is shut down
         */
    }
}

export = Dispatcher;
