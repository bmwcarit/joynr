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

(function() {
    var setupData = function(ComplexRadioStation, Country) {
        var TestEnd2EndDatatypesTestData = [
            {
                attribute : "booleanAttribute",
                jsRuntimeType : "Boolean",
                joynrType : "Boolean",
                values : [
                    true,
                    false,
                    true
                ]
            },
            {
                attribute : "int8Attribute",
                jsRuntimeType : "Number",
                joynrType : "Byte",
                values : [
                    Math.pow(2, 7) - 1,
                    0,
                    -Math.pow(2, 7)
                ]
            },
            {
                attribute : "uint8Attribute",
                joynrType : "Byte",
                jsRuntimeType : "Number",
                values : [
                    Math.pow(2, 8) - 1,
                    Math.pow(2, 7),
                    0
                ]
            },
            {
                attribute : "int16Attribute",
                joynrType : "Short",
                jsRuntimeType : "Number",
                values : [
                    Math.pow(2, 15) - 1,
                    0,
                    -Math.pow(2, 15)
                ]
            },
            {
                attribute : "uint16Attribute",
                joynrType : "Short",
                jsRuntimeType : "Number",
                values : [
                    Math.pow(2, 16) - 1,
                    Math.pow(2, 15),
                    0
                ]
            },
            {
                attribute : "int32Attribute",
                joynrType : "Integer",
                jsRuntimeType : "Number",
                values : [
                    Math.pow(2, 31) - 1,
                    0,
                    -Math.pow(2, 31)
                ]
            },
            {
                attribute : "uint32Attribute",
                joynrType : "Integer",
                jsRuntimeType : "Number",
                values : [
                    Math.pow(2, 32) - 1,
                    Math.pow(2, 31),
                    0
                ]
            },
            {
                attribute : "int64Attribute",
                joynrType : "Long",
                jsRuntimeType : "Number",
                values : [
                    Math.pow(2, 63) - 1,
                    0,
                    -Math.pow(2, 63)
                ]
            },
            {
                attribute : "uint64Attribute",
                joynrType : "Long",
                jsRuntimeType : "Number",
                values : [
                    Math.pow(2, 64) - 1,
                    Math.pow(2, 63),
                    0
                ]
            },
            {
                attribute : "floatAttribute",
                joynrType : "Float",
                jsRuntimeType : "Number",
                values : [
                    -123.456789,
                    0,
                    123.456789
                ]
            },
            {
                attribute : "doubleAttribute",
                joynrType : "Double",
                jsRuntimeType : "Number",
                values : [
                    -123.456789,
                    0,
                    123.456789
                ]
            },
            {
                attribute : "stringAttribute",
                joynrType : "String",
                jsRuntimeType : "String",
                values : [
                    "a string",
                    "",
                    "another string"
                ]
            },
            {
                attribute : "stringArrayAttribute",
                joynrType : "List",
                jsRuntimeType : "Array",
                values : [
                    [
                        "1",
                        "2",
                        "string"
                    ],
                    [
                        "3",
                        "4",
                        "another string"
                    ],
                    [
                        "456",
                        "789",
                        "last string"
                    ]
                ]
            },
            {
                attribute : "structAttribute",
                joynrType : "joynr.datatypes.exampleTypes.ComplexRadioStation",
                jsRuntimeType : "ComplexRadioStation",
                values : [
                    new ComplexRadioStation({
                        name : "name",
                        station : "station",
                        source : Country.AUSTRALIA
                    }),
                    new ComplexRadioStation({
                        name : "",
                        station : "",
                        source : Country.AUSTRIA
                    }),
                    new ComplexRadioStation({
                        name : "another name",
                        station : "other station",
                        source : Country.GERMANY
                    })
                ]
            }
        ];
        return TestEnd2EndDatatypesTestData;
    };

    // AMD support
    if (typeof define === 'function' && define.amd) {
        define("integration/TestEnd2EndDatatypesTestData", [
            "joynr/datatypes/exampleTypes/ComplexRadioStation",
            "joynr/datatypes/exampleTypes/Country"
        ], function(ComplexRadioStation, Country) {
            return setupData(ComplexRadioStation, Country);
        });
    } else {
        window.TestEnd2EndDatatypesTestData = setupData(window.ComplexRadioStation, window.Country);
    }
}());