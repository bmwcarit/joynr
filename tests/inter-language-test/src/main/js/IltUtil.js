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

var joynr = require("joynr");
var log = require("test-base").logging.log;

// imports
joynr.interlanguagetest = {};
joynr.interlanguagetest.EnumerationWithoutDefinedValues = require("../generated-javascript/joynr/interlanguagetest/EnumerationWithoutDefinedValues.js");
joynr.interlanguagetest.TestInterface = {};
joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithExtendedErrorEnumErrorEnum.js");
joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithAnonymousErrorEnumErrorEnum.js");
joynr.interlanguagetest.TestInterfaceProxy = require("../generated-javascript/joynr/interlanguagetest/TestInterfaceProxy.js");
joynr.interlanguagetest.BaseStruct = require("../generated-javascript/joynr/interlanguagetest/BaseStruct.js");
joynr.interlanguagetest.namedTypeCollection2 = {};
joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedStructOfPrimitives.js");
joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedEnumerationWithPartlyDefinedValues.js");
joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedExtendedBaseStruct.js");
joynr.interlanguagetest.namedTypeCollection2.StructOfPrimitives = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/StructOfPrimitives.js");
joynr.interlanguagetest.namedTypeCollection2.BaseStruct = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/BaseStruct.js");

joynr.interlanguagetest.namedTypeCollection2.ErrorEnumTc = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ErrorEnumTc.js");
joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/BaseStructWithoutElements.js");
joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedExtendedEnumeration.js");
joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection.js");
joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedInterfaceEnumerationInTypeCollection.js");
joynr.interlanguagetest.namedTypeCollection2.Enumeration = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/Enumeration.js");
joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedBaseStruct.js");
joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedErrorEnumTc.js");
joynr.interlanguagetest.Enumeration = require("../generated-javascript/joynr/interlanguagetest/Enumeration.js");

joynr.interlanguagetest.namedTypeCollection1 = {};
joynr.interlanguagetest.namedTypeCollection1.BaseStruct = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection1/BaseStruct.js");
joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection1/StructWithStringArray.js");
joynr.interlanguagetest.namedTypeCollection1.Enumeration = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection1/Enumeration.js").Enumeration;

var useRestricted64BitRange = true;
var useRestrictedUnsignedRange = true;

IltUtil = {};


IltUtil.checkBoolean = function(booleanValue) {
    log("IltUtil.checkBoolean called with " + booleanValue);
    expect(booleanValue).toBeTruthy();
    if (booleanValue === true){
        log("IltUtil.checkBoolean returning true");
        return true;
    } else {
        log("IltUtil.checkBoolean returning false");
        return false;
    }
}

// String Array

IltUtil.fillStringArray = function(stringArray) {
    stringArray.push("Hello");
    stringArray.push("World");
    if (!IltUtil.checkStringArray(stringArray)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillStringArray");
    }
    return stringArray;
};

IltUtil.createStringArray = function() {
    stringArray = [];
    stringArray = IltUtil.fillStringArray(stringArray);
    return stringArray;
};

IltUtil.checkStringArray = function(stringArray) {
    if (stringArray.length != 2) {
        log("checkStringArray: invalid array length");
        return false;
    }
    if (stringArray[0] != "Hello" || stringArray[1] != "World") {
        log("checkStringArray: invalid array content");
        return false;
    }
    return true;
};

// uInt8Array

IltUtil.fillByteArray = function(byteArray) {
    // TODO: clear all values
    byteArray.push(1);
    byteArray.push(127);
    if (!IltUtil.checkByteArray(byteArray)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillByteArray");
    }
    return byteArray;
};

IltUtil.createByteArray = function() {
    byteArray = [];
    byteArray = IltUtil.fillByteArray(byteArray);
    return byteArray;
};

IltUtil.checkByteArray = function(byteArray) {
    if (byteArray.length != 2) {
        log("checkByteArray: invalid array length");
        return false;
    }
    if (byteArray[0] != 1 || byteArray[1] != 127) {
        log("checkByteArray: invalid array content");
        return false;
    }
    return true;
}

// uInt64Array

