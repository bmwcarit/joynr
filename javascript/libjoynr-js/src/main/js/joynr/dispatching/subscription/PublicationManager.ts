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
import {
    ProviderAttribute,
    ProviderReadNotifyAttribute,
    ProviderReadWriteNotifyAttribute
} from "../../provider/ProviderAttribute";
/*eslint no-use-before-define: "off", no-useless-concat: "off"*/
import SubscriptionQos from "../../proxy/SubscriptionQos";
import PeriodicSubscriptionQos from "../../proxy/PeriodicSubscriptionQos";
import * as MulticastPublication from "../types/MulticastPublication";
import * as SubscriptionPublication from "../types/SubscriptionPublication";
import SubscriptionReply from "../types/SubscriptionReply";
import SubscriptionStop from "../types/SubscriptionStop";
import SubscriptionInformation from "../types/SubscriptionInformation";
import ProviderEvent from "../../provider/ProviderEvent";
import * as SubscriptionUtil from "./util/SubscriptionUtil";
import SubscriptionException from "../../exceptions/SubscriptionException";
import * as JSONSerializer from "../../util/JSONSerializer";
import LongTimer from "../../util/LongTimer";
import LoggingManager from "../../system/LoggingManager";
import Dispatcher = require("../Dispatcher");
import SubscriptionRequest = require("../types/SubscriptionRequest");
import BroadcastSubscriptionRequest = require("../types/BroadcastSubscriptionRequest");
import MulticastSubscriptionRequest = require("../types/MulticastSubscriptionRequest");
import OnChangeWithKeepAliveSubscriptionQos = require("../../proxy/OnChangeWithKeepAliveSubscriptionQos");

const log = LoggingManager.getLogger("joynr.dispatching.subscription.PublicationManager");

class PublicationManager {
    private dispatcher: Dispatcher;
    private started = true;
    private multicastSubscriptions: Record<string, any> = {};

    // map: providerId+eventName -> subscriptionIds -> subscription
    private onChangeProviderEventToSubscriptions: Record<string, any> = {};

    // map: providerId+attributeName -> subscriptionIds -> subscription
    private onChangeProviderAttributeToSubscriptions: Record<string, any> = {};

    // queued subscriptions for deferred providers
    private queuedProviderParticipantIdToSubscriptionRequestsMapping: Record<string, any> = {};

    // map: subscriptionId to SubscriptionRequest
    private queuedSubscriptionInfos: Record<string, any> = {};

    // map: subscriptionId to SubscriptionRequest
    private subscriptionInfos: Record<string, any> = {};

    private eventObserverFunctions: Record<string, any> = {};
    private attributeObserverFunctions: Record<string, any> = {};

    // map: key is the provider's participantId, value is the provider object
    private participantIdToProvider: Record<string, any> = {};
    /**
     * The PublicationManager is responsible for handling subscription requests.
     *
     * @param dispatcher
     */
    public constructor(dispatcher: Dispatcher) {
        this.dispatcher = dispatcher;

        this.triggerPublication = this.triggerPublication.bind(this);
        this.removeSubscription = this.removeSubscription.bind(this);
    }

    private isReady(): boolean {
        return this.started;
    }

    /**
     * Helper function to get attribute object based on provider's participantId and
     * attribute name
     *
     * @param participantId the participantId of the provider
     * @param attributeName the attribute name
     * @returns the provider attribute
     */
    private getAttribute(participantId: string, attributeName: string): ProviderReadWriteNotifyAttribute<any> {
        const provider = this.participantIdToProvider[participantId];
        return provider[attributeName];
    }

    /**
     * Helper function to get attribute value. In case the attribute getter function of
     * the provider returns a promise object this function waits until the promise object
     * is resolved to resolve the promise object return by this function
     *
     * @param subscriptionInfo the subscription information
     * @param subscriptionInfo.providerParticipantId the participantId of the provider
     * @param subscriptionInfo.subscribedToName the attribute to be published
     * @returns an A+ promise object which provides attribute value upon success and an
     *            error upon failure
     */
    private getAttributeValue(subscriptionInfo: {
        providerParticipantId: string;
        subscribedToName: string;
    }): Promise<any> {
        const attribute = this.getAttribute(subscriptionInfo.providerParticipantId, subscriptionInfo.subscribedToName);
        return attribute.get();
    }

    private sendPublication(subscriptionInfo: SubscriptionInformation, value: any | undefined, exception?: any): void {
        log.debug(
            `send Publication for subscriptionId ${subscriptionInfo.subscriptionId} and attribute/event ${
                subscriptionInfo.subscribedToName
            }: ${value}`
        );
        subscriptionInfo.lastPublication = Date.now();
        let subscriptionPublication;

        if (exception) {
            subscriptionPublication = SubscriptionPublication.create({
                error: exception,
                subscriptionId: subscriptionInfo.subscriptionId
            });
        } else {
            subscriptionPublication = SubscriptionPublication.create({
                response: value,
                subscriptionId: subscriptionInfo.subscriptionId
            });
        }
        this.dispatcher.sendPublication(
            {
                from: subscriptionInfo.providerParticipantId,
                to: subscriptionInfo.proxyParticipantId,
                expiryDate: Date.now() + subscriptionInfo.qos.publicationTtlMs
            },
            subscriptionPublication
        );
    }

    private getPeriod(subscriptionInfo: SubscriptionInformation): number | undefined {
        return (
            (subscriptionInfo.qos as OnChangeWithKeepAliveSubscriptionQos).maxIntervalMs ||
            (subscriptionInfo.qos as PeriodicSubscriptionQos).periodMs
        );
    }

