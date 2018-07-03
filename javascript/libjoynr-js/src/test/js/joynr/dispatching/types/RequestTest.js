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
const Request = require("../../../../../main/js/joynr/dispatching/types/Request");

describe("libjoynr-js.joynr.dispatching.types.Request", () => {
    it("is instantiable", () => {
        const methodName = "myMethodName";
        const request = Request.create({
            methodName
        });
        expect(request).toBeDefined();
        expect(request._typeName).toEqual("joynr.Request");
        expect(request.methodName).toEqual(methodName);
    });

    it("converts an untyped param to typed", () => {
        const methodName = "myMethodName";
        const request = Request.create({
            methodName,
            paramDatatypes: ["joynr.vehicle.radiotypes.RadioStation"],
            params: [
                {
                    _typeName: "joynr.vehicle.radiotypes.RadioStation",
                    name: "myRadioStation"
                }
            ]
        });
        expect(request).toBeDefined();
        expect(request._typeName).toBe("joynr.Request");
    });
});
