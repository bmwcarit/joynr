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
import * as UtilInternal from "../util/UtilInternal";
import * as Request from "../dispatching/types/Request";
import MessagingQos from "../messaging/MessagingQos";
import * as Typing from "../util/Typing";
import RequestReplyManager = require("../dispatching/RequestReplyManager");
import SubscriptionManager = require("../dispatching/subscription/SubscriptionManager");
import DiscoveryQos = require("./DiscoveryQos");
import SubscriptionQos = require("./SubscriptionQos");

type ProxyAttributeConstructor<T = ProxyAttribute> = new (...args: any[]) => T;

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function asRead<TBase extends ProxyAttributeConstructor, T = unknown>(superclass: TBase) {
    return class Readable extends superclass {
        /**
         * Getter for attribute
         *
         * @param [settings] the settings object for this function call
         * @returns returns an A+ promise object
         */
        public get(settings?: { messagingQos?: MessagingQos }): Promise<T> {
            // ensure settings variable holds a valid object and initialize
            // deferred object
            settings = settings || {};
            const request = Request.create({
                methodName: `get${UtilInternal.firstUpper(this.attributeName)}`
            });
            return this.executeRequest(request, settings);
        }
    };
}

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function asWrite<TBase extends ProxyAttributeConstructor, T = unknown>(superclass: TBase) {
    return class WriteAble extends superclass {
        /**
         * Setter for attribute
         *
         * @param settings - the settings object for this function call
         * @param settings.value - the attribute value to set
         * @returns returns an A+ promise
         */
        public set(settings: { value: T; messagingQos?: MessagingQos }): Promise<void> {
            // ensure settings variable holds a valid object and initialize deferred
            // object
            settings = settings || {};
            try {
                settings.value = Typing.augmentTypes(settings.value);
            } catch (e) {
                return Promise.reject(new Error(`error setting attribute: ${this.attributeName}: ${e.toString()}`));
            }

            const request = Request.create({
                methodName: `set${UtilInternal.firstUpper(this.attributeName)}`,
                paramDatatypes: [this.attributeType],
                params: [settings.value]
            });
            return this.executeRequest(request, settings);
        }
    };
}

export interface SubscribeSettings<T> {
    subscriptionQos: SubscriptionQos;
    onReceive: (value: T) => void;
    onError?: (e: Error) => void;
    onSubscribed?: (participantId: string) => void;
    subscriptionId?: string;
}

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function asNotify<TBase extends ProxyAttributeConstructor, T = unknown>(superclass: TBase) {
    return class Notifiable extends superclass {
        /**
         * Subscription to isOn attribute
         *
         * @param settings the settings object for this function call
         * @param settings.subscriptionQos - the subscription quality of service object
         * @param settings.onReceive this function is called if the attribute has
         *            been published successfully, method signature:
         *            "void onReceive({?}value)"
         * @param settings.onError - this function is called if a publication of the attribute value was
         *          missed, method signature: "void onError({Error} error)"
         * @param settings.onSubscribed - the callback to inform once the subscription request has been
         *          delivered successfully
         * @param [settings.subscriptionId] - optional subscriptionId to be used for the new subscription
         * @returns returns an A+ promise object
         */
        public subscribe(settings: SubscribeSettings<T>): Promise<string> {
            // return promise to caller
            return this.settings.dependencies.subscriptionManager.registerSubscription({
                proxyId: this.parent.proxyParticipantId,
                providerDiscoveryEntry: this.parent.providerDiscoveryEntry,
                attributeName: this.attributeName,
                attributeType: this.attributeType,
                qos: settings.subscriptionQos,
                subscriptionId: settings.subscriptionId,
                onReceive: settings.onReceive,
                onError: settings.onError,
                onSubscribed: settings.onSubscribed
            });
        }

        /**
         * Unsubscribe from the attribute
         *
         * @param settings - the settings object for this function call
         * @param settings.subscriptionId - the subscription id retrieved from the subscribe function
         * @returns returns an A+ promise object
         *
         * @see ProxyAttribute#subscribe
         */
        public unsubscribe(settings: { subscriptionId: string; messagingQos?: MessagingQos }): Promise<void> {
            // passed in (right-most) messagingQos have precedence; undefined values are
            // ignored
            const messagingQos = new MessagingQos(
                UtilInternal.extend({}, this.parent.messagingQos, this.settings.messagingQos, settings.messagingQos)
            );

            // return promise to caller
            return this.settings.dependencies.subscriptionManager.unregisterSubscription({
                messagingQos,
                subscriptionId: settings.subscriptionId
            });
        }
    };
}

function sendRequestOnSuccess<T>(settings: { response: [T]; settings: string }): T {
    const response = settings.response;
    const attributeType = settings.settings;
    return Typing.augmentTypes(response[0], attributeType);
}

