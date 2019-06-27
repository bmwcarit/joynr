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

import ProviderOperation from "../../../../main/js/joynr/provider/ProviderOperation";
import testDataOperation from "../../../../test/js/test/data/Operation";
import TestEnum from "../../../generated/joynr/tests/testTypes/TestEnum";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";
import ProviderRuntimeException from "../../../../main/js/joynr/exceptions/ProviderRuntimeException";
import ApplicationException from "../../../../main/js/joynr/exceptions/ApplicationException";
import { reversePromise } from "../../testUtil";

describe("libjoynr-js.joynr.provider.ProviderOperation", () => {
    let implementation: any, myOperation: any, operationSpy: any;
    const operationName = "myOperation";

    beforeEach(() => {
        implementation = jest.fn();
        myOperation = new ProviderOperation(implementation, operationName, [
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
        operationSpy = jest.fn();
        myOperation.registerOperation(operationSpy);

        /*
         * Make sure 'TestEnum' is properly registered as a type.
         * Just requiring the module is insufficient since the
         * automatically generated code called async methods.
         * Execution might be still in progress.
         */
        TypeRegistrySingleton.getInstance().addType(TestEnum);
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

    function testCallRegisteredOperation(testData: any) {
        const operation = new ProviderOperation(implementation, operationName, [testData.signature]);
        const spy = jest.fn();
        operation.registerOperation(spy);
        spy.mockReturnValue(testData.returnValue);
        return operation.callOperation(testData.params, testData.paramDatatypes).then(result => {
            expect(result).toBeDefined();
            expect(result).toEqual(testData.returnParams);
            expect(implementation).not.toHaveBeenCalled();
            return result;
        });
    }

    it("calls registered implementation with the correct operation arguments", async () => {
        const promises = [];
        let i: any;
        for (i = 0; i < testDataOperation.length; ++i) {
            promises.push(testCallRegisteredOperation(testDataOperation[i]));
        }
        await Promise.all(promises);
    });

    async function testCallProvidedOperation(testData: any) {
        const impl = jest.fn();
        const operation = new ProviderOperation(impl, operationName, [testData.signature]);

        impl.mockReturnValue(testData.returnValue);
        await operation.callOperation(testData.params, testData.paramDatatypes);
        expect(impl).toHaveBeenCalledWith(testData.namedArguments);
    }

    it("calls provided implementation with the correct operation arguments", async () => {
        for (let i = 0; i < testDataOperation.length; ++i) {
            await testCallProvidedOperation(testDataOperation[i]);
        }
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
        myOperation = new ProviderOperation(implementation, operationName, [signature] as any);

        implementation.mockClear();
        myOperation.callOperation(["ZERO"], [typeName]).then(() => {
            expect(implementation).toHaveBeenCalledWith({
                enumArgument: TestEnum.ZERO
            });
            done();
        });
    });

    async function testCallAsyncOperationResolves(testData: any) {
        myOperation = new ProviderOperation(implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.mockClear();
        operationSpy.mockImplementation(() => Promise.resolve(testData.returnValue));
        const result = await myOperation.callOperation(testData.params, testData.paramDatatypes);
        expect(result).toEqual(testData.returnParams);
        expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
        expect(implementation).not.toHaveBeenCalled();
    }

    it("resolves for async implementation ", () => {
        return testCallAsyncOperationResolves(testDataOperation[2]);
    });

    async function testCallAsyncOperationRejectsWithProviderRuntimeException(testData: any) {
        const exampleDetailMessage = "faked error";
        myOperation = new ProviderOperation(implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.mockClear();
        operationSpy.mockRejectedValue(new ProviderRuntimeException({ detailMessage: exampleDetailMessage }));
        expect.assertions(4);
        try {
            await myOperation.callOperation(testData.params, testData.paramDatatypes);
        } catch (e) {
            expect(e).toBeInstanceOf(ProviderRuntimeException);
            expect(e.detailMessage).toEqual(exampleDetailMessage);
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            expect(implementation).not.toHaveBeenCalled();
        }
    }

    it("rejects with ProviderRuntimeException for async implementation ", async () => {
        await testCallAsyncOperationRejectsWithProviderRuntimeException(testDataOperation[2]);
    });

    function testCallAsyncOperationRejectsWithApplicationException(testData: any) {
        myOperation = new ProviderOperation(implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.mockClear();
        operationSpy.mockRejectedValue(testData.error);
        expect.assertions(3);
        return myOperation.callOperation(testData.params, testData.paramDatatypes).catch((error: any) => {
            expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
            expect(error).toBeInstanceOf(ApplicationException);
            expect(error.error).toEqual(testData.error);
        });
    }

    it("rejects with ApplicationException for async implementation ", async () => {
        await testCallAsyncOperationRejectsWithApplicationException(testDataOperation[2]);
    });

    async function testCallSyncOperationThrowsWithProviderRuntimeException(testData: any) {
        const exampleDetailMessage = "faked error";
        myOperation = new ProviderOperation(implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.mockClear();
        operationSpy.mockReturnValue(42);
        operationSpy.mockImplementation(() => {
            throw new ProviderRuntimeException({
                detailMessage: exampleDetailMessage
            });
        });

        const error = await reversePromise(myOperation.callOperation(testData.params, testData.paramDatatypes));
        expect(error instanceof ProviderRuntimeException).toBeTruthy();
        expect(error.detailMessage).toEqual(exampleDetailMessage);
        expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
    }

    it("throws ProviderRuntimeException for synchronous implementation", async () => {
        await testCallSyncOperationThrowsWithProviderRuntimeException(testDataOperation[2]);
    });

    async function testCallSyncOperationThrowsWithApplicationException(testData: any) {
        myOperation = new ProviderOperation(implementation, operationName, [testData.signature]);
        myOperation.registerOperation(operationSpy);
        operationSpy.mockClear();
        operationSpy.mockReturnValue(42);
        operationSpy.mockImplementation(() => {
            throw testData.error;
        });

        const error = await reversePromise(myOperation.callOperation(testData.params, testData.paramDatatypes));
        expect(error instanceof ApplicationException).toBeTruthy();
        expect(error.error).toEqual(testData.error);
        expect(operationSpy).toHaveBeenCalledWith(testData.namedArguments);
    }

    it("throws ApplicationException for synchronous implementation", async () => {
        await testCallSyncOperationThrowsWithApplicationException(testDataOperation[2]);
    });
});