IltUtil.fillUInt64Array = function(uInt64Array) {
    uInt64Array.push(1);
    uInt64Array.push(127);
    if (!IltUtil.checkUInt64Array(uInt64Array)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillUInt64Array");
    }
    return uInt64Array;
};

IltUtil.createUInt64Array = function() {
    uInt64Array = [];
    uInt64Array = IltUtil.fillUInt64Array(uInt64Array);
    return uInt64Array;
};

IltUtil.checkUInt64Array = function(uInt64Array) {
    if (uInt64Array.length != 2) {
        log("checkUInt64Array: invalid array length");
        return false;
    }
    if (uInt64Array[0] != 1 || uInt64Array[1] != 127) {
        log("checkUInt64Array: invalid array content");
        return false;
    }
    return true;
};

// DoubleArray

IltUtil.fillDoubleArray = function(doubleArray) {
    doubleArray.push(1.1);
    doubleArray.push(2.2);
    doubleArray.push(3.3);
    if (!IltUtil.checkDoubleArray(doubleArray)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillDoubleArray");
    }
    return doubleArray;
};

IltUtil.createDoubleArray = function() {
    doubleArray = [];
    doubleArray = IltUtil.fillDoubleArray(doubleArray);
    return doubleArray;
};

IltUtil.checkDoubleArray = function(doubleArray) {
    if (doubleArray.length != 3) {
        log("checkDoubleArray: invalid array length");
        return false;
    }
    if (!IltUtil.cmpDouble(doubleArray[0], 1.1) || !IltUtil.cmpDouble(doubleArray[1], 2.2) || !IltUtil.cmpDouble(doubleArray[2], 3.3)) {
        log("checkDoubleArray: invalid array content");
        return false;
    }
    return true;
};

// ExtendedInterfaceEnumerationInTypeCollectionArray

IltUtil.fillExtendedInterfaceEnumerationInTypeCollectionArray = function(extendedInterfaceEnumerationInTypeCollectionArray) {
    extendedInterfaceEnumerationInTypeCollectionArray.push(joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_INTERFACE);
    extendedInterfaceEnumerationInTypeCollectionArray.push(joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection.ENUM_I1_VALUE_3);
    if (!IltUtil.checkExtendedInterfaceEnumerationInTypeCollectionArray(extendedInterfaceEnumerationInTypeCollectionArray)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillExtendedInterfaceEnumerationInTypeCollectionArray");
    }
    return extendedInterfaceEnumerationInTypeCollectionArray;
};

IltUtil.createExtendedInterfaceEnumerationInTypeCollectionArray = function() {
    extendedInterfaceEnumerationInTypeCollectionArray = [];
    extendedInterfaceEnumerationInTypeCollectionArray = IltUtil.fillExtendedInterfaceEnumerationInTypeCollectionArray(extendedInterfaceEnumerationInTypeCollectionArray);
    return extendedInterfaceEnumerationInTypeCollectionArray;
};

IltUtil.checkExtendedInterfaceEnumerationInTypeCollectionArray = function(extendedInterfaceEnumerationInTypeCollectionArray) {
    if (extendedInterfaceEnumerationInTypeCollectionArray.length != 2) {
        log("checkExtendedInterfaceEnumerationInTypeCollectionArray: invalid array length");
        return false;
    }
    if (extendedInterfaceEnumerationInTypeCollectionArray[0] != joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_INTERFACE ||
        extendedInterfaceEnumerationInTypeCollectionArray[1] != joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection.ENUM_I1_VALUE_3) {
        log("checkExtendedInterfaceEnumerationInTypeCollectionArray: invalid array content");
        return false;
    }
    return true;
};

// ExtendedExtendedEnumerationArray

IltUtil.fillExtendedExtendedEnumerationArray = function(extendedExtendedEnumerationArray) {
    extendedExtendedEnumerationArray[0] = joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_EXTENDED;
    extendedExtendedEnumerationArray[1] = joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
    if (!IltUtil.checkExtendedExtendedEnumerationArray(extendedExtendedEnumerationArray)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillExtendedExtendedEnumerationArray");
    }
    return extendedExtendedEnumerationArray;
};

