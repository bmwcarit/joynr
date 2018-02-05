/*jslint es5: true, node: true, node: true */
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
var RequestReplyManager = require("../../../classes/joynr/dispatching/RequestReplyManager");
var OneWayRequest = require("../../../classes/joynr/dispatching/types/OneWayRequest");
var Request = require("../../../classes/joynr/dispatching/types/Request");
var Reply = require("../../../classes/joynr/dispatching/types/Reply");
var TypeRegistrySingleton = require("../../../classes/joynr/types/TypeRegistrySingleton");
var Typing = require("../../../classes/joynr/util/Typing");
var UtilInternal = require("../../../classes/joynr/util/UtilInternal");
var JSONSerializer = require("../../../classes/joynr/util/JSONSerializer");
var MethodInvocationException = require("../../../classes/joynr/exceptions/MethodInvocationException");
var Version = require("../../../classes/joynr/types/Version");
var DiscoveryEntryWithMetaInfo = require("../../../classes/joynr/types/DiscoveryEntryWithMetaInfo");
var ProviderQos = require("../../../classes/joynr/types/ProviderQos");
var MessagingQos = require("../../../classes/joynr/messaging/MessagingQos");
var Promise = require("../../../classes/global/Promise");
var waitsFor = require("../../../test-classes/global/WaitsFor");
describe("libjoynr-js.joynr.dispatching.RequestReplyManager", function() {
    var dispatcherSpy;
    var requestReplyManager;
    var typeRegistry;
    var ttl_ms = 50;
    var toleranceMs = 1500; // at least 1000 since that's the cleanup interval
    var requestReplyId = "requestReplyId";
    var testResponse = ["testResponse"];
    var reply = new Reply({
        requestReplyId: requestReplyId,
        response: testResponse
    });

    var providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
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

    RadioStation.getMemberType = function(i) {};

    var Country = {
        AUSTRALIA: "AUSTRALIA",
        AUSTRIA: "AUSTRIA",
        CANADA: "CANADA",
        GERMANY: "GERMANY",
        ITALY: "ITALY",
        UNITED_KINGDOM: "UNITED_KINGDOM"
    };

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

    ComplexTypeWithComplexAndSimpleProperties.getMemberType = function(i) {};

    /**
     * Called before each test.
     */
    beforeEach(function() {
        dispatcherSpy = jasmine.createSpyObj("DispatcherSpy", ["sendOneWayRequest", "sendRequest"]);
        typeRegistry = TypeRegistrySingleton.getInstance();
        typeRegistry.addType("test.RadioStation", RadioStation);
        typeRegistry.addType(
            "test.ComplexTypeWithComplexAndSimpleProperties",
            ComplexTypeWithComplexAndSimpleProperties
        );
        requestReplyManager = new RequestReplyManager(dispatcherSpy, typeRegistry);
    });

    afterEach(function() {
        requestReplyManager.shutdown();
    });

    it("is instantiable", function(done) {
        expect(requestReplyManager).toBeDefined();
        done();
    });

    var tripleJ = new RadioStation("TripleJ", "107.7", "AUSTRALIA");
    var fm4 = new RadioStation("FM4", "104.0", "AUSTRIA");
    var complex = new ComplexTypeWithComplexAndSimpleProperties(tripleJ, true, "hello");
    var testData = [
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
        var providerParticipantId = "providerParticipantId";
        var provider = {};
        provider[attributeName] = {
            get: jasmine.createSpy("getSpy"),
            set: jasmine.createSpy("setSpy")
        };

        provider[attributeName].get.and.returnValue([]);
        provider[attributeName].set.and.returnValue([]);

        return promiseChain
            .then(function() {
                var request = new Request({
                    methodName: "get" + UtilInternal.firstUpper(attributeName),
                    paramDatatypes: [],
                    params: []
                });

                requestReplyManager.addRequestCaller(providerParticipantId, provider);

                requestReplyManager.handleRequest(providerParticipantId, request, jasmine.createSpy);

                return waitsFor(
                    function() {
                        return provider[attributeName].get.calls.count() > 0;
                    },
                    "getAttribute to be called",
                    100
                );
            })
            .then(function() {
                var request = new Request({
                    methodName: "set" + UtilInternal.firstUpper(attributeName),
                    paramDatatypes: [],
                    // untype objects through serialization and deserialization
                    params: JSON.parse(JSONSerializer.stringify(params))
                });

                requestReplyManager.addRequestCaller(providerParticipantId, provider);

                requestReplyManager.handleRequest(providerParticipantId, request, jasmine.createSpy);

                return waitsFor(
                    function() {
                        return provider[attributeName].set.calls.count() > 0;
                    },
                    "setAttribute to be called",
                    100
                );
            })
            .then(function() {
                expect(provider[attributeName].get).toHaveBeenCalledWith();

                expect(provider[attributeName].set).toHaveBeenCalledWith(params[0]);

                var result = provider[attributeName].set.calls.argsFor(0)[0];
                expect(result).toEqual(params[0]);
            });
    }

    function testHandleRequestWithExpectedType(paramDatatypes, params, promiseChain) {
        var providerParticipantId = "providerParticipantId";
        var provider = {
            testFunction: {
                callOperation: jasmine.createSpy("operationSpy")
            }
        };

        provider.testFunction.callOperation.and.returnValue([]);

        return promiseChain
            .then(function() {
                var request = new Request({
                    methodName: "testFunction",
                    paramDatatypes: paramDatatypes,
                    // untype objects through serialization and deserialization
                    params: JSON.parse(JSONSerializer.stringify(params))
                });

                requestReplyManager.addRequestCaller(providerParticipantId, provider);
                requestReplyManager.handleRequest(providerParticipantId, request, jasmine.createSpy);

                return waitsFor(
                    function() {
                        return provider.testFunction.callOperation.calls.count() > 0;
                    },
                    "callOperation to be called",
                    100
                );
            })
            .then(function() {
                expect(provider.testFunction.callOperation).toHaveBeenCalled();
                expect(provider.testFunction.callOperation).toHaveBeenCalledWith(params, paramDatatypes);

                var result = provider.testFunction.callOperation.calls.argsFor(0)[0];
                expect(result).toEqual(params);
                expect(Typing.getObjectType(result)).toEqual(Typing.getObjectType(params));
            });
    }

    it("calls registered requestCaller for attribute", function(done) {
        var promiseChain = Promise.resolve();
        testHandleRequestForGetterSetterMethod("attributeA", ["attributeA"], promiseChain);
        testHandleRequestForGetterSetterMethod(
            "AttributeWithStartingCapitalLetter",
            ["AttributeWithStartingCapitalLetter"],
            promiseChain
        );
        promiseChain
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls registered requestCaller with correctly typed object", function(done) {
        var i, test;
        var promiseChain = Promise.resolve();
        for (i = 0; i < testData.length; ++i) {
            test = testData[i];
            promiseChain = testHandleRequestWithExpectedType(test.paramDatatype, test.params, promiseChain);
        }
        promiseChain
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls registered replyCaller when a reply arrives", function(done) {
        var replyCallerSpy = jasmine.createSpyObj("promise", ["resolve", "reject"]);

        var timeout = toleranceMs + ttl_ms;

        requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy, ttl_ms);
        requestReplyManager.handleReply(reply);

        waitsFor(
            function() {
                return replyCallerSpy.resolve.calls.count() > 0 || replyCallerSpy.reject.calls.count() > 0;
            },
            "reject or fulfill to be called",
            timeout
        )
            .then(function() {
                expect(replyCallerSpy.resolve).toHaveBeenCalled();
                expect(replyCallerSpy.resolve).toHaveBeenCalledWith(testResponse);
                expect(replyCallerSpy.reject).not.toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls registered replyCaller fail if no reply arrives in time", function(done) {
        var replyCallerSpy = jasmine.createSpyObj("deferred", ["resolve", "reject"]);

        var timeout = toleranceMs + ttl_ms;

        requestReplyManager.addReplyCaller("requestReplyId", replyCallerSpy, ttl_ms);

        waitsFor(
            function() {
                return replyCallerSpy.resolve.calls.count() > 0 || replyCallerSpy.reject.calls.count() > 0;
            },
            "reject or fulfill to be called",
            timeout
        )
            .then(function() {
                expect(replyCallerSpy.resolve).not.toHaveBeenCalled();
                expect(replyCallerSpy.reject).toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    function testHandleReplyWithExpectedType(params, promiseChain) {
        var replyCallerSpy = jasmine.createSpyObj("deferred", ["resolve", "reject"]);
        return promiseChain
            .then(function() {
                var reply = new Reply({
                    requestReplyId: requestReplyId,
                    // untype object by serializing and deserializing it
                    response: JSON.parse(JSONSerializer.stringify(params))
                });

                requestReplyManager.addReplyCaller("requestReplyId", replyCallerSpy, ttl_ms);
                requestReplyManager.handleReply(reply);

                return waitsFor(
                    function() {
                        return replyCallerSpy.resolve.calls.count() > 0 || replyCallerSpy.reject.calls.count() > 0;
                    },
                    "reject or fulfill to be called",
                    ttl_ms * 2
                );
            })
            .then(function() {
                var i;
                expect(replyCallerSpy.resolve).toHaveBeenCalled();
                expect(replyCallerSpy.reject).not.toHaveBeenCalled();

                var result = replyCallerSpy.resolve.calls.argsFor(0)[0];
                for (i = 0; i < params.length; i++) {
                    expect(result[i]).toEqual(params[i]);
                    expect(Typing.getObjectType(result[i])).toEqual(Typing.getObjectType(params[i]));
                }
            })
            .catch(function(error) {
                fail(error);
            });
    }

    it("calls registered replyCaller with correctly typed object", function(done) {
        var i, test;
        var promiseChain = Promise.resolve();
        for (i = 0; i < testData.length; ++i) {
            test = testData[i];
            promiseChain = testHandleReplyWithExpectedType(test.params, promiseChain);
        }
        promiseChain
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    function callRequestReplyManagerSync(methodName, testParam, testParamDatatype, useInvalidProviderParticipantId) {
        var providerParticipantId = "providerParticipantId";
        var TestProvider = function() {};
        TestProvider.MAJOR_VERSION = 47;
        TestProvider.MINOR_VERSION = 11;
        var provider = new TestProvider();
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

        var callbackDispatcher = jasmine.createSpy("callbackDispatcher");

        var request = new Request({
            methodName: methodName,
            paramDatatypes: [testParamDatatype],
            params: [testParam]
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);

        if (useInvalidProviderParticipantId) {
            providerParticipantId = "nonExistentProviderId";
        }

        requestReplyManager.handleRequest(providerParticipantId, request).then(callbackDispatcher);

        return {
            provider: provider,
            callbackDispatcher: callbackDispatcher,
            request: request
        };
    }

    function callRequestReplyManager(methodName, testParam, testParamDatatype, useInvalidProviderParticipantId) {
        var test = callRequestReplyManagerSync(
            methodName,
            testParam,
            testParamDatatype,
            useInvalidProviderParticipantId
        );

        return waitsFor(
            function() {
                return test.callbackDispatcher.calls.count() > 0;
            },
            "callbackDispatcher to be called",
            100
        ).then(function() {
            return Promise.resolve(test);
        });

        //return test;
    }

    var testParam = "myTestParameter";
    var testParamDatatype = "String";

    it("calls attribute getter correctly", function(done) {
        callRequestReplyManager("getAttributeName", testParam, testParamDatatype)
            .then(function(test) {
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
                    new Reply({
                        response: [testParam],
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls attribute setter correctly", function(done) {
        callRequestReplyManager("setAttributeName", testParam, testParamDatatype)
            .then(function(test) {
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
                    new Reply({
                        response: [],
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls operation function correctly", function(done) {
        callRequestReplyManager("operationName", testParam, testParamDatatype)
            .then(function(test) {
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
                    new Reply({
                        response: [testParam],
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("calls operation function for one-way request correctly", function(done) {
        var providerParticipantId = "oneWayProviderParticipantId";
        var provider = {
            fireAndForgetMethod: {
                callOperation: jasmine.createSpy("operationSpy")
            }
        };

        var callbackDispatcher = jasmine.createSpy("callbackDispatcher");

        var oneWayRequest = new OneWayRequest({
            methodName: "fireAndForgetMethod",
            paramDatatypes: [testParamDatatype],
            params: [testParam]
        });

        requestReplyManager.addRequestCaller(providerParticipantId, provider);

        requestReplyManager.handleOneWayRequest(providerParticipantId, oneWayRequest);

        waitsFor(
            function() {
                return provider.fireAndForgetMethod.callOperation.calls.count() > 0;
            },
            "callOperation to be called",
            1000
        )
            .then(function() {
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

    it('calls operation "getOperationStartingWithGet" when no attribute "operationStartingWithGet" exists', function(
        done
    ) {
        callRequestReplyManager("getOperationStartingWithGet", testParam, testParamDatatype)
            .then(function(test) {
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

    it('calls operation "getOperationHasPriority" when attribute "operationHasPriority" exists', function(done) {
        callRequestReplyManager("getOperationHasPriority", testParam, testParamDatatype)
            .then(function(test) {
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

    it("delivers exception upon non-existent provider", function(done) {
        callRequestReplyManager("testFunction", testParam, testParamDatatype, true)
            .then(function(test) {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    new Reply({
                        error: new MethodInvocationException({
                            detailMessage:
                                'error handling request: {"paramDatatypes":["String"],"params":["myTestParameter"],"methodName":"testFunction","requestReplyId":"' +
                                test.request.requestReplyId +
                                '","_typeName":"joynr.Request"} for providerParticipantId nonExistentProviderId'
                        }),
                        requestReplyId: test.request.requestReplyId
                    })
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("delivers exception when calling not existing operation", function(done) {
        callRequestReplyManager("notExistentOperationOrAttribute", testParam, testParamDatatype)
            .then(function(test) {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    new Reply({
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
    it("delivers exception when calling getter for not existing attribute", function(done) {
        callRequestReplyManager("getNotExistentOperationOrAttribute", testParam, testParamDatatype)
            .then(function(test) {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    new Reply({
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
    it("delivers exception when calling setter for not existing attribute", function(done) {
        callRequestReplyManager("setNotExistentOperationOrAttribute", testParam, testParamDatatype)
            .then(function(test) {
                expect(test.callbackDispatcher).toHaveBeenCalled();
                expect(test.callbackDispatcher).toHaveBeenCalledWith(
                    new Reply({
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

    it(" throws exception when called while shut down", function(done) {
        requestReplyManager.shutdown();
        expect(function() {
            requestReplyManager.removeRequestCaller("providerParticipantId");
        }).toThrow();
        requestReplyManager
            .handleRequest("providerParticipantId", {
                requestReplyId: requestReplyId
            })
            .then(function(reply) {
                expect(reply instanceof Reply);
                expect(reply.error instanceof MethodInvocationException);
                expect(function() {
                    var replyCallerSpy = jasmine.createSpyObj("promise", ["resolve", "reject"]);

                    requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy);
                }).toThrow();
                expect(function() {
                    requestReplyManager.addRequestCaller("providerParticipantId", {});
                }).toThrow();
                expect(function() {
                    requestReplyManager.sendOneWayRequest({});
                }).toThrow();
                expect(function() {
                    requestReplyManager.sendRequest({});
                }).toThrow();
                done();
            });
    });
    it(" rejects reply callers when shut down", function(done) {
        var replyCallerSpy = jasmine.createSpyObj("promise", ["resolve", "reject"]);

        requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy, ttl_ms);
        expect(replyCallerSpy.reject).not.toHaveBeenCalled();
        requestReplyManager.shutdown();
        expect(replyCallerSpy.reject).toHaveBeenCalled();
        done();
    });

    it("sendOneWayRequest calls dispatcher with correct arguments", function(done) {
        var parameters = {
            from: "fromParticipantId",
            toDiscoveryEntry: providerDiscoveryEntry,
            messagingQos: new MessagingQos({
                ttl: 1024
            }),
            request: new OneWayRequest({
                methodName: "testMethodName"
            })
        };
        var expectedArguments = UtilInternal.extendDeep({}, parameters);
        expectedArguments.messagingQos = new MessagingQos(parameters.messagingQos);
        expectedArguments.toDiscoveryEntry = new DiscoveryEntryWithMetaInfo(parameters.toDiscoveryEntry);
        expectedArguments.request = new OneWayRequest(parameters.request);

        dispatcherSpy.sendOneWayRequest.and.returnValue(Promise.resolve());
        requestReplyManager.sendOneWayRequest(parameters);

        expect(dispatcherSpy.sendOneWayRequest).toHaveBeenCalledWith(expectedArguments);
        done();
    });
});
