/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import SubscriptionRequest from "../../../../../main/js/joynr/dispatching/types/SubscriptionRequest";
import PeriodicSubscriptionQos from "../../../../../main/js/joynr/proxy/PeriodicSubscriptionQos";
import SubscriptionQos = require("../../../../../main/js/joynr/proxy/SubscriptionQos");

describe("libjoynr-js.joynr.dispatching.types.SubscriptionRequest", () => {
    const qosSettings = {
        periodMs: 50,
        expiryDateMs: 3,
        alertAfterIntervalMs: 80,
        publicationTtlMs: 100
    };

    it("is defined", () => {
        expect(SubscriptionRequest).toBeDefined();
    });

    it("is instantiable", () => {
        const subscriptionRequest = new SubscriptionRequest({
            subscribedToName: "attributeName",
            subscriptionId: "testSubscriptionId",
            qos: new SubscriptionQos()
        });
        expect(subscriptionRequest).toBeDefined();
        expect(subscriptionRequest).not.toBeNull();
        expect(typeof subscriptionRequest === "object").toBeTruthy();
    });

    it("is constructs with correct member values", () => {
        const subscribedToName = "attributeName";
        const qos = new PeriodicSubscriptionQos(qosSettings);
        const subscriptionId = "testSubscriptionId";

        const subscriptionRequest = new SubscriptionRequest({
            subscribedToName,
            qos,
            subscriptionId
        });

        expect(subscriptionRequest.subscribedToName).toEqual(subscribedToName);
        expect(subscriptionRequest.qos).toEqual(qos);
        expect(subscriptionRequest.subscriptionId).toEqual(subscriptionId);
    });
});