IltUtil.createExtendedExtendedEnumerationArray = function() {
    extendedExtendedEnumerationArray = [];
    extendedExtendedEnumerationArray = IltUtil.fillExtendedExtendedEnumerationArray(extendedExtendedEnumerationArray);
    return extendedExtendedEnumerationArray;
};

IltUtil.checkExtendedExtendedEnumerationArray = function(extendedExtendedEnumerationArray) {
    if (extendedExtendedEnumerationArray.length != 2) {
        log("checkExtendedExtendedEnumerationArray: invalid array length");
        return false;
    }
    if (extendedExtendedEnumerationArray[0] != joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_EXTENDED ||
        extendedExtendedEnumerationArray[1] != joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
        log("checkExtendedExtendedEnumerationArray: invalid array content");
        return false;
    }
    return true;
};

// StructWithStringArray

IltUtil.fillStructWithStringArray = function(structWithStringArray) {
    stringArray = [];
    stringArray.push("Hello");
    stringArray.push("World");
    structWithStringArray.stringArrayElement = stringArray;
    if (!IltUtil.checkStructWithStringArray(structWithStringArray)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillStructWithStringArray");
    }
    return structWithStringArray;
};

IltUtil.createStructWithStringArray = function() {
    structWithStringArray = new joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray();
    structWithStringArray = IltUtil.fillStructWithStringArray(structWithStringArray);
    return structWithStringArray;
};

IltUtil.checkStructWithStringArray = function(structWithStringArray) {
    var stringArray = structWithStringArray.stringArrayElement;
    if (!stringArray) {
        log("checkStructWithStringArray: array not set");
        return false;
    }
    if (stringArray.length != 2) {
        log("checkStructWithStringArray: invalid array length");
        return false;
    }
    if (stringArray[0] != "Hello" ||
        stringArray[1] != "World") {
        log("checkStructWithStringArray: invalid array content");
        return false;
    }
    return true;
};

// StructWithStringArrayArray

IltUtil.fillStructWithStringArrayArray = function(structWithStringArrayArray) {
    structWithStringArrayArray.push(IltUtil.createStructWithStringArray());
    structWithStringArrayArray.push(IltUtil.createStructWithStringArray());
    if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArray)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillStructWithStringArray");
    }
    return structWithStringArrayArray;
};

IltUtil.createStructWithStringArrayArray = function() {
    structWithStringArrayArray = [];
    structWithStringArrayArray = IltUtil.fillStructWithStringArrayArray(structWithStringArrayArray);
    return structWithStringArrayArray;
};

IltUtil.checkStructWithStringArrayArray = function(structWithStringArrayArray) {
    if (!structWithStringArrayArray) {
        log("checkStructWithStringArrayArray: array not set");
        return false;
    }
    if (structWithStringArrayArray.length != 2) {
        log("checkStructWithStringArrayArray: invalid array length");
        return false;
    }
    if (!IltUtil.checkStructWithStringArray(structWithStringArrayArray[0]) ||
        !IltUtil.checkStructWithStringArray(structWithStringArrayArray[1])) {
        log("checkStructWithStringArrayArray: invalid array content");
        return false;
    }
    return true;
};

// BaseStructWithoutElements

IltUtil.createBaseStructWithoutElements = function() {
    var baseStructWithoutElements = new joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements();
    // nothing required, since this is an empty struct
    //
    // it was intended to be used for typecasts of some other struct
    // inheriting from this one.
    return baseStructWithoutElements;
};

IltUtil.checkBaseStructWithoutElements = function(baseStructWithoutElements) {
    // nothing required, since this is an empty struct
    return true;
};

// BaseStruct

IltUtil.fillBaseStruct = function(baseStruct) {
    baseStruct.baseStructString = "Hiya";
    if (!IltUtil.checkBaseStruct(baseStruct)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillStructWithStringArray");
    }
    return baseStruct;
};

IltUtil.createBaseStruct = function() {
    var baseStruct = new joynr.interlanguagetest.namedTypeCollection2.BaseStruct();
    baseStruct = IltUtil.fillBaseStruct(baseStruct);
    return baseStruct;
};

