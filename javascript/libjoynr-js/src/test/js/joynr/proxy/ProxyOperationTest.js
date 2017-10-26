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
var ProxyAttribute = require("../../../classes/joynr/proxy/ProxyAttribute");
var ProxyOperation = require("../../../classes/joynr/proxy/ProxyOperation");
var ProxyEvent = require("../../../classes/joynr/proxy/ProxyEvent");
var MessagingQos = require("../../../classes/joynr/messaging/MessagingQos");
var Request = require("../../../classes/joynr/dispatching/types/Request");
var OneWayRequest = require("../../../classes/joynr/dispatching/types/OneWayRequest");
var TypeRegistrySingleton = require("../../../classes/joynr/types/TypeRegistrySingleton");
var testDataOperation = require("../../../test-classes/test/data/Operation");
var Promise = require("../../../classes/global/Promise");
var TestEnum = require("../../../test-classes/joynr/tests/testTypes/TestEnum");
var RadioStation = require("../../../test-classes/joynr/vehicle/radiotypes/RadioStation");
var waitsFor = require("../../../test-classes/global/WaitsFor");
var DiscoveryEntryWithMetaInfo = require("../../../classes/joynr/types/DiscoveryEntryWithMetaInfo");
var Version = require("../../../classes/joynr/types/Version");
var ProviderQos = require("../../../classes/joynr/types/ProviderQos");

var asyncTimeout = 5000;

function outputPromiseError(error) {
    expect(error.toString()).toBeFalsy();
}

