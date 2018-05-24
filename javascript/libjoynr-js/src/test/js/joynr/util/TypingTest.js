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
require("../../node-unit-test-helper");
const Typing = require("../../../../main/js/joynr/util/Typing");
const LoggingManager = require("../../../../main/js/joynr/system/LoggingManager");
const TypeRegistrySingleton = require("../../../../main/js/joynr/types/TypeRegistrySingleton");
const DiscoveryEntry = require("../../../../main/js/generated/joynr/types/DiscoveryEntry");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const ProviderScope = require("../../../../main/js/generated/joynr/types/ProviderScope");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const RadioStation = require("../../../generated/joynr/vehicle/radiotypes/RadioStation");
const ComplexRadioStation = require("../../../generated/joynr/datatypes/exampleTypes/ComplexRadioStation");
const ComplexStruct = require("../../../generated/joynr/datatypes/exampleTypes/ComplexStruct");
const Country = require("../../../generated/joynr/datatypes/exampleTypes/Country");
const TestEnum = require("../../../generated/joynr/tests/testTypes/TestEnum");
const Promise = require("../../../../main/js/global/Promise");

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

MyType.getMemberType = function() {};

function MySecondType(a, b, c, d, e) {
    this._typeName = "MySecondTypeName";
    this.a = a;
    this.b = b;
    this.c = c;
    this.d = d;
    this.e = e;
}

MySecondType.getMemberType = function() {};

beforeEach(done => {
    const datatypePromises = [
        "joynr.vehicle.radiotypes.RadioStation",
        "joynr.datatypes.exampleTypes.ComplexRadioStation",
        "joynr.datatypes.exampleTypes.ComplexStruct",
        "joynr.datatypes.exampleTypes.Country",
        "joynr.tests.testTypes.TestEnum"
    ].map(datatype => {
        return TypeRegistrySingleton.getInstance().getTypeRegisteredPromise(datatype, 1000);
    });

    Promise.all(datatypePromises)
        .then(() => {
            done();
            return null;
        })
        .catch(fail);
});

describe("libjoynr-js.joynr.Typing", () => {
    it("is defined and of correct type", done => {
        expect(Typing).toBeDefined();
        expect(Typing).not.toBeNull();
        expect(typeof Typing === "object").toBeTruthy();
        done();
    });
});

describe("libjoynr-js.joynr.Typing.getObjectType", () => {
    it("returns the correct type strings", done => {
        expect(Typing.getObjectType(true)).toEqual("Boolean");
        expect(Typing.getObjectType(false)).toEqual("Boolean");

        expect(Typing.getObjectType(-123)).toEqual("Number");
        expect(Typing.getObjectType(0)).toEqual("Number");
        expect(Typing.getObjectType(123)).toEqual("Number");
        expect(Typing.getObjectType(1.2345678)).toEqual("Number");

        expect(Typing.getObjectType("a string")).toEqual("String");

        expect(Typing.getObjectType(() => {})).toEqual("Function");

        expect(Typing.getObjectType(new MyCustomObj())).toEqual("MyCustomObj");
        expect(Typing.getObjectType(new _TestConstructor123_())).toEqual("_TestConstructor123_");

        expect(Typing.getObjectType("a String")).toEqual("String");

        expect(Typing.getObjectType([])).toEqual("Array");
        expect(Typing.getObjectType({})).toEqual("Object");
        done();
    });

    it("throws if no object is provided", done => {
        expect(() => {
            Typing.getObjectType();
        }).toThrow();
        expect(() => {
            Typing.getObjectType(null);
        }).toThrow();
        expect(() => {
            Typing.getObjectType(undefined);
        }).toThrow();
        done();
    });
});