class ProxyAttribute {
    /** protected property, needs to be public for tsc -d */
    public attributeType: string;
    /** protected property, needs to be public for tsc -d */
    public attributeName: string;
    public settings: {
        dependencies: {
            requestReplyManager: RequestReplyManager;
            subscriptionManager: SubscriptionManager;
        };
        discoveryQos: DiscoveryQos;
        messagingQos: MessagingQos;
    };
    /** protected property, needs to be public for tsc -d */
    public parent: {
        proxyParticipantId: string;
        providerDiscoveryEntry: DiscoveryEntryWithMetaInfo;
        messagingQos?: MessagingQos;
    };
    /**
     * Constructor of ProxyAttribute object that is used in the generation of proxy objects
     *
     * @param parent - the proxy object that contains this attribute
     * @param parent.proxyParticipantId - participantId of the proxy itself
     * @param parent.providerDiscoveryEntry.participantId - participantId of the provider being addressed
     * @param settings - the settings object for this function call
     * @param settings.dependencies - the dependencies object for this function call
     * @param settings.dependencies.requestReplyManager
     * @param settings.dependencies.subscriptionManager
     * @param settings.discoveryQos - the Quality of Service parameters for arbitration
     * @param settings.messagingQos - the Quality of Service parameters for messaging
     * @param attributeName - the name of the attribute
     * @param attributeType - the type of the attribute
     */
    public constructor(
        parent: {
            proxyParticipantId: string;
            providerDiscoveryEntry: DiscoveryEntryWithMetaInfo;
            settings: {
                dependencies: {
                    requestReplyManager: RequestReplyManager;
                    subscriptionManager: SubscriptionManager;
                };
                discoveryQos: DiscoveryQos;
                messagingQos: MessagingQos;
            };
        },
        attributeName: string,
        attributeType: string
    ) {
        this.parent = parent;
        this.settings = parent.settings;
        this.attributeName = attributeName;
        this.attributeType = attributeType;
    }

    /**
     * protected property, but public due to tsc -d
     *
     * @param request
     * @param requestSettings
     * @param requestSettings.messagingQos
     * @returns an A+ promise
     */
    public executeRequest(request: Request.Request, requestSettings: { messagingQos?: MessagingQos }): Promise<any> {
        // passed in (right-most) messagingQos have precedence; undefined values are ignored
        const messagingQos = UtilInternal.extend(
            new MessagingQos(),
            this.parent.messagingQos,
            this.settings.messagingQos,
            requestSettings.messagingQos
        );

        // return promise to caller
        return (this.settings.dependencies.requestReplyManager.sendRequest as any)(
            {
                toDiscoveryEntry: this.parent.providerDiscoveryEntry,
                from: this.parent.proxyParticipantId,
                messagingQos,
                request
            },
            this.attributeType
        ).then(sendRequestOnSuccess);
    }
}
/*
    The empty container classes below are declared for typing purposes.
    Unfortunately declarations need to be repeated since it's impossible to give template parameters
    to mixin classes. See https://github.com/Microsoft/TypeScript/issues/26154
 */
const ProxyReadAttributeImpl = asRead<ProxyAttributeConstructor>(ProxyAttribute);
export class ProxyReadAttribute<T> extends ProxyReadAttributeImpl {
    public get!: (settings?: { messagingQos?: MessagingQos }) => Promise<T>;
}

const ProxyReadWriteAttributeImpl = asWrite<ProxyAttributeConstructor>(ProxyReadAttribute);
export class ProxyReadWriteAttribute<T> extends ProxyReadWriteAttributeImpl {
    public get!: (settings?: { messagingQos?: MessagingQos }) => Promise<T>;
    public set!: (settings: { value: T; messagingQos?: MessagingQos }) => Promise<void>;
}
const ProxyReadWriteNotifyAttributeImpl = asNotify<ProxyAttributeConstructor>(ProxyReadWriteAttribute);
export class ProxyReadWriteNotifyAttribute<T> extends ProxyReadWriteNotifyAttributeImpl {
    public get!: (settings?: { messagingQos?: MessagingQos }) => Promise<T>;
    public set!: (settings: { value: T; messagingQos?: MessagingQos }) => Promise<void>;
    public subscribe!: (settings: SubscribeSettings<T>) => Promise<any>;
    public unsubscribe!: (settings: { subscriptionId: string; messagingQos?: MessagingQos }) => Promise<void>;
}
const ProxyReadNotifyAttributeImpl = asNotify<ProxyAttributeConstructor>(ProxyReadAttribute);
export class ProxyReadNotifyAttribute<T> extends ProxyReadNotifyAttributeImpl {
    public get!: (settings?: { messagingQos?: MessagingQos }) => Promise<T>;
    public subscribe!: (settings: SubscribeSettings<T>) => Promise<any>;
    public unsubscribe!: (settings: { subscriptionId: string; messagingQos?: MessagingQos }) => Promise<void>;
}
