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
import * as DiscoveryEntryWithMetaInfo from "../../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import MessagingQos from "../../messaging/MessagingQos";

import MulticastWildcardRegexFactory from "../../messaging/util/MulticastWildcardRegexFactory";
import defaultMessagingSettings from "../../start/settings/defaultMessagingSettings";
import SubscriptionQos from "../../proxy/SubscriptionQos";
import { MulticastPublication } from "../types/MulticastPublication";
import { SubscriptionPublication } from "../types/SubscriptionPublication";
import SubscriptionStop from "../types/SubscriptionStop";
import SubscriptionRequest from "../types/SubscriptionRequest";
import MulticastSubscriptionRequest from "../types/MulticastSubscriptionRequest";
import BroadcastSubscriptionRequest from "../types/BroadcastSubscriptionRequest";
import SubscriptionListener from "./SubscriptionListener";
import * as SubscriptionUtil from "./util/SubscriptionUtil";
import LongTimer from "../../util/LongTimer";
import LoggingManager from "../../system/LoggingManager";
import { nanoid } from "nanoid";
import * as UtilInternal from "../../util/UtilInternal";
import * as Typing from "../../util/Typing";
import PublicationMissedException from "../../exceptions/PublicationMissedException";
import * as JSONSerializer from "../../util/JSONSerializer";
import util from "util";
import Dispatcher = require("../Dispatcher");
import SubscriptionReply = require("../types/SubscriptionReply");
import BroadcastFilterParameters = require("../../proxy/BroadcastFilterParameters");
import MulticastSubscriptionQos = require("../../proxy/MulticastSubscriptionQos");
import OnChangeSubscriptionQos = require("../../proxy/OnChangeSubscriptionQos");
import OnChangeWithKeepAliveSubscriptionQos = require("../../proxy/OnChangeWithKeepAliveSubscriptionQos");

const log = LoggingManager.getLogger("joynr.dispatching.subscription.SubscriptionManager");

interface SubscriptionSettings {
    proxyId: string;
    providerDiscoveryEntry: DiscoveryEntryWithMetaInfo;
    attributeType: string;
    attributeName: string;
    qos: SubscriptionQos;
    subscriptionId?: string;
    onReceive: (value: any) => void;
    onError?: (e: Error) => void;
    onSubscribed?: (participantId: string) => void;
}

interface BroadcastSubscriptionSettings {
    proxyId: string;
    providerDiscoveryEntry: DiscoveryEntryWithMetaInfo;
    broadcastName: string;
    broadcastParameter: { name: string; type: string }[];
    subscriptionQos: SubscriptionQos;
    filterParameters: BroadcastFilterParameters;
    selective?: boolean;
    partitions: string[];
    subscriptionId?: string;
    onReceive: Function;
    onSubscribed?: Function;
    onError?: Function;
}

class SubscriptionManager {
    private dispatcher: Dispatcher;

    /**
     * @name SubscriptionManager#registerBroadcastSubscription
     * @function
     * @param parameters
     * @param parameters.proxyId participantId of the sender
     * @param parameters.providerDiscoveryEntry DiscoveryEntry of the receiver
     * @param parameters.broadcastName the name of the broadcast being subscribed to
     * @param parameters.broadcastParameter the parameter meta information of the broadcast being subscribed to
     * @param [parameters.subscriptionQos] the subscriptionQos
     * @param [parameters.filterParameters] filter parameters used to indicate interest in
     *            only a subset of broadcasts that might be sent.
     * @param parameters.selective true if broadcast is selective
     * @param [parameters.partitions] partitions for multicast requests
     * @param parameters.subscriptionId optional parameter subscriptionId to reuse a
     *            pre-existing identifier for this concrete subscription request
     * @param parameters.onReceive is called when a broadcast is received.
     * @param parameters.onError is called when an error occurs with the broadcast
     * @param parameters.onSubscribed the callback to inform once the subscription request has
     *            been delivered successfully
     * @returns a promise object which provides the subscription token upon success and an error
     *          upon failure
     */
    public registerBroadcastSubscription: (parameters: BroadcastSubscriptionSettings) => Promise<string>;

    /**
     * This callback is called when a publication is received
     * @callback SubscriptionManager~onReceive
     * @param {Object} publication received
     */
    /**
     * This callback is called if there was an error with the subscription
     * @callback SubscriptionManager~onError
     * @param {Error} error
     */

