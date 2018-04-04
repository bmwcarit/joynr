/*jslint node: true */

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
var SubscriptionPublication = require("../../../../../main/js/joynr/dispatching/types/SubscriptionPublication");
var RadioStation = require("../../../../generated/joynr/vehicle/radiotypes/RadioStation");

describe("libjoynr-js.joynr.dispatching.types.SubscriptionPublication", function() {
    it("is defined", function() {
        expect(SubscriptionPublication).toBeDefined();
    });

    it("is instantiable", function() {
        var response = "response";
        var publication = new SubscriptionPublication({
            subscriptionId: "testSubscriptionId",
            response: response
        });
        expect(publication).toBeDefined();
        expect(publication).not.toBeNull();
        expect(typeof publication === "object").toBeTruthy();
        expect(publication instanceof SubscriptionPublication).toBeTruthy();
    });

    it("is constructs with correct member values", function() {
        var subscriptionId = "testSubscriptionId";
        var response = "response";

        var publication = new SubscriptionPublication({
            subscriptionId: subscriptionId,
            response: response
        });

        expect(publication.subscriptionId).toEqual(subscriptionId);
        expect(publication.response).toEqual(response);
    });
});