    private prepareAttributePublication(subscriptionInfo: SubscriptionInformation, value: any): void {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        const timeSinceLastPublication = Date.now() - subscriptionInfo.lastPublication!;
        if (
            subscriptionInfo.qos.minIntervalMs === undefined ||
            timeSinceLastPublication >= subscriptionInfo.qos.minIntervalMs
        ) {
            this.sendPublication(subscriptionInfo, value);
            // if registered interval exists => reschedule it

            if (subscriptionInfo.onChangeDebounce !== undefined) {
                LongTimer.clearTimeout(subscriptionInfo.onChangeDebounce);
                delete subscriptionInfo.onChangeDebounce;
            }

            // if there's an existing interval, clear it and restart
            if (subscriptionInfo.subscriptionInterval !== undefined) {
                LongTimer.clearTimeout(subscriptionInfo.subscriptionInterval);
                subscriptionInfo.subscriptionInterval = this.triggerPublicationTimer(
                    subscriptionInfo,
                    this.getPeriod(subscriptionInfo)
                );
            }
        } else if (subscriptionInfo.onChangeDebounce === undefined) {
            subscriptionInfo.onChangeDebounce = LongTimer.setTimeout(() => {
                subscriptionInfo.onChangeDebounce = undefined;
                this.triggerPublication(subscriptionInfo);
            }, subscriptionInfo.qos.minIntervalMs - timeSinceLastPublication);
        }
    }

    private prepareBroadcastPublication(subscriptionInfo: SubscriptionInformation, value: any): void {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        const timeSinceLastPublication = Date.now() - subscriptionInfo.lastPublication!;
        if (
            subscriptionInfo.qos.minIntervalMs === undefined ||
            timeSinceLastPublication >= subscriptionInfo.qos.minIntervalMs
        ) {
            this.sendPublication(subscriptionInfo, value);
        } else {
            log.info(
                `Two subsequent broadcasts of event ${
                    subscriptionInfo.subscribedToName
                } occured within minIntervalMs of subscription with id ${
                    subscriptionInfo.subscriptionId
                }. Event will not be sent to the subscribing client.`
            );
        }
    }

    private triggerPublication(subscriptionInfo: SubscriptionInformation): void {
        this.getAttributeValue(subscriptionInfo)
            .then((value: any) => this.prepareAttributePublication(subscriptionInfo, value))
            .catch((exception: any) => this.sendPublication(subscriptionInfo, undefined, exception));
    }

    /**
     * This functions waits the delay time before publishing the value of the attribute
     * specified in the subscription information
     *
     * @param subscriptionInfo the subscription information
     * @param subscriptionInfo.providerParticipantId the participantId of the provider
     * @param subscriptionInfo.subscribedToName the attribute to be published
     * @param delay the delay to wait for the publication
     */
    private triggerPublicationTimer(
        subscriptionInfo: {
            providerParticipantId: string;
            subscribedToName: string;
        },
        delay?: number
    ): string | number | undefined {
        if (delay !== undefined) {
            return LongTimer.setTimeout(this.triggerPublication, delay, subscriptionInfo);
        }
    }

    private getProviderIdAttributeKey(providerId: string, attributeName: string): string {
        return `${providerId}.${attributeName}`;
    }

    private getProviderIdEventKey(providerId: string, eventName: string): string {
        return `${providerId}.${eventName}`;
    }

    /**
     * Gives the list of subscriptions for the given providerId and attribute name
     *
     * @param providerId
     * @param attributeName
     * @returns a list of subscriptions
     e.g. [{ participantId : "...", attributeName : "..." }]
     */
    private getSubscriptionsForProviderAttribute(providerId: string, attributeName: string): Record<string, any> {
        const key = this.getProviderIdAttributeKey(providerId, attributeName);
        // make sure the mapping exists, so that subscriptions can register here
        if (this.onChangeProviderAttributeToSubscriptions[key] === undefined) {
            this.onChangeProviderAttributeToSubscriptions[key] = {};
        }
        return this.onChangeProviderAttributeToSubscriptions[key];
    }

    private resetSubscriptionsForProviderAttribute(providerId: string, attributeName: string): void {
        const key = this.getProviderIdAttributeKey(providerId, attributeName);
        delete this.onChangeProviderAttributeToSubscriptions[key];
    }

    /**
     * Checks whether the provider attribute is notifiable (=is even interesting to the
     * PublicationManager)
     *
     * @param providerAttribute the provider attribute to check for Notifiability
     * @returns if the provider attribute is notifiable
     */
    private providerAttributeIsNotifiable(providerAttribute: ProviderAttribute): boolean {
        return typeof providerAttribute.isNotifiable === "function" && providerAttribute.isNotifiable();
    }

    /**
     * Checks whether the provider property is an event (=is even interesting to the
     * PublicationManager)
     *
     * @param providerProperty the provider attribute to check for Notifiability
     * @returns if the provider attribute is notifiable
     */
    private propertyIsProviderEvent(providerProperty: any): boolean {
        return providerProperty instanceof ProviderEvent;
    }

    /**
     * @param providerId
     * @param attributeName
     * @param _attribute
     * @param value
     */
    private publishAttributeValue(providerId: string, attributeName: string, _attribute: any, value: any): void {
        if (!this.isReady()) {
            throw new Error(
                `attribute publication for providerId "${providerId} and attribute ${attributeName} is not forwarded to subscribers, as the publication manager is already shut down`
            );
        }
        const subscriptions = this.getSubscriptionsForProviderAttribute(providerId, attributeName);
        if (!subscriptions) {
            log.error(
                `ProviderAttribute ${attributeName} for providerId ${providerId} is not registered or notifiable`
            );
            // TODO: proper error handling for empty subscription map =>
            // ProviderAttribute is not notifiable or not registered
            return;
        }

        for (const subscriptionId in subscriptions) {
            if (Object.prototype.hasOwnProperty.call(subscriptions, subscriptionId)) {
                const subscriptionInfo = subscriptions[subscriptionId];
                this.prepareAttributePublication(subscriptionInfo, value);
            }
        }
    }

