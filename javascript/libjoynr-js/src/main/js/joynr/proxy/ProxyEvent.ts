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
import BroadcastFilterParameters from "./BroadcastFilterParameters";

import * as SubscriptionUtil from "../dispatching/subscription/util/SubscriptionUtil";
import MessagingQos from "../messaging/MessagingQos";
import SubscriptionManager from "../dispatching/subscription/SubscriptionManager";
import SubscriptionQos from "./SubscriptionQos";
import DiscoveryQos = require("./DiscoveryQos");

/**
 * Checks if the given datatypes and values match the given broadcast parameters
 *
 * @param unnamedBroadcastValues an array containing the unnamedBroadcastValues, e.g. [1234, "asdf"]
 * @param broadcastParameter an array of supported parametes
 * @returns undefined if unnamedBroadcastValues does not match broadcastSignature
 */
function getNamedParameters(unnamedBroadcastValues: any[], broadcastParameter: any[]): Record<string, any> | undefined {
    const namedParameters: Record<string, any> = {};

    // check if number of given parameters matches number
    // of parameters in broadcast signature (keys.length)
    if (unnamedBroadcastValues.length !== broadcastParameter.length) {
        return undefined;
    }

    // cycle over all parameters
    for (let i = 0; i < unnamedBroadcastValues.length; ++i) {
        const parameter = broadcastParameter[i];
        namedParameters[parameter.name] = unnamedBroadcastValues[i];
    }

    return namedParameters;
}

class ProxyEvent {
    private parent: any;
    private settings: {
        discoveryQos: DiscoveryQos;
        messagingQos: MessagingQos;
        dependencies: { subscriptionManager: SubscriptionManager };
        selective?: boolean;
        broadcastName: string;
        broadcastParameter: { name: string; type: string }[];
        filterParameters?: Record<string, any>;
    };
    /**
     * Constructor of ProxyEvent object that is used in the generation of proxy objects
     *
     * @constructor
     * @param parent is the proxy object that contains this attribute
     * @param settings the settings for this broadcast proxy
     * @param settings.discoveryQos the Quality of Service parameters for arbitration
     * @param settings.messagingQos the Quality of Service parameters for messaging
     * @param settings.dependencies the dependencies object for this function call
     * @param settings.dependencies.subscriptionManager
     * @param settings.selective true if the broadcast is selective
     * @param settings.broadcastName the name of the broadcast as modelled in Franca
     * @param settings.broadcastParameter the parameter meta information of the broadcast being subscribed to
     * @param settings.filterParameters the filter parameters of the broadcast
     * @returns {ProxyEvent} */
    public constructor(
        parent: any,
        settings: {
            discoveryQos: DiscoveryQos;
            messagingQos: MessagingQos;
            dependencies: { subscriptionManager: SubscriptionManager };
            selective?: boolean;
            broadcastName: string;
            broadcastParameter: { name: string; type: string }[];
            filterParameters?: Record<string, any>;
        }
    ) {
        this.settings = settings;
        this.parent = parent;
    }

    /**
     * @param subscribeParameters the settings object for this function call
     * @param subscribeParameters.subscriptionQos the subscription quality of service object
     * @param [subscribeParameters.subscriptionId] optional subscriptionId. Used to refresh or
     *            reinstate an existing subscription.
     * @param [subscribeParameters.partitions] optional parameter for multicast subscriptions.
     *            This parameter becomes relevant for non selective broadcasts and specifies the interested partitions
     *            of publications. It is interpreted hierarchically.
     * @param subscribeParameters.onReceive this function is called when an event as been
     *            received. method signature: "void onReceive({?}value)"
     * @param subscribeParameters.onError this function is called when an error occurs with
     *            a subscribed event. method signature: "void onError({Error} error)"
     * @param subscribeParameters.onSubscribed the callback to inform once the subscription request has
     *            been delivered successfully
     * @returns returns a promise that is resolved with the subscriptionId, which is to
     *          be used to unsubscribe from this subscription later.
     * @throws {Error} if subscribeParameters.partitions contains invalid characters
     */
    public subscribe(subscribeParameters: {
        subscriptionQos: SubscriptionQos;
        subscriptionId?: string;
        partitions?: string[];
        onReceive: Function;
        onError?: (error: any) => void;
        onSubscribed?: Function;
        filterParameters?: any;
    }): Promise<string> {
        SubscriptionUtil.validatePartitions(subscribeParameters.partitions);
        if (subscribeParameters.filterParameters !== undefined && subscribeParameters.filterParameters !== null) {
            const checkResult = SubscriptionUtil.checkFilterParameters(
                this.settings.filterParameters as Record<string, any>,
                subscribeParameters.filterParameters.filterParameters,
                this.settings.broadcastName
            );
            if (checkResult.caughtErrors.length !== 0) {
                const errorMessage = JSON.stringify(checkResult.caughtErrors);
                return Promise.reject(
                    new Error(
                        `SubscriptionRequest could not be processed, as the filterParameters "${JSON.stringify(
                            subscribeParameters.filterParameters
                        )}" are wrong: ${errorMessage}`
                    )
                );
            }
        }
        return this.settings.dependencies.subscriptionManager.registerBroadcastSubscription({
            proxyId: this.parent.proxyParticipantId,
            providerDiscoveryEntry: this.parent.providerDiscoveryEntry,
            broadcastName: this.settings.broadcastName,
            broadcastParameter: this.settings.broadcastParameter,
            subscriptionQos: subscribeParameters.subscriptionQos,
            subscriptionId: subscribeParameters.subscriptionId,
            onReceive: (response: any) => {
                subscribeParameters.onReceive(getNamedParameters(response, this.settings.broadcastParameter));
            },
            selective: this.settings.selective,
            partitions: subscribeParameters.partitions || [],
            onError: subscribeParameters.onError,
            onSubscribed: subscribeParameters.onSubscribed,
            filterParameters: subscribeParameters.filterParameters
        });
    }

    public createFilterParameters(): BroadcastFilterParameters {
        return new BroadcastFilterParameters(this.settings.filterParameters);
    }

    /**
     * @param unsubscribeParameters the settings object for this function call
     * @param unsubscribeParameters.subscriptionId the subscription token retrieved from the
     *            subscribe function
     * @returns returns a promise that is resolved when unsubscribe has been executed.
     * @see ProxyEvent#subscribe
     */
    public unsubscribe(unsubscribeParameters: { subscriptionId: string }): Promise<void> {
        return this.settings.dependencies.subscriptionManager.unregisterSubscription({
            messagingQos: this.settings.messagingQos,
            subscriptionId: unsubscribeParameters.subscriptionId
        });
    }
}

export = ProxyEvent;
