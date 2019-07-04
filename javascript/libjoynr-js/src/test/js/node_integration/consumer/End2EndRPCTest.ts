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

import joynr from "../../../../main/js/joynr";
import * as RadioProxy from "../../../generated/joynr/vehicle/RadioProxy";
import RadioStation from "../../../generated/joynr/vehicle/radiotypes/RadioStation";
import ErrorList from "../../../generated/joynr/vehicle/radiotypes/ErrorList";
import Country from "../../../generated/joynr/datatypes/exampleTypes/Country";
import StringMap from "../../../generated/joynr/datatypes/exampleTypes/StringMap";
import ComplexStructMap from "../../../generated/joynr/datatypes/exampleTypes/ComplexStructMap";
import ComplexStruct from "../../../generated/joynr/datatypes/exampleTypes/ComplexStruct";
import ComplexTestType from "../../../generated/joynr/tests/testTypes/ComplexTestType";
import End2EndAbstractTest from "../End2EndAbstractTest";
import ProviderRuntimeException = require("joynr/joynr/exceptions/ProviderRuntimeException");
import { reversePromise } from "../../testUtil";
import ApplicationException = require("../../../../main/js/joynr/exceptions/ApplicationException");

describe("libjoynr-js.integration.end2end.rpc", () => {
    let subscriptionQosOnChange: any;
    let radioProxy: RadioProxy;
    const abstractTest = new End2EndAbstractTest("End2EndRPCTest", "TestEnd2EndCommProviderProcess");

    beforeAll(async () => {
        const settings = await abstractTest.beforeEach();
        subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
            minIntervalMs: 50
        });
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        radioProxy = settings.radioProxy!;
    });

    it("gets the attribute", async () => {
        await abstractTest.getAttribute("isOn", true);
    });

    it("gets the enumAttribute", async () => {
        await abstractTest.getAttribute("enumAttribute", Country.GERMANY);
    });

    it("gets the enumArrayAttribute", async () => {
        await abstractTest.getAttribute("enumArrayAttribute", [Country.GERMANY]);
    });

    it("get/sets the complexStructMapAttribute", async () => {
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

        await abstractTest.setAttribute("complexStructMapAttribute", complexStructMap1);
        await abstractTest.getAttribute("complexStructMapAttribute", complexStructMap1);
        await abstractTest.setAttribute("complexStructMapAttribute", complexStructMap2);
        await abstractTest.getAttribute("complexStructMapAttribute", complexStructMap2);
    });

    it("gets an exception for failingSyncAttribute", async () => {
        await abstractTest.getFailingAttribute("failingSyncAttribute");
    });

    it("gets an exception for failingAsyncAttribute", async () => {
        await abstractTest.getFailingAttribute("failingAsyncAttribute");
    });

    it("sets the enumArrayAttribute", async () => {
        let value: Country[] = [];
        await abstractTest.setAttribute("enumArrayAttribute", value);
        await abstractTest.getAttribute("enumArrayAttribute", value);
        value = [Country.GERMANY, Country.AUSTRIA, Country.AUSTRALIA, Country.CANADA, Country.ITALY];
        await abstractTest.setAttribute("enumArrayAttribute", value);
        await abstractTest.getAttribute("enumArrayAttribute", value);
    });

    it("sets the typeDef attributes", async () => {
        const value = new RadioStation({
            name: "TestEnd2EndComm.typeDefForStructAttribute.RadioStation",
            byteBuffer: []
        });
        await abstractTest.setAttribute("typeDefForStruct", value);
        await abstractTest.getAttribute("typeDefForStruct", value);
        const otherValue = 1234543;
        await abstractTest.setAttribute("typeDefForPrimitive", otherValue);
        await abstractTest.getAttribute("typeDefForPrimitive", otherValue);
    });

    it("get/sets the attribute with starting capital letter", async () => {
        await abstractTest.setAttribute("StartWithCapitalLetter", true);
        await abstractTest.getAttribute("StartWithCapitalLetter", true);
        await abstractTest.setAttribute("StartWithCapitalLetter", false);
        await abstractTest.getAttribute("StartWithCapitalLetter", false);
    });

    it("sets the attribute", async () => {
        await abstractTest.setAttribute("isOn", true);
        await abstractTest.getAttribute("isOn", true);
        await abstractTest.setAttribute("isOn", false);
        await abstractTest.getAttribute("isOn", false);
    });

    it("sets the enumAttribute", async () => {
        await abstractTest.setAttribute("enumAttribute", Country.AUSTRIA);
        await abstractTest.getAttribute("enumAttribute", Country.AUSTRIA);
        await abstractTest.setAttribute("enumAttribute", Country.AUSTRALIA);
        await abstractTest.getAttribute("enumAttribute", Country.AUSTRALIA);
    });

    it("sets the byteBufferAttribute", async () => {
        const testByteBuffer = [1, 2, 3, 4];
        await abstractTest.setAttribute("byteBufferAttribute", testByteBuffer);
        await abstractTest.getAttribute("byteBufferAttribute", testByteBuffer);
        await abstractTest.setAttribute("byteBufferAttribute", testByteBuffer);
        await abstractTest.getAttribute("byteBufferAttribute", testByteBuffer);
    });

    it("sets the stringMapAttribute", async () => {
        const stringMap = new StringMap({ key1: "value1" });
        await abstractTest.setAttribute("stringMapAttribute", stringMap);
        await abstractTest.getAttribute("stringMapAttribute", stringMap);
        await abstractTest.setAttribute("stringMapAttribute", stringMap);
        await abstractTest.getAttribute("stringMapAttribute", stringMap);
    });

    it("call methodFireAndForgetWithoutParams and expect to call the provider", async () => {
        const spy = await abstractTest.setupSubscriptionAndReturnSpy(
            "fireAndForgetCallArrived",
            subscriptionQosOnChange
        );
        await abstractTest.callOperation("methodFireAndForgetWithoutParams", {});

        await abstractTest.expectPublication(spy, (call: any) => {
            expect(call[0].methodName).toEqual("methodFireAndForgetWithoutParams");
        });
    });

    it("call methodFireAndForget and expect to call the provider", async () => {
        const spy = await abstractTest.setupSubscriptionAndReturnSpy(
            "fireAndForgetCallArrived",
            subscriptionQosOnChange
        );

        await abstractTest.callOperation("methodFireAndForget", {
            intIn: 0,
            stringIn: "methodFireAndForget",
            complexTestTypeIn: new ComplexTestType({
                a: 0,
                b: 1
            })
        });

        await abstractTest.expectPublication(spy, (call: any) => {
            expect(call[0].methodName).toEqual("methodFireAndForget");
        });
    });

    it("checks whether the provider stores the attribute value", async () => {
        const recursions = 5;

        const value = await abstractTest.setAndTestAttribute("isOn", recursions - 1);
        expect(value).toEqual(0);
    });

    it("can call an operation successfully (Provider sync, String parameter)", async () => {
        await abstractTest.callOperation("addFavoriteStation", {
            radioStation: "stringStation"
        });
    });

    it("can call an operation successfully (Complex map parameter)", async () => {
        await abstractTest.callOperation("methodWithComplexMap", {
            complexStructMap: new ComplexStructMap({})
        });
    });

    it("can call an operation and get a ProviderRuntimeException (Provider sync, String parameter)", async () => {
        await expect(
            radioProxy.addFavoriteStation({
                radioStation: "stringStationerror"
            })
        ).rejects.toBeInstanceOf(ProviderRuntimeException);
    });

    it("can call an operation and get an ApplicationException (Provider sync, String parameter)", async () => {
        const exception = await reversePromise(
            radioProxy.addFavoriteStation({
                radioStation: "stringStationerrorApplicationException"
            })
        );
        expect(exception).toBeInstanceOf(ApplicationException);
        expect(exception.error).toEqual(ErrorList.EXAMPLE_ERROR_2);
    });

    it("can call an operation successfully (Provider async, String parameter)", async () => {
        await radioProxy.addFavoriteStation({
            radioStation: "stringStationasync"
        });
    });

    it("can call an operation and get a ProviderRuntimeException (Provider async, String parameter)", async () => {
        await expect(
            radioProxy.addFavoriteStation({
                radioStation: "stringStationasyncerror"
            })
        ).rejects.toBeInstanceOf(ProviderRuntimeException);
    });

    it("can call an operation and get an ApplicationException (Provider async, String parameter)", async () => {
        const exception = await reversePromise(
            radioProxy.addFavoriteStation({
                radioStation: "stringStationasyncerrorApplicationException"
            })
        );
        expect(exception).toBeInstanceOf(ApplicationException);
        expect(exception.error).toEqual(ErrorList.EXAMPLE_ERROR_1);
    });

    it("can call an operation (parameter of complex type)", async () => {
        await abstractTest.callOperation("addFavoriteStation", {
            radioStation: "stringStation"
        });
    });

    it("can call an operation (parameter of byteBuffer type", async () => {
        await abstractTest.callOperation(
            "methodWithByteBuffer",
            {
                input: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
            },
            {
                result: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
            }
        );
    });

    it("can call an operation with working parameters and return type", async () => {
        await abstractTest.callOperation(
            "addFavoriteStation",
            {
                radioStation: 'truelyContainingTheString"True"'
            },
            {
                returnValue: true
            }
        );

        await abstractTest.callOperation(
            "addFavoriteStation",
            {
                radioStation: "This is false!"
            },
            {
                returnValue: false
            }
        );

        await abstractTest.callOperation(
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

        await abstractTest.callOperation(
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
    });

    it("can call an operation with typedef arguments", async () => {
        const typeDefStructInput = new RadioStation({
            name: "TestEnd2EndComm.methodWithTypeDef.RadioStation",
            byteBuffer: []
        });
        const typeDefPrimitiveInput = 1234543;

        await abstractTest.callOperation(
            "methodWithTypeDef",
            {
                typeDefStructInput,
                typeDefPrimitiveInput
            },
            {
                typeDefStructOutput: typeDefStructInput,
                typeDefPrimitiveOutput: typeDefPrimitiveInput
            }
        );
    });

    it("can call an operation with enum arguments and enum return type", async () => {
        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: []
            },
            {
                enumOutput: Country.GERMANY
            }
        );

        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: [Country.AUSTRIA]
            },
            {
                enumOutput: Country.AUSTRIA
            }
        );

        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
            },
            {
                enumOutput: Country.AUSTRIA
            }
        );

        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: [Country.CANADA, Country.AUSTRIA, Country.ITALY]
            },
            {
                enumOutput: Country.CANADA
            }
        );
    });

    it("can call an operation with multiple return values and async provider", async () => {
        const inputData = {
            enumInput: Country.GERMANY,
            enumArrayInput: [Country.GERMANY, Country.ITALY],
            stringInput: "StringTest",
            syncTest: false
        };

        await abstractTest.callOperation("operationWithMultipleOutputParameters", inputData, {
            enumArrayOutput: inputData.enumArrayInput,
            enumOutput: inputData.enumInput,
            stringOutput: inputData.stringInput,
            booleanOutput: inputData.syncTest
        });
    });

    it("can call an operation with multiple return values and sync provider", async () => {
        const inputData = {
            enumInput: Country.GERMANY,
            enumArrayInput: [Country.GERMANY, Country.ITALY],
            stringInput: "StringTest",
            syncTest: true
        };

        await abstractTest.callOperation("operationWithMultipleOutputParameters", inputData, {
            enumArrayOutput: inputData.enumArrayInput,
            enumOutput: inputData.enumInput,
            stringOutput: inputData.stringInput,
            booleanOutput: inputData.syncTest
        });
    });

    it("can call an operation with enum arguments and enum array as return type", async () => {
        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndEnumArrayAsOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: []
            },
            {
                enumOutput: [Country.GERMANY]
            }
        );

        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndEnumArrayAsOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: [Country.AUSTRIA]
            },
            {
                enumOutput: [Country.AUSTRIA, Country.GERMANY]
            }
        );

        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndEnumArrayAsOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA]
            },
            {
                enumOutput: [Country.AUSTRIA, Country.GERMANY, Country.AUSTRALIA, Country.GERMANY]
            }
        );

        await abstractTest.callOperation(
            "operationWithEnumsAsInputAndEnumArrayAsOutput",
            {
                enumInput: Country.GERMANY,
                enumArrayInput: [Country.CANADA, Country.AUSTRIA, Country.ITALY]
            },
            {
                enumOutput: [Country.CANADA, Country.AUSTRIA, Country.ITALY, Country.GERMANY]
            }
        );
    });

    it("can call an operation with double array as argument and string array as return type", async () => {
        await abstractTest.callOperation(
            "methodWithSingleArrayParameters",
            {
                doubleArrayArg: [0.01, 1.1, 2.2, 3.3]
            },
            {
                stringArrayOut: ["0.01", "1.1", "2.2", "3.3"]
            }
        );
    });

    it("attributes are working with predefined implementation on provider side", async () => {
        await abstractTest.setAndTestAttribute("attrProvidedImpl", 0);
    });

    it("operation is working with predefined implementation on provider side", async () => {
        const testArgument = "This is my test argument";

        await abstractTest.callOperation(
            "methodProvidedImpl",
            {
                arg: testArgument
            },
            {
                returnValue: testArgument
            }
        );
    });

    afterAll(abstractTest.afterEach);
});
