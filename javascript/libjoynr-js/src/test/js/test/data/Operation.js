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
    "joynr/tests/testTypes/TestEnum"
], function(RadioStation, TestEnum) {

    var testData = [
        {
            signature : [ {
                name : "parameter",
                type : "String"
            }
            ],
            namedArguments : {
                parameter : "asdf"
            },
            paramDatatypes : [ "String"
            ],
            params : [ "asdf"
            ]
        },
        {
            signature : [ {
                name : "parameter",
                type : "joynr.tests.testTypes.TestEnum"
            }
            ],
            namedArguments : {
                parameter : TestEnum.ZERO
            },
            paramDatatypes : [ "joynr.tests.testTypes.TestEnum"
            ],
            params : [ TestEnum.ZERO
            ]
        },
        {
            signature : [ {
                name : "complex",
                type : "joynr.vehicle.radiotypes.RadioStation"
            }
            ],
            namedArguments : {
                complex : new RadioStation({
                    name : "asdf"
                })
            },
            paramDatatypes : [ "joynr.vehicle.radiotypes.RadioStation"
            ],
            params : [ new RadioStation({
                name : "asdf"
            })
            ]
        },
        {
            signature : [
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
                    type : "List"
                }
            ],
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
                "List"
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
            ]
        }
    ];

    return testData;
});