describe("libjoynr-js.joynr.Typing.augmentType", () => {
    const log = LoggingManager.getLogger("joynr.util.TypingTest");

    const tests = [
        {
            untyped: null,
            typed: null
        },
        {
            untyped: undefined,
            typed: undefined
        },
        {
            untyped: true,
            typed: true
        },
        {
            untyped: false,
            typed: false
        },
        {
            untyped: 12345,
            typed: 12345
        },
        {
            untyped: 0,
            typed: 0
        },
        {
            untyped: -12345,
            typed: -12345
        },
        {
            untyped: 1.23456789,
            typed: 1.23456789
        },
        {
            untyped: "123456",
            typed: "123456"
        },
        {
            untyped: "-123456",
            typed: "-123456"
        },
        {
            untyped: "1.23456789",
            typed: "1.23456789"
        },
        {
            untyped: "myString",
            typed: "myString"
        },
        {
            untyped: {
                a: 1,
                b: 2,
                c: 3,
                d: 4,
                e: 5,
                _typeName: "MyTypeName"
            },
            typed: new MyType(1, 2, 3, 4, 5)
        },
        {
            untyped: {
                a: 1,
                b: 2,
                c: 3,
                d: 4,
                e: 5,
                _typeName: "MySecondTypeName"
            },
            typed: new MySecondType(1, 2, 3, 4, 5)
        },
        {
            untyped: {
                a: 1,
                b: [1, 2, 3],
                c: "3",
                d: "asdf",
                _typeName: "MyTypeName"
            },
            typed: new MyType(1, [1, 2, 3], "3", "asdf")
        },
        {
            untyped: {
                a: 1,
                b: [1, 2, 3],
                c: "3",
                d: "asdf",
                _typeName: "MySecondTypeName"
            },
            typed: new MySecondType(1, [1, 2, 3], "3", "asdf")
        },
        {
            untyped: {
                a: 1,
                b: {
                    a: 1,
                    b: {
                        a: 1,
                        b: 2,
                        c: 3,
                        d: 4,
                        e: 5,
                        _typeName: "MyTypeName"
                    },
                    c: 3,
                    d: {
                        a: 1,
                        b: 2,
                        c: 3,
                        d: 4,
                        e: 5,
                        _typeName: "MyTypeName"
                    },
                    e: 5,
                    _typeName: "MySecondTypeName"
                },
                c: 3,
                d: {
                    a: 1,
                    b: {
                        a: 1,
                        b: 2,
                        c: 3,
                        d: 4,
                        e: 5,
                        _typeName: "MyTypeName"
                    },
                    c: 3,
                    d: {
                        a: 1,
                        b: 2,
                        c: 3,
                        d: 4,
                        e: 5,
                        _typeName: "MyTypeName"
                    },
                    e: 5,
                    _typeName: "MySecondTypeName"
                },
                e: 5,
                _typeName: "MyTypeName"
            },
            typed: new MyType(
                1,
                new MySecondType(1, new MyType(1, 2, 3, 4, 5), 3, new MyType(1, 2, 3, 4, 5), 5),
                3,
                new MySecondType(1, new MyType(1, 2, 3, 4, 5), 3, new MyType(1, 2, 3, 4, 5), 5),
                5
            )
        }
    ];

    it("types all objects correctly", done => {
        const typeRegistry = new TypeRegistrySingleton.getInstance();
        typeRegistry.addType("MyTypeName", MyType);
        typeRegistry.addType("MySecondTypeName", MySecondType);

        let i, typed;
        for (i = 0; i < tests.length; ++i) {
            typed = Typing.augmentTypes(tests[i].untyped);
            expect(typed).toEqual(tests[i].typed);
            if (tests[i].untyped) {
                // filter out undefined and null
                expect(Typing.getObjectType(typed)).toEqual(Typing.getObjectType(tests[i].typed));
            }
        }
        done();
    });

    xit("performance measurement of augmenting struct types", () => {
        const times = 5000;
        const typeRegistry = TypeRegistrySingleton.getInstance();
        typeRegistry.addType("joynr.datatypes.exampleTypes.ComplexStruct", ComplexStruct);
        const rawInput = {
            _typeName: "joynr.datatypes.exampleTypes.ComplexStruct",
            num32: "123456",
            num64: "123456789",
            str: "looooooooooooooooooooooooooooooooooooooongStriiiiiiiiiiiiiiiiiiiiiiiing",
            data: []
        };
        for (let i = 0; i < 1000; i++) {
            rawInput.data.push("0");
        }

        const timeStart = Date.now();
        for (let i = 0; i < times; i++) {
            Typing.augmentTypes(rawInput);
        }
        const delta = Date.now() - timeStart;
        log.info('Time took for augmenting struct type "ComplexStruct"' + times + " times: " + delta + "ms");
    });

    it("throws when giving a function or an object with a custom type", done => {
        expect(() => {
            Typing.augmentTypes(() => {});
        }).toThrow();

        expect(() => {
            Typing.augmentTypes(new MyType());
        }).toThrow();

        expect(() => {
            Typing.augmentTypes(new MySecondType());
        }).toThrow();
        done();
    });

    it("augmentTypes is able to deal with enums as input", done => {
        const fixture = "ZERO";
        const expected = TestEnum.ZERO;
        expect(Typing.augmentTypes(fixture)).toBe(fixture);
        expect(Typing.augmentTypes(fixture, "joynr.tests.testTypes.TestEnum")).toBe(expected);
        done();
    });

    it("augmentTypes is able to deal with error enums as input", () => {
        const fixture = {
            _typeName: "joynr.tests.testTypes.TestEnum",
            name: "ZERO"
        };
        const expected = TestEnum.ZERO;
        const result = Typing.augmentTypes(fixture);
        expect(result.name).toBeDefined();
        expect(result.name).toBe(expected.name);
        expect(result.value).toBeDefined();
        expect(result.value).toBe(expected.value);
        expect(result).toBe(expected);
    });

    it("augmentTypes is able to deal with structs containing enum members", done => {
        const fixture = {
            _typeName: "joynr.datatypes.exampleTypes.ComplexRadioStation",
            name: "name",
            station: "station",
            source: "AUSTRIA"
        };
        const expected = new ComplexRadioStation({
            name: fixture.name,
            station: fixture.station,
            source: Country.AUSTRIA
        });
        expect(Typing.augmentTypes(fixture)).toEqual(expected);
        done();
    });

    it("augmentTypes is able to deal with cached structs containing enum members", done => {
        const fixture = {
            _typeName: "joynr.datatypes.exampleTypes.ComplexRadioStation",
            name: "name",
            station: "station",
            source: "AUSTRIA"
        };
        const expected = new ComplexRadioStation({
            name: fixture.name,
            station: fixture.station,
            source: Country.AUSTRIA
        });
        const cachedFixture = JSON.parse(JSON.stringify(Typing.augmentTypes(fixture)));

        expect(Typing.augmentTypes(cachedFixture)).toEqual(expected);
        done();
    });

    it("augmentTypes is able to deal with complex structs containing enum array and other structs as members", done => {
        const providerQos = {
            _typeName: "joynr.types.ProviderQos",
            customParameters: [],
            priority: 234,
            scope: "GLOBAL",
            supportsOnChangeSubscriptions: false
        };
        const providerVersion = {
            _typeName: "joynr.types.Version",
            majorVersion: 1,
            minorVersion: 2
        };

        const fixture = {
            _typeName: "joynr.types.DiscoveryEntry",
            domain: "domain",
            interfaceName: "interfaceName",
            participantId: "participantId",
            qos: providerQos,
            providerVersion,
            lastSeenDateMs: 123,
            publicKeyId: "publicKeyId",
            expiryDateMs: 1234
        };

        const expected = new DiscoveryEntry({
            domain: fixture.domain,
            interfaceName: fixture.interfaceName,
            participantId: fixture.participantId,
            lastSeenDateMs: 123,
            qos: new ProviderQos({
                customParameters: providerQos.customParameters,
                priority: providerQos.priority,
                scope: ProviderScope.GLOBAL,
                supportsOnChangeSubscriptions: providerQos.supportsOnChangeSubscriptions
            }),
            providerVersion: new Version({
                majorVersion: 1,
                minorVersion: 2
            }),
            expiryDateMs: 1234,
            publicKeyId: "publicKeyId"
        });
        expect(Typing.augmentTypes(fixture)).toEqual(expected);
        done();
    });
});