    /**
     * @name SubscriptionManager#registerSubscription
     * @function
     * @param settings
     * @param settings.proxyId participantId of the sender
     * @param settings.providerDiscoveryEntry DiscoveryEntry of the receiver
     * @param settings.attributeType the type of the subscribing attribute
     * @param settings.attributeName the attribute name to subscribe to
     * @param [settings.qos] the subscriptionQos
     * @param settings.subscriptionId optional parameter subscriptionId to reuse a
     *            preexisting identifier for this concrete subscription request
     * @param settings.onReceive the callback for received publications.
     * @param settings.onError the callback for missing publication alerts or when an
     *            error occurs.
     * @param settings.onSubscribed the callback to inform once the subscription request has
     *            been delivered successfully
     * @returns an A promise object which provides the subscription token upon success and
     *          an error upon failure
     */
    public registerSubscription: (settings: SubscriptionSettings) => Promise<string>;

    private multicastSubscribers: any = {};
    private started: boolean = true;
    private subscriptionReplyCallers: any;

    // stores the object which is returned by setTimeout mapped to the subscriptionId
    private publicationCheckTimerIds: any = {};

    // stores subscriptionId - SubscriptionListener
    private subscriptionListeners: any = {};

    // stores subscriptionId - SubscriptionInfo pairs
    private subscriptionInfos: any = {};
    private multicastWildcardRegexFactory: any;
    /**
     * @constructor
     * @param dispatcher
     */
    public constructor(dispatcher: Dispatcher) {
        this.multicastWildcardRegexFactory = new MulticastWildcardRegexFactory();

        this.subscriptionReplyCallers = new Map();
        this.registerSubscription = util.promisify(this.registerSubscriptionInternal);

        this.registerBroadcastSubscription = util.promisify(this.registerBroadcastSubscriptionInternal);

        this.dispatcher = dispatcher;
        this.checkPublication = this.checkPublication.bind(this);
    }

    private isReady(): boolean {
        return this.started;
    }

    /**
     * @param subscriptionId Id of the subscription to check
     * @returns time of last received publication
     */
    private getLastPublicationTime(subscriptionId: string): number {
        return this.subscriptionInfos[subscriptionId].lastPublicationTime_ms;
    }

    private setLastPublicationTime(subscriptionId: string, timeMs: number): void {
        // eslint-disable-next-line @typescript-eslint/camelcase
        this.subscriptionInfos[subscriptionId].lastPublicationTime_ms = timeMs;
    }

    /**
     * @param subscriptionId Id of the subscription to check
     * @param delayMs Delay to the next publication check.
     * @returns true if subscription is expired, false if end date is not reached.
     */
    private subscriptionEnds(subscriptionId: string, delayMs: number): boolean {
        if (this.subscriptionInfos[subscriptionId] === undefined) {
            log.warn(`subscriptionEnds has been called with unresolved subscriptionId "${subscriptionId}"`);
            return true;
        }
        const expiryDateMs = this.subscriptionInfos[subscriptionId].qos.expiryDateMs;
        // log.debug("Checking subscription end for subscriptionId: " + subscriptionId + "
        // expiryDateMs: " + expiryDateMs + "
        // current time: " + Date.now());
        const ends = expiryDateMs <= Date.now() + delayMs;
        // if (ends === true) {
        // log.info("Subscription end date reached for id: " + subscriptionId);
        // }
        return ends;
    }

    /**
     * @param subscriptionId Id of the subscription to check
     * @param alertAfterIntervalMs maximum delay between two incoming publications
     */
    private checkPublication(subscriptionId: string, alertAfterIntervalMs: number): void {
        const subscriptionListener = this.subscriptionListeners[subscriptionId];
        const timeSinceLastPublication = Date.now() - this.getLastPublicationTime(subscriptionId);
        // log.debug("timeSinceLastPublication : " + timeSinceLastPublication + "
        // alertAfterIntervalMs: " + alertAfterIntervalMs);
        if (alertAfterIntervalMs > 0 && timeSinceLastPublication >= alertAfterIntervalMs) {
            // log.warn("publication missed for subscription id: " + subscriptionId);
            if (subscriptionListener.onError) {
                const publicationMissedException = new PublicationMissedException({
                    detailMessage: "alertAfterIntervalMs period exceeded without receiving publication",
                    subscriptionId
                });
                subscriptionListener.onError(publicationMissedException);
            }
        }

        let delayMs;
        if (timeSinceLastPublication > alertAfterIntervalMs) {
            delayMs = alertAfterIntervalMs;
        } else {
            delayMs = alertAfterIntervalMs - timeSinceLastPublication;
        }

        if (!this.subscriptionEnds(subscriptionId, delayMs)) {
            // log.debug("Rescheduling checkPublication with delay: " + delay_ms);
            this.publicationCheckTimerIds[subscriptionId] = LongTimer.setTimeout(
                this.checkPublication,
                delayMs,
                subscriptionId,
                alertAfterIntervalMs
            );
        }
    }

