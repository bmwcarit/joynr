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
require("../../node-unit-test-helper");
const ProviderOperation = require("../../../../main/js/joynr/provider/ProviderOperation");
const testDataOperation = require("../../../../test/js/test/data/Operation");
const TestEnum = require("../../../generated/joynr/tests/testTypes/TestEnum");
const UtilInternal = require("../../../../main/js/joynr/util/UtilInternal");
const TypeRegistrySingleton = require("../../../../main/js/joynr/types/TypeRegistrySingleton");
const Promise = require("../../../../main/js/global/Promise");
const ProviderRuntimeException = require("../../../../main/js/joynr/exceptions/ProviderRuntimeException");
const ApplicationException = require("../../../../main/js/joynr/exceptions/ApplicationException");
const waitsFor = require("../../../../test/js/global/WaitsFor");

describe("libjoynr-js.joynr.provider.ProviderOperation", () => {
    let implementation, myOperation, operationSpy, operationName, provider, thenSpy, catchSpy;

    beforeEach(done => {
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
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("is of correct type", done => {
        expect(myOperation).toBeDefined();
        expect(myOperation).not.toBeNull();
        expect(typeof myOperation === "object").toBeTruthy();
        expect(myOperation instanceof ProviderOperation).toBeTruthy();
        done();
    });

    it("has correct members", done => {
        expect(myOperation.registerOperation).toBeDefined();
        done();
    });

    function testCallRegisteredOperation(testData) {
        const operation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        const spy = jasmine.createSpy("operation spy");
        operation.registerOperation(spy);
        spy.and.returnValue(testData.returnValue);
        return operation.callOperation(testData.params, testData.paramDatatypes).then(result => {
            expect(result).toBeDefined();
            expect(result).toEqual(testData.returnParams);
            expect(implementation).not.toHaveBeenCalled();
            return result;
        });
    }

    it("calls registered implementation with the correct operation arguments", done => {
        const promises = [];
        let i;
        for (i = 0; i < testDataOperation.length; ++i) {
            promises.push(testCallRegisteredOperation(testDataOperation[i]));
        }
        Promise.all(promises).then(done);
    });

    function testCallProvidedOperation(testData) {
        const impl = jasmine.createSpy("implementation");
        const operation = new ProviderOperation(provider, impl, operationName, [testData.signature]);

        impl.and.returnValue(testData.returnValue);
        return operation.callOperation(testData.params, testData.paramDatatypes).then(result => {
            expect(impl).toHaveBeenCalledWith(testData.namedArguments);
            return result;
        });
    }

    it("calls provided implementation with the correct operation arguments", done => {
        const promises = [];
        let i;
        for (i = 0; i < testDataOperation.length; ++i) {
            promises.push(testCallProvidedOperation(testDataOperation[i]));
        }
        Promise.all(promises).then(done);
    });

    it("calls provided implementation with enum as operation argument", done => {
        const typeName = TestEnum.ZERO._typeName;
        const signature = {
            inputParameter: [
                {
                    name: "enumArgument",
                    type: typeName
                }
            ]
        };
        myOperation = new ProviderOperation(provider, implementation, operationName, [signature]);

        implementation.calls.reset();
        myOperation.callOperation(["ZERO"], [typeName]).then(() => {
            expect(implementation).toHaveBeenCalledWith({
                enumArgument: TestEnum.ZERO
            });
            done();
        });
    });

    function testCallAsyncOperationResolves(testData) {
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.callFake(() => Promise.resolve(testData.returnValue));
        const result = myOperation.callOperation(testData.params, testData.paramDatatypes);
        const b = UtilInternal.isPromise(result);
        expect(b).toBeTruthy();
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        result
            .then(value => {
                thenSpy(value);
            })
            .catch(error => {
                catchSpy(error);
            });

        return waitsFor(
            () => {
                return thenSpy.calls.count() > 0;
            },
            "thenSpy called",
            100
        ).then(() => {
            expect(catchSpy).not.toHaveBeenCalled();
            expect(thenSpy).toHaveBeenCalled();
            expect(thenSpy).toHaveBeenCalledWith(testData.returnParams);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            expect(implementation).not.toHaveBeenCalled();
        });
    }

    it("resolves for async implementation ", done => {
        testCallAsyncOperationResolves(testDataOperation[2])
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function testCallAsyncOperationRejectsWithProviderRuntimeException(testData) {
        const exampleDetailMessage = "faked error";
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.callFake(() => {
            return new Promise((resolve, reject) => {
                reject(new ProviderRuntimeException({ detailMessage: exampleDetailMessage }));
            });
        });
        const result = myOperation.callOperation(testData.params, testData.paramDatatypes);
        const b = UtilInternal.isPromise(result);
        expect(b).toBeTruthy();
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        result
            .then(value => {
                thenSpy(value);
            })
            .catch(error => {
                catchSpy(error);
            });

        return waitsFor(
            () => {
                return catchSpy.calls.count() > 0;
            },
            "catchSpy called",
            1000
        ).then(() => {
            expect(thenSpy).not.toHaveBeenCalled();
            expect(catchSpy).toHaveBeenCalled();
            expect(catchSpy.calls.argsFor(0)[0] instanceof ProviderRuntimeException).toBeTruthy();
            expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual(exampleDetailMessage);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            expect(implementation).not.toHaveBeenCalled();
        });
    }

    it("rejects with ProviderRuntimeException for async implementation ", done => {
        testCallAsyncOperationRejectsWithProviderRuntimeException(testDataOperation[2])
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function testCallAsyncOperationRejectsWithApplicationException(testData) {
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.callFake(() => {
            return new Promise((resolve, reject) => {
                reject(testData.error);
            });
        });
        const result = myOperation.callOperation(testData.params, testData.paramDatatypes);
        const b = UtilInternal.isPromise(result);
        expect(b).toBeTruthy();
        thenSpy = jasmine.createSpy("thenSpy");
        catchSpy = jasmine.createSpy("catchSpy");
        result
            .then(value => {
                thenSpy(value);
                return null;
            })
            .catch(error => {
                catchSpy(error);
                return null;
            });

        return waitsFor(
            () => {
                return catchSpy.calls.count() > 0;
            },
            "catchSpy called",
            1000
        ).then(() => {
            expect(thenSpy).not.toHaveBeenCalled();
            expect(catchSpy).toHaveBeenCalled();
            expect(catchSpy.calls.argsFor(0)[0] instanceof ApplicationException).toBeTruthy();
            expect(catchSpy.calls.argsFor(0)[0].error).toEqual(testData.error);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
        });
    }

    it("rejects with ApplicationException for async implementation ", done => {
        testCallAsyncOperationRejectsWithApplicationException(testDataOperation[2])
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function testCallSyncOperationThrowsWithProviderRuntimeException(testData) {
        const exampleDetailMessage = "faked error";
        myOperation = new ProviderOperation(provider, implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.calls.reset();
        operationSpy.and.returnValue(42);
        operationSpy.and.callFake(() => {
            throw new ProviderRuntimeException({ detailMessage: exampleDetailMessage });
        });

        return myOperation
            .callOperation(testData.params, testData.paramDatatypes)
            .then(fail)
            .catch(error => {
                expect(error instanceof ProviderRuntimeException).toBeTruthy();
                expect(error.detailMessage).toEqual(exampleDetailMessage);
                expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            });
    }

    it("throws ProviderRuntimeException for synchronous implementation", done => {
        testCallSyncOperationThrowsWithProviderRuntimeException(testDataOperation[2])
            .then(() => {
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
        operationSpy.and.callFake(() => {
            throw testData.error;
        });

        return myOperation
            .callOperation(testData.params, testData.paramDatatypes)
            .then(fail)
            .catch(error => {
                expect(error instanceof ApplicationException).toBeTruthy();
                expect(error.error).toEqual(testData.error);
                expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            });
    }

    it("throws ApplicationException for synchronous implementation", done => {
        testCallSyncOperationThrowsWithApplicationException(testDataOperation[2])
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });
});
