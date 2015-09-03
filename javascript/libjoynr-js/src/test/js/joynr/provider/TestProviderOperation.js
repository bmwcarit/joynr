/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire("joynr/provider/TestProviderOperation", [
    "joynr/provider/ProviderOperation",
    "joynr/types/ProviderQos",
    "test/data/Operation"
], function(ProviderOperation, ProviderQos, testDataOperation) {

    var safetyTimeoutDelta = 100;

    describe("libjoynr-js.joynr.provider.ProviderOperation", function() {

        var implementation, myOperation, operationSpy, operationName, provider;

        beforeEach(function() {
            provider = {};
            operationName = "myOperation";
            implementation = jasmine.createSpy("implementation");
            myOperation = new ProviderOperation(provider, implementation, operationName, [
                {
                    "station" : {
                        type : "String"
                    }
                },
                {
                    "station" : {
                        type : "joynr.vehicle.radiotypes.RadioStation"
                    }
                }
            ]);
            operationSpy = jasmine.createSpy("operation spy");
            myOperation.registerOperation(operationSpy);
        });

        it("is of correct type", function() {
            expect(myOperation).toBeDefined();
            expect(myOperation).not.toBeNull();
            expect(typeof myOperation === "object").toBeTruthy();
            expect(myOperation instanceof ProviderOperation).toBeTruthy();
        });

        it("has correct members", function() {
            expect(myOperation.registerOperation).toBeDefined();
        });

        function testCallRegisteredOperation(testData) {
            myOperation =
                    new ProviderOperation(
                            provider,
                            implementation,
                            operationName,
                            [ testData.signature
                            ]);
            myOperation.registerOperation(operationSpy);

            operationSpy.reset();
            myOperation.callOperation(testData.params, testData.paramDatatypes);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            expect(implementation).not.toHaveBeenCalled();
        }

        it("calls registered implementation with the correct operation arguments", function() {
            var i;
            for (i = 0; i < testDataOperation.length; ++i) {
                testCallRegisteredOperation(testDataOperation[i]);
            }
        });

        function testCallProvidedOperation(testData) {
            myOperation =
                    new ProviderOperation(
                            provider,
                            implementation,
                            operationName,
                            [ testData.signature
                            ]);

            implementation.reset();
            myOperation.callOperation(testData.params, testData.paramDatatypes);
            expect(implementation).toHaveBeenCalledWith(testData.namedArguments);
        }

        it("calls provided implementation with the correct operation arguments", function() {
            var i;
            for (i = 0; i < testDataOperation.length; ++i) {
                testCallProvidedOperation(testDataOperation[i]);
            }
        });

    });

}); // require