    private calculateTtl(subscriptionQos: SubscriptionQos): number {
        if (subscriptionQos.expiryDateMs === SubscriptionQos.NO_EXPIRY_DATE) {
            return defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
        }
        const ttl = subscriptionQos.expiryDateMs - Date.now();
        if (ttl > defaultMessagingSettings.MAX_MESSAGING_TTL_MS) {
            return defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
        }
        return ttl;
    }

    private storeSubscriptionRequest(
        settings: BroadcastSubscriptionSettings,
        subscriptionRequest: BroadcastSubscriptionRequest | MulticastSubscriptionRequest
    ): void;
    private storeSubscriptionRequest(settings: SubscriptionSettings, subscriptionRequest: SubscriptionRequest): void;
    private storeSubscriptionRequest(
        settings: BroadcastSubscriptionSettings & SubscriptionSettings,
        subscriptionRequest: SubscriptionRequest & BroadcastSubscriptionRequest & MulticastSubscriptionRequest
    ): void {
        let onReceiveWrapper;
        if (settings.attributeType !== undefined) {
            onReceiveWrapper = (response: any[]) => {
                settings.onReceive(Typing.augmentTypes(response[0], settings.attributeType));
            };
        } else {
            onReceiveWrapper = (response: any[]) => {
                for (const responseKey in response) {
                    if (response.hasOwnProperty(responseKey)) {
                        response[responseKey] = Typing.augmentTypes(
                            response[responseKey],
                            settings.broadcastParameter[responseKey].type
                        );
                    }
                }
                settings.onReceive(response);
            };
        }
        this.subscriptionListeners[subscriptionRequest.subscriptionId] = new SubscriptionListener({
            onReceive: onReceiveWrapper,
            onError: settings.onError,
            onSubscribed: settings.onSubscribed
        });
        const subscriptionInfo = UtilInternal.extend(
            {
                proxyId: settings.proxyId,
                providerDiscoveryEntry: settings.providerDiscoveryEntry,
                // eslint-disable-next-line @typescript-eslint/camelcase
                lastPublicationTime_ms: 0
            },
            subscriptionRequest
        );

        this.subscriptionInfos[subscriptionRequest.subscriptionId] = subscriptionInfo;

        const alertAfterIntervalMs = (subscriptionRequest.qos as OnChangeWithKeepAliveSubscriptionQos)
            .alertAfterIntervalMs;
        if (alertAfterIntervalMs !== undefined && alertAfterIntervalMs > 0) {
            this.publicationCheckTimerIds[subscriptionRequest.subscriptionId] = LongTimer.setTimeout(
                this.checkPublication,
                alertAfterIntervalMs,
                subscriptionRequest.subscriptionId,
                alertAfterIntervalMs
            );
        }
    }

    private removeRequestFromMulticastSubscribers(_multicastId: string, subscriptionId: string): void {
        for (const multicastIdPattern in this.multicastSubscribers) {
            if (this.multicastSubscribers.hasOwnProperty(multicastIdPattern)) {
                const subscribers = this.multicastSubscribers[multicastIdPattern];
                for (let i = 0; i < subscribers.length; i++) {
                    if (subscribers[i] === subscriptionId) {
                        subscribers.splice(i, 1);
                        if (subscribers.length === 0) {
                            delete this.multicastSubscribers[multicastIdPattern];
                        }
                    }
                }
            }
        }
    }

