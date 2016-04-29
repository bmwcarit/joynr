/*global joynrTestRequire: true*/
/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
        "joynr/proxy/TestProxyOperation",
        [
            "joynr/proxy/ProxyAttribute",
            "joynr/proxy/ProxyOperation",
            "joynr/proxy/ProxyEvent",
            "joynr/messaging/MessagingQos",
            "joynr/dispatching/types/Request",
            "joynr/dispatching/types/OneWayRequest",
            "test/data/Operation",
            "global/Promise",
            "joynr/tests/testTypes/TestEnum",
            "joynr/vehicle/radiotypes/RadioStation"
        ],
        function(
                ProxyAttribute,
                ProxyOperation,
                ProxyEvent,
                MessagingQos,
                Request,
                OneWayRequest,
                testDataOperation,
                Promise,
                TestEnum,
                RadioStation) {

            var asyncTimeout = 5000;

            function outputPromiseError(error) {
                expect(error.toString()).toBeFalsy();
            }

            describe(
                    "libjoynr-js.joynr.proxy.ProxyOperation",
                    function() {

                        var addFavoriteStation;
                        var operationName;
                        var proxyParticipantId;
                        var providerParticipantId;
                        var proxy;
                        var requestReplyManagerSpy;

                        function checkSpy(spy, errorExpected) {
                            if (errorExpected) {
                                expect(spy.onFulfilled).not.toHaveBeenCalled();
                                expect(spy.onRejected).toHaveBeenCalled();
                                expect(
                                        Object.prototype.toString
                                                .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                        .toBeTruthy();
                            } else {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onRejected).not.toHaveBeenCalled();
                            }
                        }

                        beforeEach(function() {
                            requestReplyManagerSpy =
                                    jasmine.createSpyObj("requestReplyManager", [ "sendRequest", "sendOneWayRequest"
                                    ]);
                            requestReplyManagerSpy.sendRequest.andReturn(Promise.resolve({
                                result : "resultValue"
                            }));

                            operationName = "myOperation";
                            proxyParticipantId = "proxyParticipantId";
                            providerParticipantId = "providerParticipantId";
                            proxy = {
                                proxyParticipantId : proxyParticipantId,
                                providerParticipantId : providerParticipantId
                            };

                            addFavoriteStation = new ProxyOperation(proxy, {
                                dependencies : {
                                    requestReplyManager : requestReplyManagerSpy
                                }

                            }, "addFavoriteStation", [
                                {
                                    inputParameter : [ {
                                        name : "radioStation",
                                        type : 'joynr.vehicle.radiotypes.RadioStation'
                                    }
                                    ]
                                },
                                {
                                    inputParameter : [ {
                                        name : "radioStation",
                                        type : 'String'
                                    }
                                    ],
                                    outputParameter : []
                                }
                            ]).buildFunction();

                        });

                        it("is of correct type", function() {
                            expect(addFavoriteStation).toBeDefined();
                            expect(typeof addFavoriteStation === "function").toBeTruthy();
                        });

                        it(
                                "expect correct error reporting after operation call with wrong argument",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    runs(function() {
                                        addFavoriteStation({
                                            "nonexistingArgument" : "value"
                                        }).then(spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "The promise is not pending any more", asyncTimeout);

                                    runs(function() {
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(spy.onRejected.calls[0].args[0].message).toContain(
                                                "Cannot call operation with nullable value");
                                    });
                                });

                        it(
                                "expect correct error reporting after operation call with wrong type of argument",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    runs(function() {
                                        addFavoriteStation({
                                            "radioStation" : 1
                                        }).then(spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "The promise is not pending any more", asyncTimeout);

                                    runs(function() {
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(spy.onRejected.calls[0].args[0].message).toContain(
                                                "Signature does not match");
                                    });
                                });

                        it(
                                "expect correct error reporting after operation call with correctly typed but invalid complex argument value",
                                function() {
                                    // name should be a string
                                    var radioStation = new RadioStation({
                                        name : 1
                                    });
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    runs(function() {
                                        addFavoriteStation({
                                            "radioStation" : radioStation
                                        }).then(spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "The promise is not pending any more", asyncTimeout);

                                    runs(function() {
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(spy.onRejected.calls[0].args[0].message)
                                                .toContain(
                                                        "members.name is not of type String. Actual type is Number");
                                    });
                                });

                        it(
                                "expect no error reporting after operation call with correct string argument",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    runs(function() {
                                        addFavoriteStation({
                                            "radioStation" : "correctValue"
                                        }).then(spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0;
                                    }, "The promise is not pending any more", asyncTimeout);

                                    runs(function() {
                                        checkSpy(spy);
                                        expect(spy.onFulfilled).toHaveBeenCalledWith(undefined);
                                    });
                                });

                        var testForCorrectReturnValues = function(methodName, outputParameter, replyResponse) {
                            var originalArguments = arguments;
                            var spy = jasmine.createSpyObj("spy", [
                               "onFulfilled",
                               "onRejected"
                           ]);
                           var proxy = {
                               proxyParticipantId : proxyParticipantId,
                               providerParticipantId : providerParticipantId
                           };

                           requestReplyManagerSpy.sendRequest.andReturn(Promise.resolve(replyResponse));

                           var testMethod = new ProxyOperation(proxy, {
                               dependencies : {
                                   requestReplyManager : requestReplyManagerSpy
                               }

                           }, methodName, [
                               {
                                   inputParameter : [],
                                   outputParameter : outputParameter
                               }
                           ]).buildFunction();

                           runs(function() {
                               testMethod().then(spy.onFulfilled).catch(spy.onRejected);
                           });

                           waitsFor(function() {
                               return spy.onFulfilled.callCount > 0;
                           }, "The promise is not pending any more", asyncTimeout);

                           runs(function() {
                               checkSpy(spy);
                               var expectedSpy = expect(spy.onFulfilled);
                               /* The following line takes all arguments expect the first 3 and passes them to the toHaveBeenCalledWith function.
                                * This way, it is possible to use the testForCorrectReturnValues function with a
                                */
                               expectedSpy.toHaveBeenCalledWith.apply(expectedSpy, (Array.prototype.slice.call(originalArguments, 3)));
                           });
                        };
                        it(
                                "expect correct joynr enum object as return value",
                                function() {
                                    /*jslint nomen: true */
                                    testForCorrectReturnValues("testMethodHavingEnumAsReturnValue",
                                                                [ {
                                                                    name : "returnEnum",
                                                                    type : TestEnum.ZERO._typeName
                                                                }],
                                                                ["ZERO"],
                                                                {
                                                                   returnEnum: TestEnum.ZERO
                                                                });
                                    /*jslint nomen: false */
                                });

                        it(
                                "expect undefined as return value for missing output parameters",
                                function() {
                                    testForCorrectReturnValues("testMethodHavingNoOutputParameter",
                                                                [],
                                                                [],
                                                                undefined);
                                    testForCorrectReturnValues("testMethodHavingNoOutputParameter",
                                            [],
                                            ["unexpected value"],
                                            undefined);
                                    testForCorrectReturnValues("testMethodWithUndefinedOutputParameter",
                                            undefined,
                                            [],
                                            undefined);

                                });

                        it(
                                "expect multiple return values",
                                function() {
                                    /*jslint nomen: true */
                                    testForCorrectReturnValues("testMultipleReturnValues",
                                                                [ {
                                                                    name : "returnEnum",
                                                                    type : TestEnum.ZERO._typeName
                                                                }, {
                                                                    name : "returnString",
                                                                    type : String
                                                                }],
                                                                ["ZERO", "stringValue"],
                                                                {
                                                                    returnEnum : TestEnum.ZERO,
                                                                    returnString : "stringValue"
                                                                });
                                    /*jslint nomen: false */
                                });

                        it(
                                "expect correct joynr enum object array as return value",
                                function() {
                                    /*jslint nomen: true */
                                    testForCorrectReturnValues("testMethodHavingEnumArrayAsReturnValue",
                                                                [ {
                                                                    name : "returnEnum",
                                                                    // currently, we generate the type of the array element into the signature
                                                                    type : TestEnum.ZERO._typeName
                                                                }],
                                                                [["ZERO", "ONE"]],
                                                                {
                                                                    returnEnum : [TestEnum.ZERO, TestEnum.ONE]
                                                                });
                                    /*jslint nomen: false */
                                });

                        it(
                                "expect no error reporting after operation call with correct complex argument",
                                function() {
                                    var radioStation = new RadioStation({
                                        name : "correctValue"
                                    });
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    runs(function() {
                                        addFavoriteStation({
                                            "radioStation" : radioStation
                                        }).then(spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0;
                                    }, "The promise is not pending any more", asyncTimeout);

                                    runs(function() {
                                        checkSpy(spy);
                                        expect(spy.onFulfilled).toHaveBeenCalledWith(undefined);
                                    });
                                });

                        it("notifies", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                addFavoriteStation({
                                    "radioStation" : "stringStation"
                                }).then(spy.onFulfilled).catch(spy.onRejected).catch(outputPromiseError);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "The promise is not pending any more", asyncTimeout);

                            runs(function() {
                                checkSpy(spy);
                            });
                        });

                        function testOperationOverloading(operationArguments, errorExpected) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                addFavoriteStation(operationArguments).then(
                                        spy.onFulfilled).catch(spy.onRejected).catch(outputPromiseError);
                            });

                            waitsFor(function() {
                                return errorExpected
                                        ? spy.onRejected.callCount > 0
                                        : spy.onFulfilled.callCount > 0;
                            }, "The promise is not pending any more", asyncTimeout);

                            runs(function() {
                                checkSpy(spy, errorExpected);
                            });
                        }

                        it("provides overloading operations", function() {
                            testOperationOverloading({
                                radioStation : "stringStation"
                            }); // correct version one

                            testOperationOverloading({
                                radioStation : new RadioStation({
                                    name : "typedStation"
                                })
                            }); // correct version two

                            testOperationOverloading({
                                wrongName : "stringStation"
                            }, true); // wrong argument name

                            testOperationOverloading({}, true); // wrong number of arguments

                            testOperationOverloading({
                                radioStation : []
                            }, true); // wrong number argument type (Array instead of String|RadioStation)

                            testOperationOverloading({
                                radioStation : 1
                            }, true); // wrong number argument type (Number instead of String|RadioStation)

                            testOperationOverloading({
                                radioStation : "stringStation",
                                anotherArgument : 1
                            }, true); // wrong additional argument

                            testOperationOverloading({
                                radioStation : new RadioStation({
                                    name : "stringStation"
                                }),
                                anotherArgument : 2
                            }, true); // wrong additional arguments

                            testOperationOverloading({
                                radioStation : null
                            }, true); // nullable argument

                            testOperationOverloading({
                                radioStation : undefined
                            }, true); // nullable argument

                            testOperationOverloading(undefined, true); // nullable settings object
                            testOperationOverloading(null, true); // nullable settings object
                        });

                        it(
                                "does not throw when giving wrong or nullable operation arguments",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    expect(function() {
                                        addFavoriteStation({
                                            radioStation : "myRadioStation"
                                        }).then(spy.onFulfilled).catch(spy.onRejected);
                                    }).not.toThrow();

                                    expect(function() {
                                        addFavoriteStation({
                                            radioStation : undefined
                                        }).then(spy.onFulfilled).catch(spy.onRejected);
                                    }).not.toThrow();

                                    expect(function() {
                                        addFavoriteStation({}).then(spy.onFulfilled).catch(spy.onRejected);
                                    }).not.toThrow();

                                    expect(function() {
                                        addFavoriteStation(undefined).then(spy.onFulfilled).catch(spy.onRejected);
                                    }).not.toThrow();

                                    expect(function() {
                                        addFavoriteStation(null).then(spy.onFulfilled).catch(spy.onRejected);
                                    }).not.toThrow();
                                });

                        function checkRequestReplyManagerCall(testData) {
                            runs(function() {
                                // construct new ProxyOperation
                                var myOperation = new ProxyOperation(proxy, {
                                    dependencies : {
                                        requestReplyManager : requestReplyManagerSpy
                                    }

                                }, operationName, [ testData.signature
                                ]).buildFunction();
                                requestReplyManagerSpy.sendRequest.andReturn(Promise.resolve(testData.returnParams));
                                requestReplyManagerSpy.sendRequest.reset();

                                // do operation call
                                myOperation(testData.namedArguments).catch(outputPromiseError);
                            });

                            waitsFor(function() {
                                return requestReplyManagerSpy.sendRequest.callCount > 0;
                            }, "requestReplyManagerSpy.sendRequest call", 100);

                            runs(function() {

                                // check if requestReplyManager has been called correctly
                                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();

                                var requestReplyId =
                                        requestReplyManagerSpy.sendRequest.calls[0].args[0].request.requestReplyId;
                                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith({
                                    to : providerParticipantId,
                                    from : proxyParticipantId,
                                    messagingQos : new MessagingQos(),
                                    request : new Request({
                                        methodName : operationName,
                                        paramDatatypes : testData.paramDatatypes,
                                        params : testData.params,
                                        requestReplyId : requestReplyId
                                    })
                                });
                            });
                        }

                        function checkRequestReplyManagerFireAndForgetCall(testData) {
                            runs(function() {
                                // construct new ProxyOperation
                                var myOperation = new ProxyOperation(proxy, {
                                    dependencies : {
                                        requestReplyManager : requestReplyManagerSpy
                                    }

                                }, operationName, [ testData.signature
                                ]).buildFunction();
                                requestReplyManagerSpy.sendOneWayRequest.andReturn(Promise.resolve());
                                requestReplyManagerSpy.sendOneWayRequest.reset();

                                // do operation call
                                myOperation(testData.namedArguments).catch(outputPromiseError);
                            });

                            waitsFor(function() {
                                return requestReplyManagerSpy.sendOneWayRequest.callCount > 0;
                            }, "requestReplyManagerSpy.sendOneWayRequest call", 100);

                            runs(function() {
                                // check if requestReplyManager has been called correctly
                                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalled();
                                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalledWith({
                                    to : providerParticipantId,
                                    from : proxyParticipantId,
                                    messagingQos : new MessagingQos(),
                                    request : new OneWayRequest({
                                        methodName : operationName,
                                        paramDatatypes : testData.paramDatatypes,
                                        params : testData.params
                                    })
                                });
                            });
                        }

                        it("calls RequestReplyManager with correct request", function() {
                            var i;
                            var requestReplyManagerSpy =
                                    jasmine.createSpyObj("requestReplyManager", [ "sendRequest"
                                    ]);

                            for (i = 0; i < testDataOperation.length; ++i) {
                                if (testDataOperation[i].signature.fireAndForget) {
                                    checkRequestReplyManagerFireAndForgetCall(testDataOperation[i]);
                                } else {
                                    checkRequestReplyManagerCall(testDataOperation[i]);
                                }
                            }
                        });
                    });

        }); // require
