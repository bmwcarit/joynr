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
// This data file is used in Test[ProxyProvider]Operation
import RadioStation from "../../../generated/joynr/vehicle/radiotypes/RadioStation";
import ErrorList from "../../../generated/joynr/vehicle/radiotypes/ErrorList";
import TestEnum from "../../../generated/joynr/tests/testTypes/TestEnum";

const radioStationVar = new RadioStation({
    name: "asdf",
    byteBuffer: []
});
const testData = [
    {
        signature: {
            inputParameter: [
                {
                    name: "parameter",
                    type: "String"
                }
            ],
            error: {
                type: "no error enumeration given"
            },
            outputParameter: [],
            fireAndForget: false
        },
        namedArguments: {
            parameter: "asdf"
        },
        paramDatatypes: ["String"],
        params: ["asdf"],
        returnValue: undefined,
        returnParams: [],
        errorEnumType: "no error enumeration given"
    },
    {
        signature: {
            inputParameter: [
                {
                    name: "parameter",
                    type: "joynr.tests.testTypes.TestEnum"
                }
            ],
            outputParameter: [],
            fireAndForget: false
        },
        namedArguments: {
            parameter: TestEnum.ZERO
        },
        paramDatatypes: ["joynr.tests.testTypes.TestEnum"],
        params: [TestEnum.ZERO],
        returnValue: undefined,
        returnParams: [],
        errorEnumType: "no error enumeration given"
    },
    {
        signature: {
            inputParameter: [
                {
                    name: "complex",
                    type: "joynr.vehicle.radiotypes.RadioStation"
                }
            ],
            error: {
                type: ErrorList.EXAMPLE_ERROR_1._typeName
            },
            outputParameter: [],
            fireAndForget: false
        },
        namedArguments: {
            complex: radioStationVar
        },
        paramDatatypes: ["joynr.vehicle.radiotypes.RadioStation"],
        params: [radioStationVar],
        returnValue: undefined,
        returnParams: [],
        error: ErrorList.EXAMPLE_ERROR_1
    },
    {
        signature: {
            inputParameter: [],
            outputParameter: [
                {
                    name: "bool",
                    type: "Boolean"
                }
            ],
            fireAndForget: false
        },
        namedArguments: {},
        paramDatatypes: [],
        params: [],
        returnValue: {
            bool: true
        },
        returnParams: [true]
    },
    {
        signature: {
            inputParameter: [],
            outputParameter: [
                {
                    name: "int",
                    type: "Integer"
                }
            ],
            fireAndForget: false
        },
        namedArguments: {},
        paramDatatypes: [],
        params: [],
        returnValue: {
            int: 123
        },
        returnParams: [123]
    },
    {
        signature: {
            inputParameter: [],
            outputParameter: [
                {
                    name: "radioStation",
                    type: "joynr.vehicle.radiotypes.RadioStation"
                }
            ],
            fireAndForget: false
        },
        namedArguments: {},
        paramDatatypes: [],
        params: [],
        returnValue: {
            radioStation: radioStationVar
        },
        returnParams: [radioStationVar]
    },
    {
        signature: {
            inputParameter: [],
            outputParameter: [
                {
                    name: "radioStation",
                    type: "joynr.vehicle.radiotypes.RadioStation"
                },
                {
                    name: "secondOutputParameter",
                    type: "String"
                }
            ],
            fireAndForget: false
        },
        namedArguments: {},
        paramDatatypes: [],
        params: [],
        returnValue: {
            radioStation: radioStationVar,
            secondOutputParameter: "StringValue"
        },
        returnParams: [radioStationVar, "StringValue"]
    },
    {
        signature: {
            inputParameter: [
                {
                    name: "bool",
                    type: "Boolean"
                },
                {
                    name: "number",
                    type: "Integer"
                },
                {
                    name: "string",
                    type: "String"
                },
                {
                    name: "array",
                    type: "Integer[]"
                }
            ],
            error: {
                type: "no error enumeration given"
            },
            outputParameter: [],
            fireAndForget: false
        },
        namedArguments: {
            bool: true,
            string: "asdf",
            number: 1234,
            array: [1, 2, 3, 4]
        },
        paramDatatypes: ["Boolean", "Integer", "String", "Integer[]"],
        params: [true, 1234, "asdf", [1, 2, 3, 4]],
        returnValue: undefined,
        returnParams: [],
        errorEnumType: "no error enumeration given"
    },
    {
        signature: {
            inputParameter: [
                {
                    name: "parameter",
                    type: "String"
                }
            ],
            error: {
                type: "no error enumeration given"
            },
            outputParameter: [],
            fireAndForget: true
        },
        namedArguments: {
            parameter: "asdf"
        },
        paramDatatypes: ["String"],
        params: ["asdf"],
        returnValue: undefined,
        returnParams: [],
        errorEnumType: "no error enumeration given"
    }
];

export = testData;