IltUtil.checkBaseStruct = function(baseStruct) {
    if (!baseStruct) {
        log("checkBaseStruct: struct not present");
        return false;
    }
    if (!baseStruct.baseStructString) {
        log("checkBaseStruct: baseStructString not set");
        return false;
    }
    if (baseStruct.baseStructString != "Hiya") {
        log("checkBaseStruct: invalid content of baseStructString");
        return false;
    }
    return true;
};

// ExtendedBaseStruct

IltUtil.fillExtendedBaseStruct = function(extendedBaseStruct) {
    extendedBaseStruct.enumElement = joynr.interlanguagetest.Enumeration.ENUM_0_VALUE_3;
    // fill inherited parts
    IltUtil.fillBaseStruct(extendedBaseStruct);
    if (!IltUtil.checkExtendedBaseStruct(extendedBaseStruct)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillStructWithStringArray");
    }
    return extendedBaseStruct;
};

IltUtil.createExtendedBaseStruct = function() {
    var extendedBaseStruct = new joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct();
    IltUtil.fillExtendedBaseStruct(extendedBaseStruct);
    return extendedBaseStruct;
};

IltUtil.checkExtendedBaseStruct = function(extendedBaseStruct) {
    if (!extendedBaseStruct) {
        log("checkBaseStruct: extendedBaseStruct not set");
        return false;
    }
    if (extendedBaseStruct.enumElement === null || extendedBaseStruct.enumElement === undefined) {
        log("checkBaseStruct: enumElement not set");
        return false;
    }
    if (extendedBaseStruct.enumElement != joynr.interlanguagetest.Enumeration.ENUM_0_VALUE_3) {
        log("checkBaseStruct: invalid content of enumElement");
        return false;
    }
    return true;
};

// ExtendedExtendedBaseStruct

IltUtil.fillExtendedExtendedBaseStruct = function(extendedExtendedBaseStruct) {
    extendedExtendedBaseStruct.enumWithoutDefinedValuesElement = joynr.interlanguagetest.EnumerationWithoutDefinedValues.ENUM_0_VALUE_1;
    // fill inherited stuff
    IltUtil.fillExtendedBaseStruct(extendedExtendedBaseStruct);
    if (!IltUtil.checkExtendedExtendedBaseStruct(extendedExtendedBaseStruct)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillExtendedExtendedBaseStruct");
    }
    return extendedExtendedBaseStruct;
};

IltUtil.createExtendedExtendedBaseStruct = function() {
    var extendedExtendedBaseStruct = new joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct();
    extendedExtendedBaseStruct = IltUtil.fillExtendedExtendedBaseStruct(extendedExtendedBaseStruct);
    return extendedExtendedBaseStruct;
};

IltUtil.checkExtendedExtendedBaseStruct = function(extendedExtendedBaseStruct) {
    if (!extendedExtendedBaseStruct) {
        return false;
    }
    if (extendedExtendedBaseStruct.enumWithoutDefinedValuesElement === null ||
        extendedExtendedBaseStruct.enumWithoutDefinedValuesElement === undefined) {
        log("checkExtendedExtendedBaseStruct: enumWithoutDefinedValuesElement not set");
        return false;
    }
    if (extendedExtendedBaseStruct.enumWithoutDefinedValuesElement != joynr.interlanguagetest.EnumerationWithoutDefinedValues.ENUM_0_VALUE_1) {
        log("checkExtendedExtendedBaseStruct: invalid content of enumWithoutDefinedValuesElement");
        return false;
    }
    if (!IltUtil.checkExtendedBaseStruct(extendedExtendedBaseStruct)) {
        log("checkExtendedExtendedBaseStruct: invalid extendedBaseStruct");
        return false;
    }
    return true;
};

// StructOfPrimitives

