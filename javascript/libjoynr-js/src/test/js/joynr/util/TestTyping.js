/*jslint newcap: true, nomen: true */

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
        "joynr/Util/TestTyping",
        [
            "joynr/util/Typing",
            "joynr/start/TypeRegistry",
            "joynr/types/TypeRegistrySingleton",
            "joynr/types/DiscoveryEntry",
            "joynr/types/CommunicationMiddleware",
            "joynr/types/ProviderQos",
            "joynr/types/ProviderScope",
            "joynr/vehicle/radiotypes/RadioStation",
            "joynr/datatypes/exampleTypes/ComplexRadioStation",
            "joynr/datatypes/exampleTypes/Country",
            "joynr/tests/testTypes/TestEnum"
        ],
        function(
                Typing,
                TypeRegistry,
                TypeRegistrySingleton,
                DiscoveryEntry,
                CommunicationMiddleware,
                ProviderQos,
                ProviderScope,
                RadioStation,
                ComplexRadioStation,
                Country,
                TestEnum) {

            function MyCustomObj() {}
            function _TestConstructor123_() {}
            function MyType(a, b, c, d, e) {
                this._typeName = "MyTypeName";
                this.a = a;
                this.b = b;
                this.c = c;
                this.d = d;
                this.e = e;
            }

            function MySecondType(a, b, c, d, e) {
                this._typeName = "MySecondTypeName";
                this.a = a;
                this.b = b;
                this.c = c;
                this.d = d;
                this.e = e;
            }

            describe("libjoynr-js.joynr.Typing", function() {
                it("is defined and of correct type", function() {
                    expect(Typing).toBeDefined();
                    expect(Typing).not.toBeNull();
                    expect(typeof Typing === "object").toBeTruthy();
                });
            });

            describe("libjoynr-js.joynr.Typing.getObjectType", function() {
                it("returns the correct type strings", function() {
                    expect(Typing.getObjectType(true)).toEqual("Boolean");
                    expect(Typing.getObjectType(false)).toEqual("Boolean");

                    expect(Typing.getObjectType(-123)).toEqual("Number");
                    expect(Typing.getObjectType(0)).toEqual("Number");
                    expect(Typing.getObjectType(123)).toEqual("Number");
                    expect(Typing.getObjectType(1.2345678)).toEqual("Number");

                    expect(Typing.getObjectType("a string")).toEqual("String");

                    expect(Typing.getObjectType(function() {})).toEqual("Function");

                    expect(Typing.getObjectType(new MyCustomObj())).toEqual("MyCustomObj");
                    expect(Typing.getObjectType(new _TestConstructor123_())).toEqual(
                            "_TestConstructor123_");

                    expect(Typing.getObjectType("a String")).toEqual("String");

                    expect(Typing.getObjectType([])).toEqual("Array");
                    expect(Typing.getObjectType({})).toEqual("Object");
                });

                it("throws if no object is provided", function() {
                    expect(function() {
                        Typing.getObjectType();
                    }).toThrow();
                    expect(function() {
                        Typing.getObjectType(null);
                    }).toThrow();
                    expect(function() {
                        Typing.getObjectType(undefined);
                    }).toThrow();
                });
            });

            describe(
                    "libjoynr-js.joynr.Typing.augmentType",
                    function() {

                        var tests =
                                [
                                    {
                                        untyped : null,
                                        typed : null
                                    },
                                    {
                                        untyped : undefined,
                                        typed : undefined
                                    },
                                    {
                                        untyped : true,
                                        typed : true
                                    },
                                    {
                                        untyped : false,
                                        typed : false
                                    },
                                    {
                                        untyped : 12345,
                                        typed : 12345
                                    },
                                    {
                                        untyped : 0,
                                        typed : 0
                                    },
                                    {
                                        untyped : -12345,
                                        typed : -12345
                                    },
                                    {
                                        untyped : 1.23456789,
                                        typed : 1.23456789
                                    },
                                    {
                                        untyped : "123456",
                                        typed : "123456"
                                    },
                                    {
                                        untyped : "-123456",
                                        typed : "-123456"
                                    },
                                    {
                                        untyped : "1.23456789",
                                        typed : "1.23456789"
                                    },
                                    {
                                        untyped : "myString",
                                        typed : "myString"
                                    },
                                    {
                                        untyped : {
                                            a : 1,
                                            b : 2,
                                            c : 3,
                                            d : 4,
                                            e : 5,
                                            "_typeName" : "MyTypeName"
                                        },
                                        typed : new MyType(1, 2, 3, 4, 5)
                                    },
                                    {
                                        untyped : {
                                            a : 1,
                                            b : 2,
                                            c : 3,
                                            d : 4,
                                            e : 5,
                                            "_typeName" : "MySecondTypeName"
                                        },
                                        typed : new MySecondType(1, 2, 3, 4, 5)
                                    },
                                    {
                                        untyped : {
                                            a : 1,
                                            b : [
                                                1,
                                                2,
                                                3
                                            ],
                                            c : "3",
                                            d : "asdf",
                                            "_typeName" : "MyTypeName"
                                        },
                                        typed : new MyType(1, [
                                            1,
                                            2,
                                            3
                                        ], "3", "asdf")
                                    },
                                    {
                                        untyped : {
                                            a : 1,
                                            b : [
                                                1,
                                                2,
                                                3
                                            ],
                                            c : "3",
                                            d : "asdf",
                                            "_typeName" : "MySecondTypeName"
                                        },
                                        typed : new MySecondType(1, [
                                            1,
                                            2,
                                            3
                                        ], "3", "asdf")
                                    },
                                    {
                                        untyped : {
                                            a : 1,
                                            b : {
                                                a : 1,
                                                b : {
                                                    a : 1,
                                                    b : 2,
                                                    c : 3,
                                                    d : 4,
                                                    e : 5,
                                                    "_typeName" : "MyTypeName"
                                                },
                                                c : 3,
                                                d : {
                                                    a : 1,
                                                    b : 2,
                                                    c : 3,
                                                    d : 4,
                                                    e : 5,
                                                    "_typeName" : "MyTypeName"
                                                },
                                                e : 5,
                                                "_typeName" : "MySecondTypeName"
                                            },
                                            c : 3,
                                            d : {
                                                a : 1,
                                                b : {
                                                    a : 1,
                                                    b : 2,
                                                    c : 3,
                                                    d : 4,
                                                    e : 5,
                                                    "_typeName" : "MyTypeName"
                                                },
                                                c : 3,
                                                d : {
                                                    a : 1,
                                                    b : 2,
                                                    c : 3,
                                                    d : 4,
                                                    e : 5,
                                                    "_typeName" : "MyTypeName"
                                                },
                                                e : 5,
                                                "_typeName" : "MySecondTypeName"
                                            },
                                            e : 5,
                                            "_typeName" : "MyTypeName"
                                        },
                                        typed : new MyType(
                                                1,
                                                new MySecondType(
                                                        1,
                                                        new MyType(1, 2, 3, 4, 5),
                                                        3,
                                                        new MyType(1, 2, 3, 4, 5),
                                                        5),
                                                3,
                                                new MySecondType(
                                                        1,
                                                        new MyType(1, 2, 3, 4, 5),
                                                        3,
                                                        new MyType(1, 2, 3, 4, 5),
                                                        5),
                                                5)
                                    }
                                ];

                        var typeRegistry = new TypeRegistry();
                        typeRegistry.addType("MyTypeName", MyType);
                        typeRegistry.addType("MySecondTypeName", MySecondType);

                        it("types all objects correctly", function() {
                            var i, typed;
                            for (i = 0; i < tests.length; ++i) {
                                typed = Typing.augmentTypes(tests[i].untyped, typeRegistry);
                                expect(typed).toEqual(tests[i].typed);
                                if (tests[i].untyped) { // filter out undefined and null
                                    expect(Typing.getObjectType(typed)).toEqual(
                                            Typing.getObjectType(tests[i].typed));
                                }
                            }
                        });

                        it(
                                "throws when giving a function or an object with a custom type",
                                function() {
                                    expect(function() {
                                        Typing.augmentTypes(function() {}, typeRegistry);
                                    }).toThrow();

                                    expect(function() {
                                        Typing.augmentTypes(new MyType(), typeRegistry);
                                    }).toThrow();

                                    expect(function() {
                                        Typing.augmentTypes(new MySecondType(), typeRegistry);
                                    }).toThrow();
                                });

                        it("augmentTypes is able to deal with enums as input", function() {
                            var fixture, expected;
                            fixture = "ZERO";
                            expected = TestEnum.ZERO;
                            expect(
                                    Typing.augmentTypes(fixture, TypeRegistrySingleton
                                            .getInstance())).toBe(fixture);
                            expect(
                                    Typing.augmentTypes(fixture, TypeRegistrySingleton
                                            .getInstance(), "joynr.tests.testTypes.TestEnum"))
                                    .toBe(expected);
                        });

                        it(
                                "augmentTypes is able to deal with structs containing enum members",
                                function() {
                                    var fixture, expected;
                                    /*jslint nomen: true */
                                    fixture =
                                            {
                                                _typeName : "joynr.datatypes.exampleTypes.ComplexRadioStation",
                                                name : "name",
                                                station : "station",
                                                source : "AUSTRIA"
                                            };
                                    /*jslint nomen: false */
                                    expected = new ComplexRadioStation({
                                        name : fixture.name,
                                        station : fixture.station,
                                        source : Country.AUSTRIA
                                    });
                                    expect(
                                            Typing.augmentTypes(fixture, TypeRegistrySingleton
                                                    .getInstance())).toEqual(expected);
                                });

                        it(
                                "augmentTypes is able to deal with complex structs containing enum array and other structs as members",
                                function() {
                                    var fixture, providerQos, expected;
                                    providerQos = {
                                        _typeName : "joynr.types.ProviderQos",
                                        customParameters : [],
                                        priority : 234,
                                        scope : "GLOBAL",
                                        supportsOnChangeSubscriptions : false
                                    };
                                    /*jslint nomen: true */
                                    fixture = {
                                        _typeName : "joynr.types.DiscoveryEntry",
                                        domain : "domain",
                                        interfaceName : "interfaceName",
                                        participantId : "participantId",
                                        qos : providerQos,
                                        connections : [
                                            "COMMONAPI_DBUS",
                                            "JOYNR"
                                        ]
                                    };
                                    /*jslint nomen: false */

                                    expected =
                                            new DiscoveryEntry(
                                                    {
                                                        domain : fixture.domain,
                                                        interfaceName : fixture.interfaceName,
                                                        participantId : fixture.participantId,
                                                        qos : new ProviderQos(
                                                                {
                                                                    customParameters : providerQos.customParameters,
                                                                    priority : providerQos.priority,
                                                                    scope : ProviderScope.GLOBAL,
                                                                    supportsOnChangeSubscriptions : providerQos.supportsOnChangeSubscriptions
                                                                }),
                                                        connections : [
                                                            CommunicationMiddleware.COMMONAPI_DBUS,
                                                            CommunicationMiddleware.JOYNR
                                                        ]
                                                    });
                                    expect(
                                            Typing.augmentTypes(fixture, TypeRegistrySingleton
                                                    .getInstance())).toEqual(expected);
                                });

                    });

            function augmentTypeName(obj, expectedType, customMember) {
                var objWithTypeName = Typing.augmentTypeName(obj, "joynr", customMember);
                /*jslint nomen: true */
                expect(objWithTypeName[customMember || "_typeName"]).toEqual(
                        "joynr." + expectedType);
                /*jslint nomen: false */
            }

            describe("libjoynr-js.joynr.Typing.augmentTypeName", function() {
                it("augments type into _typeName member", function() {
                    augmentTypeName(new MyCustomObj(), "MyCustomObj");
                    augmentTypeName(new _TestConstructor123_(), "_TestConstructor123_");
                    augmentTypeName(new MyType(), "MyType");
                    augmentTypeName(new MySecondType(), "MySecondType");
                });

                it("augments type into custom member", function() {
                    augmentTypeName(new MyCustomObj(), "MyCustomObj", "myCustomMember");
                    augmentTypeName(
                            new _TestConstructor123_(),
                            "_TestConstructor123_",
                            "myCustomMember");
                    augmentTypeName(new MyType(), "MyType", "myCustomMember");
                    augmentTypeName(new MySecondType(), "MySecondType", "myCustomMember");
                });

                it("throws if no object is provided", function() {
                    expect(function() {
                        Typing.augmentTypeName();
                    }).toThrow();
                    expect(function() {
                        Typing.augmentTypeName(null);
                    }).toThrow();
                    expect(function() {
                        Typing.augmentTypeName(undefined);
                    }).toThrow();
                });

                it("isEnumType accepts enum types", function() {
                    var fixture = TestEnum.ZERO, radioStation;
                    radioStation = new RadioStation({
                        name : "name",
                        trafficService : false,
                        country : {}
                    });
                    expect(Typing.isEnumType(fixture)).toBe(true);
                    expect(Typing.isEnumType("TestString")).toBe(false);
                    expect(Typing.isEnumType(123)).toBe(false);
                    expect(Typing.isEnumType(radioStation)).toBe(false);
                });

            });

        });