    private cleanupSubscription(subscriptionId: string): void {
        if (this.publicationCheckTimerIds[subscriptionId] !== undefined) {
            LongTimer.clearTimeout(this.publicationCheckTimerIds[subscriptionId]);
            delete this.publicationCheckTimerIds[subscriptionId];
        }

        if (this.subscriptionInfos[subscriptionId] !== undefined) {
            const subscriptionInfo = this.subscriptionInfos[subscriptionId];
            if (subscriptionInfo.multicastId !== undefined) {
                this.removeRequestFromMulticastSubscribers(subscriptionInfo.multicastId, subscriptionId);
            }
            delete this.subscriptionInfos[subscriptionId];
        }
        if (this.subscriptionListeners[subscriptionId] !== undefined) {
            delete this.subscriptionListeners[subscriptionId];
        }
        this.subscriptionReplyCallers.delete(subscriptionId);
    }

    private registerSubscriptionInternal(settings: SubscriptionSettings, cb: Function): void {
        if (!this.isReady()) {
            cb(new Error("SubscriptionManager is already shut down"));
            return;
        }
        const subscriptionId = settings.subscriptionId || nanoid();
        // log.debug("Registering Subscription Id " + subscriptionId);

        if (settings.attributeName === undefined) {
            cb(
                new Error(
                    `Error: attributeName not provided in call to registerSubscription, settings = ${JSON.stringify(
                        settings
                    )}`
                )
            );
        }
        if (settings.attributeType === undefined) {
            cb(
                new Error(
                    `Error: attributeType not provided in call to registerSubscription, settings = ${JSON.stringify(
                        settings
                    )}`
                )
            );
        }

        if (settings.onError === undefined) {
            log.warn(
                `Warning: subscription for attribute "${
                    settings.attributeName
                }" has been done without error callback function. You will not be informed about missed publications. Please specify the "onError" parameter while subscribing!`
            );
        }
        if (settings.onReceive === undefined) {
            log.warn(
                `Warning: subscription for attribute "${
                    settings.attributeName
                }" has been done without receive callback function. You will not be informed about incoming publications. Please specify the "onReceive" parameter while subscribing!`
            );
        }
        const subscriptionRequest = new SubscriptionRequest({
            subscriptionId,
            subscribedToName: settings.attributeName,
            qos: settings.qos
        });

        const ttl = this.calculateTtl(subscriptionRequest.qos);
        const messagingQos = new MessagingQos({ ttl });

        const timeout = LongTimer.setTimeout(() => {
            this.cleanupSubscription(subscriptionId);
            cb(new Error(`SubscriptionRequest with id ${subscriptionId} failed: tll expired`));
        }, ttl);

        this.subscriptionReplyCallers.set(subscriptionId, {
            cb: (...args: any[]) => {
                LongTimer.clearTimeout(timeout);
                cb(...args);
            }
        });

        this.storeSubscriptionRequest(settings, subscriptionRequest);

        this.dispatcher.sendSubscriptionRequest({
            from: settings.proxyId,
            toDiscoveryEntry: settings.providerDiscoveryEntry,
            messagingQos,
            subscriptionRequest
        });
    }

    private addRequestToMulticastSubscribers(multicastId: string, subscriptionId: string): void {
        const multicastIdPattern = this.multicastWildcardRegexFactory.createIdPattern(multicastId);
        if (this.multicastSubscribers[multicastIdPattern] === undefined) {
            this.multicastSubscribers[multicastIdPattern] = [];
        }
        const subscribers = this.multicastSubscribers[multicastIdPattern];
        for (let i = 0; i < subscribers.length; i++) {
            if (subscribers[i] === subscriptionId) {
                return;
            }
        }
        subscribers.push(subscriptionId);
    }

    private createBroadcastSubscriptionRequest(
        parameters: BroadcastSubscriptionSettings
    ): BroadcastSubscriptionRequest | MulticastSubscriptionRequest {
        let request;
        if (parameters.selective) {
            request = new BroadcastSubscriptionRequest({
                subscriptionId: parameters.subscriptionId || nanoid(),
                subscribedToName: parameters.broadcastName,
                qos: (parameters.subscriptionQos as any) as OnChangeSubscriptionQos,
                filterParameters: parameters.filterParameters
            });
        } else {
            request = new MulticastSubscriptionRequest({
                multicastId: SubscriptionUtil.createMulticastId(
                    parameters.providerDiscoveryEntry.participantId,
                    parameters.broadcastName,
                    parameters.partitions
                ),
                subscriptionId: parameters.subscriptionId || nanoid(),
                subscribedToName: parameters.broadcastName,
                qos: (parameters.subscriptionQos as any) as MulticastSubscriptionQos
            });
            this.addRequestToMulticastSubscribers(request.multicastId, request.subscriptionId);
        }
        return request;
    }