function augmentTypeName(obj, expectedType, customMember) {
    const objWithTypeName = Typing.augmentTypeName(obj, "joynr", customMember);
    expect(objWithTypeName[customMember || "_typeName"]).toEqual("joynr." + expectedType);
}

describe("libjoynr-js.joynr.Typing.augmentTypeName", () => {
    it("augments type into _typeName member", done => {
        augmentTypeName(new MyCustomObj(), "MyCustomObj");
        augmentTypeName(new _TestConstructor123_(), "_TestConstructor123_");
        augmentTypeName(new MyType(), "MyType");
        augmentTypeName(new MySecondType(), "MySecondType");
        done();
    });

    it("augments type into custom member", done => {
        augmentTypeName(new MyCustomObj(), "MyCustomObj", "myCustomMember");
        augmentTypeName(new _TestConstructor123_(), "_TestConstructor123_", "myCustomMember");
        augmentTypeName(new MyType(), "MyType", "myCustomMember");
        augmentTypeName(new MySecondType(), "MySecondType", "myCustomMember");
        done();
    });

    it("throws if no object is provided", done => {
        expect(() => {
            Typing.augmentTypeName();
        }).toThrow();
        expect(() => {
            Typing.augmentTypeName(null);
        }).toThrow();
        expect(() => {
            Typing.augmentTypeName(undefined);
        }).toThrow();
        done();
    });

    it("isEnumType accepts enum types", done => {
        const fixture = TestEnum.ZERO;
        const radioStation = new RadioStation({
            name: "name",
            trafficService: false,
            country: {}
        });
        expect(Typing.isEnumType(fixture)).toBe(true);
        expect(Typing.isEnumType("TestString")).toBe(false);
        expect(Typing.isEnumType(123)).toBe(false);
        expect(Typing.isEnumType(radioStation)).toBe(false);
        done();
    });
});

