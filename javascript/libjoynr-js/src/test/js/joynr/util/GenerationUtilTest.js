/*jslint es5: true, node: true, nomen: true */
/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
var Util = require("../../../classes/joynr/util/GenerationUtil");

describe("libjoynr-js.joynr.GenerationUtil", function() {
    it("is defined and of correct type", function() {
        expect(Util).toBeDefined();
        expect(Util).not.toBeNull();
        expect(typeof Util === "object").toBeTruthy();
    });

    describe(".addEqualsEnum", function() {
        var EnumObject, enumObject, comparatorObject;

        beforeEach(function() {
            EnumObject = function(settings) {
                this._typeName = "EnumJoynrObject";
                var key;
                for (key in settings) {
                    if (settings.hasOwnProperty(key)) {
                        this[key] = settings[key];
                    }
                }
            };
            Util.addEqualsEnum(EnumObject);
            enumObject = new EnumObject({ name: "name", value: "value" });
        });

        it("adds equals function to prototype of compoundJoynrObject", function() {
            expect(typeof EnumObject.prototype.equals === "function");
        });

        it("returns false when the other Object is undefined or null", function() {
            enumObject = new EnumObject();
            expect(enumObject.equals(undefined)).toBeFalsy();
            expect(enumObject.equals(null)).toBeFalsy();
        });

        it("returns false when the _typeName is not set", function() {
            comparatorObject = new EnumObject({ name: "name", value: "value" });
            delete comparatorObject._typeName;
            expect(enumObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns true when name and value are the same", function() {
            comparatorObject = new EnumObject({ name: "name", value: "value" });
            expect(enumObject.equals(comparatorObject)).toBeTruthy();
        });

        it("returns false when name or value are different", function() {
            comparatorObject = new EnumObject({ name: "otherName", value: "value" });
            expect(enumObject.equals(comparatorObject)).toBeFalsy();

            comparatorObject = new EnumObject({ name: "name", value: "otherValue" });
            expect(enumObject.equals(comparatorObject)).toBeFalsy();
        });
    });

    describe(".addEqualsCompound", function() {
        var CompoundJoynrObject, compoundObject, comparatorObject;

        beforeEach(function() {
            CompoundJoynrObject = function(settings) {
                this._typeName = "CompoundJoynrObject";
                var key;
                for (key in settings) {
                    if (settings.hasOwnProperty(key)) {
                        this[key] = settings[key];
                    }
                }
            };
            Util.addEqualsCompound(CompoundJoynrObject);
            comparatorObject = { _typeName: "CompoundJoynrObject" };
        });

        it("adds equals function to prototype of compoundJoynrObject", function() {
            expect(typeof CompoundJoynrObject.prototype.equals === "function");
        });

        it("returns false when the other Object is undefined or null", function() {
            compoundObject = new CompoundJoynrObject();
            expect(compoundObject.equals(undefined)).toBeFalsy();
            expect(compoundObject.equals(null)).toBeFalsy();
        });

        it("returns true for simple identical Objects", function() {
            compoundObject = new CompoundJoynrObject({ key1: "key1" });
            comparatorObject = new CompoundJoynrObject({ key1: "key1" });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
        });

        it("returns false when the _typeName is not set", function() {
            comparatorObject = new CompoundJoynrObject({ name: "name", value: "value" });
            delete comparatorObject._typeName;
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns false when the second Object has additional keys", function() {
            compoundObject = new CompoundJoynrObject({ key1: "key1" });
            comparatorObject = new CompoundJoynrObject({ key1: "key1", key2: "key2" });
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns false when array keys are different", function() {
            compoundObject = new CompoundJoynrObject({ key1: [1, 2] });
            comparatorObject = new CompoundJoynrObject({ key1: [1, 3] });
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
            comparatorObject = new CompoundJoynrObject({ key1: [1, 2, 3] });
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns true when array keys are identical", function() {
            compoundObject = new CompoundJoynrObject({ key1: [1, 2] });
            comparatorObject = new CompoundJoynrObject({ key1: [1, 2] });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
        });

        it("calls equals when the childObject has an equals function", function() {
            var equalsSpy = jasmine.createSpy("equalsSpy").and.returnValue(true);
            var otherMember = { someData: "data" };
            compoundObject = new CompoundJoynrObject({ key1: { equals: equalsSpy } });
            comparatorObject = new CompoundJoynrObject({ key1: otherMember });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
            expect(equalsSpy).toHaveBeenCalledWith(otherMember);
        });

        it("calls equals when the array elements have an equals function", function() {
            var equalsSpy = jasmine.createSpy("equalsSpy").and.returnValue(true);
            var otherMember = { someData: "data" };
            compoundObject = new CompoundJoynrObject({ key1: [{ equals: equalsSpy }] });
            comparatorObject = new CompoundJoynrObject({ key1: [otherMember] });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
            expect(equalsSpy).toHaveBeenCalledWith(otherMember);
        });
    });
});
