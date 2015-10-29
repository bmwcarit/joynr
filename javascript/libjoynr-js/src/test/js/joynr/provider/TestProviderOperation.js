/*jslint es5: true, nomen: true */
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

joynrTestRequire(
        "joynr/provider/TestProviderOperation",
        [
            "joynr/provider/ProviderOperation",
            "joynr/types/ProviderQos",
            "test/data/Operation",
            "joynr/tests/testTypes/TestEnum",
            "joynr/util/UtilInternal",
            "global/Promise",
            "joynr/exceptions/ProviderRuntimeException",
            "joynr/exceptions/ApplicationException"
        ],
        function(
            ProviderOperation,
            ProviderQos,
            testDataOperation,
            TestEnum,
            Util,
            Promise,
            ProviderRuntimeException,
            ApplicationException) {

    var safetyTimeoutDelta = 100;

    describe("libjoynr-js.joynr.provider.ProviderOperation", function() {

        var implementation, myOperation, operationSpy, operationName, provider, thenSpy, catchSpy;

        beforeEach(function() {
            provider = {};
            operationName = "myOperation";
            implementation = jasmine.createSpy("implementation");
            myOperation = new ProviderOperation(provider, implementation, operationName, [
                {
                    inputParameter : [ {
                        name : "station",
                        type : "String"
                    }
                    ],
                    error : {
                        type : "joynr.vehicle.Radio.MyOperationErrorEnum"
                    },
                    outputParameter : []
                },
                {
                    inputParameter : [ {
                        name : "station",
                        type : "joynr.vehicle.radiotypes.RadioStation"
                    }
                    ],
                    error : {
                        type : "no error enumeration given"
                    },
                    outputParameter : []
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
            operationSpy.andReturn(42);
            var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
            expect(result).toBeDefined();
            expect(result).toEqual(42);
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

        it("calls provided implementation with enum as operation argument", function() {
            /*jslint nomen: true */
            var typeName = TestEnum.ZERO._typeName;
            /*jslint nomen: false */
            var signature = {
                inputParameter : [ {
                    name : "enumArgument",
                    type : typeName
                }
                ]
            };
            myOperation =
                    new ProviderOperation(provider, implementation, operationName, [ signature
                    ]);

            implementation.reset();
            myOperation.callOperation([ "ZERO"
            ], [ typeName
            ]);
            expect(implementation).toHaveBeenCalledWith({
                enumArgument : TestEnum.ZERO
            });
        });

        function testCallAsyncOperationResolves(testData) {
            runs(function() {
                myOperation =
                        new ProviderOperation(
                                provider,
                                implementation,
                                operationName,
                                [ testData.signature
                                ]);
                myOperation.registerOperation(operationSpy);
                operationSpy.reset();
                operationSpy.andCallFake(function() {
                    return new Promise(function(resolve, reject) {
                        resolve(42);
                    });
                });
                var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
                var b = Util.isPromise(result);
                expect(b).toBeTruthy();
                thenSpy = jasmine.createSpy("thenSpy");
                catchSpy = jasmine.createSpy("catchSpy");
                result.then(function(value) {
                    thenSpy(value);
                }).catch(function(error) {
                    catchSpy(error);
                });
            });

            waitsFor(function() {
                return thenSpy.callCount > 0;
            }, "thenSpy called", 100);

            runs(function() {
                expect(catchSpy).not.toHaveBeenCalled();
                expect(thenSpy).toHaveBeenCalled();
                expect(thenSpy).toHaveBeenCalledWith(42);
                expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
                expect(implementation).not.toHaveBeenCalled();
            });
        }

        it("resolves for async implementation ", function() {
            testCallAsyncOperationResolves(testDataOperation[2]);
        });

        function testCallAsyncOperationRejectsWithProviderRuntimeException(testData) {
            var exampleDetailMessage = "faked error";
            runs(function() {
                myOperation =
                        new ProviderOperation(
                                provider,
                                implementation,
                                operationName,
                                [ testData.signature
                                ]);
                myOperation.registerOperation(operationSpy);
                operationSpy.reset();
                operationSpy.andCallFake(function() {
                    return new Promise(function(resolve, reject) {
                        reject(new ProviderRuntimeException({ detailMessage : exampleDetailMessage }));
                    });
                });
                var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
                var b = Util.isPromise(result);
                expect(b).toBeTruthy();
                thenSpy = jasmine.createSpy("thenSpy");
                catchSpy = jasmine.createSpy("catchSpy");
                result.then(function(value) {
                    thenSpy(value);
                }).catch(function(error) {
                    catchSpy(error);
                });
            });

            waitsFor(function() {
                return catchSpy.callCount > 0;
            }, "catchSpy called", 1000);

            runs(function() {
                expect(thenSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls[0].args[0] instanceof ProviderRuntimeException).toBeTruthy();
                expect(catchSpy.calls[0].args[0].detailMessage).toEqual(exampleDetailMessage);
                expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
                expect(implementation).not.toHaveBeenCalled();
            });
        }

        it("rejects with ProviderRuntimeException for async implementation ", function() {
            testCallAsyncOperationRejectsWithProviderRuntimeException(testDataOperation[2]);
        });

        function testCallAsyncOperationRejectsWithApplicationException(testData) {
            runs(function() {
                myOperation =
                        new ProviderOperation(
                                provider,
                                implementation,
                                operationName,
                                [ testData.signature
                                ]);
                myOperation.registerOperation(operationSpy);
                operationSpy.reset();
                operationSpy.andCallFake(function() {
                    return new Promise(function(resolve, reject) {
                        reject(testData.error);
                    });
                });
                var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
                var b = Util.isPromise(result);
                expect(b).toBeTruthy();
                thenSpy = jasmine.createSpy("thenSpy");
                catchSpy = jasmine.createSpy("catchSpy");
                result.then(function(value) {
                    thenSpy(value);
                }).catch(function(error) {
                    catchSpy(error);
                });
            });

            waitsFor(function() {
                return catchSpy.callCount > 0;
            }, "catchSpy called", 1000);

            runs(function() {
                expect(thenSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls[0].args[0] instanceof ApplicationException).toBeTruthy();
                expect(catchSpy.calls[0].args[0].error).toEqual(testData.error);
                expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            });
        }

        it("rejects with ApplicationException for async implementation ", function() {
            testCallAsyncOperationRejectsWithApplicationException(testDataOperation[2]);
        });

        function testCallSyncOperationThrowsWithProviderRuntimeException(testData) {
            var exampleDetailMessage = "faked error";
            runs(function() {
                myOperation =
                        new ProviderOperation(
                                provider,
                                implementation,
                                operationName,
                                [ testData.signature
                                ]);
                myOperation.registerOperation(operationSpy);
                operationSpy.reset();
                operationSpy.andReturn(42);
                operationSpy.andCallFake(function() {
                    throw new ProviderRuntimeException({ detailMessage : exampleDetailMessage });
                });
                thenSpy = jasmine.createSpy("thenSpy");
                catchSpy = jasmine.createSpy("catchSpy");
                try {
                    myOperation.callOperation(testData.params, testData.paramDatatypes);
                    thenSpy();
                } catch(e) {
                    catchSpy(e);
                }
            });
            waitsFor(function() {
                return catchSpy.callCount > 0;
            }, "catchSpy called", 1000);
            runs(function() {
                expect(thenSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls[0].args[0] instanceof ProviderRuntimeException).toBeTruthy();
                expect(catchSpy.calls[0].args[0].detailMessage).toEqual(exampleDetailMessage);
                expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            });
        }

        it("throws ProviderRuntimeException for synchronous implementation", function() {
            testCallSyncOperationThrowsWithProviderRuntimeException(testDataOperation[2]);
        });

        function testCallSyncOperationThrowsWithApplicationException(testData) {
            runs(function() {
                myOperation =
                        new ProviderOperation(
                                provider,
                                implementation,
                                operationName,
                                [ testData.signature
                                ]);
                myOperation.registerOperation(operationSpy);
                operationSpy.reset();
                operationSpy.andReturn(42);
                operationSpy.andCallFake(function() {
                    throw testData.error;
                });
                thenSpy = jasmine.createSpy("thenSpy");
                catchSpy = jasmine.createSpy("catchSpy");
                try {
                    myOperation.callOperation(testData.params, testData.paramDatatypes);
                    thenSpy();
                } catch(e) {
                    catchSpy(e);
                }
            });
            waitsFor(function() {
                return catchSpy.callCount > 0;
            }, "catchSpy called", 1000);
            runs(function() {
                expect(thenSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls[0].args[0] instanceof ApplicationException).toBeTruthy();
                expect(catchSpy.calls[0].args[0].error).toEqual(testData.error);
                expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            });
        }

        it("throws ApplicationException for synchronous implementation", function() {
            testCallSyncOperationThrowsWithApplicationException(testDataOperation[2]);
        });
    });
}); // require