IltUtil.fillStructOfPrimitives = function(structOfPrimitives) {
    structOfPrimitives.booleanElement = true;
    structOfPrimitives.doubleElement = 1.1;
    structOfPrimitives.floatElement = 1.1;
    structOfPrimitives.int8MinElement = -128;
    structOfPrimitives.int8MaxElement = 127;
    structOfPrimitives.int16MinElement = -32768;
    structOfPrimitives.int16MaxElement = 32767;
    structOfPrimitives.int32MinElement = -2147483648;
    structOfPrimitives.int32MaxElement = 2147483647;
    // The original 64-bit maximum values for Java and C++
    // exceed the safe integer range of Javascript, hence
    // we have to use reduced values.
    // The original C++ values are
    //structOfPrimitives.int64MinElement = -9223372036854775808;
    //structOfPrimitives.int64MaxElement = 9223372036854775807;
    structOfPrimitives.int64MinElement = -9007199254740991;
    structOfPrimitives.int64MaxElement = 9007199254740991;
    structOfPrimitives.constString = "Hiya";
    structOfPrimitives.uInt8MinElement = 0;
    if (useRestrictedUnsignedRange) {
        structOfPrimitives.uInt8MaxElement = 127;
    } else {
        structOfPrimitives.uInt8MaxElement = 255;
    }
    structOfPrimitives.uInt16MinElement = 0;
    if (useRestrictedUnsignedRange) {
        structOfPrimitives.uInt16MaxElement = 32767;
    } else {
        structOfPrimitives.uInt16MaxElement = 65535;
    }
    structOfPrimitives.uInt32MinElement = 0;
    if (useRestrictedUnsignedRange) {
        structOfPrimitives.uInt32MaxElement = 2147483647;
    } else {
        structOfPrimitives.uInt32MaxElement = 4294967295;
    }
    structOfPrimitives.uInt64MinElement = 0;
    // The original 64-bit maximum values for Java and C++
    // exceeds the safe integer range of Javascript, hence
    // we have to use reduced values.
    // TODO: should be 18446744073709551615 (original C++)
    // But the value would translate to
    // 18437736874454810625 in C++
    structOfPrimitives.uInt64MaxElement = 9007199254740991;

    structOfPrimitives.booleanArray = [];
    structOfPrimitives.booleanArray.push(true);
    structOfPrimitives.booleanArray.push(false);
    structOfPrimitives.doubleArray = [];
    structOfPrimitives.doubleArray.push(1.1);
    structOfPrimitives.doubleArray.push(2.2);
    structOfPrimitives.floatArray = [];
    structOfPrimitives.floatArray.push(1.1);
    structOfPrimitives.floatArray.push(2.2);
    structOfPrimitives.int8Array = [];
    structOfPrimitives.int8Array.push(1);
    structOfPrimitives.int8Array.push(2);
    structOfPrimitives.int16Array = [];
    structOfPrimitives.int16Array.push(1);
    structOfPrimitives.int16Array.push(2);
    structOfPrimitives.int32Array = [];
    structOfPrimitives.int32Array.push(1);
    structOfPrimitives.int32Array.push(2);
    structOfPrimitives.int64Array = [];
    structOfPrimitives.int64Array.push(1);
    structOfPrimitives.int64Array.push(2);
    structOfPrimitives.stringArray = [];
    structOfPrimitives.stringArray.push("Hello");
    structOfPrimitives.stringArray.push("World");
    structOfPrimitives.uInt8Array = [];
    structOfPrimitives.uInt8Array.push(1);
    structOfPrimitives.uInt8Array.push(2);
    structOfPrimitives.uInt16Array = [];
    structOfPrimitives.uInt16Array.push(1);
    structOfPrimitives.uInt16Array.push(2);
    structOfPrimitives.uInt32Array = [];
    structOfPrimitives.uInt32Array.push(1);
    structOfPrimitives.uInt32Array.push(2);
    structOfPrimitives.uInt64Array = [];
    structOfPrimitives.uInt64Array.push(1);
    structOfPrimitives.uInt64Array.push(2);

    if (!IltUtil.checkStructOfPrimitives(structOfPrimitives)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in fillExtendedExtendedBaseStruct");
    }
    return structOfPrimitives;
};

