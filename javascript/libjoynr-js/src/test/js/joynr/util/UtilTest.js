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
const Promise = require("../../../../main/js/global/Promise");
const Util = require("../../../../main/js/joynr/util/UtilInternal");
const JoynrMessage = require("../../../../main/js/joynr/messaging/JoynrMessage");
const TypeRegistry = require("../../../../main/js/joynr/start/TypeRegistry");
const RadioStation = require("../../../generated/joynr/vehicle/radiotypes/RadioStation");

const argument = {
    someObjectKey: "andValue"
};

describe("libjoynr-js.joynr.Util", () => {
    it("is defined and of correct type", () => {
        expect(Util).toBeDefined();
        expect(Util).not.toBeNull();
        expect(typeof Util === "object").toBeTruthy();
    });
});

describe("libjoynr-js.joynr.Util.extend", () => {
    it("extends objects", () => {
        let merged, message, subobject, object1, object2, object3;
        merged = {};

        message = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        message.payload = {
            payload1: 1,
            payload2: 2
        };
        subobject = {
            sublevel20: "sublevel20",
            sublevel21: "sublevel21"
        };

        object1 = {
            originalField: "originalField"
        };
        object2 = {
            message,
            number: 2.0,
            array: [1, 2, 3, 4, 5],
            string: "string",
            bool: true
        };
        object3 = {
            level11: "level11",
            level12: subobject
        };

        Util.extend(merged, object1, object2, object3);

        expect(merged.originalField).toEqual("originalField");
        expect(merged.message).toEqual(message);
        expect(merged.level11).toEqual(object3.level11);
        expect(merged.level12).toEqual(subobject);

        expect(typeof merged.number === "number").toBeTruthy();
        expect(Array.isArray(merged.array)).toBeTruthy();
        expect(typeof merged.string === "string").toBeTruthy();
        expect(typeof merged.bool === "boolean").toBeTruthy();
    });
    it("deep extends objects", () => {
        let merged, from;
        merged = {};

        from = {
            subobject: {
                number: 2.0,
                array: [0, 1, 2, 3, 4],
                string: "string",
                bool: true
            }
        };

        Util.extendDeep(merged, from);

        delete from.subobject.number;
        delete from.subobject.array;
        delete from.subobject.string;
        delete from.subobject.bool;

        expect(typeof merged.subobject.number === "number").toBeTruthy();
        expect(merged.subobject.number).toEqual(2.0);

        expect(Array.isArray(merged.subobject.array)).toBeTruthy();
        expect(merged.subobject.array[0]).toEqual(0);
        expect(merged.subobject.array[1]).toEqual(1);
        expect(merged.subobject.array[2]).toEqual(2);
        expect(merged.subobject.array[3]).toEqual(3);
        expect(merged.subobject.array[4]).toEqual(4);

        expect(typeof merged.subobject.string === "string").toBeTruthy();
        expect(merged.subobject.string).toEqual("string");

        expect(typeof merged.subobject.bool === "boolean").toBeTruthy();
        expect(merged.subobject.bool).toEqual(true);
    });
});

describe("libjoynr-js.joynr.Util.transform", () => {
    it("transform array", () => {
        let origin, element, transformed, postFix;
        postFix = "_transformed";
        origin = [];
        element = {
            a: "a"
        };
        origin.push(element);

        // now, let's transform
        transformed = Util.transform(origin, (element, key) => {
            let id,
                member,
                result = {};
            for (id in element) {
                if (element.hasOwnProperty(id)) {
                    member = element[id];
                    result[id] = member + postFix;
                }
            }
            return result;
        });

        expect(transformed.length).toEqual(1);
        expect(transformed[0].a).toEqual("a" + postFix);
    });
});

describe("libjoynr-js.joynr.Util.firstLower", () => {
    it("decapitalizes first character correctly", () => {
        expect(Util.firstLower("")).toEqual("");
        expect(Util.firstLower("asdf")).toEqual("asdf");
        expect(Util.firstLower("b")).toEqual("b");
        expect(Util.firstLower("Csdf")).toEqual("csdf");
        expect(Util.firstLower("D")).toEqual("d");
        expect(Util.firstLower("ESDFASDF")).toEqual("eSDFASDF");
        expect(Util.firstLower("FsDfAsDf")).toEqual("fsDfAsDf");
        const rettyLongString =
            "RETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRING";
        expect(Util.firstLower("P" + rettyLongString)).toEqual("p" + rettyLongString);
    });

    it("throws on nullable input", () => {
        expect(() => {
            Util.firstLower(null);
        }).toThrow();
        expect(() => {
            Util.firstLower(undefined);
        }).toThrow();
    });
});

