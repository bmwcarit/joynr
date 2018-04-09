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
require("../../../node-unit-test-helper");
const SubscriptionPublication = require("../../../../../main/js/joynr/dispatching/types/SubscriptionPublication");
const RadioStation = require("../../../../generated/joynr/vehicle/radiotypes/RadioStation");

describe("libjoynr-js.joynr.dispatching.types.SubscriptionPublication", () => {
    it("is defined", () => {
        expect(SubscriptionPublication).toBeDefined();
    });

    it("is instantiable", () => {
        const response = "response";
        const publication = new SubscriptionPublication({
            subscriptionId: "testSubscriptionId",
            response
        });
        expect(publication).toBeDefined();
        expect(publication).not.toBeNull();
        expect(typeof publication === "object").toBeTruthy();
        expect(publication instanceof SubscriptionPublication).toBeTruthy();
    });

    it("is constructs with correct member values", () => {
        const subscriptionId = "testSubscriptionId";
        const response = "response";

        const publication = new SubscriptionPublication({
            subscriptionId,
            response
        });

        expect(publication.subscriptionId).toEqual(subscriptionId);
        expect(publication.response).toEqual(response);
    });
});
