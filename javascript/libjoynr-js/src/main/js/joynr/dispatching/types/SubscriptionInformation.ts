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
import MulticastSubscriptionRequest = require("./MulticastSubscriptionRequest");
import BroadcastSubscriptionRequest = require("./BroadcastSubscriptionRequest");
import BroadcastFilterParameters = require("../../proxy/BroadcastFilterParameters");
import SubscriptionQos = require("../../proxy/SubscriptionQos");
import SubscriptionRequest = require("./SubscriptionRequest");
import MulticastSubscriptionQos = require("../../proxy/MulticastSubscriptionQos");
import OnChangeSubscriptionQos = require("../../proxy/OnChangeSubscriptionQos");
import OnChangeWithKeepAliveSubscriptionQos = require("../../proxy/OnChangeWithKeepAliveSubscriptionQos");
import PeriodicSubscriptionQos from "../../proxy/PeriodicSubscriptionQos";

type SubscriptionType = "subscription_type_multicast" | "subscription_type_broadcast" | "subscription_type_attribute";
class SubscriptionInformation {
    public static SUBSCRIPTION_TYPE_MULTICAST: "subscription_type_multicast" = "subscription_type_multicast";
    public static SUBSCRIPTION_TYPE_BROADCAST: "subscription_type_broadcast" = "subscription_type_broadcast";
    public static SUBSCRIPTION_TYPE_ATTRIBUTE: "subscription_type_attribute" = "subscription_type_attribute";
    public qos: SubscriptionQos &
        MulticastSubscriptionQos &
        OnChangeSubscriptionQos &
        OnChangeWithKeepAliveSubscriptionQos &
        PeriodicSubscriptionQos;
    public subscribedToName: string;
    public subscriptionId: string;
    public providerParticipantId: string;
    public proxyParticipantId: string;
    public subscriptionType: SubscriptionType;
    public multicastId?: string;
    public lastPublication?: number;
    public filterParameters?: BroadcastFilterParameters;
    public _typeName = "joynr.SubscriptionInformation";
    /** Return type due to LongTimer.setTimeout */
    public subscriptionInterval?: string | number;
    public endDateTimeout?: string | number;
    /** Return type due to LongTimer.setTimeout */
    public onChangeDebounce?: string | number;

    /**
     * @constructor
     * @param subscriptionType
     * @param proxyParticipantId Id of the proxy
     * @param providerParticipantId Id of the provider
     * @param [request] the subscription request
     */
    public constructor(
        subscriptionType: "subscription_type_multicast",
        proxyParticipantId: string,
        providerParticipantId: string,
        request: MulticastSubscriptionRequest
    );
    public constructor(
        subscriptionType: "subscription_type_broadcast",
        proxyParticipantId: string,
        providerParticipantId: string,
        request: BroadcastSubscriptionRequest
    );
    public constructor(
        subscriptionType: "subscription_type_attribute",
        proxyParticipantId: string,
        providerParticipantId: string,
        request: SubscriptionRequest
    );
    public constructor(
        subscriptionType: SubscriptionType,
        proxyParticipantId: string,
        providerParticipantId: string,
        request: any
    ) {
        this.subscriptionType = subscriptionType;
        this.proxyParticipantId = proxyParticipantId;
        this.providerParticipantId = providerParticipantId;
        this.subscriptionId = request.subscriptionId;
        this.subscribedToName = request.subscribedToName;
        this.qos = request.qos;
        if (subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_MULTICAST) {
            this.multicastId = request.multicastId;
        } else {
            this.lastPublication = 0;
            if (subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST) {
                this.filterParameters = request.filterParameters;
            }
        }

        /*
         * the following members may contain native timer objects, which cannot
         * be serialized via JSON.stringify(), hence they must be excluded
         */
        Object.defineProperty(this, "endDateTimeout", {
            enumerable: false,
            configurable: false,
            writable: true,
            value: undefined
        });
        Object.defineProperty(this, "subscriptionInterval", {
            enumerable: false,
            configurable: false,
            writable: true,
            value: undefined
        });
    }
}

export = SubscriptionInformation;
