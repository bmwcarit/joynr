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
const ProxyOperation = require("../../../../main/js/joynr/proxy/ProxyOperation");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const Request = require("../../../../main/js/joynr/dispatching/types/Request");
const OneWayRequest = require("../../../../main/js/joynr/dispatching/types/OneWayRequest");
const TypeRegistrySingleton = require("../../../../main/js/joynr/types/TypeRegistrySingleton");
const testDataOperation = require("../../../../test/js/test/data/Operation");
const TestEnum = require("../../../generated/joynr/tests/testTypes/TestEnum");
const RadioStation = require("../../../generated/joynr/vehicle/radiotypes/RadioStation");
const waitsFor = require("../../../../test/js/global/WaitsFor");
const DiscoveryEntryWithMetaInfo = require("../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");

const asyncTimeout = 5000;

function outputPromiseError(error) {
    expect(error.toString()).toBeFalsy();
}

describe("libjoynr-js.joynr.proxy.ProxyOperation", () => {
    let addFavoriteStation;
    let operationName;
    let proxyParticipantId;
    let providerParticipantId;
    let providerDiscoveryEntry;
    let proxy;
    let requestReplyManagerSpy;

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

    beforeEach(done => {
        requestReplyManagerSpy = jasmine.createSpyObj("requestReplyManager", ["sendRequest", "sendOneWayRequest"]);
        requestReplyManagerSpy.sendRequest.and.callFake((settings, callbackSettings) => {
            const response = { result: "resultValue" };

            return Promise.resolve({
                response,
                settings: callbackSettings
            });
        });

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
            proxyParticipantId,
            providerDiscoveryEntry
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
                            type: "joynr.vehicle.radiotypes.RadioStation"
                        }
                    ]
                },
                {
                    inputParameter: [
                        {
                            name: "radioStation",
                            type: "String"
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
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("is of correct type", done => {
        expect(addFavoriteStation).toBeDefined();
        expect(typeof addFavoriteStation === "function").toBeTruthy();
        done();
    });

    it("expect correct error reporting after operation call with wrong argument", done => {
        addFavoriteStation({
            nonexistingArgument: "value"
        })
            .then(fail)
            .catch(message => {
                //expect(message).toContain(
                //        "Cannot call operation with nullable value");
                expect(message).toMatch("Cannot call operation with nullable value");
                done();
                return null;
            });
    });

    it("expect correct error reporting after operation call with wrong type of argument", done => {
        addFavoriteStation({
            radioStation: 1
        })
            .then(fail)
            .catch(message => {
                //expect(message).toContain(
                //    "Signature does not match");
                expect(message).toMatch("Signature does not match");
                done();
                return null;
            });
    });

    it("expect correct error reporting after operation call with correctly typed but invalid complex argument value", done => {
        // name should be a string
        const radioStation = new RadioStation({
            name: 1
        });

        addFavoriteStation({
            radioStation
        })
            .then(fail)
            .catch(message => {
                //expect(message)
                //    .toContain(
                //        "members.name is not of type String. Actual type is Number");
                expect(message).toMatch("members.name is not of type String. Actual type is Number");
                done();
                return null;
            });
    });

    it("expect no error reporting after operation call with correct string argument", done => {
        addFavoriteStation({
            radioStation: "correctValue"
        })
            .then(result => {
                expect(result).toBeUndefined();
                done();
            })
            .catch(fail);
    });

    const testForCorrectReturnValues = function(methodName, outputParameter, replyResponse) {
        const originalArguments = arguments;
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        const proxy = {
            proxyParticipantId,
            providerDiscoveryEntry
        };

        requestReplyManagerSpy.sendRequest.and.callFake((settings, callbackSettings) => {
            return Promise.resolve({
                response: replyResponse,
                settings: callbackSettings
            });
        });

        const testMethod = new ProxyOperation(
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
                    outputParameter
                }
            ]
        ).buildFunction();

        testMethod()
            .then(spy.onFulfilled)
            .catch(spy.onRejected);

        return waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "The promise is not pending any more",
            asyncTimeout
        )
            .then(() => {
                checkSpy(spy);
                const expectedSpy = expect(spy.onFulfilled);
                /* The following line takes all arguments expect the first 3 and passes them to the toHaveBeenCalledWith function.
                 * This way, it is possible to use the testForCorrectReturnValues function with a
                 */
                expectedSpy.toHaveBeenCalledWith(...Array.prototype.slice.call(originalArguments, 3));
            })
            .catch(() => {
                checkSpy(spy);
            });
    };
    it("expect correct joynr enum object as return value", done => {
        testForCorrectReturnValues(
            "testMethodHavingEnumAsReturnValue",
            [
                {
                    name: "returnEnum",
                    type: TestEnum.ZERO._typeName
                }
            ],
            ["ZERO"],
            {
                returnEnum: TestEnum.ZERO
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(() => {
                return null;
            });
    });

    it("expect undefined as return value for missing output parameters", done => {
        testForCorrectReturnValues("testMethodHavingNoOutputParameter", [], [], undefined)
            .then(() => {
                return testForCorrectReturnValues(
                    "testMethodHavingNoOutputParameter",
                    [],
                    ["unexpected value"],
                    undefined
                );
            })
            .then(() => {
                return testForCorrectReturnValues("testMethodWithUndefinedOutputParameter", undefined, [], undefined);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(() => {
                fail("Test failure detected");
                return null;
            });
    });

    it("expect multiple return values", done => {
        testForCorrectReturnValues(
            "testMultipleReturnValues",
            [
                {
                    name: "returnEnum",
                    type: TestEnum.ZERO._typeName
                },
                {
                    name: "returnString",
                    type: "String"
                }
            ],
            ["ZERO", "stringValue"],
            {
                returnEnum: TestEnum.ZERO,
                returnString: "stringValue"
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("expect correct joynr enum object array as return value", done => {
        testForCorrectReturnValues(
            "testMethodHavingEnumArrayAsReturnValue",
            [
                {
                    name: "returnEnum",
                    // currently, we generate the type of the array element into the signature
                    type: `${TestEnum.ZERO._typeName}[]`
                }
            ],
            [["ZERO", "ONE"]],
            {
                returnEnum: [TestEnum.ZERO, TestEnum.ONE]
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("expect no error reporting after operation call with correct complex argument", done => {
        const radioStation = new RadioStation({
            name: "correctValue",
            byteBuffer: []
        });

        addFavoriteStation({
            radioStation
        })
            .then(returnValue => {
                expect(returnValue).toEqual(undefined);
                done();
                return null;
            })
            .catch(fail);
    });

    it("notifies", done => {
        addFavoriteStation({
            radioStation: "stringStation"
        })
            .then(done)
            .catch(fail);
    });

    function testOperationOverloading(operationArguments, errorExpected) {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        addFavoriteStation(operationArguments)
            .then(spy.onFulfilled)
            .catch(spy.onRejected)
            .catch(outputPromiseError);

        return waitsFor(
            () => {
                return errorExpected ? spy.onRejected.calls.count() > 0 : spy.onFulfilled.calls.count() > 0;
            },
            "The promise is not pending any more",
            asyncTimeout
        )
            .then(() => {
                checkSpy(spy, errorExpected);
                return null;
            })
            .catch(() => {
                checkSpy(spy, errorExpected);
                return null;
            });
    }

    it("provides overloading operations", done => {
        // correct version one
        testOperationOverloading({
            radioStation: "stringStation"
        })
            .then(() => {
                return testOperationOverloading({
                    radioStation: new RadioStation({
                        name: "typedStation",
                        byteBuffer: []
                    })
                }); // correct version two
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        wrongName: "stringStation"
                    },
                    true
                ); // wrong argument name
            })
            .then(() => {
                return testOperationOverloading({}, true); // wrong number of arguments
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: []
                    },
                    true
                ); // wrong number argument type (Array instead of String|RadioStation)
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: 1
                    },
                    true
                ); // wrong number argument type (Number instead of String|RadioStation)
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: "stringStation",
                        anotherArgument: 1
                    },
                    true
                ); // wrong additional argument
            })
            .then(() => {
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
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: null
                    },
                    true
                ); // nullable argument
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: undefined
                    },
                    true
                ); // nullable argument
            })
            .then(() => {
                return testOperationOverloading(undefined, true); // nullable settings object
            })
            .then(() => {
                return testOperationOverloading(null, true); // nullable settings object
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("does not throw when giving wrong or nullable operation arguments", done => {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        expect(() => {
            addFavoriteStation({
                radioStation: "myRadioStation"
            })
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation({
                radioStation: undefined
            })
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation({})
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation(undefined)
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation(null)
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        done();
    });

    function checkRequestReplyManagerCall(testData) {
        // construct new ProxyOperation
        const myOperation = new ProxyOperation(
            proxy,
            {
                dependencies: {
                    requestReplyManager: requestReplyManagerSpy
                }
            },
            operationName,
            [testData.signature]
        ).buildFunction();

        requestReplyManagerSpy.sendRequest.and.callFake((settings, callbackSettings) => {
            return Promise.resolve({
                response: testData.returnParams,
                settings: callbackSettings
            });
        });
        requestReplyManagerSpy.sendRequest.calls.reset();

        // do operation call
        myOperation(testData.namedArguments).catch(outputPromiseError);

        return waitsFor(
            () => {
                return requestReplyManagerSpy.sendRequest.calls.count() > 0;
            },
            "requestReplyManagerSpy.sendRequest call",
            100
        )
            .then(() => {
                // check if requestReplyManager has been called correctly
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();

                const requestReplyId = requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith(
                    {
                        toDiscoveryEntry: providerDiscoveryEntry,
                        from: proxyParticipantId,
                        messagingQos: new MessagingQos(),
                        request: Request.create({
                            methodName: operationName,
                            paramDatatypes: testData.paramDatatypes,
                            params: testData.params,
                            requestReplyId
                        })
                    },
                    jasmine.any(Object)
                );
            })
            .catch(() => {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
            });
    }

    function checkRequestReplyManagerFireAndForgetCall(testData) {
        const myOperation = new ProxyOperation(
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
            () => {
                return requestReplyManagerSpy.sendOneWayRequest.calls.count() > 0;
            },
            "requestReplyManagerSpy.sendOneWayRequest call",
            100
        )
            .then(() => {
                // check if requestReplyManager has been called correctly
                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalled();
                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalledWith({
                    toDiscoveryEntry: providerDiscoveryEntry,
                    from: proxyParticipantId,
                    messagingQos: new MessagingQos(),
                    request: OneWayRequest.create({
                        methodName: operationName,
                        paramDatatypes: testData.paramDatatypes,
                        params: testData.params
                    })
                });
            })
            .catch(() => {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
            });
    }

    it("calls RequestReplyManager with correct request", done => {
        let i;

        function makeFunc(promiseChain, testOp) {
            if (testOp.signature.fireAndForget) {
                return promiseChain.then(() => {
                    return checkRequestReplyManagerFireAndForgetCall(testOp);
                });
            }
            return promiseChain.then(() => {
                return checkRequestReplyManagerCall(testOp);
            });
        }

        let promiseChain;
        if (testDataOperation[0].signature.fireAndForget) {
            promiseChain = checkRequestReplyManagerFireAndForgetCall(testDataOperation[0]);
        } else {
            promiseChain = checkRequestReplyManagerCall(testDataOperation[0]);
        }

        for (i = 1; i < testDataOperation.length; ++i) {
            promiseChain = makeFunc(promiseChain, testDataOperation[i]);
        }
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });
});
