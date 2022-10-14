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

import _ from "lodash";

import { createDeferred } from "joynr/joynr/util/UtilInternal";
import * as util from "util";
import Enumeration from "../generated-javascript/joynr/interlanguagetest/Enumeration";
import * as EnumerationWithoutDefinedValues from "../generated-javascript/joynr/interlanguagetest/EnumerationWithoutDefinedValues";
import StructWithStringArray from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection1/StructWithStringArray";
import BaseStruct2 from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/BaseStruct";
import BaseStructWithoutElements from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/BaseStructWithoutElements";
import ExtendedBaseStruct from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedBaseStruct";
import ExtendedInterfaceEnumerationInTypeCollection from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedInterfaceEnumerationInTypeCollection";
import ExtendedStructOfPrimitives from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedStructOfPrimitives";
import ExtendedTypeCollectionEnumerationInTypeCollection from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection";
import StructOfPrimitives from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/StructOfPrimitives";
import ExtendedExtendedEnumeration = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedExtendedEnumeration");
import ExtendedExtendedBaseStruct = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedExtendedBaseStruct");
import JoynrRuntimeException = require("joynr/joynr/exceptions/JoynrRuntimeException");

export { createDeferred };

const log = require("test-base").logging.log;
const useRestrictedUnsignedRange = true;

const FillMap = {
    stringArrayElement: ["Hello", "World"],
    baseStructString: "Hiya",
    enumElement: Enumeration.ENUM_0_VALUE_3,
    enumWithoutDefinedValuesElement: EnumerationWithoutDefinedValues.ENUM_0_VALUE_1
};

const structWithStringArray = new StructWithStringArray(createSettings(["stringArrayElement"]));

export const Constants = {
    StringArray: FillMap.stringArrayElement,
    ByteArray: [1, 127],
    DoubleArray: [1.1, 2.2, 3.3],
    UInt64Array: [1, 127],
    ExtendedInterfaceEnumInTypeCollectionArray: [
        ExtendedInterfaceEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_INTERFACE,
        ExtendedInterfaceEnumerationInTypeCollection.ENUM_I1_VALUE_3
    ],
    ExtendedExtendedEnumerationArray: [
        ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_EXTENDED,
        ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
    ],
    StructWithStringArray: structWithStringArray,
    StructWithStringArrayArray: [structWithStringArray, structWithStringArray],
    BaseStructWithoutElements: new BaseStructWithoutElements(),
    BaseStruct: new BaseStruct2(createSettings(["baseStructString"])),
    ExtendedBaseStruct: new ExtendedBaseStruct(createSettings(["baseStructString", "enumElement"])),
    ExtendedExtendedBaseStruct: new ExtendedExtendedBaseStruct(
        createSettings(["baseStructString", "enumElement", "enumWithoutDefinedValuesElement"])
    )
};

type Keys = keyof typeof Constants;

export function create<T extends Keys>(key: T): typeof Constants[T] {
    return _.cloneDeep(Constants[key]);
}

/**
 * checks equality against preset objects specified by the key
 * @param key of preset object to compare against
 * @param value to compare
 */
export function check(key: Keys, value: any): boolean {
    const equal = _.isEqual(Constants[key], value);
    if (!equal) {
        log(`expected ${util.inspect(Constants[key])} to Equal ${util.inspect(value)}`);
    }
    return equal;
}

/**
 * checks equality ignoring the object type by checking stringified values
 * @param key type of object to compare against
 * @param value to compare
 */
export function checkObject(key: Keys, value: any): boolean {
    const equal = JSON.stringify(Constants[key]) === JSON.stringify(value);
    if (!equal) {
        log(`expected ${util.inspect(Constants[key])} to Equal ${util.inspect(value)}`);
    }
    return equal;
}

export function fill(struct: Record<string, any>, key: keyof typeof FillMap): void {
    if (!FillMap[key]) {
        throw new Error("fill can't be called with this key");
    }
    struct[key] = FillMap[key];
}