    /**
     * Adds a notifiable provider attribute to the internal map if it does not exist =>
     * onChange subscriptions can be registered here
     *
     * @param providerId
     * @param attributeName
     * @param attribute
     */
    private addPublicationAttribute(
        providerId: string,
        attributeName: string,
        attribute: ProviderReadWriteNotifyAttribute<any>
    ): void {
        const key = this.getProviderIdAttributeKey(providerId, attributeName);

        this.attributeObserverFunctions[key] = (value: any) => {
            this.publishAttributeValue(providerId, attributeName, attribute, value);
        };
        attribute.registerObserver(this.attributeObserverFunctions[key]);
    }

    /* Broadcast specific implementation*/
    /**
     * Gives the list of subscriptions for the given providerId and event name
     * @name PublicationManager#getSubscriptionsForProviderEvent
     * @private
     *
     * @param providerId
     * @param eventName
     * @returns a list of subscriptions e.g. [{ participantId : "...", eventName : "..." }]
     */
    private getSubscriptionsForProviderEvent(providerId: string, eventName: string): Record<string, any> {
        const key = this.getProviderIdEventKey(providerId, eventName);
        // make sure the mapping exists, so that subscriptions can register here
        if (this.onChangeProviderEventToSubscriptions[key] === undefined) {
            this.onChangeProviderEventToSubscriptions[key] = {};
        }
        return this.onChangeProviderEventToSubscriptions[key];
    }

    private prepareMulticastPublication(
        providerId: string,
        eventName: string,
        partitions: any[],
        outputParameters: any
    ): void {
        const multicastId = SubscriptionUtil.createMulticastId(providerId, eventName, partitions);
        const publication = MulticastPublication.create({
            response: outputParameters,
            multicastId
        });
        this.dispatcher.sendMulticastPublication(
            {
                from: providerId,
                expiryDate: Date.now() + SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS //TODO: what should be the ttl?
            },
            publication
        );
    }

    /**
     * @param providerId
     * @param eventName
     * @param event
     * @param data
     */
    private publishEventValue(providerId: string, eventName: string, event: ProviderEvent, data: any): void {
        const value = data.broadcastOutputParameters;
        if (!this.isReady()) {
            throw new Error(
                `event publication for providerId "${providerId} and eventName ${eventName} is not forwarded to subscribers, as the publication manager is ` +
                    `already shut down`
            );
        }
        if (!event.selective) {
            //handle multicast
            this.prepareMulticastPublication(providerId, eventName, data.partitions, value.outputParameters);
            return;
        }
        const subscriptions = this.getSubscriptionsForProviderEvent(providerId, eventName);
        const filters = data.filters;
        if (!subscriptions) {
            log.error(`ProviderEvent ${eventName} for providerId ${providerId} is not registered`);
            // TODO: proper error handling for empty subscription map =>
            // ProviderEvent is not registered
            return;
        }

        for (const subscriptionId in subscriptions) {
            if (Object.prototype.hasOwnProperty.call(subscriptions, subscriptionId)) {
                const subscriptionInfo = subscriptions[subscriptionId];
                // if any filters present, check them
                let publish = true;
                if (filters && filters.length > 0) {
                    for (let i = 0; i < filters.length; i++) {
                        if (subscriptionInfo.filterParameters && subscriptionInfo.filterParameters.filterParameters) {
                            publish = filters[i].filter(value, subscriptionInfo.filterParameters.filterParameters);
                            // stop on first filter failure
                            if (publish === false) {
                                break;
                            }
                        }
                    }
                }
                if (publish) {
                    this.prepareBroadcastPublication(subscriptionInfo, value.outputParameters);
                }
            }
        }
    }

    /**
     * Adds a provider event to the internal map if it does not exist =>
     * onChange subscriptions can be registered here
     *
     * @param providerId
     * @param eventName
     * @param event
     */
    private addPublicationEvent(providerId: string, eventName: string, event: ProviderEvent): void {
        const key = this.getProviderIdEventKey(providerId, eventName);

        this.eventObserverFunctions[key] = (data: any) => {
            this.publishEventValue(providerId, eventName, event, data || {});
        };
        event.registerObserver(this.eventObserverFunctions[key]);
    }

    /* */
    private resetSubscriptionsForProviderEvent(providerId: string, eventName: string): void {
        const key = this.getProviderIdEventKey(providerId, eventName);
        delete this.onChangeProviderEventToSubscriptions[key];
    }

    /* End of broadcast specific implementation*/
    private addRequestToMulticastSubscriptions(multicastId: string, subscriptionId: string): void {
        if (this.multicastSubscriptions[multicastId] === undefined) {
            this.multicastSubscriptions[multicastId] = [];
        }
        const subscriptions = this.multicastSubscriptions[multicastId];
        for (let i = 0; i < subscriptions.length; i++) {
            if (subscriptions[i] === subscriptionId) {
                return;
            }
        }
        subscriptions.push(subscriptionId);
    }

    private removeRequestFromMulticastSubscriptions(multicastId: string, subscriptionId: string): void {
        if (multicastId !== undefined && this.multicastSubscriptions[multicastId] !== undefined) {
            let i;
            for (i = 0; i < this.multicastSubscriptions[multicastId].length; i++) {
                if (this.multicastSubscriptions[multicastId][i] === subscriptionId) {
                    this.multicastSubscriptions[multicastId].splice(i, 1);
                    break;
                }
            }
            if (this.multicastSubscriptions[multicastId].length === 0) {
                delete this.multicastSubscriptions[multicastId];
            }
        }
    }

