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

import RequestReplyManager from "../../../../main/js/joynr/dispatching/RequestReplyManager";
import * as OneWayRequest from "../../../../main/js/joynr/dispatching/types/OneWayRequest";
import * as Request from "../../../../main/js/joynr/dispatching/types/Request";
import * as Reply from "../../../../main/js/joynr/dispatching/types/Reply";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";
import * as Typing from "../../../../main/js/joynr/util/Typing";
import * as UtilInternal from "../../../../main/js/joynr/util/UtilInternal";
import * as JSONSerializer from "../../../../main/js/joynr/util/JSONSerializer";
import MethodInvocationException from "../../../../main/js/joynr/exceptions/MethodInvocationException";
import Version from "../../../../main/js/generated/joynr/types/Version";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import testUtil from "../../../js/testUtil";

describe("libjoynr-js.joynr.dispatching.RequestReplyManager", () => {
    let dispatcherSpy: any;
    let requestReplyManager: RequestReplyManager;
    let typeRegistry: any;
    const ttlMs = 50;
    const requestReplyId = "requestReplyId";
    const testResponse = ["testResponse"];
    const reply = Reply.create({
        requestReplyId,
        response: testResponse
    });
    const replySettings = {};
    let fakeTime = Date.now();

    async function increaseFakeTime(timeMs: number): Promise<void> {
        fakeTime += timeMs;
        jest.advanceTimersByTime(timeMs);
        await testUtil.multipleSetImmediate();
    }

    const providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
        providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
        domain: "testProviderDomain",
        interfaceName: "interfaceName",
        participantId: "providerParticipantId",
        qos: new ProviderQos(undefined as any),
        lastSeenDateMs: Date.now(),
        expiryDateMs: Date.now() + 60000,
        publicKeyId: "publicKeyId",
        isLocal: true
    });

    class RadioStation {
        public name: string;
        public station: any;
        public source: any;
        public checkMembers: any;
        public _typeName = "test.RadioStation";
        public static _typeName = "test.RadioStation";
        public getMemberType(): void {
            // do nothing
        }
        public constructor(name: string, station: any, source: any) {
            this.name = name;
            this.station = station;
            this.source = source;
            Object.defineProperty(this, "checkMembers", {
                configurable: false,
                writable: false,
                enumerable: false,
                value: jest.fn()
            });
        }
    }

    class ComplexTypeWithComplexAndSimpleProperties {
        public radioStation: any;
        public myBoolean: any;
        public myString: any;
        public checkMembers: any;
        public _typeName = "test.ComplexTypeWithComplexAndSimpleProperties";
        public static _typeName = "test.ComplexTypeWithComplexAndSimpleProperties";
        public constructor(radioStation: any, myBoolean: any, myString: any) {
            this.radioStation = radioStation;
            this.myBoolean = myBoolean;
            this.myString = myString;
            Object.defineProperty(this, "checkMembers", {
                configurable: false,
                writable: false,
                enumerable: false,
                value: jest.fn()
            });
        }
        public getMemberType(): void {
            // do nothing
        }
    }

    /**
     * Called before each test.
     */
    beforeEach(() => {
        jest.useFakeTimers();
        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });
        dispatcherSpy = {
            sendOneWayRequest: jest.fn(),
            sendRequest: jest.fn()
        };
        typeRegistry = TypeRegistrySingleton.getInstance();
        typeRegistry.addType(RadioStation);
        typeRegistry.addType(ComplexTypeWithComplexAndSimpleProperties);
        requestReplyManager = new RequestReplyManager(dispatcherSpy);
    });

    afterEach(() => {
        requestReplyManager.shutdown();
        jest.useRealTimers();
    });

    it("is instantiable", () => {
        expect(requestReplyManager).toBeDefined();
    });

    const tripleJ = new RadioStation("TripleJ", "107.7", "AUSTRALIA");
    const fm4 = new RadioStation("FM4", "104.0", "AUSTRIA");
    const complex = new ComplexTypeWithComplexAndSimpleProperties(tripleJ, true, "hello");

    Object.setPrototypeOf(complex, Object);
    Object.setPrototypeOf(fm4, Object);
    Object.setPrototypeOf(tripleJ, Object);

    const testData = [
        {
            paramDatatype: ["Boolean"],
            params: [true]
        },
        {
            paramDatatype: ["Integer"],
            params: [123456789]
        },
        {
            paramDatatype: ["Double"],
            params: [-123.456789]
        },
        {
            paramDatatype: ["String"],
            params: ["lalala"]
        },
        {
            paramDatatype: ["Integer[]"],
            params: [[1, 2, 3, 4, 5]]
        },
        {
            paramDatatype: ["joynr.vehicle.radiotypes.RadioStation[]"],
            params: [[fm4, tripleJ]]
        },
        {
            paramDatatype: ["joynr.vehicle.radiotypes.RadioStation"],
            params: [tripleJ]
        },
        {
            paramDatatype: ["joynr.vehicle.radiotypes.RadioStation", "String"],
            params: [tripleJ, "testParam"]
        },
        {
            paramDatatype: ["vehicle.ComplexTypeWithComplexAndSimpleProperties"],
            params: [complex]
        }
    ];

    async function testHandleRequestForGetterSetterMethod(attributeName: string, params: any[]) {
        const providerParticipantId = "providerParticipantId";
        const provider = {
            [attributeName]: {
                get: jest.fn().mockReturnValue([]),
                set: jest.fn().mockReturnValue([])
            }
        };

        let request = Request.create({
            methodName: `get${UtilInternal.firstUpper(attributeName)}`,
            paramDatatypes: [],
            params: []
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);
        await requestReplyManager.handleRequest(providerParticipantId, request, jest.fn, {});
        await testUtil.multipleSetImmediate();
        request = Request.create({
            methodName: `set${UtilInternal.firstUpper(attributeName)}`,
            paramDatatypes: [],
            // untype objects through serialization and deserialization
            params: JSON.parse(JSONSerializer.stringify(params))
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);

        await requestReplyManager.handleRequest(providerParticipantId, request, jest.fn, {});

        await testUtil.multipleSetImmediate();
        expect(provider[attributeName].get).toHaveBeenCalledWith();

        expect(provider[attributeName].set).toHaveBeenCalledWith(params[0]);

        const result = provider[attributeName].set.mock.calls[0][0];
        expect(result).toEqual(params[0]);
    }

    async function testHandleRequestWithExpectedType(paramDatatypes: any, params: any[]) {
        const providerParticipantId = "providerParticipantId";
        const provider = {
            testFunction: {
                callOperation: jest.fn()
            }
        };

        provider.testFunction.callOperation.mockReturnValue([]);

        const request = Request.create({
            methodName: "testFunction",
            paramDatatypes,
            // untype objects through serialization and deserialization
            params: JSON.parse(JSONSerializer.stringify(params))
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);
        requestReplyManager.handleRequest(providerParticipantId, request, jest.fn, {});

        await testUtil.multipleSetImmediate();
        expect(provider.testFunction.callOperation).toHaveBeenCalled();
        expect(provider.testFunction.callOperation).toHaveBeenCalledWith(params, paramDatatypes);

        const result = provider.testFunction.callOperation.mock.calls[0][0];
        expect(result).toEqual(params);
        expect(Typing.getObjectType(result)).toEqual(Typing.getObjectType(params));
    }

    it("calls registered requestCaller for attribute", async () => {
        await testHandleRequestForGetterSetterMethod("attributeA", ["attributeA"]);
        await testHandleRequestForGetterSetterMethod("AttributeWithStartingCapitalLetter", [
            "AttributeWithStartingCapitalLetter"
        ]);
    });

    it("calls registered requestCaller with correctly typed object", async () => {
        for (let i = 0; i < testData.length; ++i) {
            const test = testData[i];
            await testHandleRequestWithExpectedType(test.paramDatatype, test.params);
        }
    });

    it("calls registered replyCaller when a reply arrives", async () => {
        const replyCallerSpy = {
            callback: jest.fn(),
            callbackSettings: {}
        };

        requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy, ttlMs);
        requestReplyManager.handleReply(reply);

        await testUtil.multipleSetImmediate();

        expect(replyCallerSpy.callback).toHaveBeenCalled();
        expect(replyCallerSpy.callback).toHaveBeenCalledWith(undefined, {
            response: testResponse,
            settings: replyCallerSpy.callbackSettings
        });
    });

    it("calls registered replyCaller fail if no reply arrives in time", async () => {
        const replyCallerSpy = {
            callback: jest.fn(),
            callbackSettings: {}
        };

        requestReplyManager.addReplyCaller("requestReplyId", replyCallerSpy, ttlMs);
        const toleranceMs = 1500; // at least 1000 since that's the cleanup interval
        await increaseFakeTime(toleranceMs + ttlMs);

        expect(replyCallerSpy.callback).toHaveBeenCalledWith(expect.any(Error));
    });

    async function testHandleReplyWithExpectedType(params: any[]): Promise<void> {
        const replyCallerSpy = {
            callback: jest.fn(),
            callbackSettings: {}
        };

        const reply = Reply.create({
            requestReplyId,
            // untype object by serializing and deserializing it
            response: JSON.parse(JSONSerializer.stringify(params))
        });

        requestReplyManager.addReplyCaller("requestReplyId", replyCallerSpy, ttlMs);
        requestReplyManager.handleReply(reply);
        await testUtil.multipleSetImmediate();

        expect(replyCallerSpy.callback).toHaveBeenCalled();

        const result = replyCallerSpy.callback.mock.calls[0][1];
        for (let i = 0; i < params.length; i++) {
            expect(result.response[i]).toEqual(params[i]);
        }
    }

    it("calls registered replyCaller with correctly typed object", async () => {
        for (let i = 0; i < testData.length; ++i) {
            const test = testData[i];
            await testHandleReplyWithExpectedType(test.params);
        }
    });

    function callRequestReplyManagerSync(
        methodName: string,
        testParam: any,
        testParamDatatype: any,
        useInvalidProviderParticipantId: any,
        callbackContext: any
    ) {
        let providerParticipantId = "providerParticipantId";
        class TestProvider {
            public static MAJOR_VERSION = 47;
            public static MINOR_VERSION = 11;
            public attributeName = {
                get: jest.fn(),
                set: jest.fn()
            };
            public operationName = {
                callOperation: jest.fn()
            };
            public getOperationStartingWithGet = {
                callOperation: jest.fn()
            };
            public getOperationHasPriority = {
                callOperation: jest.fn()
            };
            public operationHasPriority = {
                get: jest.fn(),
                set: jest.fn()
            };
        }

        const provider = new TestProvider();
        provider.attributeName.get.mockReturnValue([testParam]);
        provider.attributeName.set.mockReturnValue([]);
        provider.operationName.callOperation.mockReturnValue([testParam]);
        provider.getOperationStartingWithGet.callOperation.mockReturnValue([testParam]);
        provider.getOperationHasPriority.callOperation.mockReturnValue([testParam]);

        const callbackDispatcher = jest.fn();

        const request = Request.create({
            methodName,
            paramDatatypes: [testParamDatatype],
            params: [testParam]
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);

        if (useInvalidProviderParticipantId) {
            providerParticipantId = "nonExistentProviderId";
        }

        requestReplyManager.handleRequest(providerParticipantId, request, callbackDispatcher, callbackContext);

        return {
            provider,
            callbackDispatcher,
            request
        };
    }

    async function callRequestReplyManager(
        methodName: string,
        testParam: any,
        testParamDatatype: any,
        useInvalidProviderParticipantId?: any,
        callbackContext?: any
    ) {
        const test = callRequestReplyManagerSync(
            methodName,
            testParam,
            testParamDatatype,
            useInvalidProviderParticipantId,
            callbackContext
        );

        await testUtil.multipleSetImmediate();
        return test;
    }

    const testParam = "myTestParameter";
    const testParamDatatype = "String";

    it("calls attribute getter correctly", async () => {
        const test = await callRequestReplyManager(
            "getAttributeName",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        );
        expect(test.provider.attributeName.get).toHaveBeenCalled();
        expect(test.provider.attributeName.get).toHaveBeenCalledWith();
        expect(test.provider.attributeName.set).not.toHaveBeenCalled();
        expect(test.provider.operationName.callOperation).not.toHaveBeenCalled();
        expect(test.provider.getOperationStartingWithGet.callOperation).not.toHaveBeenCalled();
        expect(test.provider.operationHasPriority.set).not.toHaveBeenCalled();
        expect(test.provider.operationHasPriority.get).not.toHaveBeenCalled();
        expect(test.provider.getOperationHasPriority.callOperation).not.toHaveBeenCalled();

        expect(test.callbackDispatcher).toHaveBeenCalled();
        expect(test.callbackDispatcher).toHaveBeenCalledWith(
            replySettings,
            Reply.create({
                response: [testParam],
                requestReplyId: test.request.requestReplyId
            })
        );
    });

    it("calls attribute setter correctly", async () => {
        const test = await callRequestReplyManager(
            "setAttributeName",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        );
        expect(test.provider.attributeName.get).not.toHaveBeenCalled();
        expect(test.provider.attributeName.set).toHaveBeenCalled();
        expect(test.provider.attributeName.set).toHaveBeenCalledWith(testParam);
        expect(test.provider.operationName.callOperation).not.toHaveBeenCalled();
        expect(test.provider.getOperationStartingWithGet.callOperation).not.toHaveBeenCalled();
        expect(test.provider.operationHasPriority.set).not.toHaveBeenCalled();
        expect(test.provider.operationHasPriority.get).not.toHaveBeenCalled();
        expect(test.provider.getOperationHasPriority.callOperation).not.toHaveBeenCalled();

        expect(test.callbackDispatcher).toHaveBeenCalled();
        expect(test.callbackDispatcher).toHaveBeenCalledWith(
            replySettings,
            Reply.create({
                response: [],
                requestReplyId: test.request.requestReplyId
            })
        );
    });

    it("calls operation function correctly", async () => {
        const test = await callRequestReplyManager(
            "operationName",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        );
        expect(test.provider.attributeName.set).not.toHaveBeenCalled();
        expect(test.provider.attributeName.get).not.toHaveBeenCalled();
        expect(test.provider.operationName.callOperation).toHaveBeenCalled();
        expect(test.provider.operationName.callOperation).toHaveBeenCalledWith([testParam], [testParamDatatype]);
        expect(test.provider.getOperationStartingWithGet.callOperation).not.toHaveBeenCalled();
        expect(test.provider.operationHasPriority.set).not.toHaveBeenCalled();
        expect(test.provider.operationHasPriority.get).not.toHaveBeenCalled();
        expect(test.provider.getOperationHasPriority.callOperation).not.toHaveBeenCalled();

        expect(test.callbackDispatcher).toHaveBeenCalled();
        expect(test.callbackDispatcher).toHaveBeenCalledWith(
            replySettings,
            Reply.create({
                response: [testParam],
                requestReplyId: test.request.requestReplyId
            })
        );
    });

    it("calls operation function for one-way request correctly", async () => {
        const providerParticipantId = "oneWayProviderParticipantId";
        const provider = {
            fireAndForgetMethod: {
                callOperation: jest.fn()
            }
        };

        const oneWayRequest = OneWayRequest.create({
            methodName: "fireAndForgetMethod",
            paramDatatypes: [testParamDatatype],
            params: [testParam]
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);

        requestReplyManager.handleOneWayRequest(providerParticipantId, oneWayRequest);

        await testUtil.multipleSetImmediate();

        expect(provider.fireAndForgetMethod.callOperation).toHaveBeenCalled();
        expect(provider.fireAndForgetMethod.callOperation).toHaveBeenCalledWith([testParam], [testParamDatatype]);
    });

    it('calls operation "getOperationStartingWithGet" when no attribute "operationStartingWithGet" exists', async () => {
        const test = await callRequestReplyManager("getOperationStartingWithGet", testParam, testParamDatatype);
        expect(test.provider.getOperationStartingWithGet.callOperation).toHaveBeenCalled();
        expect(test.provider.getOperationStartingWithGet.callOperation).toHaveBeenCalledWith(
            [testParam],
            [testParamDatatype]
        );
    });

    it('calls operation "getOperationHasPriority" when attribute "operationHasPriority" exists', async () => {
        const test = await callRequestReplyManager("getOperationHasPriority", testParam, testParamDatatype);
        expect(test.provider.operationHasPriority.set).not.toHaveBeenCalled();
        expect(test.provider.operationHasPriority.get).not.toHaveBeenCalled();
        expect(test.provider.getOperationHasPriority.callOperation).toHaveBeenCalled();
        expect(test.provider.getOperationHasPriority.callOperation).toHaveBeenCalledWith(
            [testParam],
            [testParamDatatype]
        );
    });

    it("delivers exception upon non-existent provider", async () => {
        const test = await callRequestReplyManager("testFunction", testParam, testParamDatatype, true, replySettings);
        expect(test.callbackDispatcher).toHaveBeenCalled();
        expect(test.callbackDispatcher).toHaveBeenCalledWith(
            replySettings,
            Reply.create({
                error: new MethodInvocationException({
                    detailMessage: `error handling request: {"methodName":"testFunction","paramDatatypes":["String"],"params":["myTestParameter"],"requestReplyId":"${
                        test.request.requestReplyId
                    }","_typeName":"joynr.Request"} for providerParticipantId nonExistentProviderId`
                }),

                // {"methodName":"testFunction","paramDatatypes":["String"],"params":["myTestParameter"],"requestReplyId":"FVeNNFkkEDgfHMrslmHU__18","_typeName":"joynr.Request"}
                requestReplyId: test.request.requestReplyId
            })
        );
    });

    it("delivers exception when calling not existing operation", async () => {
        const test = await callRequestReplyManager(
            "notExistentOperationOrAttribute",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        );

        expect(test.callbackDispatcher).toHaveBeenCalled();
        expect(test.callbackDispatcher).toHaveBeenCalledWith(
            replySettings,
            Reply.create({
                error: new MethodInvocationException({
                    detailMessage: 'Could not find an operation "notExistentOperationOrAttribute" in the provider',
                    providerVersion: new Version({
                        majorVersion: 47,
                        minorVersion: 11
                    })
                }),
                requestReplyId: test.request.requestReplyId
            })
        );
    });
    it("delivers exception when calling getter for not existing attribute", async () => {
        const test = await callRequestReplyManager(
            "getNotExistentOperationOrAttribute",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        );

        expect(test.callbackDispatcher).toHaveBeenCalled();
        expect(test.callbackDispatcher).toHaveBeenCalledWith(
            replySettings,
            Reply.create({
                error: new MethodInvocationException({
                    detailMessage:
                        'Could not find an operation "getNotExistentOperationOrAttribute" or an attribute "NotExistentOperationOrAttribute" in the provider',
                    providerVersion: new Version({
                        majorVersion: 47,
                        minorVersion: 11
                    })
                }),
                requestReplyId: test.request.requestReplyId
            })
        );
    });
    it("delivers exception when calling setter for not existing attribute", async () => {
        const test = await callRequestReplyManager(
            "setNotExistentOperationOrAttribute",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        );

        expect(test.callbackDispatcher).toHaveBeenCalled();
        expect(test.callbackDispatcher).toHaveBeenCalledWith(
            replySettings,
            Reply.create({
                error: new MethodInvocationException({
                    detailMessage:
                        'Could not find an operation "setNotExistentOperationOrAttribute" or an attribute "NotExistentOperationOrAttribute" in the provider',
                    providerVersion: new Version({
                        majorVersion: 47,
                        minorVersion: 11
                    })
                }),
                requestReplyId: test.request.requestReplyId
            })
        );
    });

    it("throws exception when called while shut down", async () => {
        requestReplyManager.shutdown();
        expect(() => {
            requestReplyManager.removeRequestCaller("providerParticipantId");
        }).toThrow();

        const replySettings = {};

        await requestReplyManager.handleRequest(
            "providerParticipantId",
            {
                requestReplyId
            } as any,
            (settings: any, reply: Reply.Reply) => {
                expect(settings).toBe(replySettings);
                expect(reply._typeName).toEqual("joynr.Reply");
                expect(reply.error).toBeInstanceOf(MethodInvocationException);
            },
            replySettings
        );

        expect(() => {
            const replyCallerSpy = {
                resolve: jest.fn(),
                reject: jest.fn()
            };

            requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy as any, undefined as any);
        }).toThrow();
        expect(() => {
            requestReplyManager.addRequestCaller("providerParticipantId", {});
        }).toThrow();
        expect(() => {
            requestReplyManager.sendOneWayRequest({} as any);
        }).toThrow();
        await testUtil.reversePromise(requestReplyManager.sendRequest({} as any, {}));
        expect(dispatcherSpy.sendRequest).not.toHaveBeenCalled();
    });
    it("rejects reply callers when shut down", () => {
        const replyCallerSpy = {
            callback: jest.fn()
        };

        requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy as any, ttlMs);
        requestReplyManager.shutdown();
        expect(replyCallerSpy.callback).toHaveBeenCalledWith(expect.any(Error));
    });

    it("sendOneWayRequest calls dispatcher with correct arguments", () => {
        const parameters = {
            from: "fromParticipantId",
            toDiscoveryEntry: providerDiscoveryEntry,
            messagingQos: new MessagingQos({
                ttl: 1024
            }),
            request: OneWayRequest.create({
                methodName: "testMethodName"
            })
        };
        const expectedArguments: any = UtilInternal.extendDeep({}, parameters);
        expectedArguments.messagingQos = new MessagingQos(parameters.messagingQos);
        expectedArguments.toDiscoveryEntry = new DiscoveryEntryWithMetaInfo(parameters.toDiscoveryEntry);
        expectedArguments.request = OneWayRequest.create(parameters.request);

        dispatcherSpy.sendOneWayRequest.mockReturnValue(Promise.resolve());
        requestReplyManager.sendOneWayRequest(parameters);

        expect(dispatcherSpy.sendOneWayRequest).toHaveBeenCalledWith(expectedArguments);
    });
});