IltUtil.checkStructOfPrimitives = function(structOfPrimitives) {
    if (!structOfPrimitives) {
        log("checkStructOfPrimitives: not set");
        return false;
    }
    if (structOfPrimitives.booleanElement != true) {
        log("checkStructOfPrimitives: invalid content of booleanElement");
        return false;
    }
    if (!IltUtil.cmpDouble(structOfPrimitives.doubleElement, 1.1)) {
        log("checkStructOfPrimitives: invalid content of doubleElement");
        return false;
    }
    if (!IltUtil.cmpFloat(structOfPrimitives.floatElement, 1.1)) {
        log("checkStructOfPrimitives: invalid content of floatElement");
        return false;
    }
    if (structOfPrimitives.int8MinElement != -128) {
        log("checkStructOfPrimitives: invalid content of int8MinElement");
        return false;
    }
    if (structOfPrimitives.int8MaxElement != 127) {
        log("checkStructOfPrimitives: invalid content of int8MaxElement");
        return false;
    }
    if (structOfPrimitives.int16MinElement != -32768) {
        log("checkStructOfPrimitives: invalid content of int16MinElement");
        return false;
    }
    if (structOfPrimitives.int16MaxElement != 32767) {
        log("checkStructOfPrimitives: invalid content of int16MaxElement");
        return false;
    }
    if (structOfPrimitives.int32MinElement != -2147483648) {
        log("checkStructOfPrimitives: invalid content of int32MinElement");
        return false;
    }
    if (structOfPrimitives.int32MaxElement != 2147483647) {
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
    //if (structOfPrimitives.int64MinElement != -9223372036854775808) {
    //    log("checkStructOfPrimitives: invalid content of int64MinElement");
    //    return false;
    //}
    //if (structOfPrimitives.int64MaxElement != 9223372036854775807) {
    //    log("checkStructOfPrimitives: invalid content of int64MaxElement");
    //    return false;
    //}
    if (structOfPrimitives.int64MinElement != -9007199254740991) {
        log("checkStructOfPrimitives: invalid content of int64MinElement");
        return false;
    }
    if (structOfPrimitives.int64MaxElement != 9007199254740991) {
        log("checkStructOfPrimitives: invalid content of int64MaxElement");
        return false;
    }

    if (structOfPrimitives.constString != "Hiya") {
        log("checkStructOfPrimitives: invalid content of constString");
        return false;
    }
    if (structOfPrimitives.uInt8MinElement != 0) {
        log("checkStructOfPrimitives: invalid content of uInt8MinElement");
        return false;
    }
    if (useRestrictedUnsignedRange) {
        if (structOfPrimitives.uInt8MaxElement != 127) {
            log("checkStructOfPrimitives: invalid content of uInt8MaxElement");
            return false;
        }
    } else {
        if (structOfPrimitives.uInt8MaxElement != 255) {
            log("checkStructOfPrimitives: invalid content of uInt8MaxElement");
            return false;
        }
    }
    if (structOfPrimitives.uInt16MinElement != 0) {
        log("checkStructOfPrimitives: invalid content of uInt16MinElement");
        return false;
    }
    if (useRestrictedUnsignedRange) {
        if (structOfPrimitives.uInt16MaxElement != 32767) {
            log("checkStructOfPrimitives: invalid content of uInt16MaxElement");
            return false;
        }
    } else {
        if (structOfPrimitives.uInt16MaxElement != 65535) {
            log("checkStructOfPrimitives: invalid content of uInt16MaxElement");
            return false;
        }
    }
    if (structOfPrimitives.uInt32MinElement != 0) {
        log("checkStructOfPrimitives: invalid content of uInt32MinElement");
        return false;
    }
    if (useRestrictedUnsignedRange) {
        if (structOfPrimitives.uInt32MaxElement != 2147483647) {
            log("checkStructOfPrimitives: invalid content of uInt32MaxElement");
            return false;
        }
    } else {
        if (structOfPrimitives.uInt32MaxElement != 4294967295) {
            log("checkStructOfPrimitives: invalid content of uInt32MaxElement");
            return false;
        }
    }
    if (structOfPrimitives.uInt64MinElement != 0) {
        log("checkStructOfPrimitives: invalid content of uInt64MinElement");
        return false;
    }

    // If there were no limitations, the check would make sense with
    // structOfPrimitives.uInt64MaxElement != 18446744073709551615
    if (structOfPrimitives.uInt64MaxElement != 9007199254740991) {
        log("checkStructOfPrimitives: invalid content of uInt64MaxElement");
        return false;
    }

    if (!structOfPrimitives.booleanArray) {
        log("checkStructOfPrimitives: booleanArray not set");
        return false;
    }
    if (structOfPrimitives.booleanArray.length != 2) {
        log("checkStructOfPrimitives: booleanArray has invalid length");
        return false;
    }
    if (structOfPrimitives.booleanArray[0] != true) {
        log("checkStructOfPrimitives: booleanArray has invalid content");
        return false;
    }
    if (structOfPrimitives.booleanArray[1] != false) {
        log("checkStructOfPrimitives: booleanArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.doubleArray) {
        log("checkStructOfPrimitives: doubleArray not set");
        return false;
    }
    if (structOfPrimitives.doubleArray.length != 2) {
        log("checkStructOfPrimitives: doubleArray has invalid length");
        return false;
    }
    if (!IltUtil.cmpDouble(structOfPrimitives.doubleArray[0], 1.1)) {
        log("checkStructOfPrimitives: doubleArray has invalid content");
        return false;
    }
    if (!IltUtil.cmpDouble(structOfPrimitives.doubleArray[1], 2.2)) {
        log("checkStructOfPrimitives: doubleArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.floatArray) {
        log("checkStructOfPrimitives: floatArray not set");
        return false;
    }
    if (structOfPrimitives.floatArray.length != 2) {
        log("checkStructOfPrimitives: floatArray has invalid length");
        return false;
    }
    if (!IltUtil.cmpFloat(structOfPrimitives.floatArray[0], 1.1)) {
        log("checkStructOfPrimitives: floatArray has invalid content");
        return false;
    }
    if (!IltUtil.cmpFloat(structOfPrimitives.floatArray[1], 2.2)) {
        log("checkStructOfPrimitives: floatArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.int8Array) {
        log("checkStructOfPrimitives: int8Array not set");
        return false;
    }
    if (structOfPrimitives.int8Array.length != 2) {
        log("checkStructOfPrimitives: int8Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int8Array[0] != 1) {
        log("checkStructOfPrimitives: int8Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int8Array[1] != 2) {
        log("checkStructOfPrimitives: int8Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.int16Array) {
        log("checkStructOfPrimitives: int16Array not set");
        return false;
    }
    if (structOfPrimitives.int16Array.length != 2) {
        log("checkStructOfPrimitives: int16Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int16Array[0] != 1) {
        log("checkStructOfPrimitives: int16Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int16Array[1] != 2) {
        log("checkStructOfPrimitives: int16Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.int32Array) {
        log("checkStructOfPrimitives: int32Array not set");
        return false;
    }
    if (structOfPrimitives.int32Array.length != 2) {
        log("checkStructOfPrimitives: int32Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int32Array[0] != 1) {
        log("checkStructOfPrimitives: int32Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int32Array[1] != 2) {
        log("checkStructOfPrimitives: int32Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.int64Array) {
        log("checkStructOfPrimitives: int64Array not set");
        return false;
    }
    if (structOfPrimitives.int64Array.length != 2) {
        log("checkStructOfPrimitives: int64Array has invalid length");
        return false;
    }
    if (structOfPrimitives.int64Array[0] != 1) {
        log("checkStructOfPrimitives: int64Array has invalid content");
        return false;
    }
    if (structOfPrimitives.int64Array[1] != 2) {
        log("checkStructOfPrimitives: int64Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.stringArray) {
        log("checkStructOfPrimitives: stringArray not set");
        return false;
    }
    if (structOfPrimitives.stringArray.length != 2) {
        log("checkStructOfPrimitives: stringArray has invalid length");
        return false;
    }
    if (structOfPrimitives.stringArray[0] != "Hello") {
        log("checkStructOfPrimitives: stringArray has invalid content");
        return false;
    }
    if (structOfPrimitives.stringArray[1] != "World") {
        log("checkStructOfPrimitives: stringArray has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt8Array) {
        log("checkStructOfPrimitives: uInt8Array not set");
        return false;
    }
    if (structOfPrimitives.uInt8Array.length != 2) {
        log("checkStructOfPrimitives: uInt8Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt8Array[0] != 1) {
        log("checkStructOfPrimitives: uInt8Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt8Array[1] != 2) {
        log("checkStructOfPrimitives: uInt8Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt16Array) {
        log("checkStructOfPrimitives: uInt16Array not set");
        return false;
    }
    if (structOfPrimitives.uInt16Array.length != 2) {
        log("checkStructOfPrimitives: uInt16Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt16Array[0] != 1) {
        log("checkStructOfPrimitives: uInt16Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt16Array[1] != 2) {
        log("checkStructOfPrimitives: uInt16Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt32Array) {
        log("checkStructOfPrimitives: uInt32Array not set");
        return false;
    }
    if (structOfPrimitives.uInt32Array.length != 2) {
        log("checkStructOfPrimitives: uInt32Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt32Array[0] != 1) {
        log("checkStructOfPrimitives: uInt32Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt32Array[1] != 2) {
        log("checkStructOfPrimitives: uInt32Array has invalid content");
        return false;
    }
    if (!structOfPrimitives.uInt64Array) {
        log("checkStructOfPrimitives: uInt64Array not set");
        return false;
    }
    if (structOfPrimitives.uInt64Array.length != 2) {
        log("checkStructOfPrimitives: uInt64Array has invalid length");
        return false;
    }
    if (structOfPrimitives.uInt64Array[0] != 1) {
        log("checkStructOfPrimitives: uInt64Array has invalid content");
        return false;
    }
    if (structOfPrimitives.uInt64Array[1] != 2) {
        log("checkStructOfPrimitives: uInt64Array has invalid content");
        return false;
    }
    return true;
};

IltUtil.createStructOfPrimitives = function() {
    var structOfPrimitives = new joynr.interlanguagetest.namedTypeCollection2.StructOfPrimitives();
    structOfPrimitives = IltUtil.fillStructOfPrimitives(structOfPrimitives);
    return structOfPrimitives;
};

// ExtendedStructOfPrimitives

IltUtil.fillExtendedStructOfPrimitives = function(extendedStructOfPrimitives) {
    extendedStructOfPrimitives.extendedEnumElement = joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
    extendedStructOfPrimitives.extendedStructElement = IltUtil.createExtendedBaseStruct();
    IltUtil.fillStructOfPrimitives(extendedStructOfPrimitives);
    if (!IltUtil.checkExtendedStructOfPrimitives(extendedStructOfPrimitives)) {
        throw new joynr.exceptions.JoynrRuntimeException("Internal error in checkExtendedStructOfPrimitives");
    }
    return extendedStructOfPrimitives;
};

IltUtil.createExtendedStructOfPrimitives = function() {
    var extendedStructOfPrimitives = new joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives();
    IltUtil.fillExtendedStructOfPrimitives(extendedStructOfPrimitives);
    return extendedStructOfPrimitives;
};

IltUtil.checkExtendedStructOfPrimitives = function(extendedStructOfPrimitives) {
    if (!extendedStructOfPrimitives) {
        log("checkExtendedStructOfPrimitives: extendedStructOfPrimitives not set");
        return false;
    }
    if (extendedStructOfPrimitives.extendedEnumElement != joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
        log("checkExtendedStructOfPrimitives: extendedEnumElement has invalid content");
        return false;
    }
    if (!IltUtil.checkExtendedBaseStruct(extendedStructOfPrimitives.extendedStructElement)) {
        log("checkExtendedStructOfPrimitives: extendedBaseStruct has invalid content");
        return false;
    }
    if (!IltUtil.checkStructOfPrimitives(extendedStructOfPrimitives)) {
        log("checkExtendedStructOfPrimitives: structOfPrimitives has invalid content");
        return false;
    }
    return true;
};

// Javascript has only one sort of floating point numbers

IltUtil.cmpFloat = function(a, b) {
    return Math.abs(a - b) < 0.001;
};

IltUtil.cmpDouble = function(a, b) {
    return Math.abs(a - b) < 0.001;
};

module.exports = IltUtil;
