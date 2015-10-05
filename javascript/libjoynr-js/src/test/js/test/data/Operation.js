/*jslint nomen: true */
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

// This data file is used in Test[ProxyProvider]Operation
define("test/data/Operation", [
    "joynr/vehicle/radiotypes/RadioStation",
    "joynr/vehicle/radiotypes/ErrorList",
    "joynr/tests/testTypes/TestEnum"
], function(RadioStation, ErrorList, TestEnum) {
    var radioStationVar = new RadioStation({
        name : "asdf"
    });
    var testData = [
        {
            signature : {
                inputParameter : [ {
                    name : "parameter",
                    type : "String"
                }
                ],
                error : {
                    type : "no error enumeration given"
                },
                outputParameter : []
            },
            namedArguments : {
                parameter : "asdf"
            },
            paramDatatypes : [ "String"
            ],
            params : [ "asdf"
            ],
            returnValue : undefined,
            returnParams : [],
            errorEnumType : "no error enumeration given"
        },
        {
            signature : {
                inputParameter : [ {
                    name : "parameter",
                    type : "joynr.tests.testTypes.TestEnum"
                }
                ],
                outputParameter : []
            },
            namedArguments : {
                parameter : TestEnum.ZERO
            },
            paramDatatypes : [ "joynr.tests.testTypes.TestEnum"
            ],
            params : [ TestEnum.ZERO
            ],
            returnValue : undefined,
            returnParams : [],
            errorEnumType : "no error enumeration given"
        },
        {
            signature : {
                inputParameter : [ {
                    name : "complex",
                    type : "joynr.vehicle.radiotypes.RadioStation"
                }
                ],
                error : {
                    type : ErrorList.EXAMPLE_ERROR_1._typeName
                },
                outputParameter : []
            },
            namedArguments : {
                complex : radioStationVar
            },
            paramDatatypes : [ "joynr.vehicle.radiotypes.RadioStation"
            ],
            params : [ radioStationVar
            ],
            returnValue : undefined,
            returnParams : [],
            error : ErrorList.EXAMPLE_ERROR_1
        },
        {
            signature : {
                inputParameter : [],
                outputParameter : [ {
                    name : "bool",
                    type : "Boolean"
                }
                ]
            },
            namedArguments : {},
            paramDatatypes : [],
            params : [],
            returnValue : true,
            returnParams : [ true
            ]
        },
        {
            signature : {
                inputParameter : [],
                outputParameter : [ {
                    name : "int",
                    type : "Integer"
                }
                ]
            },
            namedArguments : {},
            paramDatatypes : [],
            params : [],
            returnValue : 123,
            returnParams : [ 123
            ]
        },
        {
            signature : {
                inputParameter : [],
                outputParameter : [ {
                    name : "radioStation",
                    type : "joynr.vehicle.radiotypes.RadioStation"
                }
                ]
            },
            namedArguments : {},
            paramDatatypes : [],
            params : [],
            returnValue : radioStationVar,
            returnParams : [ radioStationVar
            ]
        },
        {
            signature : {
                inputParameter : [],
                outputParameter : [
                    {
                        name : "radioStation",
                        type : "joynr.vehicle.radiotypes.RadioStation"
                    },
                    {
                        name : "secondOutputParameter",
                        type : "String"
                    }
                ]
            },
            namedArguments : {},
            paramDatatypes : [],
            params : [],
            returnValue : {
                radioStation : radioStationVar,
                secondOutputParameter : "StringValue"
            },
            returnParams : [
                radioStationVar,
                "StringValue"
            ]
        },
        {
            signature : {
                inputParameter : [
                    {
                        name : "bool",
                        type : "Boolean"
                    },
                    {
                        name : "number",
                        type : "Integer"
                    },
                    {
                        name : "string",
                        type : "String"
                    },
                    {
                        name : "array",
                        type : "Integer[]"
                    }
                ],
                error : {
                    type : "no error enumeration given"
                },
                outputParameter : []
            },
            namedArguments : {
                bool : true,
                string : "asdf",
                number : 1234,
                array : [
                    1,
                    2,
                    3,
                    4
                ]
            },
            paramDatatypes : [
                "Boolean",
                "Integer",
                "String",
                "Integer[]"
            ],
            params : [
                true,
                1234,
                "asdf",
                [
                    1,
                    2,
                    3,
                    4
                ]
            ],
            returnValue : undefined,
            returnParams : [],
            errorEnumType : "no error enumeration given"
        }
    ];

    return testData;
});
