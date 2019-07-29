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
import MulticastSubscriptionQos from "../../proxy/MulticastSubscriptionQos";
import LoggingManager from "../../system/LoggingManager";
import OnChangeSubscriptionQos = require("../../proxy/OnChangeSubscriptionQos");
const log = LoggingManager.getLogger("joynr/dispatching/types/MulticastSubscriptionRequest");
const defaultSettings = {
    qos: new MulticastSubscriptionQos()
};

class MulticastSubscriptionRequest {
    public qos: MulticastSubscriptionQos;
    public subscribedToName: string;
    public subscriptionId: string;
    public multicastId: string;
    public _typeName = "joynr.MulticastSubscriptionRequest";
    public static _typeName = "joynr.MulticastSubscriptionRequest";

    /**
     * @constructor
     * @param settings.subscriptionId Id of the new subscription
     * @param settings.subscribedToName the name of the element to subscribe to
     * @param [settings.subscriptionQos] the subscriptionQos
     */
    public constructor(settings: {
        subscriptionId: string;
        subscribedToName: string;
        multicastId: string;
        qos?: MulticastSubscriptionQos;
    }) {
        if (settings.qos instanceof OnChangeSubscriptionQos) {
            log.warn(
                "multicast subscription was passed an OnChangeSubscriptionQos. " +
                    "The minIntervalMs and publicationTtlMs will be discarded"
            );
            settings.qos = new MulticastSubscriptionQos({
                expiryDateMs: settings.qos.expiryDateMs
            });
        }

        this.multicastId = settings.multicastId;
        this.subscriptionId = settings.subscriptionId;
        this.subscribedToName = settings.subscribedToName;
        this.qos = settings.qos || defaultSettings.qos;
    }
}

export = MulticastSubscriptionRequest;