    /**
     * @name SubscriptionManager#registerBroadcastSubscription
     * @function
     * @param parameters
     * @param parameters.proxyId participantId of the sender
     * @param parameters.providerDiscoveryEntry DiscoveryEntry of the receiver
     * @param parameters.broadcastName the name of the broadcast being subscribed to
     * @param parameters.broadcastParameter the parameter meta information of the broadcast being subscribed to
     * @param [parameters.subscriptionQos] the subscriptionQos
     * @param [parameters.filterParameters] filter parameters used to indicate interest in
     *            only a subset of broadcasts that might be sent.
     * @param parameters.selective true if broadcast is selective
     * @param [parameters.partitions] partitions for multicast requests
     * @param parameters.subscriptionId optional parameter subscriptionId to reuse a
     *            pre-existing identifier for this concrete subscription request
     * @param {SubscriptionManager~onReceive} parameters.onReceive is called when a broadcast is received.
     * @param {SubscriptionManager~onError} parameters.onError is called when an error occurs with the broadcast
     * @param {SubscriptionManager~onSubscribed} parameters.onSubscribed the callback to inform once the subscription request has
     *            been delivered successfully
     * @param cb
     */
    private registerBroadcastSubscriptionInternal(parameters: BroadcastSubscriptionSettings, cb: Function): void {
        if (!this.isReady()) {
            cb(new Error("SubscriptionManager is already shut down"));
            return;
        }

        const subscriptionRequest = this.createBroadcastSubscriptionRequest(parameters);
        const subscriptionId = subscriptionRequest.subscriptionId;

        const ttl = this.calculateTtl(subscriptionRequest.qos);
        const messagingQos = new MessagingQos({ ttl });

        const timeout = LongTimer.setTimeout(() => {
            this.cleanupSubscription(subscriptionId);
            cb(new Error(`BroadcastSubscriptionRequest with id ${subscriptionId} failed: tll expired`));
        }, ttl);

        this.subscriptionReplyCallers.set(subscriptionId, {
            cb: (...args: any[]) => {
                LongTimer.clearTimeout(timeout);
                cb(...args);
            }
        });

        this.storeSubscriptionRequest(parameters, subscriptionRequest);

        this.dispatcher
            .sendBroadcastSubscriptionRequest({
                from: parameters.proxyId,
                toDiscoveryEntry: parameters.providerDiscoveryEntry,
                messagingQos,
                subscriptionRequest
            })
            .then(() => {
                const type = Typing.getObjectType(subscriptionRequest);
                if (type === "MulticastSubscriptionRequest") {
                    const subscriptionReplyCaller = this.subscriptionReplyCallers.get(subscriptionId);
                    const subscriptionListener = this.subscriptionListeners[subscriptionId];
                    if (subscriptionReplyCaller !== undefined) {
                        subscriptionReplyCaller.cb(undefined, subscriptionId);
                    }
                    if (subscriptionListener !== undefined && subscriptionListener.onSubscribed !== undefined) {
                        subscriptionListener.onSubscribed(subscriptionId);
                    }
                    this.subscriptionReplyCallers.delete(subscriptionId);
                    this.addRequestToMulticastSubscribers(
                        (subscriptionRequest as MulticastSubscriptionRequest).multicastId,
                        subscriptionRequest.subscriptionId
                    );
                }
            })
            .catch(error => {
                this.cleanupSubscription(subscriptionRequest.subscriptionId);
                if (parameters.onError) {
                    parameters.onError(error);
                }
                // eslint-disable-next-line promise/no-callback-in-promise
                cb(error);
            });
    }