describe("libjoynr-js.joynr.proxy.ProxyOperation", function() {
    var addFavoriteStation;
    var operationName;
    var proxyParticipantId;
    var providerParticipantId;
    var providerDiscoveryEntry;
    var proxy;
    var requestReplyManagerSpy;

    function checkSpy(spy, errorExpected) {
        if (errorExpected) {
            expect(spy.onFulfilled).not.toHaveBeenCalled();
            expect(spy.onRejected).toHaveBeenCalled();
            expect(
                Object.prototype.toString.call(spy.onRejected.calls.mostRecent().args[0]) === "[object Error]"
            ).toBeTruthy();
        } else {
            expect(spy.onFulfilled).toHaveBeenCalled();
            expect(spy.onRejected).not.toHaveBeenCalled();
        }
    }

    beforeEach(function(done) {
        requestReplyManagerSpy = jasmine.createSpyObj("requestReplyManager", ["sendRequest", "sendOneWayRequest"]);
        requestReplyManagerSpy.sendRequest.and.returnValue(
            Promise.resolve({
                result: "resultValue"
            })
        );

        operationName = "myOperation";
        proxyParticipantId = "proxyParticipantId";
        providerParticipantId = "providerParticipantId";
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: providerParticipantId,
            qos: new ProviderQos(),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        proxy = {
            proxyParticipantId: proxyParticipantId,
            providerDiscoveryEntry: providerDiscoveryEntry
        };

        addFavoriteStation = new ProxyOperation(
            proxy,
            {
                dependencies: {
                    requestReplyManager: requestReplyManagerSpy
                }
            },
            "addFavoriteStation",
            [
                {
                    inputParameter: [
                        {
                            name: "radioStation",
                            type: "joynr.vehicle.radiotypes.RadioStation",
                            javascriptType: "joynr.vehicle.radiotypes.RadioStation"
                        }
                    ]
                },
                {
                    inputParameter: [
                        {
                            name: "radioStation",
                            type: "String",
                            javascriptType: "string"
                        }
                    ],
                    outputParameter: []
                }
            ]
        ).buildFunction();

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
        expect(addFavoriteStation).toBeDefined();
        expect(typeof addFavoriteStation === "function").toBeTruthy();
        done();
    });

    it("expect correct error reporting after operation call with wrong argument", function(done) {
        addFavoriteStation({
            nonexistingArgument: "value"
        })
            .then(function(message) {
                fail("unexpectedly returned from addFavoriteStation");
                return null;
            })
            .catch(function(message) {
                //expect(message).toContain(
                //        "Cannot call operation with nullable value");
                expect(message).toMatch("Cannot call operation with nullable value");
                done();
                return null;
            });
    });

    it("expect correct error reporting after operation call with wrong type of argument", function(done) {
        addFavoriteStation({
            radioStation: 1
        })
            .then(function(message) {
                fail("unexpected resolve from addFavoriteStation");
                return null;
            })
            .catch(function(message) {
                //expect(message).toContain(
                //    "Signature does not match");
                expect(message).toMatch("Signature does not match");
                done();
                return null;
            });
    });

    it("expect correct error reporting after operation call with correctly typed but invalid complex argument value", function(
        done
    ) {
        // name should be a string
        var radioStation = new RadioStation({
            name: 1
        });

        addFavoriteStation({
            radioStation: radioStation
        })
            .then(function() {
                fail("unpexected resolve from addFavoriteStation");
                return null;
            })
            .catch(function(message) {
                //expect(message)
                //    .toContain(
                //        "members.name is not of type String. Actual type is Number");
                expect(message).toMatch("members.name is not of type String. Actual type is Number");
                done();
                return null;
            });
    });

    it("expect no error reporting after operation call with correct string argument", function(done) {
        addFavoriteStation({
            radioStation: "correctValue"
        })
            .then(function(result) {
                expect(result).toBeUndefined();
                done();
                return null;
            })
            .catch(function(error) {
                fail("unexpected reject from addFavoriteStation");
                return null;
            });
    });

    var testForCorrectReturnValues = function(methodName, outputParameter, replyResponse, done) {
        var originalArguments = arguments;
        var spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        var proxy = {
            proxyParticipantId: proxyParticipantId,
            providerDiscoveryEntry: providerDiscoveryEntry
        };

        requestReplyManagerSpy.sendRequest.and.returnValue(Promise.resolve(replyResponse));

        var testMethod = new ProxyOperation(
            proxy,
            {
                dependencies: {
                    requestReplyManager: requestReplyManagerSpy
                }
            },
            methodName,
            [
                {
                    inputParameter: [],
                    outputParameter: outputParameter
                }
            ]
        ).buildFunction();

        testMethod()
            .then(spy.onFulfilled)
            .catch(spy.onRejected);

        return waitsFor(
            function() {
                return spy.onFulfilled.calls.count() > 0;
            },
            "The promise is not pending any more",
            asyncTimeout
        )
            .then(function() {
                checkSpy(spy);
                var expectedSpy = expect(spy.onFulfilled);
                /* The following line takes all arguments expect the first 3 and passes them to the toHaveBeenCalledWith function.
                                * This way, it is possible to use the testForCorrectReturnValues function with a
                                */
                expectedSpy.toHaveBeenCalledWith.apply(expectedSpy, Array.prototype.slice.call(originalArguments, 3));
            })
            .catch(function() {
                checkSpy(spy);
            });
    };
    it("expect correct joynr enum object as return value", function(done) {
        /*jslint nomen: true */
        testForCorrectReturnValues(
            "testMethodHavingEnumAsReturnValue",
            [
                {
                    name: "returnEnum",
                    type: TestEnum.ZERO._typeName,
                    javascriptType: TestEnum.ZERO._typeName
                }
            ],
            ["ZERO"],
            {
                returnEnum: TestEnum.ZERO
            }
        )
            .then(function() {
                done();
                return null;
            })
            .catch(function() {
                return null;
            });
        /*jslint nomen: false */
    });

    it("expect undefined as return value for missing output parameters", function(done) {
        testForCorrectReturnValues("testMethodHavingNoOutputParameter", [], [], undefined)
            .then(function() {
                return testForCorrectReturnValues(
                    "testMethodHavingNoOutputParameter",
                    [],
                    ["unexpected value"],
                    undefined
                );
            })
            .then(function() {
                return testForCorrectReturnValues("testMethodWithUndefinedOutputParameter", undefined, [], undefined);
            })
            .then(function() {
                done();
                return null;
            })
            .catch(function() {
                fail("Test failure detected");
                return null;
            });
    });

    it("expect multiple return values", function(done) {
        /*jslint nomen: true */
        testForCorrectReturnValues(
            "testMultipleReturnValues",
            [
                {
                    name: "returnEnum",
                    type: TestEnum.ZERO._typeName,
                    javascriptType: TestEnum.ZERO._typeName
                },
                {
                    name: "returnString",
                    type: "String",
                    javascriptType: "string"
                }
            ],
            ["ZERO", "stringValue"],
            {
                returnEnum: TestEnum.ZERO,
                returnString: "stringValue"
            }
        )
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
        /*jslint nomen: false */
    });

    it("expect correct joynr enum object array as return value", function(done) {
        /*jslint nomen: true */
        testForCorrectReturnValues(
            "testMethodHavingEnumArrayAsReturnValue",
            [
                {
                    name: "returnEnum",
                    // currently, we generate the type of the array element into the signature
                    type: TestEnum.ZERO._typeName + "[]",
                    javascriptType: "Array"
                }
            ],
            [["ZERO", "ONE"]],
            {
                returnEnum: [TestEnum.ZERO, TestEnum.ONE]
            }
        )
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
        /*jslint nomen: false */
    });

    it("expect no error reporting after operation call with correct complex argument", function(done) {
        var radioStation = new RadioStation({
            name: "correctValue",
            byteBuffer: []
        });

        addFavoriteStation({
            radioStation: radioStation
        })
            .then(function(returnValue) {
                expect(returnValue).toEqual(undefined);
                done();
                return null;
            })
            .catch(fail);
    });

    it("notifies", function(done) {
        addFavoriteStation({
            radioStation: "stringStation"
        })
            .then(function(returnValue) {
                done();
                return null;
            })
            .catch(fail);
    });

    function testOperationOverloading(operationArguments, errorExpected) {
        var spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        addFavoriteStation(operationArguments)
            .then(spy.onFulfilled)
            .catch(spy.onRejected)
            .catch(outputPromiseError);

        return waitsFor(
            function() {
                return errorExpected ? spy.onRejected.calls.count() > 0 : spy.onFulfilled.calls.count() > 0;
            },
            "The promise is not pending any more",
            asyncTimeout
        )
            .then(function() {
                checkSpy(spy, errorExpected);
                return null;
            })
            .catch(function() {
                checkSpy(spy, errorExpected);
                return null;
            });
    }

    it("provides overloading operations", function(done) {
        // correct version one
        testOperationOverloading({
            radioStation: "stringStation"
        })
            .then(function() {
                return testOperationOverloading({
                    radioStation: new RadioStation({
                        name: "typedStation",
                        byteBuffer: []
                    })
                }); // correct version two
            })
            .then(function() {
                return testOperationOverloading(
                    {
                        wrongName: "stringStation"
                    },
                    true
                ); // wrong argument name
            })
            .then(function() {
                return testOperationOverloading({}, true); // wrong number of arguments
            })
            .then(function() {
                return testOperationOverloading(
                    {
                        radioStation: []
                    },
                    true
                ); // wrong number argument type (Array instead of String|RadioStation)
            })
            .then(function() {
                return testOperationOverloading(
                    {
                        radioStation: 1
                    },
                    true
                ); // wrong number argument type (Number instead of String|RadioStation)
            })
            .then(function() {
                return testOperationOverloading(
                    {
                        radioStation: "stringStation",
                        anotherArgument: 1
                    },
                    true
                ); // wrong additional argument
            })
            .then(function() {
                return testOperationOverloading(
                    {
                        radioStation: new RadioStation({
                            name: "stringStation",
                            byteBuffer: []
                        }),
                        anotherArgument: 2
                    },
                    true
                ); // wrong additional arguments
            })
            .then(function() {
                return testOperationOverloading(
                    {
                        radioStation: null
                    },
                    true
                ); // nullable argument
            })
            .then(function() {
                return testOperationOverloading(
                    {
                        radioStation: undefined
                    },
                    true
                ); // nullable argument
            })
            .then(function() {
                return testOperationOverloading(undefined, true); // nullable settings object
            })
            .then(function() {
                return testOperationOverloading(null, true); // nullable settings object
            })
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    it("does not throw when giving wrong or nullable operation arguments", function(done) {
        var spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        expect(function() {
            addFavoriteStation({
                radioStation: "myRadioStation"
            })
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(function() {
            addFavoriteStation({
                radioStation: undefined
            })
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(function() {
            addFavoriteStation({})
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(function() {
            addFavoriteStation(undefined)
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(function() {
            addFavoriteStation(null)
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        done();
    });

    function checkRequestReplyManagerCall(testData) {
        // construct new ProxyOperation
        var myOperation = new ProxyOperation(
            proxy,
            {
                dependencies: {
                    requestReplyManager: requestReplyManagerSpy
                }
            },
            operationName,
            [testData.signature]
        ).buildFunction();
        requestReplyManagerSpy.sendRequest.and.returnValue(Promise.resolve(testData.returnParams));
        requestReplyManagerSpy.sendRequest.calls.reset();

        // do operation call
        myOperation(testData.namedArguments).catch(outputPromiseError);

        return waitsFor(
            function() {
                return requestReplyManagerSpy.sendRequest.calls.count() > 0;
            },
            "requestReplyManagerSpy.sendRequest call",
            100
        )
            .then(function() {
                // check if requestReplyManager has been called correctly
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();

                var requestReplyId = requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith({
                    toDiscoveryEntry: providerDiscoveryEntry,
                    from: proxyParticipantId,
                    messagingQos: new MessagingQos(),
                    request: new Request({
                        methodName: operationName,
                        paramDatatypes: testData.paramDatatypes,
                        params: testData.params,
                        requestReplyId: requestReplyId
                    })
                });
            })
            .catch(function() {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
            });
    }

    function checkRequestReplyManagerFireAndForgetCall(testData) {
        var myOperation = new ProxyOperation(
            proxy,
            {
                dependencies: {
                    requestReplyManager: requestReplyManagerSpy
                }
            },
            operationName,
            [testData.signature]
        ).buildFunction();
        requestReplyManagerSpy.sendOneWayRequest.and.returnValue(Promise.resolve());
        requestReplyManagerSpy.sendOneWayRequest.calls.reset();

        // do operation call
        myOperation(testData.namedArguments).catch(outputPromiseError);

        return waitsFor(
            function() {
                return requestReplyManagerSpy.sendOneWayRequest.calls.count() > 0;
            },
            "requestReplyManagerSpy.sendOneWayRequest call",
            100
        )
            .then(function() {
                // check if requestReplyManager has been called correctly
                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalled();
                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalledWith({
                    toDiscoveryEntry: providerDiscoveryEntry,
                    from: proxyParticipantId,
                    messagingQos: new MessagingQos(),
                    request: new OneWayRequest({
                        methodName: operationName,
                        paramDatatypes: testData.paramDatatypes,
                        params: testData.params
                    })
                });
            })
            .catch(function() {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
            });
    }

    it("calls RequestReplyManager with correct request", function(done) {
        var i;
        var requestReplyManagerSpy = jasmine.createSpyObj("requestReplyManager", ["sendRequest"]);

        function makeFunc(promiseChain, testOp) {
            if (testOp.signature.fireAndForget) {
                return promiseChain.then(function() {
                    return checkRequestReplyManagerFireAndForgetCall(testOp);
                });
            }
            return promiseChain.then(function() {
                return checkRequestReplyManagerCall(testOp);
            });
        }

        var promiseChain;
        if (testDataOperation[0].signature.fireAndForget) {
            promiseChain = checkRequestReplyManagerFireAndForgetCall(testDataOperation[0]);
        } else {
            promiseChain = checkRequestReplyManagerCall(testDataOperation[0]);
        }

        for (i = 1; i < testDataOperation.length; ++i) {
            promiseChain = makeFunc(promiseChain, testDataOperation[i]);
        }
        promiseChain
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });
});
