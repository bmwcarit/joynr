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
        "joynr/dispatching/TestRequestReplyManager",
        [
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/types/Request",
            "joynr/dispatching/types/Reply",
            "joynr/types/TypeRegistrySingleton",
            "joynr/util/Typing"
        ],
        function(RequestReplyManager, Request, Reply, TypeRegistrySingleton, Typing) {

            describe(
                    "libjoynr-js.joynr.dispatching.RequestReplyManager",
                    function() {

                        var requestReplyManager;
                        var typeRegistry;
                        var ttl_ms = 50;
                        var requestReplyId = "requestReplyId";
                        var testResponse = [ "testResponse"
                        ];
                        var reply = new Reply({
                            requestReplyId : requestReplyId,
                            response : testResponse
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
                                configurable : false,
                                writable : false,
                                enumerable : false,
                                value : jasmine.createSpy("checkMembers")
                            });

                            Object.defineProperty(this, "_typeName", {
                                configurable : false,
                                writable : false,
                                enumerable : true,
                                value : "test.RadioStation"
                            });
                        }

                        var Country = {
                            AUSTRALIA : "AUSTRALIA",
                            AUSTRIA : "AUSTRIA",
                            CANADA : "CANADA",
                            GERMANY : "GERMANY",
                            ITALY : "ITALY",
                            UNITED_KINGDOM : "UNITED_KINGDOM"
                        };

                        function ComplexTypeWithComplexAndSimpleProperties(
                                radioStation,
                                myBoolean,
                                myString) {
                            if (!(this instanceof ComplexTypeWithComplexAndSimpleProperties)) {
                                // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
                                return new ComplexTypeWithComplexAndSimpleProperties(
                                        radioStation,
                                        myBoolean,
                                        myString);
                            }
                            this.radioStation = radioStation;
                            this.myBoolean = myBoolean;
                            this.myString = myString;

                            Object.defineProperty(this, "checkMembers", {
                                configurable : false,
                                writable : false,
                                enumerable : false,
                                value : jasmine.createSpy("checkMembers")
                            });

                            Object.defineProperty(this, "_typeName", {
                                configurable : false,
                                writable : false,
                                enumerable : true,
                                value : "test.ComplexTypeWithComplexAndSimpleProperties"
                            });
                        }

                        /**
                         * Called before each test.
                         */
                        beforeEach(function() {
                            typeRegistry = TypeRegistrySingleton.getInstance();
                            typeRegistry.addType("test.RadioStation", RadioStation);
                            typeRegistry.addType(
                                    "test.ComplexTypeWithComplexAndSimpleProperties",
                                    ComplexTypeWithComplexAndSimpleProperties);
                            requestReplyManager = new RequestReplyManager({}, typeRegistry);
                        });

                        it("is instantiable", function() {
                            expect(requestReplyManager).toBeDefined();
                        });

                        var tripleJ = new RadioStation("TripleJ", "107.7", "AUSTRALIA");
                        var fm4 = new RadioStation("FM4", "104.0", "AUSTRIA");
                        var complex =
                                new ComplexTypeWithComplexAndSimpleProperties(
                                        tripleJ,
                                        true,
                                        "hello");
                        var testData =
                                [
                                    {
                                        paramDatatype : [ "Boolean"
                                        ],
                                        params : [ true
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "Integer"
                                        ],
                                        params : [ 123456789
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "Double"
                                        ],
                                        params : [ -123.456789
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "String"
                                        ],
                                        params : [ "lalala"
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "Integer[]"
                                        ],
                                        params : [ [
                                            1,
                                            2,
                                            3,
                                            4,
                                            5
                                        ]
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "joynr.vehicle.radiotypes.RadioStation[]"
                                        ],
                                        params : [ [
                                            fm4,
                                            tripleJ
                                        ]
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "joynr.vehicle.radiotypes.RadioStation"
                                        ],
                                        params : [ tripleJ
                                        ]
                                    },
                                    {
                                        paramDatatype : [
                                            "joynr.vehicle.radiotypes.RadioStation",
                                            "String"
                                        ],
                                        params : [
                                            tripleJ,
                                            "testParam"
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "vehicle.ComplexTypeWithComplexAndSimpleProperties"
                                        ],
                                        params : [ complex
                                        ]
                                    }
                                ];

                        function testHandleRequestWithExpectedType(paramDatatypes, params) {
                            var providerParticipantId = "providerParticipantId";
                            var provider = {
                                testFunction : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                }
                            };

                            provider.testFunction.callOperation.andReturn([]);

                            var request = new Request({
                                methodName : "testFunction",
                                paramDatatypes : paramDatatypes,
                                // untype objects through serialization and deserialization
                                params : JSON.parse(JSON.stringify(params))
                            });

                            runs(function() {
                                requestReplyManager.addRequestCaller(
                                        providerParticipantId,
                                        provider);
                                requestReplyManager.handleRequest(
                                        providerParticipantId,
                                        request,
                                        jasmine.createSpy);
                            });

                            waitsFor(function() {
                                return provider.testFunction.callOperation.calls.length > 0;
                            }, "callOperation to be called", 100);

                            runs(function() {
                                expect(provider.testFunction.callOperation).toHaveBeenCalled();
                                expect(provider.testFunction.callOperation).toHaveBeenCalledWith(
                                        params,
                                        paramDatatypes);

                                var result = provider.testFunction.callOperation.calls[0].args[0];
                                expect(result).toEqual(params);
                                expect(Typing.getObjectType(result)).toEqual(
                                        Typing.getObjectType(params));
                            });
                        }

                        it(
                                "calls registered requestCaller with correctly typed object",
                                function() {
                                    spyOn(Typing, 'augmentTypes').andCallThrough();

                                    var i, test;
                                    for (i = 0; i < testData.length; ++i) {
                                        test = testData[i];
                                        testHandleRequestWithExpectedType(
                                                test.paramDatatype,
                                                test.params);
                                    }
                                });

                        it("calls registered replyCaller when a reply arrives", function() {
                            var replyCallerSpy = jasmine.createSpyObj("promise", [
                                "resolve",
                                "reject"
                            ]);

                            runs(function() {
                                requestReplyManager.addReplyCaller(
                                        requestReplyId,
                                        replyCallerSpy,
                                        ttl_ms);
                                requestReplyManager.handleReply(reply);
                            });

                            waitsFor(function() {
                                return replyCallerSpy.resolve.calls.length > 0
                                    || replyCallerSpy.reject.calls.length > 0;
                            }, "reject or fulfill to be called", ttl_ms * 2);

                            runs(function() {
                                expect(replyCallerSpy.resolve).toHaveBeenCalled();
                                expect(replyCallerSpy.resolve).toHaveBeenCalledWith(testResponse);
                                expect(replyCallerSpy.reject).not.toHaveBeenCalled();
                            });
                        });

                        it(
                                "calls registered replyCaller fail if no reply arrives in time",
                                function() {
                                    var replyCallerSpy = jasmine.createSpyObj("deferred", [
                                        "resolve",
                                        "reject"
                                    ]);

                                    runs(function() {
                                        requestReplyManager.addReplyCaller(
                                                "requestReplyId",
                                                replyCallerSpy,
                                                ttl_ms);
                                    });

                                    waitsFor(function() {
                                        return replyCallerSpy.resolve.calls.length > 0
                                            || replyCallerSpy.reject.calls.length > 0;
                                    }, "reject or fulfill to be called", ttl_ms * 2);

                                    runs(function() {
                                        expect(replyCallerSpy.resolve).not.toHaveBeenCalled();
                                        expect(replyCallerSpy.reject).toHaveBeenCalled();
                                    });

                                });

                        function testHandleReplyWithExpectedType(paramDatatypes, params) {
                            var replyCallerSpy = jasmine.createSpyObj("deferred", [
                                "resolve",
                                "reject"
                            ]);

                            runs(function() {
                                var reply = new Reply({
                                    requestReplyId : requestReplyId,
                                    // untype object by serializing and deserializing it
                                    response : [ JSON.parse(JSON.stringify(params))
                                    ]
                                });

                                requestReplyManager.addReplyCaller(
                                        "requestReplyId",
                                        replyCallerSpy,
                                        ttl_ms);
                                requestReplyManager.handleReply(reply);
                            });

                            waitsFor(function() {
                                return replyCallerSpy.resolve.calls.length > 0
                                    || replyCallerSpy.reject.calls.length > 0;
                            }, "reject or fulfill to be called", ttl_ms * 2);

                            runs(function() {
                                expect(replyCallerSpy.resolve).toHaveBeenCalled();
                                expect(replyCallerSpy.reject).not.toHaveBeenCalled();

                                var result = replyCallerSpy.resolve.calls[0].args[0];
                                expect(result).toEqual([ params
                                ]);
                                expect(Typing.getObjectType(result)).toEqual(
                                        Typing.getObjectType(params));
                            });
                        }

                        it("calls registered replyCaller with correctly typed object", function() {
                            spyOn(Typing, 'augmentTypes').andCallThrough();

                            var i, test;
                            for (i = 0; i < testData.length; ++i) {
                                test = testData[i];
                                testHandleReplyWithExpectedType(test.paramDatatype, test.params);
                            }
                        });

                        function callRequestReplyManagerSync(
                                methodName,
                                testParam,
                                testParamDatatype,
                                useInvalidProviderParticipantId) {
                            var providerParticipantId = "providerParticipantId";
                            var provider = {
                                attributeName : {
                                    get : jasmine.createSpy("getterSpy"),
                                    set : jasmine.createSpy("setterSpy")
                                },
                                operationName : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                },
                                getOperationStartingWithGet : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                },
                                getOperationHasPriority : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                },
                                operationHasPriority : {
                                    get : jasmine.createSpy("getterSpy"),
                                    set : jasmine.createSpy("setterSpy")
                                }

                            };
                            provider.attributeName.get.andReturn([ testParam
                            ]);
                            provider.attributeName.set.andReturn([]);
                            provider.operationName.callOperation.andReturn([ testParam
                            ]);
                            provider.getOperationStartingWithGet.callOperation
                                    .andReturn([ testParam
                                    ]);
                            provider.getOperationHasPriority.callOperation.andReturn([ testParam
                            ]);

                            var callbackDispatcher = jasmine.createSpy("callbackDispatcher");

                            var request = new Request({
                                methodName : methodName,
                                paramDatatypes : [ testParamDatatype
                                ],
                                params : [ testParam
                                ]
                            });

                            requestReplyManager.addRequestCaller(providerParticipantId, provider);

                            if (useInvalidProviderParticipantId) {
                                providerParticipantId = "nonExistentProviderId";
                            }

                            requestReplyManager.handleRequest(
                                    providerParticipantId,
                                    request,
                                    callbackDispatcher);

                            return {
                                provider : provider,
                                callbackDispatcher : callbackDispatcher,
                                request : request
                            };
                        }

                        function callRequestReplyManager(
                                methodName,
                                testParam,
                                testParamDatatype,
                                useInvalidProviderParticipantId) {
                            var test =
                                    callRequestReplyManagerSync(
                                            methodName,
                                            testParam,
                                            testParamDatatype,
                                            useInvalidProviderParticipantId);

                            waitsFor(function() {
                                return test.callbackDispatcher.calls.length > 0;
                            }, "callbackDispatcher to be called", 100);

                            return test;
                        }

                        var testParam = "myTestParameter";
                        var testParamDatatype = "String";

                        it("calls attribute getter correctly", function() {
                            var test =
                                    callRequestReplyManager(
                                            "getAttributeName",
                                            testParam,
                                            testParamDatatype);

                            runs(function() {
                                expect(test.provider.attributeName.get).toHaveBeenCalled();
                                expect(test.provider.attributeName.get).toHaveBeenCalledWith();
                                expect(test.provider.attributeName.set).not.toHaveBeenCalled();
                                expect(test.provider.operationName.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationStartingWithGet.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.set).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.get).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationHasPriority.callOperation).not
                                        .toHaveBeenCalled();

                                expect(test.callbackDispatcher).toHaveBeenCalled();
                                expect(test.callbackDispatcher).toHaveBeenCalledWith(new Reply({
                                    response : [ testParam
                                    ],
                                    requestReplyId : test.request.requestReplyId
                                }));
                            });
                        });

                        it("calls attribute setter correctly", function() {
                            var test =
                                    callRequestReplyManager(
                                            "setAttributeName",
                                            testParam,
                                            testParamDatatype);

                            runs(function() {
                                expect(test.provider.attributeName.get).not.toHaveBeenCalled();
                                expect(test.provider.attributeName.set).toHaveBeenCalled();
                                expect(test.provider.attributeName.set).toHaveBeenCalledWith(
                                        testParam);
                                expect(test.provider.operationName.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationStartingWithGet.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.set).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.get).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationHasPriority.callOperation).not
                                        .toHaveBeenCalled();

                                expect(test.callbackDispatcher).toHaveBeenCalled();
                                expect(test.callbackDispatcher).toHaveBeenCalledWith(new Reply({
                                    response : [],
                                    requestReplyId : test.request.requestReplyId
                                }));
                            });
                        });

                        it("calls operation function correctly", function() {
                            var test =
                                    callRequestReplyManager(
                                            "operationName",
                                            testParam,
                                            testParamDatatype);

                            runs(function() {
                                expect(test.provider.attributeName.set).not.toHaveBeenCalled();
                                expect(test.provider.attributeName.get).not.toHaveBeenCalled();
                                expect(test.provider.operationName.callOperation)
                                        .toHaveBeenCalled();
                                expect(test.provider.operationName.callOperation)
                                        .toHaveBeenCalledWith([ testParam
                                        ], [ testParamDatatype
                                        ]);
                                expect(test.provider.getOperationStartingWithGet.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.set).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.get).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationHasPriority.callOperation).not
                                        .toHaveBeenCalled();

                                expect(test.callbackDispatcher).toHaveBeenCalled();
                                expect(test.callbackDispatcher).toHaveBeenCalledWith(new Reply({
                                    response : [ testParam
                                    ],
                                    requestReplyId : test.request.requestReplyId
                                }));
                            });
                        });

                        it(
                                "calls operation \"getOperationStartingWithGet\" when no attribute \"operationStartingWithGet\" exists",
                                function() {
                                    var test =
                                            callRequestReplyManager(
                                                    "getOperationStartingWithGet",
                                                    testParam,
                                                    testParamDatatype);

                                    runs(function() {
                                        expect(
                                                test.provider.getOperationStartingWithGet.callOperation)
                                                .toHaveBeenCalled();
                                        expect(
                                                test.provider.getOperationStartingWithGet.callOperation)
                                                .toHaveBeenCalledWith([ testParam
                                                ], [ testParamDatatype
                                                ]);
                                    });
                                });

                        it(
                                "calls operation \"getOperationHasPriority\" when attribute \"operationHasPriority\" exists",
                                function() {
                                    var test =
                                            callRequestReplyManager(
                                                    "getOperationHasPriority",
                                                    testParam,
                                                    testParamDatatype);

                                    runs(function() {
                                        expect(test.provider.operationHasPriority.set).not
                                                .toHaveBeenCalled();
                                        expect(test.provider.operationHasPriority.get).not
                                                .toHaveBeenCalled();
                                        expect(test.provider.getOperationHasPriority.callOperation)
                                                .toHaveBeenCalled();
                                        expect(test.provider.getOperationHasPriority.callOperation)
                                                .toHaveBeenCalledWith([ testParam
                                                ], [ testParamDatatype
                                                ]);
                                    });
                                });

                        it(
                                "delivers exception upon non-existent provider",
                                function() {
                                    var test =
                                            callRequestReplyManager(
                                                    "testFunction",
                                                    testParam,
                                                    testParamDatatype,
                                                    true);
                                    runs(function() {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'error handling request: {"paramDatatypes":["String"],"params":["myTestParameter"],"methodName":"testFunction","requestReplyId":"'
                                                                                            + test.request.requestReplyId
                                                                                            + '","_typeName":"joynr.Request"} for providerParticipantId nonExistentProviderId'
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                    });
                                });

                        it(
                                "delivers exception when calling not existing operation",
                                function() {
                                    var test =
                                            callRequestReplyManager(
                                                    "notExistentOperationOrAttribute",
                                                    testParam,
                                                    testParamDatatype);
                                    runs(function() {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'Could not find an operation "notExistentOperationOrAttribute" in the provider'
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                    });
                                });
                        it(
                                "delivers exception when calling getter for not existing attribute",
                                function() {
                                    var test =
                                            callRequestReplyManager(
                                                    "getNotExistentOperationOrAttribute",
                                                    testParam,
                                                    testParamDatatype);
                                    runs(function() {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'Could not find an operation "getNotExistentOperationOrAttribute" or an attribute "notExistentOperationOrAttribute" in the provider'
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                    });
                                });
                        it(
                                "delivers exception when calling setter for not existing attribute",
                                function() {
                                    var test =
                                            callRequestReplyManager(
                                                    "setNotExistentOperationOrAttribute",
                                                    testParam,
                                                    testParamDatatype);
                                    runs(function() {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'Could not find an operation "setNotExistentOperationOrAttribute" or an attribute "notExistentOperationOrAttribute" in the provider'
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                    });
                                });
                    });
        }); // require