    /**
     * Removes a subscription, stops scheduled timers
     *
     * @param subscriptionId
     * @param silent suppress log outputs if subscription cannot be found
     */
    private removeSubscription(subscriptionId: string, silent?: boolean): void {
        // make sure subscription info exists
        let subscriptionInfo = this.subscriptionInfos[subscriptionId];
        let pendingSubscriptions;
        let pendingSubscription;
        let subscriptionObject;
        if (subscriptionInfo === undefined) {
            if (silent !== true) {
                log.warn(`no subscription info found for subscriptionId ${subscriptionId}`);
            }
            // TODO: proper handling for a non-existent subscription

            //check if a subscriptionRequest is queued
            subscriptionInfo = this.queuedSubscriptionInfos[subscriptionId];

            if (subscriptionInfo === undefined) {
                return;
            }

            pendingSubscriptions = this.queuedProviderParticipantIdToSubscriptionRequestsMapping[
                subscriptionInfo.providerParticipantId
            ];
            if (pendingSubscriptions !== undefined) {
                for (pendingSubscription in pendingSubscriptions) {
                    if (Object.prototype.hasOwnProperty.call(pendingSubscriptions, pendingSubscription)) {
                        subscriptionObject = pendingSubscriptions[pendingSubscription];
                        if (
                            subscriptionObject !== undefined &&
                            subscriptionObject.subscriptionId === subscriptionInfo.subscriptionId
                        ) {
                            delete pendingSubscriptions[pendingSubscription];
                        }
                    }
                }
            }
            delete this.queuedSubscriptionInfos[subscriptionId];
            return;
        }
        const providerParticipantId = subscriptionInfo.providerParticipantId;

        // make sure the provider exists for the given participantId
        const provider = this.participantIdToProvider[providerParticipantId];
        if (provider === undefined) {
            log.error(`no provider found for ${providerParticipantId}`);
            // TODO: proper error handling for a non-existent provider
            return;
        }

        let subscription;

        if (subscriptionInfo.subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE) {
            // This is an attribute subscription

            const attributeName = subscriptionInfo.subscribedToName;

            const attributeSubscriptions = this.getSubscriptionsForProviderAttribute(
                providerParticipantId,
                attributeName
            );
            if (attributeSubscriptions === undefined) {
                log.error(
                    `ProviderAttribute ${attributeName} for providerId ${providerParticipantId} is not registered or notifiable`
                );
                // TODO: proper error handling for empty subscription map =>
                // ProviderAttribute is not notifiable or not registered
                return;
            }
            subscription = attributeSubscriptions[subscriptionId];
            delete attributeSubscriptions[subscriptionId];
            if (Object.keys(attributeSubscriptions).length === 0) {
                this.resetSubscriptionsForProviderAttribute(providerParticipantId, attributeName);
            }
        } else {
            // subscriptionInfo.type === SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST
            // This is a event subscription
            const eventName = subscriptionInfo.subscribedToName;

            // find all subscriptions for the event/providerParticipantId
            const eventSubscriptions = this.getSubscriptionsForProviderEvent(providerParticipantId, eventName);
            if (eventSubscriptions === undefined) {
                log.error(
                    `ProviderEvent ${eventName} for providerId ${providerParticipantId} is not registered or notifiable`
                );
                // TODO: proper error handling for empty subscription map =>
                return;
            }
            subscription = eventSubscriptions[subscriptionId];
            delete eventSubscriptions[subscriptionId];
            if (Object.keys(eventSubscriptions).length === 0) {
                this.resetSubscriptionsForProviderEvent(providerParticipantId, eventName);
            }
        }

        // make sure subscription exists
        if (subscription === undefined) {
            log.error(`no subscription found for subscriptionId ${subscriptionId}`);
            // TODO: proper error handling when subscription does not exist
            return;
        }

        // clear subscription interval if it exists
        if (subscription.subscriptionInterval !== undefined) {
            LongTimer.clearTimeout(subscription.subscriptionInterval);
        }

        // clear endDate timeout if it exists
        if (subscription.endDateTimeout !== undefined) {
            LongTimer.clearTimeout(subscription.endDateTimeout);
        }

        this.removeRequestFromMulticastSubscriptions(subscription.multicastId, subscriptionId);

        delete this.subscriptionInfos[subscriptionId];
    }

