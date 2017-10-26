/*jslint nomen: true, node: true */
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
var RadioStation = require("../../../test-classes/joynr/vehicle/radiotypes/RadioStation");
var ErrorList = require("../../../test-classes/joynr/vehicle/radiotypes/ErrorList");
var TestEnum = require("../../../test-classes/joynr/tests/testTypes/TestEnum");

var radioStationVar = new RadioStation({
    name: "asdf",
    byteBuffer: []
});
var testData = [
    {
        signature: {
            inputParameter: [
                {
                    name: "parameter",
                    type: "String",
                    javascriptType: "string"
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
                    type: "joynr.tests.testTypes.TestEnum",
                    javascriptType: "joynr.tests.testTypes.TestEnum"
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
                    type: "joynr.vehicle.radiotypes.RadioStation",
                    javascriptType: "joynr.vehicle.radiotypes.RadioStation"
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
                    type: "Boolean",
                    javascriptType: "boolean"
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
                    type: "Integer",
                    javascriptType: "number"
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
                    type: "joynr.vehicle.radiotypes.RadioStation",
                    javascriptType: "joynr.vehicle.radiotypes.RadioStation"
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
                    type: "joynr.vehicle.radiotypes.RadioStation",
                    javascriptType: "joynr.vehicle.radiotypes.RadioStation"
                },
                {
                    name: "secondOutputParameter",
                    type: "String",
                    javascriptType: "string"
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
                    type: "Boolean",
                    javascriptType: "boolean"
                },
                {
                    name: "number",
                    type: "Integer",
                    javascriptType: "number"
                },
                {
                    name: "string",
                    type: "String",
                    javascriptType: "string"
                },
                {
                    name: "array",
                    type: "Integer[]",
                    javascriptType: "Array"
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
                    type: "String",
                    javascriptType: "string"
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

module.exports = testData;