describe("libjoynr-js.joynr.Util.firstUpper", () => {
    it("capitalizes first character correctly", () => {
        expect(Util.firstUpper("")).toEqual("");
        expect(Util.firstUpper("asdf")).toEqual("Asdf");
        expect(Util.firstUpper("b")).toEqual("B");
        expect(Util.firstUpper("Csdf")).toEqual("Csdf");
        expect(Util.firstUpper("D")).toEqual("D");
        expect(Util.firstUpper("esdfasdf")).toEqual("Esdfasdf");
        expect(Util.firstUpper("fSdFaSdF")).toEqual("FSdFaSdF");
        const rettyLongString =
            "rettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstring";
        expect(Util.firstUpper("p" + rettyLongString)).toEqual("P" + rettyLongString);
    });

    it("throws on nullable input", () => {
        expect(() => {
            Util.firstLower(null);
        }).toThrow();
        expect(() => {
            Util.firstLower(undefined);
        }).toThrow();
    });
});

describe("libjoynr-js.joynr.Util.isPromise", () => {
    it("returns only true if param is promis", () => {
        expect(Util.isPromise(Promise.resolve())).toBe(true);
        expect(Util.isPromise("")).toBe(false);
        expect(Util.isPromise(true)).toBe(false);
        expect(Util.isPromise()).toBe(false);
    });
});

describe("libjoynr-js.joynr.Util.ensureTypedValues", () => {
    const typeRegistry = new TypeRegistry();
    typeRegistry.addType("joynr.vehicle.radiotypes.RadioStation", RadioStation);

    it("types untyped objects", () => {
        let returnValue = null;
        const untypedValue = {
            name: "radioStationName",
            _typeName: "joynr.vehicle.radiotypes.RadioStation"
        };

        returnValue = Util.ensureTypedValues(untypedValue, typeRegistry);
        expect(returnValue instanceof RadioStation).toBe(true);
        expect(returnValue.name === untypedValue.name).toBe(true);
    });

    it("types untyped arrays", () => {
        let returnValue = null;
        const untypedArray = [
            {
                name: "radioStationName1",
                _typeName: "joynr.vehicle.radiotypes.RadioStation"
            },
            {
                name: "radioStationName2",
                _typeName: "joynr.vehicle.radiotypes.RadioStation"
            }
        ];

        returnValue = Util.ensureTypedValues(untypedArray, typeRegistry);
        expect(returnValue[0] instanceof RadioStation).toBe(true);
        expect(returnValue[1] instanceof RadioStation).toBe(true);
        expect(returnValue[0].name === untypedArray[0].name).toBe(true);
        expect(returnValue[1].name === untypedArray[1].name).toBe(true);
    });

    it("accepts primitive types", () => {
        let returnValue = null;
        const numberValue = 1;
        const booleanValue = true;
        const stringValue = "string";

        returnValue = Util.ensureTypedValues(numberValue, typeRegistry);
        expect(typeof returnValue === "number").toBe(true);

        returnValue = Util.ensureTypedValues(booleanValue, typeRegistry);
        expect(typeof returnValue === "boolean").toBe(true);

        returnValue = Util.ensureTypedValues(stringValue, typeRegistry);
        expect(typeof returnValue === "string").toBe(true);
    });
});

describe("libjoynr-js.joynr.Util.forward", () => {
    it("forwards methods to another object", () => {
        const receiver = {};
        const producer = {
            someFunction() {
                return "some result";
            },
            someObject: "some Value"
        };

        const augmentedReceiver = Util.forward(receiver, producer);

        expect(typeof augmentedReceiver.someFunction).toBe("function");
        expect(augmentedReceiver.someObject).toBeUndefined();
    });
});

describe("libjoynr-js.joynr.Util.forwardPrototype", () => {
    it("forwards methods to another object", () => {
        const someValue = "some Value";
        const ObjectWithPrivateVars = function() {
            this.someObject = someValue;
            return Util.forwardPrototype(this);
        };
        ObjectWithPrivateVars.prototype.someFunction = function() {
            return this.someObject;
        };
        const privateObject = new ObjectWithPrivateVars();

        expect(typeof privateObject.someFunction).toBe("function");
        expect(privateObject.someObject).toBeUndefined();
        expect(privateObject.someFunction()).toBe(someValue);
    });
});

describe("libjoynr-js.joynr.Util.timeoutPromise", () => {
    beforeEach(() => {
        jasmine.clock().install();
    });
    afterEach(() => {
        jasmine.clock().uninstall();
    });
    it("resolves Promise normally when Promise finished before timeout", done => {
        const promise = new Promise((resolve, reject) => {
            setTimeout(resolve, 100);
        });
        Util.timeoutPromise(promise, 200)
            .then(done)
            .catch(fail);
        jasmine.clock().tick(101);
        jasmine.clock().tick(100);
    });

    it("timeouts after before the promise resolves", done => {
        const promise = new Promise((resolve, reject) => {
            setTimeout(resolve, 200);
        });
        Util.timeoutPromise(promise, 100)
            .then(fail)
            .catch(done);
        jasmine.clock().tick(101);
        jasmine.clock().tick(100);
    });
});

describe("libjoynr-js.joynr.Util.createDeferred", () => {
    it("create a correct Deferred Object", done => {
        const deferred = Util.createDeferred();
        deferred.resolve();
        deferred.promise.then(done).catch(fail);
    });
});