    /**
     * @param subscriptionReply incoming subscriptionReply
     */
    public handleSubscriptionReply(subscriptionReply: SubscriptionReply): void {
        const subscriptionReplyCaller = this.subscriptionReplyCallers.get(subscriptionReply.subscriptionId);
        const subscriptionListener = this.subscriptionListeners[subscriptionReply.subscriptionId];

        if (subscriptionReplyCaller === undefined && subscriptionListener === undefined) {
            log.error(
                `error handling subscription reply, because subscriptionReplyCaller and subscriptionListener could not be found: ${JSONSerializer.stringify(
                    subscriptionReply
                )}`
            );
            return;
        }

        try {
            if (subscriptionReply.error) {
                if (!(subscriptionReply.error instanceof Error)) {
                    subscriptionReply.error = Typing.augmentTypes(subscriptionReply.error);
                }
                if (subscriptionReplyCaller !== undefined) {
                    subscriptionReplyCaller.cb(subscriptionReply.error);
                }
                if (subscriptionListener !== undefined && subscriptionListener.onError !== undefined) {
                    subscriptionListener.onError(subscriptionReply.error);
                }
                this.cleanupSubscription(subscriptionReply.subscriptionId);
            } else {
                if (subscriptionReplyCaller !== undefined) {
                    subscriptionReplyCaller.cb(undefined, subscriptionReply.subscriptionId);
                }
                if (subscriptionListener !== undefined && subscriptionListener.onSubscribed !== undefined) {
                    subscriptionListener.onSubscribed(subscriptionReply.subscriptionId);
                }
                this.subscriptionReplyCallers.delete(subscriptionReply.subscriptionId);
            }
        } catch (e) {
            log.error(
                `exception thrown during handling subscription reply ${JSONSerializer.stringify(subscriptionReply)}:\n${
                    e.stack
                }`
            );
            this.subscriptionReplyCallers.delete(subscriptionReply.subscriptionId);
        }
    }

