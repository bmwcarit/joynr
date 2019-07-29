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
import OnChangeSubscriptionQos from "../../proxy/OnChangeSubscriptionQos";
import BroadcastFilterParameters = require("../../proxy/BroadcastFilterParameters");

const defaultSettings = {
    qos: new OnChangeSubscriptionQos()
};

class BroadcastSubscriptionRequest {
    public qos: OnChangeSubscriptionQos;

    /**
     * @name BroadcastSubscriptionRequest#subscribedToName
     * @type String
     */
    public subscribedToName: string;

    /**
     * @name BroadcastSubscriptionRequest#subscriptionId
     * @type String
     */
    public subscriptionId: string;

    public filterParameters?: BroadcastFilterParameters;

    public _typeName = "joynr.BroadcastSubscriptionRequest";
    public static _typeName = "joynr.BroadcastSubscriptionRequest";

    /**
     * @constructor
     * @param settings.subscriptionId Id of the new subscription
     * @param settings.subscribedToName the name of the element to subscribe to
     * @param [settings.subscriptionQos] the subscriptionQos
     */
    public constructor(settings: {
        subscriptionId: string;
        subscribedToName: string;
        filterParameters?: BroadcastFilterParameters;
        qos: OnChangeSubscriptionQos;
    }) {
        this.subscriptionId = settings.subscriptionId;
        this.subscribedToName = settings.subscribedToName;
        this.qos = settings.qos || defaultSettings.qos;
        this.filterParameters = settings.filterParameters;
    }
}

export = BroadcastSubscriptionRequest;
