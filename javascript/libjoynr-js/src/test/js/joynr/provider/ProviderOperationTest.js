/*jslint es5: true, node: true, nomen: true */
/*global fail: true */
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
var ProviderOperation = require("../../../classes/joynr/provider/ProviderOperation");
var ProviderQos = require("../../../classes/joynr/types/ProviderQos");
var testDataOperation = require("../../../test-classes/test/data/Operation");
var TestEnum = require("../../../test-classes/joynr/tests/testTypes/TestEnum");
var Util = require("../../../classes/joynr/util/UtilInternal");
var TypeRegistrySingleton = require("../../../classes/joynr/types/TypeRegistrySingleton");
var Promise = require("../../../classes/global/Promise");
var ProviderRuntimeException = require("../../../classes/joynr/exceptions/ProviderRuntimeException");
var ApplicationException = require("../../../classes/joynr/exceptions/ApplicationException");
var waitsFor = require("../../../test-classes/global/WaitsFor");

var safetyTimeoutDelta = 100;

describe("libjoynr-js.joynr.provider.ProviderOperation", function() {
    var implementation, myOperation, operationSpy, operationName, provider, thenSpy, catchSpy;

    beforeEach(function(done) {
        provider = {};
        operationName = "myOperation";
        implementation = jasmine.createSpy("implementation");
        myOperation = new ProviderOperation(provider, implementation, operationName, [
            {
                inputParameter: [
                    {
                        name: "station",
                        type: "String"
                    }
                ],
                error: {
                    type: "joynr.vehicle.Radio.MyOperationErrorEnum"
                },
                outputParameter: []
            },
            {
                inputParameter: [
                    {
                        name: "station",
                        type: "joynr.vehicle.radiotypes.RadioStation"
                    }
                ],
                error: {
                    type: "no error enumeration given"
                },
                outputParameter: []
            }
        ]);
        operationSpy = jasmine.createSpy("operation spy");
        myOperation.registerOperation(operationSpy);

        /*
         * Make sure 'TestEnum' is properly registered as a type.
         * Just requiring the module is insufficient since the
         * automatically generated code called async methods.
         * Execution might be still in progress.
         */
        TypeRegistrySingleton.getInstance()
            .getTypeRegisteredPromise("joynr.tests.testTypes.TestEnum", 1000)
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    it("is of correct type", function(done) {
        expect(myOperation).toBeDefined();
        expect(myOperation).not.toBeNull();
        expect(typeof myOperation === "object").toBeTruthy();
        expect(myOperation instanceof ProviderOperation).toBeTruthy();
        done();
    });

    it("has correct members", function(done) {
        expect(myOperation.registerOperation).toBeDefined();
        done();
    });

    function testCallRegisteredOperation(testData) {
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.returnValue(testData.returnValue);
        var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
        expect(result).toBeDefined();
        expect(result).toEqual(testData.returnParams);
        expect(implementation).not.toHaveBeenCalled();
    }

    it("calls registered implementation with the correct operation arguments", function(done) {
        var i;
        for (i = 0; i < testDataOperation.length; ++i) {
            testCallRegisteredOperation(testDataOperation[i]);
        }
        done();
    });

    function testCallProvidedOperation(testData) {
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);

        implementation.calls.reset();
        implementation.and.returnValue(testData.returnValue);
        myOperation.callOperation(testData.params, testData.paramDatatypes);
        expect(implementation).toHaveBeenCalledWith(testData.namedArguments);
    }

    it("calls provided implementation with the correct operation arguments", function(done) {
        var i;
        for (i = 0; i < testDataOperation.length; ++i) {
            testCallProvidedOperation(testDataOperation[i]);
        }
        done();
    });

    it("calls provided implementation with enum as operation argument", function(done) {
        /*jslint nomen: true */
        var typeName = TestEnum.ZERO._typeName;
        /*jslint nomen: false */
        var signature = {
            inputParameter: [
                {
                    name: "enumArgument",
                    type: typeName
                }
            ]
        };
        myOperation = new ProviderOperation(provider, implementation, operationName, [signature]);

        implementation.calls.reset();
        myOperation.callOperation(["ZERO"], [typeName]);
        expect(implementation).toHaveBeenCalledWith({
            enumArgument: TestEnum.ZERO
        });
        done();
    });

    function testCallAsyncOperationResolves(testData) {
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.callFake(function() {
            return new Promise(function(resolve, reject) {
                resolve(testData.returnValue);
            });
        });
        var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
        var b = Util.isPromise(result);
        expect(b).toBeTruthy();
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        result
            .then(function(value) {
                thenSpy(value);
            })
            .catch(function(error) {
                catchSpy(error);
            });

        return waitsFor(
            function() {
                return thenSpy.calls.count() > 0;
            },
            "thenSpy called",
            100
        ).then(function() {
            expect(catchSpy).not.toHaveBeenCalled();
            expect(thenSpy).toHaveBeenCalled();
            expect(thenSpy).toHaveBeenCalledWith(testData.returnParams);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            expect(implementation).not.toHaveBeenCalled();
        });
    }

    it("resolves for async implementation ", function(done) {
        testCallAsyncOperationResolves(testDataOperation[2])
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    function testCallAsyncOperationRejectsWithProviderRuntimeException(testData) {
        var exampleDetailMessage = "faked error";
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.callFake(function() {
            return new Promise(function(resolve, reject) {
                reject(new ProviderRuntimeException({ detailMessage: exampleDetailMessage }));
            });
        });
        var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
        var b = Util.isPromise(result);
        expect(b).toBeTruthy();
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        result
            .then(function(value) {
                thenSpy(value);
            })
            .catch(function(error) {
                catchSpy(error);
            });

        return waitsFor(
            function() {
                return catchSpy.calls.count() > 0;
            },
            "catchSpy called",
            1000
        ).then(function() {
            expect(thenSpy).not.toHaveBeenCalled();
            expect(catchSpy).toHaveBeenCalled();
            expect(catchSpy.calls.argsFor(0)[0] instanceof ProviderRuntimeException).toBeTruthy();
            expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual(exampleDetailMessage);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            expect(implementation).not.toHaveBeenCalled();
        });
    }

    it("rejects with ProviderRuntimeException for async implementation ", function(done) {
        testCallAsyncOperationRejectsWithProviderRuntimeException(testDataOperation[2])
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    function testCallAsyncOperationRejectsWithApplicationException(testData) {
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.callFake(function() {
            return new Promise(function(resolve, reject) {
                reject(testData.error);
            });
        });
        var result = myOperation.callOperation(testData.params, testData.paramDatatypes);
        var b = Util.isPromise(result);
        expect(b).toBeTruthy();
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        result
            .then(function(value) {
                thenSpy(value);
                return null;
            })
            .catch(function(error) {
                catchSpy(error);
                return null;
            });

        return waitsFor(
            function() {
                return catchSpy.calls.count() > 0;
            },
            "catchSpy called",
            1000
        ).then(function() {
            expect(thenSpy).not.toHaveBeenCalled();
            expect(catchSpy).toHaveBeenCalled();
            expect(catchSpy.calls.argsFor(0)[0] instanceof ApplicationException).toBeTruthy();
            expect(catchSpy.calls.argsFor(0)[0].error).toEqual(testData.error);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
        });
    }

    it("rejects with ApplicationException for async implementation ", function(done) {
        testCallAsyncOperationRejectsWithApplicationException(testDataOperation[2])
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    function testCallSyncOperationThrowsWithProviderRuntimeException(testData) {
        var exampleDetailMessage = "faked error";
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.returnValue(42);
        operationSpy.and.callFake(function() {
            throw new ProviderRuntimeException({ detailMessage: exampleDetailMessage });
        });
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        try {
            myOperation.callOperation(testData.params, testData.paramDatatypes);
            thenSpy();
        } catch (e) {
            catchSpy(e);
        }
        return waitsFor(
            function() {
                return catchSpy.calls.count() > 0;
            },
            "catchSpy called",
            1000
        ).then(function() {
            expect(thenSpy).not.toHaveBeenCalled();
            expect(catchSpy).toHaveBeenCalled();
            expect(catchSpy.calls.argsFor(0)[0] instanceof ProviderRuntimeException).toBeTruthy();
            expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual(exampleDetailMessage);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
        });
    }

    it("throws ProviderRuntimeException for synchronous implementation", function(done) {
        testCallSyncOperationThrowsWithProviderRuntimeException(testDataOperation[2])
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    function testCallSyncOperationThrowsWithApplicationException(testData) {
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.returnValue(42);
        operationSpy.and.callFake(function() {
            throw testData.error;
        });
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        try {
            myOperation.callOperation(testData.params, testData.paramDatatypes);
            thenSpy();
        } catch (e) {
            catchSpy(e);
        }
        return waitsFor(
            function() {
                return catchSpy.calls.count() > 0;
            },
            "catchSpy called",
            1000
        ).then(function() {
            expect(thenSpy).not.toHaveBeenCalled();
            expect(catchSpy).toHaveBeenCalled();
            expect(catchSpy.calls.argsFor(0)[0] instanceof ApplicationException).toBeTruthy();
            expect(catchSpy.calls.argsFor(0)[0].error).toEqual(testData.error);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
        });
    }

    it("throws ApplicationException for synchronous implementation", function(done) {
        testCallSyncOperationThrowsWithApplicationException(testDataOperation[2])
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });
});
