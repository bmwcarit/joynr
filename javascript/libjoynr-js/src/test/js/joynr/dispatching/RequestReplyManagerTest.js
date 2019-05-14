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
const RequestReplyManager = require("../../../../main/js/joynr/dispatching/RequestReplyManager");
const OneWayRequest = require("../../../../main/js/joynr/dispatching/types/OneWayRequest");
const Request = require("../../../../main/js/joynr/dispatching/types/Request");
const Reply = require("../../../../main/js/joynr/dispatching/types/Reply");
const TypeRegistrySingleton = require("../../../../main/js/joynr/types/TypeRegistrySingleton");
const Typing = require("../../../../main/js/joynr/util/Typing");
const UtilInternal = require("../../../../main/js/joynr/util/UtilInternal");
const JSONSerializer = require("../../../../main/js/joynr/util/JSONSerializer");
const MethodInvocationException = require("../../../../main/js/joynr/exceptions/MethodInvocationException");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const DiscoveryEntryWithMetaInfo = require("../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const waitsFor = require("../../../../test/js/global/WaitsFor");
const testUtil = require("../../../js/testUtil");
describe("libjoynr-js.joynr.dispatching.RequestReplyManager", () => {
    let dispatcherSpy;
    let requestReplyManager;
    let typeRegistry;
    const ttl_ms = 50;
    const toleranceMs = 1500; // at least 1000 since that's the cleanup interval
    const requestReplyId = "requestReplyId";
    const testResponse = ["testResponse"];
    const reply = Reply.create({
        requestReplyId,
        response: testResponse
    });
    const replySettings = {};

    const providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
        providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
        domain: "testProviderDomain",
        interfaceName: "interfaceName",
        participantId: "providerParticipantId",
        qos: new ProviderQos(),
        lastSeenDateMs: Date.now(),
        expiryDateMs: Date.now() + 60000,
        publicKeyId: "publicKeyId",
        isLocal: true
    });

    function RadioStation(name, station, source) {
        if (!(this instanceof RadioStation)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new RadioStation(name, station, source);
        }
        this.name = name;
        this.station = station;
        this.source = source;

        Object.defineProperty(this, "checkMembers", {
            configurable: false,
            writable: false,
            enumerable: false,
            value: jasmine.createSpy("checkMembers")
        });

        Object.defineProperty(this, "_typeName", {
            configurable: false,
            writable: false,
            enumerable: true,
            value: "test.RadioStation"
        });
    }

    RadioStation.getMemberType = function() {};

    function ComplexTypeWithComplexAndSimpleProperties(radioStation, myBoolean, myString) {
        if (!(this instanceof ComplexTypeWithComplexAndSimpleProperties)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new ComplexTypeWithComplexAndSimpleProperties(radioStation, myBoolean, myString);
        }
        this.radioStation = radioStation;
        this.myBoolean = myBoolean;
        this.myString = myString;

        Object.defineProperty(this, "checkMembers", {
            configurable: false,
            writable: false,
            enumerable: false,
            value: jasmine.createSpy("checkMembers")
        });

        Object.defineProperty(this, "_typeName", {
            configurable: false,
            writable: false,
            enumerable: true,
            value: "test.ComplexTypeWithComplexAndSimpleProperties"
        });
    }

    ComplexTypeWithComplexAndSimpleProperties.getMemberType = function() {};

    /**
     * Called before each test.
     */
    beforeEach(() => {
        dispatcherSpy = jasmine.createSpyObj("DispatcherSpy", ["sendOneWayRequest", "sendRequest"]);
        typeRegistry = TypeRegistrySingleton.getInstance();
        typeRegistry.addType("test.RadioStation", RadioStation);
        typeRegistry.addType(
            "test.ComplexTypeWithComplexAndSimpleProperties",
            ComplexTypeWithComplexAndSimpleProperties
        );
        requestReplyManager = new RequestReplyManager(dispatcherSpy, typeRegistry);
    });

    afterEach(() => {
        requestReplyManager.shutdown();
    });

    it("is instantiable", done => {
        expect(requestReplyManager).toBeDefined();
        done();
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

    function testHandleRequestForGetterSetterMethod(attributeName, params, promiseChain) {
        const providerParticipantId = "providerParticipantId";
        const provider = {};
        provider[attributeName] = {
            get: jasmine.createSpy("getSpy"),
            set: jasmine.createSpy("setSpy")
        };

        provider[attributeName].get.and.returnValue([]);
        provider[attributeName].set.and.returnValue([]);

        return promiseChain
            .then(() => {
                const request = Request.create({
                    methodName: `get${UtilInternal.firstUpper(attributeName)}`,
                    paramDatatypes: [],
                    params: []
                });

                requestReplyManager.addRequestCaller(providerParticipantId, provider);

                requestReplyManager.handleRequest(providerParticipantId, request, jasmine.createSpy);

                return waitsFor(
                    () => {
                        return provider[attributeName].get.calls.count() > 0;
                    },
                    "getAttribute to be called",
                    100
                );
            })
            .then(() => {
                const request = Request.create({
                    methodName: `set${UtilInternal.firstUpper(attributeName)}`,
                    paramDatatypes: [],
                    // untype objects through serialization and deserialization
                    params: JSON.parse(JSONSerializer.stringify(params))
                });

                requestReplyManager.addRequestCaller(providerParticipantId, provider);

                requestReplyManager.handleRequest(providerParticipantId, request, jasmine.createSpy);

                return waitsFor(
                    () => {
                        return provider[attributeName].set.calls.count() > 0;
                    },
                    "setAttribute to be called",
                    100
                );
            })
            .then(() => {
                expect(provider[attributeName].get).toHaveBeenCalledWith();

                expect(provider[attributeName].set).toHaveBeenCalledWith(params[0]);

                const result = provider[attributeName].set.calls.argsFor(0)[0];
                expect(result).toEqual(params[0]);
            });
    }

    function testHandleRequestWithExpectedType(paramDatatypes, params, promiseChain) {
        const providerParticipantId = "providerParticipantId";
        const provider = {
            testFunction: {
                callOperation: jasmine.createSpy("operationSpy")
            }
        };

        provider.testFunction.callOperation.and.returnValue([]);

        return promiseChain
            .then(() => {
                const request = Request.create({
                    methodName: "testFunction",
                    paramDatatypes,
                    // untype objects through serialization and deserialization
                    params: JSON.parse(JSONSerializer.stringify(params))
                });

                requestReplyManager.addRequestCaller(providerParticipantId, provider);
                requestReplyManager.handleRequest(providerParticipantId, request, jasmine.createSpy);

                return waitsFor(
                    () => {
                        return provider.testFunction.callOperation.calls.count() > 0;
                    },
                    "callOperation to be called",
                    100
                );
            })
            .then(() => {
                expect(provider.testFunction.callOperation).toHaveBeenCalled();
                expect(provider.testFunction.callOperation).toHaveBeenCalledWith(params, paramDatatypes);

                const result = provider.testFunction.callOperation.calls.argsFor(0)[0];
                expect(result).toEqual(params);
                expect(Typing.getObjectType(result)).toEqual(Typing.getObjectType(params));
            });
    }

    it("calls registered requestCaller for attribute", done => {
        const promiseChain = Promise.resolve();
        testHandleRequestForGetterSetterMethod("attributeA", ["attributeA"], promiseChain);
        testHandleRequestForGetterSetterMethod(
            "AttributeWithStartingCapitalLetter",
            ["AttributeWithStartingCapitalLetter"],
            promiseChain
        );
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls registered requestCaller with correctly typed object", done => {
        let i, test;
        let promiseChain = Promise.resolve();
        for (i = 0; i < testData.length; ++i) {
            test = testData[i];
            promiseChain = testHandleRequestWithExpectedType(test.paramDatatype, test.params, promiseChain);
        }
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls registered replyCaller when a reply arrives", done => {
        const replyCallerSpy = jasmine.createSpyObj("promise", ["callback"]);
        replyCallerSpy.callbackSettings = {};

        const timeout = toleranceMs + ttl_ms;

        requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy, ttl_ms);
        requestReplyManager.handleReply(reply);

        waitsFor(
            () => {
                return replyCallerSpy.callback.calls.count() > 0;
            },
            "reject or fulfill to be called",
            timeout
        )
            .then(() => {
                expect(replyCallerSpy.callback).toHaveBeenCalled();
                expect(replyCallerSpy.callback).toHaveBeenCalledWith(undefined, {
                    response: testResponse,
                    settings: replyCallerSpy.callbackSettings
                });
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls registered replyCaller fail if no reply arrives in time", done => {
        const replyCallerSpy = jasmine.createSpyObj("deferred", ["callback"]);

        const timeout = toleranceMs + ttl_ms;

        requestReplyManager.addReplyCaller("requestReplyId", replyCallerSpy, ttl_ms);

        waitsFor(
            () => {
                return replyCallerSpy.callback.calls.count() > 0;
            },
            "reject or fulfill to be called",
            timeout
        )
            .then(() => {
                expect(replyCallerSpy.callback).toHaveBeenCalledWith(jasmine.any(Error));
                done();
            })
            .catch(fail);
    });

    function testHandleReplyWithExpectedType(params, promiseChain) {
        const replyCallerSpy = jasmine.createSpyObj("deferred", ["callback"]);
        return promiseChain
            .then(() => {
                const reply = Reply.create({
                    requestReplyId,
                    // untype object by serializing and deserializing it
                    response: JSON.parse(JSONSerializer.stringify(params))
                });

                requestReplyManager.addReplyCaller("requestReplyId", replyCallerSpy, ttl_ms);
                requestReplyManager.handleReply(reply);

                return waitsFor(
                    () => {
                        return replyCallerSpy.callback.calls.count() > 0;
                    },
                    "reject or fulfill to be called",
                    ttl_ms * 2
                );
            })
            .then(() => {
                let i;
                expect(replyCallerSpy.callback).toHaveBeenCalled();

                const result = replyCallerSpy.callback.calls.argsFor(0)[1];
                for (i = 0; i < params.length; i++) {
                    expect(result.response[i]).toEqual(params[i]);
                }
            })
            .catch(error => {
                fail(error);
            });
    }

    it("calls registered replyCaller with correctly typed object", done => {
        let i, test;
        let promiseChain = Promise.resolve();
        for (i = 0; i < testData.length; ++i) {
            test = testData[i];
            promiseChain = testHandleReplyWithExpectedType(test.params, promiseChain);
        }
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function callRequestReplyManagerSync(
        methodName,
        testParam,
        testParamDatatype,
        useInvalidProviderParticipantId,
        callbackContext
    ) {
        let providerParticipantId = "providerParticipantId";
        const TestProvider = function() {};
        TestProvider.MAJOR_VERSION = 47;
        TestProvider.MINOR_VERSION = 11;
        const provider = new TestProvider();
        UtilInternal.extend(
            provider,
            {
                attributeName: {
                    get: jasmine.createSpy("getterSpy"),
                    set: jasmine.createSpy("setterSpy")
                },
                operationName: {
                    callOperation: jasmine.createSpy("operationSpy")
                },
                getOperationStartingWithGet: {
                    callOperation: jasmine.createSpy("operationSpy")
                },
                getOperationHasPriority: {
                    callOperation: jasmine.createSpy("operationSpy")
                },
                operationHasPriority: {
                    get: jasmine.createSpy("getterSpy"),
                    set: jasmine.createSpy("setterSpy")
                }
            },
            true
        );
        provider.attributeName.get.and.returnValue([testParam]);
        provider.attributeName.set.and.returnValue([]);
        provider.operationName.callOperation.and.returnValue([testParam]);
        provider.getOperationStartingWithGet.callOperation.and.returnValue([testParam]);
        provider.getOperationHasPriority.callOperation.and.returnValue([testParam]);

        const callbackDispatcher = jasmine.createSpy("callbackDispatcher");

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

    function callRequestReplyManager(
        methodName,
        testParam,
        testParamDatatype,
        useInvalidProviderParticipantId,
        callbackContext
    ) {
        const test = callRequestReplyManagerSync(
            methodName,
            testParam,
            testParamDatatype,
            useInvalidProviderParticipantId,
            callbackContext
        );

        return waitsFor(
            () => {
                return test.callbackDispatcher.calls.count() > 0;
            },
            "callbackDispatcher to be called",
            100
        ).then(() => {
            return Promise.resolve(test);
        });

        //return test;
    }

    const testParam = "myTestParameter";
    const testParamDatatype = "String";

    it("calls attribute getter correctly", done => {
        callRequestReplyManager("getAttributeName", testParam, testParamDatatype, undefined, replySettings)
            .then(test => {
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
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls attribute setter correctly", done => {
        callRequestReplyManager("setAttributeName", testParam, testParamDatatype, undefined, replySettings)
            .then(test => {
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
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls operation function correctly", done => {
        callRequestReplyManager("operationName", testParam, testParamDatatype, undefined, replySettings)
            .then(test => {
                expect(test.provider.attributeName.set).not.toHaveBeenCalled();
                expect(test.provider.attributeName.get).not.toHaveBeenCalled();
                expect(test.provider.operationName.callOperation).toHaveBeenCalled();
                expect(test.provider.operationName.callOperation).toHaveBeenCalledWith(
                    [testParam],
                    [testParamDatatype]
                );
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
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls operation function for one-way request correctly", done => {
        const providerParticipantId = "oneWayProviderParticipantId";
        const provider = {
            fireAndForgetMethod: {
                callOperation: jasmine.createSpy("operationSpy")
            }
        };

        const oneWayRequest = OneWayRequest.create({
            methodName: "fireAndForgetMethod",
            paramDatatypes: [testParamDatatype],
            params: [testParam]
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);

        requestReplyManager.handleOneWayRequest(providerParticipantId, oneWayRequest);

        waitsFor(
            () => {
                return provider.fireAndForgetMethod.callOperation.calls.count() > 0;
            },
            "callOperation to be called",
            1000
        )
            .then(() => {
                expect(provider.fireAndForgetMethod.callOperation).toHaveBeenCalled();
                expect(provider.fireAndForgetMethod.callOperation).toHaveBeenCalledWith(
                    [testParam],
                    [testParamDatatype]
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it('calls operation "getOperationStartingWithGet" when no attribute "operationStartingWithGet" exists', done => {
        callRequestReplyManager("getOperationStartingWithGet", testParam, testParamDatatype)
            .then(test => {
                expect(test.provider.getOperationStartingWithGet.callOperation).toHaveBeenCalled();
                expect(test.provider.getOperationStartingWithGet.callOperation).toHaveBeenCalledWith(
                    [testParam],
                    [testParamDatatype]
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it('calls operation "getOperationHasPriority" when attribute "operationHasPriority" exists', done => {
        callRequestReplyManager("getOperationHasPriority", testParam, testParamDatatype)
            .then(test => {
                expect(test.provider.operationHasPriority.set).not.toHaveBeenCalled();
                expect(test.provider.operationHasPriority.get).not.toHaveBeenCalled();
                expect(test.provider.getOperationHasPriority.callOperation).toHaveBeenCalled();
                expect(test.provider.getOperationHasPriority.callOperation).toHaveBeenCalledWith(
                    [testParam],
                    [testParamDatatype]
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("delivers exception upon non-existent provider", done => {
        callRequestReplyManager("testFunction", testParam, testParamDatatype, true, replySettings)
            .then(test => {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    replySettings,
                    Reply.create({
                        error: new MethodInvocationException({
                            detailMessage: `error handling request: {"paramDatatypes":["String"],"params":["myTestParameter"],"methodName":"testFunction","requestReplyId":"${
                                test.request.requestReplyId
                            }","_typeName":"joynr.Request"} for providerParticipantId nonExistentProviderId`
                        }),
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("delivers exception when calling not existing operation", done => {
        callRequestReplyManager(
            "notExistentOperationOrAttribute",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        )
            .then(test => {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    replySettings,
                    Reply.create({
                        error: new MethodInvocationException({
                            detailMessage:
                                'Could not find an operation "notExistentOperationOrAttribute" in the provider',
                            providerVersion: new Version({
                                majorVersion: 47,
                                minorVersion: 11
                            })
                        }),
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
    });
    it("delivers exception when calling getter for not existing attribute", done => {
        callRequestReplyManager(
            "getNotExistentOperationOrAttribute",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        )
            .then(test => {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    replySettings,
                    Reply.create({
                        error: new MethodInvocationException({
                            detailMessage:
                                'Could not find an operation "getNotExistentOperationOrAttribute" or an attribute "notExistentOperationOrAttribute" in the provider',
                            providerVersion: new Version({
                                majorVersion: 47,
                                minorVersion: 11
                            })
                        }),
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
    });
    it("delivers exception when calling setter for not existing attribute", done => {
        callRequestReplyManager(
            "setNotExistentOperationOrAttribute",
            testParam,
            testParamDatatype,
            undefined,
            replySettings
        )
            .then(test => {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    replySettings,
                    Reply.create({
                        error: new MethodInvocationException({
                            detailMessage:
                                'Could not find an operation "setNotExistentOperationOrAttribute" or an attribute "notExistentOperationOrAttribute" in the provider',
                            providerVersion: new Version({
                                majorVersion: 47,
                                minorVersion: 11
                            })
                        }),
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
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
            },
            (settings, reply) => {
                expect(settings).toBe(replySettings);
                expect(reply._typeName).toEqual("joynr.Reply");
                expect(reply.error instanceof MethodInvocationException);
            },
            replySettings
        );

        expect(() => {
            const replyCallerSpy = jasmine.createSpyObj("promise", ["resolve", "reject"]);

            requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy);
        }).toThrow();
        expect(() => {
            requestReplyManager.addRequestCaller("providerParticipantId", {});
        }).toThrow();
        expect(() => {
            requestReplyManager.sendOneWayRequest({});
        }).toThrow();
        await testUtil.reversePromise(requestReplyManager.sendRequest({}, {}));
        expect(dispatcherSpy.sendRequest).not.toHaveBeenCalled();
    });
    it("rejects reply callers when shut down", done => {
        const replyCallerSpy = jasmine.createSpyObj("promise", ["callback"]);

        requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy, ttl_ms);
        requestReplyManager.shutdown();
        expect(replyCallerSpy.callback).toHaveBeenCalledWith(jasmine.any(Error));
        done();
    });

    it("sendOneWayRequest calls dispatcher with correct arguments", done => {
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
        const expectedArguments = UtilInternal.extendDeep({}, parameters);
        expectedArguments.messagingQos = new MessagingQos(parameters.messagingQos);
        expectedArguments.toDiscoveryEntry = new DiscoveryEntryWithMetaInfo(parameters.toDiscoveryEntry);
        expectedArguments.request = OneWayRequest.create(parameters.request);

        dispatcherSpy.sendOneWayRequest.and.returnValue(Promise.resolve());
        requestReplyManager.sendOneWayRequest(parameters);

        expect(dispatcherSpy.sendOneWayRequest).toHaveBeenCalledWith(expectedArguments);
        done();
    });
});
