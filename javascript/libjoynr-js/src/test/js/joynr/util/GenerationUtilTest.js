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
require("../../node-unit-test-helper");
const Util = require("../../../../main/js/joynr/util/GenerationUtil");

describe("libjoynr-js.joynr.GenerationUtil", () => {
    it("is defined and of correct type", () => {
        expect(Util).toBeDefined();
        expect(Util).not.toBeNull();
        expect(typeof Util === "object").toBeTruthy();
    });

    describe(".addEqualsEnum", () => {
        let EnumObject, enumObject, comparatorObject;

        beforeEach(() => {
            EnumObject = function(settings) {
                this._typeName = "EnumJoynrObject";
                let key;
                for (key in settings) {
                    if (settings.hasOwnProperty(key)) {
                        this[key] = settings[key];
                    }
                }
            };
            Util.addEqualsEnum(EnumObject);
            enumObject = new EnumObject({ name: "name", value: "value" });
        });

        it("adds equals function to prototype of compoundJoynrObject", () => {
            expect(typeof EnumObject.prototype.equals === "function");
        });

        it("returns false when the other Object is undefined or null", () => {
            enumObject = new EnumObject();
            expect(enumObject.equals(undefined)).toBeFalsy();
            expect(enumObject.equals(null)).toBeFalsy();
        });

        it("returns false when the _typeName is not set", () => {
            comparatorObject = new EnumObject({ name: "name", value: "value" });
            delete comparatorObject._typeName;
            expect(enumObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns true when name and value are the same", () => {
            comparatorObject = new EnumObject({ name: "name", value: "value" });
            expect(enumObject.equals(comparatorObject)).toBeTruthy();
        });

        it("returns false when name or value are different", () => {
            comparatorObject = new EnumObject({ name: "otherName", value: "value" });
            expect(enumObject.equals(comparatorObject)).toBeFalsy();

            comparatorObject = new EnumObject({ name: "name", value: "otherValue" });
            expect(enumObject.equals(comparatorObject)).toBeFalsy();
        });
    });

    describe(".addEqualsCompound", () => {
        let CompoundJoynrObject, compoundObject, comparatorObject;

        beforeEach(() => {
            CompoundJoynrObject = function(settings) {
                this._typeName = "CompoundJoynrObject";
                let key;
                for (key in settings) {
                    if (settings.hasOwnProperty(key)) {
                        this[key] = settings[key];
                    }
                }
            };
            Util.addEqualsCompound(CompoundJoynrObject);
            comparatorObject = { _typeName: "CompoundJoynrObject" };
        });

        it("adds equals function to prototype of compoundJoynrObject", () => {
            expect(typeof CompoundJoynrObject.prototype.equals === "function");
        });

        it("returns false when the other Object is undefined or null", () => {
            compoundObject = new CompoundJoynrObject();
            expect(compoundObject.equals(undefined)).toBeFalsy();
            expect(compoundObject.equals(null)).toBeFalsy();
        });

        it("returns true for simple identical Objects", () => {
            compoundObject = new CompoundJoynrObject({ key1: "key1" });
            comparatorObject = new CompoundJoynrObject({ key1: "key1" });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
        });

        it("returns false when the _typeName is not set", () => {
            comparatorObject = new CompoundJoynrObject({ name: "name", value: "value" });
            delete comparatorObject._typeName;
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns false when the second Object has additional keys", () => {
            compoundObject = new CompoundJoynrObject({ key1: "key1" });
            comparatorObject = new CompoundJoynrObject({ key1: "key1", key2: "key2" });
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns false when array keys are different", () => {
            compoundObject = new CompoundJoynrObject({ key1: [1, 2] });
            comparatorObject = new CompoundJoynrObject({ key1: [1, 3] });
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
            comparatorObject = new CompoundJoynrObject({ key1: [1, 2, 3] });
            expect(compoundObject.equals(comparatorObject)).toBeFalsy();
        });

        it("returns true when array keys are identical", () => {
            compoundObject = new CompoundJoynrObject({ key1: [1, 2] });
            comparatorObject = new CompoundJoynrObject({ key1: [1, 2] });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
        });

        it("calls equals when the childObject has an equals function", () => {
            const equalsSpy = jasmine.createSpy("equalsSpy").and.returnValue(true);
            const otherMember = { someData: "data" };
            compoundObject = new CompoundJoynrObject({ key1: { equals: equalsSpy } });
            comparatorObject = new CompoundJoynrObject({ key1: otherMember });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
            expect(equalsSpy).toHaveBeenCalledWith(otherMember);
        });

        it("calls equals when the array elements have an equals function", () => {
            const equalsSpy = jasmine.createSpy("equalsSpy").and.returnValue(true);
            const otherMember = { someData: "data" };
            compoundObject = new CompoundJoynrObject({ key1: [{ equals: equalsSpy }] });
            comparatorObject = new CompoundJoynrObject({ key1: [otherMember] });
            expect(compoundObject.equals(comparatorObject)).toBeTruthy();
            expect(equalsSpy).toHaveBeenCalledWith(otherMember);
        });
    });

    describe(".addMemberTypeGetter", () => {
        it("adds the getMemberType function, which works successfully", () => {
            const memberName = "someName";
            const testObject = { _memberTypes: { memberName } };
            Util.addMemberTypeGetter(testObject);
            expect(testObject.getMemberType).toEqual(jasmine.any(Function));
            expect(testObject.getMemberType("memberName")).toEqual(memberName);
        });
    });
});