export function createSettings(keys: (keyof typeof FillMap)[]): any {
    const base: Record<string, any> = {};
    keys.forEach(key => fill(base, key));
    return base;
}

/* StructOfPrimitives*/
export function checkStructOfPrimitives(structOfPrimitives: any): boolean {
    if (!structOfPrimitives) {
        log("checkStructOfPrimitives: not set");
        return false;
    }
    if (structOfPrimitives.booleanElement !== true) {
        log("checkStructOfPrimitives: invalid content of booleanElement");
        return false;
    }
    if (!cmpDouble(structOfPrimitives.doubleElement, 1.1)) {
        log("checkStructOfPrimitives: invalid content of doubleElement");
        return false;
    }
    if (!cmpFloat(structOfPrimitives.floatElement, 1.1)) {
        log("checkStructOfPrimitives: invalid content of floatElement");
        return false;
    }
    if (structOfPrimitives.int8MinElement !== -128) {
        log("checkStructOfPrimitives: invalid content of int8MinElement");
        return false;
    }
    if (structOfPrimitives.int8MaxElement !== 127) {
        log("checkStructOfPrimitives: invalid content of int8MaxElement");
        return false;
    }
    if (structOfPrimitives.int16MinElement !== -32768) {
        log("checkStructOfPrimitives: invalid content of int16MinElement");
        return false;
    }
    if (structOfPrimitives.int16MaxElement !== 32767) {
        log("checkStructOfPrimitives: invalid content of int16MaxElement");
        return false;
    }
    if (structOfPrimitives.int32MinElement !== -2147483648) {
        log("checkStructOfPrimitives: invalid content of int32MinElement");
        return false;
    }
    if (structOfPrimitives.int32MaxElement !== 2147483647) {
        log("checkStructOfPrimitives: invalid content of int32MaxElement");
        return false;
    }

    //
    // Use reduced value range for int64, since it must stay within
    // Javascript maximum safe integer range.
    //
    // Original Java and C++ limits would require the code below.
    // However, the values the get rounded up and will not match expectations
    // on Java or C++ side.
    //
    //if (structOfPrimitives.int64MinElement !== -9223372036854775808) {
    //    log("checkStructOfPrimitives: invalid content of int64MinElement");
    //    return false;
    //}
    //if (structOfPrimitives.int64MaxElement !== 9223372036854775807) {
    //    log("checkStructOfPrimitives: invalid content of int64MaxElement");
    //    return false;
    //}
    if (structOfPrimitives.int64MinElement !== -9007199254740991) {
        log("checkStructOfPrimitives: invalid content of int64MinElement");
        return false;
    }
    if (structOfPrimitives.int64MaxElement !== 9007199254740991) {
        log("checkStructOfPrimitives: invalid content of int64MaxElement");
        return false;
    }

    if (structOfPrimitives.constString !== "Hiya") {
        log("checkStructOfPrimitives: invalid content of constString");
        return false;
    }
    if (structOfPrimitives.uInt8MinElement !== 0) {
        log("checkStructOfPrimitives: invalid content of uInt8MinElement");
        return false;
    }
    if (useRestrictedUnsignedRange) {
        if (structOfPrimitives.uInt8MaxElement !== 127) {
            log("checkStructOfPrimitives: invalid content of uInt8MaxElement");
            return false;
        }
    } else if (structOfPrimitives.uInt8MaxElement !== 255) {
        log("checkStructOfPrimitives: invalid content of uInt8MaxElement");
        return false;
    }
    if (structOfPrimitives.uInt16MinElement !== 0) {
        log("checkStructOfPrimitives: invalid content of uInt16MinElement");
        return false;
    }
    if (useRestrictedUnsignedRange) {
        if (structOfPrimitives.uInt16MaxElement !== 32767) {
            log("checkStructOfPrimitives: invalid content of uInt16MaxElement");
            return false;
        }
    } else if (structOfPrimitives.uInt16MaxElement !== 65535) {
        log("checkStructOfPrimitives: invalid content of uInt16MaxElement");
        return false;
    }
    if (structOfPrimitives.uInt32MinElement !== 0) {
        log("checkStructOfPrimitives: invalid content of uInt32MinElement");
        return false;
    }
    if (useRestrictedUnsignedRange) {
        if (structOfPrimitives.uInt32MaxElement !== 2147483647) {
            log("checkStructOfPrimitives: invalid content of uInt32MaxElement");
            return false;
        }
    } else if (structOfPrimitives.uInt32MaxElement !== 4294967295) {
        log("checkStructOfPrimitives: invalid content of uInt32MaxElement");
        return false;
    }
    if (structOfPrimitives.uInt64MinElement !== 0) {
        log("checkStructOfPrimitives: invalid content of uInt64MinElement");
        return false;
    }

    // If there were no limitations, the check would make sense with
    // structOfPrimitives.uInt64MaxElement !== 18446744073709551615
    if (structOfPrimitives.uInt64MaxElement !== 9007199254740991) {
        log("checkStructOfPrimitives: invalid content of uInt64MaxElement");
        return false;
    }

    if (!structOfPrimitives.booleanArray) {
        log("checkStructOfPrimitives: booleanArray not set");
        return false;
    }
    if (structOfPrimitives.booleanArray.length !== 2) {
        log("checkStructOfPrimitives: booleanArray has invalid length");
        return false;
    }
    if (structOfPrimitives.booleanArray[0] !== true) {
        log("checkStructOfPrimitives: booleanArray has invalid content");
        return false;
    }
    if (structOfPrimitives.booleanArray[1] !== false) {
        log("checkStructOfPrimitives: booleanArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.doubleArray) {
        log("checkStructOfPrimitives: doubleArray not set");
        return false;
    }
    if (structOfPrimitives.doubleArray.length !== 2) {
        log("checkStructOfPrimitives: doubleArray has invalid length");
        return false;
    }
    if (!cmpDouble(structOfPrimitives.doubleArray[0], 1.1)) {
        log("checkStructOfPrimitives: doubleArray has invalid content");
        return false;
    }
    if (!cmpDouble(structOfPrimitives.doubleArray[1], 2.2)) {
        log("checkStructOfPrimitives: doubleArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.floatArray) {
        log("checkStructOfPrimitives: floatArray not set");
        return false;
    }
    if (structOfPrimitives.floatArray.length !== 2) {
        log("checkStructOfPrimitives: floatArray has invalid length");
        return false;
    }
    if (!cmpFloat(structOfPrimitives.floatArray[0], 1.1)) {
        log("checkStructOfPrimitives: floatArray has invalid content");
        return false;
    }
    if (!cmpFloat(structOfPrimitives.floatArray[1], 2.2)) {
        log("checkStructOfPrimitives: floatArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.int8Array) {
        log("checkStructOfPrimitives: int8Array not set");
        return false;
    }
    if (structOfPrimitives.int8Array.length !== 2) {
        log("checkStructOfPrimitives: int8Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int8Array[0] !== 1) {
        log("checkStructOfPrimitives: int8Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int8Array[1] !== 2) {
        log("checkStructOfPrimitives: int8Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.int16Array) {
        log("checkStructOfPrimitives: int16Array not set");
        return false;
    }
    if (structOfPrimitives.int16Array.length !== 2) {
        log("checkStructOfPrimitives: int16Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int16Array[0] !== 1) {
        log("checkStructOfPrimitives: int16Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int16Array[1] !== 2) {
        log("checkStructOfPrimitives: int16Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.int32Array) {
        log("checkStructOfPrimitives: int32Array not set");
        return false;
    }
    if (structOfPrimitives.int32Array.length !== 2) {
        log("checkStructOfPrimitives: int32Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int32Array[0] !== 1) {
        log("checkStructOfPrimitives: int32Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int32Array[1] !== 2) {
        log("checkStructOfPrimitives: int32Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.int64Array) {
        log("checkStructOfPrimitives: int64Array not set");
        return false;
    }
    if (structOfPrimitives.int64Array.length !== 2) {
        log("checkStructOfPrimitives: int64Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int64Array[0] !== 1) {
        log("checkStructOfPrimitives: int64Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int64Array[1] !== 2) {
        log("checkStructOfPrimitives: int64Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.stringArray) {
        log("checkStructOfPrimitives: stringArray not set");
        return false;
    }
    if (structOfPrimitives.stringArray.length !== 2) {
        log("checkStructOfPrimitives: stringArray has invalid length");
        return false;
    }
    if (structOfPrimitives.stringArray[0] !== "Hello") {
        log("checkStructOfPrimitives: stringArray has invalid content");
        return false;
    }
    if (structOfPrimitives.stringArray[1] !== "World") {
        log("checkStructOfPrimitives: stringArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt8Array) {
        log("checkStructOfPrimitives: uInt8Array not set");
        return false;
    }
    if (structOfPrimitives.uInt8Array.length !== 2) {
        log("checkStructOfPrimitives: uInt8Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt8Array[0] !== 1) {
        log("checkStructOfPrimitives: uInt8Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt8Array[1] !== 2) {
        log("checkStructOfPrimitives: uInt8Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt16Array) {
        log("checkStructOfPrimitives: uInt16Array not set");
        return false;
    }
    if (structOfPrimitives.uInt16Array.length !== 2) {
        log("checkStructOfPrimitives: uInt16Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt16Array[0] !== 1) {
        log("checkStructOfPrimitives: uInt16Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt16Array[1] !== 2) {
        log("checkStructOfPrimitives: uInt16Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt32Array) {
        log("checkStructOfPrimitives: uInt32Array not set");
        return false;
    }
    if (structOfPrimitives.uInt32Array.length !== 2) {
        log("checkStructOfPrimitives: uInt32Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt32Array[0] !== 1) {
        log("checkStructOfPrimitives: uInt32Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt32Array[1] !== 2) {
        log("checkStructOfPrimitives: uInt32Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt64Array) {
        log("checkStructOfPrimitives: uInt64Array not set");
        return false;
    }
    if (structOfPrimitives.uInt64Array.length !== 2) {
        log("checkStructOfPrimitives: uInt64Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt64Array[0] !== 1) {
        log("checkStructOfPrimitives: uInt64Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt64Array[1] !== 2) {
        log("checkStructOfPrimitives: uInt64Array has invalid content");
        return false;
    }
    return true;
}

export function createStructOfPrimitivesMembers(): any {
    const members: any = {};
    members.booleanElement = true;
    members.doubleElement = 1.1;
    members.floatElement = 1.1;
    members.int8MinElement = -128;
    members.int8MaxElement = 127;
    members.int16MinElement = -32768;
    members.int16MaxElement = 32767;
    members.int32MinElement = -2147483648;
    members.int32MaxElement = 2147483647;
    // The original 64-bit maximum values for Java and C++
    // exceed the safe integer range of Javascript, hence
    // we have to use reduced values.
    // The original C++ values are
    //members.int64MinElement = -9223372036854775808;
    //members.int64MaxElement = 9223372036854775807;
    members.int64MinElement = -9007199254740991;
    members.int64MaxElement = 9007199254740991;
    members.constString = "Hiya";
    members.uInt8MinElement = 0;
    if (useRestrictedUnsignedRange) {
        members.uInt8MaxElement = 127;
    } else {
        members.uInt8MaxElement = 255;
    }
    members.uInt16MinElement = 0;
    if (useRestrictedUnsignedRange) {
        members.uInt16MaxElement = 32767;
    } else {
        members.uInt16MaxElement = 65535;
    }
    members.uInt32MinElement = 0;
    if (useRestrictedUnsignedRange) {
        members.uInt32MaxElement = 2147483647;
    } else {
        members.uInt32MaxElement = 4294967295;
    }
    members.uInt64MinElement = 0;
    // The original 64-bit maximum values for Java and C++
    // exceeds the safe integer range of Javascript, hence
    // we have to use reduced values.
    // TODO: should be 18446744073709551615 (original C++)
    // But the value would translate to
    // 18437736874454810625 in C++
    members.uInt64MaxElement = 9007199254740991;

    members.booleanArray = [];
    members.booleanArray.push(true);
    members.booleanArray.push(false);
    members.doubleArray = [];
    members.doubleArray.push(1.1);
    members.doubleArray.push(2.2);
    members.floatArray = [];
    members.floatArray.push(1.1);
    members.floatArray.push(2.2);
    members.int8Array = [];
    members.int8Array.push(1);
    members.int8Array.push(2);
    members.int16Array = [];
    members.int16Array.push(1);
    members.int16Array.push(2);
    members.int32Array = [];
    members.int32Array.push(1);
    members.int32Array.push(2);
    members.int64Array = [];
    members.int64Array.push(1);
    members.int64Array.push(2);
    members.stringArray = [];
    members.stringArray.push("Hello");
    members.stringArray.push("World");
    members.uInt8Array = [];
    members.uInt8Array.push(1);
    members.uInt8Array.push(2);
    members.uInt16Array = [];
    members.uInt16Array.push(1);
    members.uInt16Array.push(2);
    members.uInt32Array = [];
    members.uInt32Array.push(1);
    members.uInt32Array.push(2);
    members.uInt64Array = [];
    members.uInt64Array.push(1);
    members.uInt64Array.push(2);

    return members;
}

export function createStructOfPrimitives(): StructOfPrimitives {
    const members = createStructOfPrimitivesMembers();
    const structOfPrimitives = new StructOfPrimitives(members);

    if (!checkStructOfPrimitives(structOfPrimitives)) {
        throw new JoynrRuntimeException({
            detailMessage: "Internal error in fillExtendedExtendedBaseStruct"
        });
    }
    return structOfPrimitives;
}

/* ExtendedStructOfPrimitives*/
export function checkExtendedStructOfPrimitives(extendedStructOfPrimitives: ExtendedStructOfPrimitives): boolean {
    if (!extendedStructOfPrimitives) {
        log("checkExtendedStructOfPrimitives: extendedStructOfPrimitives not set");
        return false;
    }
    if (
        extendedStructOfPrimitives.extendedEnumElement !==
        ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
    ) {
        log("checkExtendedStructOfPrimitives: extendedEnumElement has invalid content");
        return false;
    }
    if (!check("ExtendedBaseStruct", extendedStructOfPrimitives.extendedStructElement)) {
        log("checkExtendedStructOfPrimitives: extendedBaseStruct has invalid content");
        return false;
    }
    if (!checkStructOfPrimitives(extendedStructOfPrimitives)) {
        log("checkExtendedStructOfPrimitives: structOfPrimitives has invalid content");
        return false;
    }
    return true;
}

export function createExtendedStructOfPrimitives(): ExtendedStructOfPrimitives {
    const members = createStructOfPrimitivesMembers();
    members.extendedStructElement = create("ExtendedBaseStruct");
    members.extendedEnumElement =
        ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;

    const extendedStructOfPrimitives = new ExtendedStructOfPrimitives(members);
    if (!checkExtendedStructOfPrimitives(extendedStructOfPrimitives)) {
        throw new JoynrRuntimeException({
            detailMessage: "Internal error in checkExtendedStructOfPrimitives"
        });
    }
    return extendedStructOfPrimitives;
}

export function cmpFloat(a: number, b: number): boolean {
    return Math.abs(a - b) < 0.001;
}

export function cmpDouble(a: number, b: number): boolean {
    return Math.abs(a - b) < 0.001;
}

export function cmpByteBuffers(a: number[], b: number[]): boolean {
    if (a.length !== b.length) {
        return false;
    }
    for (let i = 0; i < a.length; ++i) {
        if (a[i] !== b[i]) {
            return false;
        }
    }
    return true;
}
