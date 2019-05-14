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
const MulticastPublication = require("../../../../../main/js/joynr/dispatching/types/MulticastPublication");

describe("libjoynr-js.joynr.dispatching.types.MulticastPublication", () => {
    it("is defined", () => {
        expect(MulticastPublication).toBeDefined();
    });

    it("is instantiable", () => {
        const response = "response";
        const publication = MulticastPublication.create({
            multicastId: "testMulticastId",
            response
        });
        expect(publication).toBeDefined();
        expect(publication).not.toBeNull();
        expect(typeof publication === "object").toBeTruthy();
    });

    it("is constructs with correct member values", () => {
        const multicastId = "testMulticastId";
        const response = "response";

        const publication = MulticastPublication.create({
            multicastId,
            response
        });

        expect(publication.multicastId).toEqual(multicastId);
        expect(publication.response).toEqual(response);
    });
});
