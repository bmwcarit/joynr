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

define([
            "global/Promise",
            "joynr",
            "joynr/vehicle/RadioProxy",
            "joynr/vehicle/radiotypes/RadioStation",
            "joynr/vehicle/radiotypes/ErrorList",
            "joynr/datatypes/exampleTypes/Country",
            "joynr/datatypes/exampleTypes/StringMap",
            "joynr/datatypes/exampleTypes/ComplexStructMap",
            "joynr/datatypes/exampleTypes/ComplexStruct",
            "joynr/tests/testTypes/ComplexTestType",
            "joynr/exceptions/SubscriptionException",
            "integration/IntegrationUtils",
            "integration/End2EndAbstractTest",
            "joynr/provisioning/provisioning_cc",
            "integration/provisioning_end2end_common",
            "global/WaitsFor"
        ],
        function(
                Promise,
                joynr,
                RadioProxy,
                RadioStation,
                ErrorList,
                Country,
                StringMap,
                ComplexStructMap,
                ComplexStruct,
                ComplexTestType,
                SubscriptionException,
                IntegrationUtils,
                End2EndAbstractTest,
                provisioning,
                provisioning_end2end,
                waitsFor) {

            describe(
                    "libjoynr-js.integration.end2end.rpc",
                    function() {

                        var subscriptionQosOnChange;
                        var radioProxy;
                        var abstractTest = new End2EndAbstractTest("End2EndRPCTest");
                        var getAttribute = abstractTest.getAttribute;
                        var getFailingAttribute = abstractTest.getFailingAttribute;
                        var setAttribute = abstractTest.setAttribute;
                        var setupSubscriptionAndReturnSpy = abstractTest.setupSubscriptionAndReturnSpy;
                        var callOperation = abstractTest.callOperation;
                        var expectPublication = abstractTest.expectPublication;
                        var setAndTestAttribute = abstractTest.setAndTestAttribute;

                        beforeEach(function(done) {
                            abstractTest.beforeEach().then(function(settings) {
                                subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                                    minIntervalMs : 50
                                });
                                radioProxy = settings.radioProxy;
                                done();
                            });
                        });

                        it("gets the attribute", function(done) {
                            getAttribute("isOn", true).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets the enumAttribute", function(done) {
                            getAttribute("enumAttribute", Country.GERMANY).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets the enumArrayAttribute", function(done) {
                            getAttribute("enumArrayAttribute", [Country.GERMANY]).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("get/sets the complexStructMapAttribute", function(done) {
                            var complexStruct = new ComplexStruct ({
                                num32: 1,
                                num64: 2,
                                data: [1,2,3],
                                str: "string"
                            });

                            var complexStructMap1 = new ComplexStructMap({
                                key1: complexStruct
                            });

                            var complexStructMap2 = new ComplexStructMap({
                                "key2": complexStruct
                            });

                            setAttribute("complexStructMapAttribute", complexStructMap1).then(function() {
                                return getAttribute("complexStructMapAttribute", complexStructMap1);
                            }).then(function() {
                                return setAttribute("complexStructMapAttribute", complexStructMap2);
                            }).then(function() {
                                return getAttribute("complexStructMapAttribute", complexStructMap2);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets an exception for failingSyncAttribute", function(done) {
                            getFailingAttribute("failingSyncAttribute").then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("gets an exception for failingAsyncAttribute", function(done) {
                            getFailingAttribute("failingAsyncAttribute").then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the enumArrayAttribute", function(done) {
                            var value = [];
                            setAttribute("enumArrayAttribute", value).then(function() {
                                return getAttribute("enumArrayAttribute", value);
                            }).then(function() {
                                value = [Country.GERMANY, Country.AUSTRIA, Country.AUSTRALIA, Country.CANADA, Country.ITALY];
                                return setAttribute("enumArrayAttribute", value);
                            }).then(function() {
                                return getAttribute("enumArrayAttribute", value);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the typeDef attributes", function(done) {
                            var value = new RadioStation({
                                name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
                                byteBuffer: []
                            });
                            setAttribute("typeDefForStruct", value).then(function() {
                                return getAttribute("typeDefForStruct", value);
                            }).then(function() {
                                value = 1234543;
                                return setAttribute("typeDefForPrimitive", value);
                            }).then(function() {
                                return getAttribute("typeDefForPrimitive", value);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("get/sets the attribute with starting capital letter", function(done) {
                            setAttribute("StartWithCapitalLetter", true).then(function() {
                                return getAttribute("StartWithCapitalLetter", true);
                            }).then(function() {
                                return setAttribute("StartWithCapitalLetter", false);
                            }).then(function() {
                                return getAttribute("StartWithCapitalLetter", false);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the attribute", function(done) {
                            setAttribute("isOn", true).then(function() {
                                return getAttribute("isOn", true);
                            }).then(function() {
                                return setAttribute("isOn", false);
                            }).then(function() {
                                return getAttribute("isOn", false);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the enumAttribute", function(done) {
                            setAttribute("enumAttribute", Country.AUSTRIA).then(function() {
                                return getAttribute("enumAttribute", Country.AUSTRIA);
                            }).then(function() {
                                return setAttribute("enumAttribute", Country.AUSTRALIA);
                            }).then(function() {
                                return getAttribute("enumAttribute", Country.AUSTRALIA);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the byteBufferAttribute", function(done) {
                            var testByteBuffer = [1,2,3,4];
                            setAttribute("byteBufferAttribute", testByteBuffer).then(function() {
                                return getAttribute("byteBufferAttribute", testByteBuffer);
                            }).then(function() {
                                return setAttribute("byteBufferAttribute", testByteBuffer);
                            }).then(function() {
                                return getAttribute("byteBufferAttribute", testByteBuffer);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("sets the stringMapAttribute", function(done) {
                            var stringMap = new StringMap({key1: "value1"});
                            setAttribute("stringMapAttribute", stringMap).then(function() {
                                return getAttribute("stringMapAttribute", stringMap);
                            }).then(function() {
                                return setAttribute("stringMapAttribute", stringMap);
                            }).then(function() {
                                return getAttribute("stringMapAttribute", stringMap);
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        //enable this test once the proxy side is ready for fire n' forget
                        it("call methodFireAndForgetWithoutParams and expect to call the provider", function(done) {
                            var mySpy;
                            setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("methodFireAndForgetWithoutParams", {});
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0].methodName).toEqual("methodFireAndForgetWithoutParams");
                                });
                            }).then(function() {
                                done();
                            }).catch(fail);
                        });

                        //enable this test once the proxy side is ready for fire n' forget
                        it("call methodFireAndForget and expect to call the provider", function(done) {
                            var mySpy;
                            setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange).then(function(spy) {
                                mySpy = spy;
                                return callOperation("methodFireAndForget", {
                                    intIn: 0,
                                    stringIn : "methodFireAndForget",
                                    complexTestTypeIn : new ComplexTestType({
                                        a : 0,
                                        b : 1
                                    })
                                });
                            }).then(function() {
                                return expectPublication(mySpy, function(call) {
                                   expect(call.args[0].methodName).toEqual("methodFireAndForget");
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });


                        function setAndTestAttributeTester(attributeName) {
                            var lastRecursionIndex = -1, recursions = 5, onFulfilledSpy =
                                    jasmine.createSpy("onFulfilledSpy");

                            setAndTestAttribute(attributeName, recursions - 1).then(
                                    onFulfilledSpy).catch(function(error) {
                                        return IntegrationUtils.outputPromiseError(new Error("End2EndRPCTest.setAndTestAttributeTester. Error while calling setAndTest: " + error.message));
                                    });

                            return waitsFor(function() {
                                return onFulfilledSpy.calls.count() > 0;
                            }, "get/set test to finish", 2 * provisioning.ttl * recursions).then(function() {
                                // each
                                // repetition
                                // consists
                                // of a
                                // get
                                // and
                                // set
                                // => 2
                                // ttls per repetition

                                // additional sanity check whether recursion level
                                // really went down
                                // to 0
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalledWith(0);
                            });
                        }

                        // when provider is working this should also work
                        it("checks whether the provider stores the attribute value", function(done) {
                            setAndTestAttributeTester("isOn").then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation successfully (Provider sync, String parameter)", function(done) {
                            callOperation("addFavoriteStation",
                                    {
                                radioStation : 'stringStation'
                                    }
                            ).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation successfully (Complex map parameter)", function(done) {
                            callOperation("methodWithComplexMap",
                                    {
                                complexStructMap : new ComplexStructMap()
                                    }
                            ).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get a ProviderRuntimeException (Provider sync, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationerror'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual("example message sync");
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get an ApplicationException (Provider sync, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationerrorApplicationException'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                                expect(catchSpy.calls.argsFor(0)[0].error).toBeDefined();
                                //expect(catchSpy.calls.argsFor(0)[0].error).toEqual(ErrorList.EXAMPLE_ERROR_2);
                                expect(catchSpy.calls.argsFor(0)[0].error).toEqual(jasmine.objectContaining(ErrorList.EXAMPLE_ERROR_2));
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation successfully (Provider async, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationasync'
                            }).then(onFulfilledSpy).catch(function(error) {
                                return IntegrationUtils.outputPromiseError(new Error("End2EndRPCTest.can call an operation successfully (Provider async, String parameter).addFavoriteStation: " + error.message));
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.calls.count() > 0;
                            }, "operation call to finish", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get a ProviderRuntimeException (Provider async, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationasyncerror'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual("example message async");
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation and get an ApplicationException (Provider async, String parameter)", function(done) {
                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var catchSpy = jasmine.createSpy("catchSpy");

                            radioProxy.addFavoriteStation({
                                radioStation : 'stringStationasyncerrorApplicationException'
                            }).then(onFulfilledSpy).catch(catchSpy);

                            waitsFor(function() {
                                return catchSpy.calls.count() > 0;
                            }, "operation call to fail", provisioning.ttl).then(function() {
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(catchSpy).toHaveBeenCalled();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                                expect(catchSpy.calls.argsFor(0)[0].error).toBeDefined();
                                //expect(catchSpy.calls.argsFor(0)[0].error).toEqual(ErrorList.EXAMPLE_ERROR_1);
                                expect(catchSpy.calls.argsFor(0)[0].error).toEqual(jasmine.objectContaining(ErrorList.EXAMPLE_ERROR_1));
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("can call an operation (parameter of complex type)", function(done) {
                            callOperation("addFavoriteStation",
                                    {
                                        radioStation : 'stringStation'
                                    }
                            ).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "can call an operation (parameter of byteBuffer type",
                                function(done) {
                                    callOperation(
                                            "methodWithByteBuffer",
                                            {
                                                input : [0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]
                                            },
                                            {
                                                result : [0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with working parameters and return type",
                                function(done) {
                                    callOperation(
                                            "addFavoriteStation",
                                            {
                                                radioStation : "truelyContainingTheString\"True\""
                                            },
                                            {
                                                returnValue : true
                                    }).then(function() {
                                        return callOperation(
                                                "addFavoriteStation",
                                                {
                                                    radioStation : "This is false!"
                                                },
                                                {
                                                    returnValue : false
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "addFavoriteStation",
                                                {
                                                    radioStation : new RadioStation(
                                                            {
                                                                name : "truelyContainingTheRadioStationString\"True\""
                                                            })
                                                },
                                                {
                                                    returnValue : true
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "addFavoriteStation",
                                                {
                                                    radioStation : new RadioStation({
                                                        name : "This is a false RadioStation!"
                                                    })
                                                },
                                                {
                                                    returnValue : false
                                                });
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "can call an operation with typedef arguments",
                                function(done) {
                                    var typeDefStructInput = new RadioStation({
                                        name: "TestEnd2EndComm.methodWithTypeDef.RadioStation",
                                        byteBuffer: []
                                    });
                                    var typeDefPrimitiveInput = 1234543;
                                    callOperation(
                                            "methodWithTypeDef",
                                            {
                                                typeDefStructInput : typeDefStructInput,
                                                typeDefPrimitiveInput : typeDefPrimitiveInput
                                            },
                                            {
                                                typeDefStructOutput : typeDefStructInput,
                                                typeDefPrimitiveOutput : typeDefPrimitiveInput
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with enum arguments and enum return type",
                                function(done) {
                                    callOperation(
                                            "operationWithEnumsAsInputAndOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : []
                                            },
                                            {
                                                enumOutput : Country.GERMANY
                                            }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA]
                                                },
                                                {
                                                    enumOutput : Country.AUSTRIA
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                                                },
                                                {
                                                    enumOutput : Country.AUSTRIA
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                                                },
                                                {
                                                    enumOutput : Country.CANADA
                                                });
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with multiple return values and async provider",
                                function(done) {
                                    var inputData = {
                                        enumInput : Country.GERMANY,
                                        enumArrayInput : [Country.GERMANY, Country.ITALY],
                                        stringInput : "StringTest",
                                        syncTest : false
                                    };
                                    callOperation(
                                            "operationWithMultipleOutputParameters",
                                            inputData,
                                            {
                                                enumArrayOutput : inputData.enumArrayInput,
                                                enumOutput : inputData.enumInput,
                                                stringOutput : inputData.stringInput,
                                                booleanOutput : inputData.syncTest
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);

                        });

                        it(
                                "can call an operation with multiple return values and sync provider",
                                function(done) {
                                    var inputData = {
                                        enumInput : Country.GERMANY,
                                        enumArrayInput : [Country.GERMANY, Country.ITALY],
                                        stringInput : "StringTest",
                                        syncTest : true
                                    };
                                    callOperation(
                                            "operationWithMultipleOutputParameters",
                                            inputData,
                                            {
                                                enumArrayOutput : inputData.enumArrayInput,
                                                enumOutput : inputData.enumInput,
                                                stringOutput : inputData.stringInput,
                                                booleanOutput : inputData.syncTest
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                        });

                        it(
                                "can call an operation with enum arguments and enum array as return type",
                                function(done) {
                                    callOperation(
                                            "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                            {
                                                enumInput : Country.GERMANY,
                                                enumArrayInput : []
                                            },
                                            {
                                                enumOutput : [Country.GERMANY]
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA]
                                                },
                                                {
                                                    enumOutput : [Country.AUSTRIA, Country.GERMANY]
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                                                },
                                                {
                                                    enumOutput : [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA, Country.GERMANY]
                                                });
                                    }).then(function() {
                                        return callOperation(
                                                "operationWithEnumsAsInputAndEnumArrayAsOutput",
                                                {
                                                    enumInput : Country.GERMANY,
                                                    enumArrayInput : [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                                                },
                                                {
                                                    enumOutput : [Country.CANADA, Country.AUSTRIA, Country.ITALY, Country.GERMANY]
                                                });
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "can call an operation with double array as argument and string array as return type",
                                function(done) {
                                    callOperation(
                                            "methodWithSingleArrayParameters",
                                            {
                                                doubleArrayArg : [0.01,1.1,2.2,3.3]
                                            },
                                            {
                                                stringArrayOut : ["0.01", "1.1", "2.2", "3.3"]
                                            }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                               });

                        it(
                                "attributes are working with predefined implementation on provider side",
                                function(done) {
                                    setAndTestAttribute("attrProvidedImpl", 0).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "operation is working with predefined implementation on provider side",
                                function(done) {
                                    var testArgument = "This is my test argument";
                                    callOperation(
                                            "methodProvidedImpl",
                                            {
                                                arg : testArgument
                                            },
                                            {
                                                returnValue : testArgument
                                    }).then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        afterEach(abstractTest.afterEach);

                    });
        });
