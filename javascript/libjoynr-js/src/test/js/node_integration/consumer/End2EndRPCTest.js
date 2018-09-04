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

const joynr = require("joynr");
const RadioStation = require("../../../generated/joynr/vehicle/radiotypes/RadioStation");
const ErrorList = require("../../../generated/joynr/vehicle/radiotypes/ErrorList");
const Country = require("../../../generated/joynr/datatypes/exampleTypes/Country");
const StringMap = require("../../../generated/joynr/datatypes/exampleTypes/StringMap");
const ComplexStructMap = require("../../../generated/joynr/datatypes/exampleTypes/ComplexStructMap");
const ComplexStruct = require("../../../generated/joynr/datatypes/exampleTypes/ComplexStruct");
const ComplexTestType = require("../../../generated/joynr/tests/testTypes/ComplexTestType");
const IntegrationUtils = require("../IntegrationUtils");
const End2EndAbstractTest = require("../End2EndAbstractTest");
const provisioning = require("../../../resources/joynr/provisioning/provisioning_cc");
const waitsFor = require("../../global/WaitsFor");

describe("libjoynr-js.integration.end2end.rpc", () => {
    let subscriptionQosOnChange;
    let radioProxy;
    const abstractTest = new End2EndAbstractTest("End2EndRPCTest", "TestEnd2EndCommProviderProcess");
    const getAttribute = abstractTest.getAttribute;
    const getFailingAttribute = abstractTest.getFailingAttribute;
    const setAttribute = abstractTest.setAttribute;
    const setupSubscriptionAndReturnSpy = abstractTest.setupSubscriptionAndReturnSpy;
    const callOperation = abstractTest.callOperation;
    const expectPublication = abstractTest.expectPublication;
    const setAndTestAttribute = abstractTest.setAndTestAttribute;

    beforeAll(done => {
        abstractTest.beforeEach().then(settings => {
            subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50
            });
            radioProxy = settings.radioProxy;
            done();
        });
    });

    it("gets the attribute", done => {
        getAttribute("isOn", true)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("gets the enumAttribute", done => {
        getAttribute("enumAttribute", Country.GERMANY)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("gets the enumArrayAttribute", done => {
        getAttribute("enumArrayAttribute", [Country.GERMANY])
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("get/sets the complexStructMapAttribute", done => {
        const complexStruct = new ComplexStruct({
            num32: 1,
            num64: 2,
            data: [1, 2, 3],
            str: "string"
        });

        const complexStructMap1 = new ComplexStructMap({
            key1: complexStruct
        });

        const complexStructMap2 = new ComplexStructMap({
            key2: complexStruct
        });

        setAttribute("complexStructMapAttribute", complexStructMap1)
            .then(() => {
                return getAttribute("complexStructMapAttribute", complexStructMap1);
            })
            .then(() => {
                return setAttribute("complexStructMapAttribute", complexStructMap2);
            })
            .then(() => {
                return getAttribute("complexStructMapAttribute", complexStructMap2);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("gets an exception for failingSyncAttribute", done => {
        getFailingAttribute("failingSyncAttribute")
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("gets an exception for failingAsyncAttribute", done => {
        getFailingAttribute("failingAsyncAttribute")
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("sets the enumArrayAttribute", done => {
        let value = [];
        setAttribute("enumArrayAttribute", value)
            .then(() => {
                return getAttribute("enumArrayAttribute", value);
            })
            .then(() => {
                value = [Country.GERMANY, Country.AUSTRIA, Country.AUSTRALIA, Country.CANADA, Country.ITALY];
                return setAttribute("enumArrayAttribute", value);
            })
            .then(() => {
                return getAttribute("enumArrayAttribute", value);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("sets the typeDef attributes", done => {
        let value = new RadioStation({
            name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
            byteBuffer: []
        });
        setAttribute("typeDefForStruct", value)
            .then(() => {
                return getAttribute("typeDefForStruct", value);
            })
            .then(() => {
                value = 1234543;
                return setAttribute("typeDefForPrimitive", value);
            })
            .then(() => {
                return getAttribute("typeDefForPrimitive", value);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("get/sets the attribute with starting capital letter", done => {
        setAttribute("StartWithCapitalLetter", true)
            .then(() => {
                return getAttribute("StartWithCapitalLetter", true);
            })
            .then(() => {
                return setAttribute("StartWithCapitalLetter", false);
            })
            .then(() => {
                return getAttribute("StartWithCapitalLetter", false);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("sets the attribute", done => {
        setAttribute("isOn", true)
            .then(() => {
                return getAttribute("isOn", true);
            })
            .then(() => {
                return setAttribute("isOn", false);
            })
            .then(() => {
                return getAttribute("isOn", false);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("sets the enumAttribute", done => {
        setAttribute("enumAttribute", Country.AUSTRIA)
            .then(() => {
                return getAttribute("enumAttribute", Country.AUSTRIA);
            })
            .then(() => {
                return setAttribute("enumAttribute", Country.AUSTRALIA);
            })
            .then(() => {
                return getAttribute("enumAttribute", Country.AUSTRALIA);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("sets the byteBufferAttribute", done => {
        const testByteBuffer = [1, 2, 3, 4];
        setAttribute("byteBufferAttribute", testByteBuffer)
            .then(() => {
                return getAttribute("byteBufferAttribute", testByteBuffer);
            })
            .then(() => {
                return setAttribute("byteBufferAttribute", testByteBuffer);
            })
            .then(() => {
                return getAttribute("byteBufferAttribute", testByteBuffer);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("sets the stringMapAttribute", done => {
        const stringMap = new StringMap({ key1: "value1" });
        setAttribute("stringMapAttribute", stringMap)
            .then(() => {
                return getAttribute("stringMapAttribute", stringMap);
            })
            .then(() => {
                return setAttribute("stringMapAttribute", stringMap);
            })
            .then(() => {
                return getAttribute("stringMapAttribute", stringMap);
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    //enable this test once the proxy side is ready for fire n' forget
    it("call methodFireAndForgetWithoutParams and expect to call the provider", done => {
        let mySpy;
        setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange)
            .then(spy => {
                mySpy = spy;
                return callOperation("methodFireAndForgetWithoutParams", {});
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0].methodName).toEqual("methodFireAndForgetWithoutParams");
                });
            })
            .then(() => {
                done();
            })
            .catch(fail);
    });

    //enable this test once the proxy side is ready for fire n' forget
    it("call methodFireAndForget and expect to call the provider", done => {
        let mySpy;
        setupSubscriptionAndReturnSpy("fireAndForgetCallArrived", subscriptionQosOnChange)
            .then(spy => {
                mySpy = spy;
                return callOperation("methodFireAndForget", {
                    intIn: 0,
                    stringIn: "methodFireAndForget",
                    complexTestTypeIn: new ComplexTestType({
                        a: 0,
                        b: 1
                    })
                });
            })
            .then(() => {
                return expectPublication(mySpy, call => {
                    expect(call.args[0].methodName).toEqual("methodFireAndForget");
                });
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function setAndTestAttributeTester(attributeName, done) {
        const recursions = 5;

        return setAndTestAttribute(attributeName, recursions - 1)
            .then(value => {
                expect(value).toEqual(0);
                done();
            })
            .catch(fail);
    }

    // when provider is working this should also work
    it("checks whether the provider stores the attribute value", done => {
        setAndTestAttributeTester("isOn", done);
    });

    it("can call an operation successfully (Provider sync, String parameter)", done => {
        callOperation("addFavoriteStation", {
            radioStation: "stringStation"
        })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation successfully (Complex map parameter)", done => {
        callOperation("methodWithComplexMap", {
            complexStructMap: new ComplexStructMap()
        })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation and get a ProviderRuntimeException (Provider sync, String parameter)", done => {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
        const catchSpy = jasmine.createSpy("catchSpy");

        radioProxy
            .addFavoriteStation({
                radioStation: "stringStationerror"
            })
            .then(onFulfilledSpy)
            .catch(catchSpy);

        waitsFor(
            () => {
                return catchSpy.calls.count() > 0;
            },
            "operation call to fail",
            provisioning.ttl
        )
            .then(() => {
                expect(onFulfilledSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual("example message sync");
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation and get an ApplicationException (Provider sync, String parameter)", done => {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
        const catchSpy = jasmine.createSpy("catchSpy");

        radioProxy
            .addFavoriteStation({
                radioStation: "stringStationerrorApplicationException"
            })
            .then(onFulfilledSpy)
            .catch(catchSpy);

        waitsFor(
            () => {
                return catchSpy.calls.count() > 0;
            },
            "operation call to fail",
            provisioning.ttl
        )
            .then(() => {
                expect(onFulfilledSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                expect(catchSpy.calls.argsFor(0)[0].error).toBeDefined();
                //expect(catchSpy.calls.argsFor(0)[0].error).toEqual(ErrorList.EXAMPLE_ERROR_2);
                expect(catchSpy.calls.argsFor(0)[0].error).toEqual(jasmine.objectContaining(ErrorList.EXAMPLE_ERROR_2));
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation successfully (Provider async, String parameter)", done => {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

        radioProxy
            .addFavoriteStation({
                radioStation: "stringStationasync"
            })
            .then(onFulfilledSpy)
            .catch(error => {
                return IntegrationUtils.outputPromiseError(
                    new Error(
                        `End2EndRPCTest.can call an operation successfully (Provider async, String parameter).addFavoriteStation: ${
                            error.message
                        }`
                    )
                );
            });

        waitsFor(
            () => {
                return onFulfilledSpy.calls.count() > 0;
            },
            "operation call to finish",
            provisioning.ttl
        )
            .then(() => {
                expect(onFulfilledSpy).toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation and get a ProviderRuntimeException (Provider async, String parameter)", done => {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
        const catchSpy = jasmine.createSpy("catchSpy");

        radioProxy
            .addFavoriteStation({
                radioStation: "stringStationasyncerror"
            })
            .then(onFulfilledSpy)
            .catch(catchSpy);

        waitsFor(
            () => {
                return catchSpy.calls.count() > 0;
            },
            "operation call to fail",
            provisioning.ttl
        )
            .then(() => {
                expect(onFulfilledSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0].detailMessage).toEqual("example message async");
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation and get an ApplicationException (Provider async, String parameter)", done => {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
        const catchSpy = jasmine.createSpy("catchSpy");

        radioProxy
            .addFavoriteStation({
                radioStation: "stringStationasyncerrorApplicationException"
            })
            .then(onFulfilledSpy)
            .catch(catchSpy);

        waitsFor(
            () => {
                return catchSpy.calls.count() > 0;
            },
            "operation call to fail",
            provisioning.ttl
        )
            .then(() => {
                expect(onFulfilledSpy).not.toHaveBeenCalled();
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ApplicationException");
                expect(catchSpy.calls.argsFor(0)[0].error).toBeDefined();
                //expect(catchSpy.calls.argsFor(0)[0].error).toEqual(ErrorList.EXAMPLE_ERROR_1);
                expect(catchSpy.calls.argsFor(0)[0].error).toEqual(jasmine.objectContaining(ErrorList.EXAMPLE_ERROR_1));
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation (parameter of complex type)", done => {
        callOperation("addFavoriteStation", {
            radioStation: "stringStation"
        })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation (parameter of byteBuffer type", done => {
        callOperation(
            "methodWithByteBuffer",
            {
                input: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
            },
            {
                result: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation with working parameters and return type", done => {
        callOperation(
            "addFavoriteStation",
            {
                radioStation: 'truelyContainingTheString"True"'
            },
            {
                returnValue: true
            }
        )
            .then(() => {
                return callOperation(
                    "addFavoriteStation",
                    {
                        radioStation: "This is false!"
                    },
                    {
                        returnValue: false
                    }
                );
            })
            .then(() => {
                return callOperation(
                    "addFavoriteStation",
                    {
                        radioStation: new RadioStation({
                            name: 'truelyContainingTheRadioStationString"True"',
                            byteBuffer: []
                        })
                    },
                    {
                        returnValue: true
                    }
                );
            })
            .then(() => {
                return callOperation(
                    "addFavoriteStation",
                    {
                        radioStation: new RadioStation({
                            name: "This is a false RadioStation!",
                            byteBuffer: []
                        })
                    },
                    {
                        returnValue: false
                    }
                );
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation with typedef arguments", done => {
        const typeDefStructInput = new RadioStation({
            name: "TestEnd2EndComm.methodWithTypeDef.RadioStation",
            byteBuffer: []
        });
        const typeDefPrimitiveInput = 1234543;
        callOperation(
            "methodWithTypeDef",
            {
                typeDefStructInput,
                typeDefPrimitiveInput
            },
            {
                typeDefStructOutput: typeDefStructInput,
                typeDefPrimitiveOutput: typeDefPrimitiveInput
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation with enum arguments and enum return type", done => {
        callOperation(
            "operationWithEnumsAsInputAndOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: []
            },
            {
                enumOutput: Country.GERMANY
            }
        )
            .then(() => {
                return callOperation(
                    "operationWithEnumsAsInputAndOutput",
                    {
                        enumInput: Country.GERMANY,
                        enumArrayInput: [Country.AUSTRIA]
                    },
                    {
                        enumOutput: Country.AUSTRIA
                    }
                );
            })
            .then(() => {
                return callOperation(
                    "operationWithEnumsAsInputAndOutput",
                    {
                        enumInput: Country.GERMANY,
                        enumArrayInput: [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                    },
                    {
                        enumOutput: Country.AUSTRIA
                    }
                );
            })
            .then(() => {
                return callOperation(
                    "operationWithEnumsAsInputAndOutput",
                    {
                        enumInput: Country.GERMANY,
                        enumArrayInput: [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                    },
                    {
                        enumOutput: Country.CANADA
                    }
                );
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation with multiple return values and async provider", done => {
        const inputData = {
            enumInput: Country.GERMANY,
            enumArrayInput: [Country.GERMANY, Country.ITALY],
            stringInput: "StringTest",
            syncTest: false
        };
        callOperation("operationWithMultipleOutputParameters", inputData, {
            enumArrayOutput: inputData.enumArrayInput,
            enumOutput: inputData.enumInput,
            stringOutput: inputData.stringInput,
            booleanOutput: inputData.syncTest
        })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation with multiple return values and sync provider", done => {
        const inputData = {
            enumInput: Country.GERMANY,
            enumArrayInput: [Country.GERMANY, Country.ITALY],
            stringInput: "StringTest",
            syncTest: true
        };
        callOperation("operationWithMultipleOutputParameters", inputData, {
            enumArrayOutput: inputData.enumArrayInput,
            enumOutput: inputData.enumInput,
            stringOutput: inputData.stringInput,
            booleanOutput: inputData.syncTest
        })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation with enum arguments and enum array as return type", done => {
        callOperation(
            "operationWithEnumsAsInputAndEnumArrayAsOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: []
            },
            {
                enumOutput: [Country.GERMANY]
            }
        )
            .then(() => {
                return callOperation(
                    "operationWithEnumsAsInputAndEnumArrayAsOutput",
                    {
                        enumInput: Country.GERMANY,
                        enumArrayInput: [Country.AUSTRIA]
                    },
                    {
                        enumOutput: [Country.AUSTRIA, Country.GERMANY]
                    }
                );
            })
            .then(() => {
                return callOperation(
                    "operationWithEnumsAsInputAndEnumArrayAsOutput",
                    {
                        enumInput: Country.GERMANY,
                        enumArrayInput: [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
                    },
                    {
                        enumOutput: [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA, Country.GERMANY]
                    }
                );
            })
            .then(() => {
                return callOperation(
                    "operationWithEnumsAsInputAndEnumArrayAsOutput",
                    {
                        enumInput: Country.GERMANY,
                        enumArrayInput: [Country.CANADA, Country.AUSTRIA, Country.ITALY]
                    },
                    {
                        enumOutput: [Country.CANADA, Country.AUSTRIA, Country.ITALY, Country.GERMANY]
                    }
                );
            })
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("can call an operation with double array as argument and string array as return type", done => {
        callOperation(
            "methodWithSingleArrayParameters",
            {
                doubleArrayArg: [0.01, 1.1, 2.2, 3.3]
            },
            {
                stringArrayOut: ["0.01", "1.1", "2.2", "3.3"]
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("attributes are working with predefined implementation on provider side", done => {
        setAndTestAttribute("attrProvidedImpl", 0)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("operation is working with predefined implementation on provider side", done => {
        const testArgument = "This is my test argument";
        callOperation(
            "methodProvidedImpl",
            {
                arg: testArgument
            },
            {
                returnValue: testArgument
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    afterAll(abstractTest.afterEach);
});