    /**
     * @param publication incoming multicast publication
     */
    public handleMulticastPublication(publication: MulticastPublication): void {
        let subscribersFound = false;
        for (const multicastIdPattern in this.multicastSubscribers) {
            if (this.multicastSubscribers.hasOwnProperty(multicastIdPattern)) {
                if (publication.multicastId.match(new RegExp(multicastIdPattern)) !== null) {
                    const subscribers = this.multicastSubscribers[multicastIdPattern];
                    if (subscribers !== undefined) {
                        subscribersFound = true;
                        for (let i = 0; i < subscribers.length; i++) {
                            const subscriptionListener = this.subscriptionListeners[subscribers[i]];
                            if (publication.error) {
                                if (subscriptionListener.onError) {
                                    subscriptionListener.onError(publication.error);
                                } else {
                                    log.debug(
                                        `subscriptionListener with Id "${
                                            subscribers[i]
                                        }" has no onError callback. Skipping error publication`
                                    );
                                }
                            } else if (publication.response) {
                                if (subscriptionListener.onReceive) {
                                    subscriptionListener.onReceive(publication.response);
                                } else {
                                    log.debug(
                                        `subscriptionListener with Id "${
                                            subscribers[i]
                                        }" has no onReceive callback. Skipping multicast publication`
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!subscribersFound) {
            throw new Error(
                `${"Publication cannot be handled, as no subscription with multicastId "}${
                    publication.multicastId
                } is known.`
            );
        }
    }

    /**
     * @param publication incoming publication
     */
    public handlePublication(publication: SubscriptionPublication): void {
        if (this.subscriptionInfos[publication.subscriptionId] === undefined) {
            throw new Error(
                `${"Publication cannot be handled, as no subscription with subscriptionId "}${
                    publication.subscriptionId
                } is known.`
            );
        }
        this.setLastPublicationTime(publication.subscriptionId, Date.now());
        const subscriptionListener = this.subscriptionListeners[publication.subscriptionId];
        if (publication.error) {
            if (subscriptionListener.onError) {
                subscriptionListener.onError(publication.error);
            } else {
                log.debug(
                    `subscriptionListener with Id "${
                        publication.subscriptionId
                    }" has no onError callback. Skipping error publication`
                );
            }
        } else if (publication.response) {
            if (subscriptionListener.onReceive) {
                subscriptionListener.onReceive(publication.response);
            } else {
                log.debug(
                    `subscriptionListener with Id "${
                        publication.subscriptionId
                    }" has no onReceive callback. Skipping publication`
                );
            }
        }
    }

    /**
     * @param settings
     * @param settings.messagingQos the messaging Qos object for the ttl
     * @param settings.subscriptionId of the subscriptionId to stop
     * @returns A promise object
     */
    public unregisterSubscription(settings: { messagingQos: MessagingQos; subscriptionId: string }): Promise<void> {
        if (!this.isReady()) {
            throw new Error("SubscriptionManager is already shut down");
        }

        const subscriptionInfo = this.subscriptionInfos[settings.subscriptionId];
        let errorMessage;
        if (subscriptionInfo === undefined) {
            errorMessage = `Cannot find subscription with id: ${settings.subscriptionId}`;
            log.error(errorMessage);
            return Promise.reject(new Error(errorMessage));
        }

        const subscriptionStop = new SubscriptionStop({
            subscriptionId: settings.subscriptionId
        });

        let promise;
        if (subscriptionInfo.multicastId !== undefined) {
            promise = this.dispatcher.sendMulticastSubscriptionStop({
                from: subscriptionInfo.proxyId,
                toDiscoveryEntry: subscriptionInfo.providerDiscoveryEntry,
                messagingQos: settings.messagingQos,
                multicastId: subscriptionInfo.multicastId,
                subscriptionStop
            });
        } else {
            promise = this.dispatcher.sendSubscriptionStop({
                from: subscriptionInfo.proxyId,
                toDiscoveryEntry: subscriptionInfo.providerDiscoveryEntry,
                messagingQos: settings.messagingQos,
                subscriptionStop
            });
        }

        this.cleanupSubscription(settings.subscriptionId);

        return promise;
    }

    public hasMulticastSubscriptions(): boolean {
        return Object.keys(this.multicastSubscribers).length > 0;
    }

    public hasOpenSubscriptions(): boolean {
        const hasSubscriptionInfos = Object.keys(this.subscriptionInfos).length > 0;
        const hasSubscriptionListeners = Object.keys(this.subscriptionListeners).length > 0;
        const hasPublicationCheckTimerIds = Object.keys(this.publicationCheckTimerIds).length > 0;
        const hasSubscriptionReplyCallers = this.subscriptionReplyCallers.size > 0;
        return (
            hasSubscriptionInfos ||
            hasSubscriptionListeners ||
            hasPublicationCheckTimerIds ||
            hasSubscriptionReplyCallers ||
            this.hasMulticastSubscriptions()
        );
    }

    /**
     * This method is meant to be called by the runtime before shutdown is called.
     * It turns out that there is a necessary shutdown order and SubscriptionManager can't be shutdown first.
     *
     * @param timeoutMs timeout in ms after which this operation shall timeout. 0 defaults to no timeout.
     * @returns - resolves if subscriptionStop message has been sent for each active subscription
     * - rejects in case of any issues or timeout occurs
     */
    public terminateSubscriptions(timeoutMs: number): Promise<any> {
        const logPrefix = "SubscriptionManager::terminateSubscriptions";
        log.info(`${logPrefix} ${timeoutMs}`);

        const cleanUpPromises = [];
        let activeSubscriptionId;
        for (activeSubscriptionId in this.subscriptionInfos) {
            if (Object.prototype.hasOwnProperty.call(this.subscriptionInfos, activeSubscriptionId)) {
                const promise = this.unregisterSubscription({
                    subscriptionId: activeSubscriptionId,
                    messagingQos: new MessagingQos({})
                });
                cleanUpPromises.push(promise);
            }
        }
        const cleanUpPromise = Promise.all(cleanUpPromises);
        log.info(`${logPrefix} terminating a total of ${cleanUpPromises.length} subscriptions`);

        return timeoutMs === 0 ? cleanUpPromise : UtilInternal.timeoutPromise(cleanUpPromise, timeoutMs);
    }

    /**
     * Shutdown the subscription manager
     */
    public shutdown(): void {
        for (const subscriptionId in this.publicationCheckTimerIds) {
            if (this.publicationCheckTimerIds.hasOwnProperty(subscriptionId)) {
                const timerId = this.publicationCheckTimerIds[subscriptionId];
                if (timerId !== undefined) {
                    LongTimer.clearTimeout(timerId);
                }
            }
        }
        this.publicationCheckTimerIds = {};
        for (const subscriptionReplyCaller of this.subscriptionReplyCallers.values()) {
            if (subscriptionReplyCaller) {
                subscriptionReplyCaller.cb(new Error("Subscription Manager is already shut down"));
            }
        }
        this.subscriptionReplyCallers.clear();
        this.started = false;
    }
}

export = SubscriptionManager;
