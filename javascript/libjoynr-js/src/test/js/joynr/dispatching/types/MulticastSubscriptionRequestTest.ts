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

import MulticastSubscriptionRequest from "joynr/joynr/dispatching/types/MulticastSubscriptionRequest";
import MulticastSubscriptionQos from "joynr/joynr/proxy/MulticastSubscriptionQos";

describe("libjoynr-js.joynr.dispatching.types.MulticastSubscriptionRequest", () => {
    const qosSettings = {
        expiryDateMs: 1
    };

    it("is defined", () => {
        expect(MulticastSubscriptionRequest).toBeDefined();
    });

    it("is instantiable", () => {
        const multicastSubscriptionRequest = new MulticastSubscriptionRequest({
            multicastId: "multicastId",
            subscribedToName: "multicastName",
            subscriptionId: "testSubscriptionId",
            qos: new MulticastSubscriptionQos(qosSettings)
        });
        expect(multicastSubscriptionRequest).toBeDefined();
        expect(multicastSubscriptionRequest).not.toBeNull();
        expect(typeof multicastSubscriptionRequest === "object").toBeTruthy();
        expect(multicastSubscriptionRequest instanceof MulticastSubscriptionRequest).toBeTruthy();
    });

    it("is constructs with correct member values", () => {
        const multicastId = "multicastId";
        const subscribedToName = "subscribedToName";
        const subscriptionQos = new MulticastSubscriptionQos(qosSettings);
        const subscriptionId = "testSubscriptionId";

        const subscriptionRequest = new MulticastSubscriptionRequest({
            multicastId: "multicastId",
            subscribedToName,
            qos: subscriptionQos,
            subscriptionId
        });

        expect(subscriptionRequest.multicastId).toEqual(multicastId);
        expect(subscriptionRequest.subscribedToName).toEqual(subscribedToName);
        expect(subscriptionRequest.qos).toEqual(subscriptionQos);
        expect(subscriptionRequest.subscriptionId).toEqual(subscriptionId);
    });
});