    /**
     * Removes a notifiable provider attribute from the internal map if it exists
     *
     * @param providerId
     * @param attributeName
     * @param attribute
     */
    private removePublicationAttribute(
        providerId: string,
        attributeName: string,
        attribute: ProviderReadNotifyAttribute<any>
    ): void {
        const key = this.getProviderIdAttributeKey(providerId, attributeName);
        const subscriptions = this.getSubscriptionsForProviderAttribute(providerId, attributeName);

        if (subscriptions !== undefined) {
            for (const subscription in subscriptions) {
                if (Object.prototype.hasOwnProperty.call(subscriptions, subscription)) {
                    this.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: subscription
                        })
                    );
                }
            }
            this.resetSubscriptionsForProviderAttribute(providerId, attributeName);
        }

        attribute.unregisterObserver(this.attributeObserverFunctions[key]);
        delete this.attributeObserverFunctions[key];
    }

    /**
     * Removes a provider event from the internal map if it exists
     *
     * @param providerId
     * @param eventName
     */
    private removePublicationEvent(providerId: string, eventName: string, event: ProviderEvent): void {
        const key = this.getProviderIdEventKey(providerId, eventName);

        const subscriptions = this.getSubscriptionsForProviderEvent(providerId, eventName);
        if (subscriptions !== undefined) {
            for (const subscription in subscriptions) {
                if (Object.prototype.hasOwnProperty.call(subscriptions, subscription)) {
                    this.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: subscription
                        })
                    );
                }
            }
            this.resetSubscriptionsForProviderEvent(providerId, eventName);
        }

        event.unregisterObserver(this.eventObserverFunctions[key]);
        delete this.eventObserverFunctions[key];
    }

    /**
     * the parameter "callbackDispatcher" is optional, as in case of restoring
     * subscriptions, no reply must be sent back via the dispatcher
     */
    private optionalCallbackDispatcher(
        callbackDispatcherSettings: any,
        reply: { subscriptionId: string; error?: SubscriptionException },
        callbackDispatcher?: Function
    ): void {
        if (callbackDispatcher !== undefined) {
            callbackDispatcher(callbackDispatcherSettings, new SubscriptionReply(reply));
        }
    }

    private handleBroadcastSubscriptionRequestInternal(
        proxyParticipantId: string,
        providerParticipantId: string,
        subscriptionRequest: BroadcastSubscriptionRequest,
        callbackDispatcher: Function | undefined,
        callbackDispatcherSettings: any,
        multicast: boolean
    ): void;
    private handleBroadcastSubscriptionRequestInternal(
        proxyParticipantId: string,
        providerParticipantId: string,
        subscriptionRequest: MulticastSubscriptionRequest,
        callbackDispatcher: Function | undefined,
        callbackDispatcherSettings: any,
        multicast: boolean
    ): void;
    private handleBroadcastSubscriptionRequestInternal(
        proxyParticipantId: string,
        providerParticipantId: string,
        subscriptionRequest: BroadcastSubscriptionRequest & MulticastSubscriptionRequest,
        callbackDispatcher: Function | undefined,
        callbackDispatcherSettings: any,
        multicast: boolean
    ): void {
        const requestType = `${multicast ? "multicast" : "broadcast"} subscription request`;
        let exception;
        let timeToEndDate = 0;
        const eventName = subscriptionRequest.subscribedToName;
        const subscriptionId = subscriptionRequest.subscriptionId;

        // if endDate is defined (also exclude default value 0 for
        // the expiryDateMs qos-property)
        if (
            subscriptionRequest.qos !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE
        ) {
            timeToEndDate = subscriptionRequest.qos.expiryDateMs - Date.now();

            // if endDate lies in the past => don't add the subscription
            if (timeToEndDate <= 0) {
                exception = new SubscriptionException({
                    detailMessage: `error handling ${requestType}: ${JSONSerializer.stringify(
                        subscriptionRequest
                    )}. expiryDateMs ${
                        subscriptionRequest.qos.expiryDateMs
                    } for ProviderEvent ${eventName} for providerId ${providerParticipantId} lies in the past`,
                    subscriptionId
                });
                log.error(exception.detailMessage);
                this.optionalCallbackDispatcher(
                    callbackDispatcherSettings,
                    {
                        error: exception,
                        subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        if (!this.isReady()) {
            exception = new SubscriptionException({
                detailMessage: `error handling ${requestType}: ${JSONSerializer.stringify(
                    subscriptionRequest
                )} and provider ParticipantId ${providerParticipantId}: joynr runtime already shut down`,
                subscriptionId: subscriptionRequest.subscriptionId
            });
            log.debug(exception.detailMessage);
            this.optionalCallbackDispatcher(
                callbackDispatcherSettings,
                {
                    error: exception,
                    subscriptionId: subscriptionRequest.subscriptionId
                },
                callbackDispatcher
            );
            return;
        }
        const provider = this.participantIdToProvider[providerParticipantId];
        // construct subscriptionInfo from subscriptionRequest and participantIds

        const subscriptionInfo = multicast
            ? new SubscriptionInformation(
                  SubscriptionInformation.SUBSCRIPTION_TYPE_MULTICAST,
                  proxyParticipantId,
                  providerParticipantId,
                  subscriptionRequest
              )
            : new SubscriptionInformation(
                  SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST,
                  proxyParticipantId,
                  providerParticipantId,
                  subscriptionRequest
              );

        // in case the subscriptionId is already used in a previous
        // subscription, remove this one
        this.removeSubscription(subscriptionId, true);

        // make sure the provider is registered
        if (provider === undefined) {
            log.warn(`Provider with participantId ${providerParticipantId} not found. Queueing ${requestType}...`);
            this.queuedSubscriptionInfos[subscriptionId] = subscriptionInfo;
            let pendingSubscriptions = this.queuedProviderParticipantIdToSubscriptionRequestsMapping[
                providerParticipantId
            ];
            if (pendingSubscriptions === undefined) {
                pendingSubscriptions = [];
                this.queuedProviderParticipantIdToSubscriptionRequestsMapping[
                    providerParticipantId
                ] = pendingSubscriptions;
            }
            pendingSubscriptions[pendingSubscriptions.length] = subscriptionInfo;
            return;
        }

        // make sure the provider contains the event being subscribed to
        const event = provider[eventName];
        if (event === undefined) {
            exception = new SubscriptionException({
                detailMessage: `error handling ${requestType}: ${JSONSerializer.stringify(
                    subscriptionRequest
                )}. Provider: ${providerParticipantId} misses event ${eventName}`,
                subscriptionId
            });
            log.error(exception.detailMessage);
            this.optionalCallbackDispatcher(
                callbackDispatcherSettings,
                {
                    error: exception,
                    subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // make sure a ProviderEvent is registered
        const subscriptions = this.getSubscriptionsForProviderEvent(providerParticipantId, eventName);
        if (subscriptions === undefined) {
            exception = new SubscriptionException({
                detailMessage: `error handling ${requestType}: ${JSONSerializer.stringify(
                    subscriptionRequest
                )}. ProviderEvent ${eventName} for providerId ${providerParticipantId} is not registered`,
                subscriptionId
            });
            log.error(exception.detailMessage);
            this.optionalCallbackDispatcher(
                callbackDispatcherSettings,
                {
                    error: exception,
                    subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        if (multicast) {
            const multicastId = subscriptionInfo.multicastId;
            if (event.selective) {
                exception = new SubscriptionException({
                    detailMessage: `error handling multicast subscription request: ${JSONSerializer.stringify(
                        subscriptionRequest
                    )}. Provider: ${providerParticipantId} event ${eventName} is marked as selective, which is not allowed for multicasts`,
                    subscriptionId
                });
                log.error(exception.detailMessage);
                this.optionalCallbackDispatcher(
                    callbackDispatcherSettings,
                    {
                        error: exception,
                        subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
            // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
            this.addRequestToMulticastSubscriptions(multicastId!, subscriptionId);
        } else {
            const checkResult = event.checkFilterParameters(subscriptionRequest.filterParameters);
            if (checkResult.caughtErrors.length !== 0) {
                exception = new SubscriptionException({
                    detailMessage: `The incoming subscription request does not contain the expected filter parameters to subscribe to broadcast ${eventName} for providerId ${providerParticipantId}: ${JSON.stringify(
                        checkResult.caughtErrors
                    )}`,
                    subscriptionId
                });
                log.error(exception.detailMessage);
                this.optionalCallbackDispatcher(
                    callbackDispatcherSettings,
                    {
                        error: exception,
                        subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        if (timeToEndDate > 0) {
            // schedule to remove subscription from internal maps
            subscriptionInfo.endDateTimeout = LongTimer.setTimeout(
                this.removeSubscription,
                timeToEndDate,
                subscriptionId
            );
        }

        // save subscriptionInfo to subscriptionId => subscription and
        // ProviderEvent => subscription map
        this.subscriptionInfos[subscriptionId] = subscriptionInfo;
        subscriptions[subscriptionId] = subscriptionInfo;

        this.optionalCallbackDispatcher(
            callbackDispatcherSettings,
            {
                subscriptionId
            },
            callbackDispatcher
        );
    }

    /**
     * @param subscriptionStop incoming subscriptionStop
     */
    public handleSubscriptionStop(subscriptionStop: SubscriptionStop): void {
        this.removeSubscription(subscriptionStop.subscriptionId);
    }

    /**
     * @param providerId
     * @param eventName
     * @returns true if a subscription exists for the given event
     */
    public hasSubscriptionsForProviderEvent(providerId: string, eventName: string): boolean {
        const subscriptions = this.getSubscriptionsForProviderEvent(providerId, eventName);
        let subscriptionId;
        if (subscriptions !== undefined) {
            for (subscriptionId in subscriptions) {
                if (Object.prototype.hasOwnProperty.call(subscriptions, subscriptionId)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @param providerId
     * @param attributeName
     * @returns true if a subscription exists for the given attribute
     */
    public hasSubscriptionsForProviderAttribute(providerId: string, attributeName: string): boolean {
        const subscriptions = this.getSubscriptionsForProviderAttribute(providerId, attributeName);
        let subscriptionId;
        if (subscriptions !== undefined) {
            for (subscriptionId in subscriptions) {
                if (Object.prototype.hasOwnProperty.call(subscriptions, subscriptionId)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Handles SubscriptionRequests
     *
     * @param proxyParticipantId - participantId of proxy consuming the attribute publications
     *            providerParticipantId - participantId of provider producing the attribute publications
     *            subscriptionRequest incoming subscriptionRequest
     *            callbackDispatcher callback function to inform the caller about the handling result
     * @param providerParticipantId
     * @param subscriptionRequest
     * @param callbackDispatcher
     * @param callbackDispatcherSettings
     * @throws {Error} when no provider exists or the provider does not have the attribute
     */
    public handleSubscriptionRequest(
        proxyParticipantId: string,
        providerParticipantId: string,
        subscriptionRequest: SubscriptionRequest,
        callbackDispatcher?: Function,
        callbackDispatcherSettings?: any
    ): void {
        let exception;
        let timeToEndDate = 0;
        const attributeName = subscriptionRequest.subscribedToName;
        const subscriptionId = subscriptionRequest.subscriptionId;

        // if endDate is defined (also exclude default value 0 for
        // the expiryDateMs qos-property)
        if (
            subscriptionRequest.qos !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE
        ) {
            timeToEndDate = subscriptionRequest.qos.expiryDateMs - Date.now();

            // if endDate lies in the past => don't add the subscription
            if (timeToEndDate <= 0) {
                exception = new SubscriptionException({
                    detailMessage: `error handling subscription request: ${JSONSerializer.stringify(
                        subscriptionRequest
                    )}. expiryDateMs ${
                        subscriptionRequest.qos.expiryDateMs
                    } for ProviderAttribute ${attributeName} for providerId ${providerParticipantId} lies in the past`,
                    subscriptionId
                });
                log.error(exception.detailMessage);
                this.optionalCallbackDispatcher(
                    callbackDispatcherSettings,
                    {
                        error: exception,
                        subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        if (!this.isReady()) {
            exception = new SubscriptionException({
                detailMessage: `error handling subscription request: ${JSONSerializer.stringify(
                    subscriptionRequest
                )} and provider ParticipantId ${providerParticipantId}: joynr runtime already shut down`,
                subscriptionId: subscriptionRequest.subscriptionId
            });
            log.debug(exception.detailMessage);
            this.optionalCallbackDispatcher(
                callbackDispatcherSettings,
                {
                    error: exception,
                    subscriptionId: subscriptionRequest.subscriptionId
                },
                callbackDispatcher
            );
            return;
        }
        const provider = this.participantIdToProvider[providerParticipantId];
        // construct subscriptionInfo from subscriptionRequest and participantIds
        const subscriptionInfo = new SubscriptionInformation(
            SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE,
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest
        );

        // in case the subscriptionId is already used in a previous
        // subscription, remove this one
        this.removeSubscription(subscriptionId, true);

        // make sure the provider is registered
        if (provider === undefined) {
            log.info(
                `Provider with participantId ${providerParticipantId} not found. Queueing subscription request...`
            );
            this.queuedSubscriptionInfos[subscriptionId] = subscriptionInfo;
            let pendingSubscriptions = this.queuedProviderParticipantIdToSubscriptionRequestsMapping[
                providerParticipantId
            ];
            if (pendingSubscriptions === undefined) {
                pendingSubscriptions = [];
                this.queuedProviderParticipantIdToSubscriptionRequestsMapping[
                    providerParticipantId
                ] = pendingSubscriptions;
            }
            pendingSubscriptions[pendingSubscriptions.length] = subscriptionInfo;
            return;
        }

        // make sure the provider contains the attribute being subscribed to
        const attribute = provider[attributeName];
        if (attribute === undefined) {
            exception = new SubscriptionException({
                detailMessage: `error handling subscription request: ${JSONSerializer.stringify(
                    subscriptionRequest
                )}. Provider: ${providerParticipantId} misses attribute ${attributeName}`,
                subscriptionId
            });
            log.error(exception.detailMessage);
            this.optionalCallbackDispatcher(
                callbackDispatcherSettings,
                {
                    error: exception,
                    subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // make sure the provider attribute is a notifiable provider attribute
        // (e.g.: ProviderAttributeNotify[Read][Write])
        if (!this.providerAttributeIsNotifiable(attribute)) {
            exception = new SubscriptionException({
                detailMessage: `error handling subscription request: ${JSONSerializer.stringify(
                    subscriptionRequest
                )}. Provider: ${providerParticipantId} attribute ${attributeName} is not notifiable`,
                subscriptionId
            });
            log.error(exception.detailMessage);
            this.optionalCallbackDispatcher(
                callbackDispatcherSettings,
                {
                    error: exception,
                    subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // make sure a ProviderAttribute is registered
        const subscriptions = this.getSubscriptionsForProviderAttribute(providerParticipantId, attributeName);
        if (subscriptions === undefined) {
            exception = new SubscriptionException({
                detailMessage: `error handling subscription request: ${JSONSerializer.stringify(
                    subscriptionRequest
                )}. ProviderAttribute ${attributeName} for providerId ${providerParticipantId} is not registered or notifiable`,
                subscriptionId
            });
            log.error(exception.detailMessage);
            this.optionalCallbackDispatcher(
                callbackDispatcherSettings,
                {
                    error: exception,
                    subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // Set up publication interval if maxIntervalMs is a number
        //(not (is not a number)) ...
        const periodMs = this.getPeriod(subscriptionInfo);

        if (periodMs && !isNaN(periodMs)) {
            if (periodMs < PeriodicSubscriptionQos.MIN_PERIOD_MS) {
                exception = new SubscriptionException({
                    detailMessage: `error handling subscription request: ${JSONSerializer.stringify(
                        subscriptionRequest
                    )}. periodMs ${periodMs} is smaller than PeriodicSubscriptionQos.MIN_PERIOD_MS ${
                        PeriodicSubscriptionQos.MIN_PERIOD_MS
                    }`,
                    subscriptionId
                });
                log.error(exception.detailMessage);
                this.optionalCallbackDispatcher(
                    callbackDispatcherSettings,
                    {
                        error: exception,
                        subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        if (timeToEndDate > 0) {
            // schedule to remove subscription from internal maps
            subscriptionInfo.endDateTimeout = LongTimer.setTimeout(
                this.removeSubscription,
                timeToEndDate,
                subscriptionId
            );
        }

        // call the get method on the provider at the set interval
        subscriptionInfo.subscriptionInterval = this.triggerPublicationTimer(subscriptionInfo, periodMs);

        // save subscriptionInfo to subscriptionId => subscription and
        // ProviderAttribute => subscription map
        this.subscriptionInfos[subscriptionId] = subscriptionInfo;
        subscriptions[subscriptionId] = subscriptionInfo;

        this.triggerPublication(subscriptionInfo);
        this.optionalCallbackDispatcher(callbackDispatcherSettings, { subscriptionId }, callbackDispatcher);
    }

    /**
     * Handles BroadcastSubscriptionRequests
     *
     * @param proxyParticipantId - participantId of proxy consuming the broadcast
     *            providerParticipantId - participantId of provider producing the broadcast
     *            subscriptionRequest incoming subscriptionRequest
     *            callbackDispatcher callback function to inform the caller about the handling result
     * @param providerParticipantId
     * @param subscriptionRequest
     * @param callbackDispatcher
     * @param callbackDispatcherSettings
     * @throws {Error} when no provider exists or the provider does not have the event
     */
    public handleBroadcastSubscriptionRequest(
        proxyParticipantId: string,
        providerParticipantId: string,
        subscriptionRequest: BroadcastSubscriptionRequest,
        callbackDispatcher?: Function,
        callbackDispatcherSettings?: any
    ): void {
        return this.handleBroadcastSubscriptionRequestInternal(
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest,
            callbackDispatcher,
            callbackDispatcherSettings,
            false
        );
    }

    /**
     * Handles MulticastSubscriptionRequests
     *
     * @param proxyParticipantId
     * @param providerParticipantId - participantId of provider producing the multicast
     *            subscriptionRequest incoming subscriptionRequest
     *            callbackDispatcher callback function to inform the caller about the handling result
     * @param subscriptionRequest
     * @param callbackDispatcher
     * @param callbackDispatcherSettings
     * @throws {Error} when no provider exists or the provider does not have the event
     */
    public handleMulticastSubscriptionRequest(
        proxyParticipantId: string,
        providerParticipantId: string,
        subscriptionRequest: MulticastSubscriptionRequest,
        callbackDispatcher?: Function,
        callbackDispatcherSettings?: any
    ): void {
        return this.handleBroadcastSubscriptionRequestInternal(
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest,
            callbackDispatcher,
            callbackDispatcherSettings,
            true
        );
    }

    /**
     * Unregisters all attributes of a provider to handle subscription requests
     *
     * @param participantId of the provider handling the subsription
     * @param provider
     */
    public removePublicationProvider(participantId: string, provider: any): void {
        if (!this.isReady()) {
            throw new Error("PublicationManager is already shut down");
        }
        let propertyName;

        // cycles over all provider members
        for (propertyName in provider) {
            if (Object.prototype.hasOwnProperty.call(provider, propertyName)) {
                // checks whether the member is a notifiable provider attribute
                // and adds it if this is the case
                if (this.providerAttributeIsNotifiable(provider[propertyName])) {
                    this.removePublicationAttribute(participantId, propertyName, provider[propertyName]);
                }

                // checks whether the member is an event
                // and adds it if this is the case
                if (this.propertyIsProviderEvent(provider[propertyName])) {
                    this.removePublicationEvent(participantId, propertyName, provider[propertyName]);
                }
            }
        }

        // stores the participantId to
        delete this.participantIdToProvider[participantId];
    }

    /**
     * Registers all attributes of a provider to handle subscription requests
     *
     * @param participantId of the provider handling the subsription
     * @param provider
     */
    public addPublicationProvider(participantId: string, provider: any): void {
        let pendingSubscription, subscriptionObject;
        if (!this.isReady()) {
            throw new Error("PublicationManager is already shut down");
        }

        // stores the participantId to
        this.participantIdToProvider[participantId] = provider;

        // cycles over all provider members
        for (const propertyName in provider) {
            if (Object.prototype.hasOwnProperty.call(provider, propertyName)) {
                // checks whether the member is a notifiable provider attribute
                // and adds it if this is the case
                if (this.providerAttributeIsNotifiable(provider[propertyName])) {
                    this.addPublicationAttribute(participantId, propertyName, provider[propertyName]);
                }

                // checks whether the member is a event
                // and adds it if this is the case
                if (this.propertyIsProviderEvent(provider[propertyName])) {
                    this.addPublicationEvent(participantId, propertyName, provider[propertyName]);
                }
            }
        }

        const pendingSubscriptions = this.queuedProviderParticipantIdToSubscriptionRequestsMapping[participantId];
        if (pendingSubscriptions !== undefined) {
            for (pendingSubscription in pendingSubscriptions) {
                if (Object.prototype.hasOwnProperty.call(pendingSubscriptions, pendingSubscription)) {
                    subscriptionObject = pendingSubscriptions[pendingSubscription];
                    delete pendingSubscriptions[pendingSubscription];

                    if (subscriptionObject.subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE) {
                        // call attribute subscription handler
                        this.handleSubscriptionRequest(
                            subscriptionObject.proxyParticipantId,
                            subscriptionObject.providerParticipantId,
                            subscriptionObject
                        );
                    } else if (
                        subscriptionObject.subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST
                    ) {
                        this.handleBroadcastSubscriptionRequest(
                            subscriptionObject.proxyParticipantId,
                            subscriptionObject.providerParticipantId,
                            subscriptionObject
                        );
                    } else {
                        this.handleMulticastSubscriptionRequest(
                            subscriptionObject.proxyParticipantId,
                            subscriptionObject.providerParticipantId,
                            subscriptionObject
                        );
                    }
                }
            }
        }
    }

    public hasMulticastSubscriptions(): boolean {
        return Object.keys(this.multicastSubscriptions).length > 0;
    }

    public hasSubscriptions(): boolean {
        const hasSubscriptionInfos = Object.keys(this.subscriptionInfos).length > 0;

        const hasQueuedSubscriptionInfos = Object.keys(this.queuedSubscriptionInfos).length > 0;

        const hasQueuedProviderParticipantIdToSubscriptionRequestsMapping =
            Object.keys(this.queuedProviderParticipantIdToSubscriptionRequestsMapping).length > 0;

        const hasOnChangeProviderAttributeToSubscriptions =
            Object.keys(this.onChangeProviderAttributeToSubscriptions).length > 0;

        const hasOnChangeProviderEventToSubscriptions =
            Object.keys(this.onChangeProviderEventToSubscriptions).length > 0;

        return (
            hasSubscriptionInfos ||
            hasQueuedSubscriptionInfos ||
            hasQueuedProviderParticipantIdToSubscriptionRequestsMapping ||
            hasOnChangeProviderAttributeToSubscriptions ||
            hasOnChangeProviderEventToSubscriptions ||
            this.hasMulticastSubscriptions()
        );
    }

    /**
     * Shutdown the publication manager
     */
    public shutdown(): void {
        let subscriptionId;
        for (subscriptionId in this.subscriptionInfos) {
            if (Object.prototype.hasOwnProperty.call(this.subscriptionInfos, subscriptionId)) {
                const subscriptionInfo = this.subscriptionInfos[subscriptionId];
                if (subscriptionInfo.subscriptionInterval !== undefined) {
                    LongTimer.clearTimeout(subscriptionInfo.subscriptionInterval);
                }
                if (subscriptionInfo.onChangeDebounce !== undefined) {
                    LongTimer.clearTimeout(subscriptionInfo.onChangeDebounce);
                }
            }
        }
        this.started = false;
    }
}

export = PublicationManager;