function testTypingCheckProperty(functionName) {
    function CustomObj() {}
    function AnotherCustomObj() {}
    const objects = [true, 1, "a string", [], {}, function() {}, new CustomObj(), new AnotherCustomObj()];
    const types = ["Boolean", "Number", "String", "Array", "Object", "Function", CustomObj, AnotherCustomObj];

    it("provides the correct type information", () => {
        let i, j;
        function functionBuilder(object, type) {
            return function() {
                Typing[functionName](object, type, "some description");
            };
        }

        for (i = 0; i < objects.length; ++i) {
            for (j = 0; j < types.length; ++j) {
                const test = expect(functionBuilder(objects[i], types[j]));

                if (i === j) {
                    test.not.toThrow();
                } else {
                    test.toThrow();
                }
            }
        }
    });

    it("supports type alternatives", () => {
        const type = ["Object", "CustomObj"];
        expect(() => {
            Typing[functionName]({}, type, "some description");
        }).not.toThrow();
        expect(() => {
            Typing[functionName](new CustomObj(), type, "some description");
        }).not.toThrow();
        expect(() => {
            Typing[functionName](new AnotherCustomObj(), type, "some description");
        }).toThrow();
    });
}

describe("libjoynr-js.joynr.Typing.checkProperty", () => {
    testTypingCheckProperty("checkProperty");

    it("throws on null and undefined", () => {
        expect(() => {
            Typing.checkProperty(undefined, "undefined", "some description");
        }).toThrow();
        expect(() => {
            Typing.checkProperty(null, "null", "some description");
        }).toThrow();
    });
});

describe("libjoynr-js.joynr.Typing.checkPropertyIfDefined", () => {
    testTypingCheckProperty("checkPropertyIfDefined");

    it("does not throw on null or undefined", () => {
        expect(() => {
            Typing.checkPropertyIfDefined(undefined, "undefined", "some description");
        }).not.toThrow();
        expect(() => {
            Typing.checkPropertyIfDefined(null, "null", "some description");
        }).not.toThrow();
    });
});
