/*jslint nomen: true, node: true */
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
var Request = require('../../../../classes/joynr/dispatching/types/Request');
var TypeRegistrySingleton = require('../../../../classes/joynr/types/TypeRegistrySingleton');
var RadioStation = require('../../../../test-classes/joynr/vehicle/radiotypes/RadioStation');

describe("libjoynr-js.joynr.dispatching.types.Request", function() {

    it("is instantiable", function() {
        var methodName = "myMethodName";
        var request = new Request({
            methodName : methodName
        });
        expect(request).toBeDefined();
        expect(request instanceof Request).toBeTruthy();
        expect(request._typeName).toEqual("joynr.Request");
        expect(request.methodName).toEqual(methodName);
    });

    it("converts an untyped param to typed", function() {

        var methodName = "myMethodName";
        var request = new Request({
            methodName : methodName,
            paramDatatypes : [ "joynr.vehicle.radiotypes.RadioStation"
            ],
            params : [ {
                _typeName : "joynr.vehicle.radiotypes.RadioStation",
                name : "myRadioStation"
            }
            ]
        });
        expect(request).toBeDefined();
        expect(request instanceof Request).toBeTruthy();
        expect(request.params[0] instanceof RadioStation).toBeTruthy();
    });

});
