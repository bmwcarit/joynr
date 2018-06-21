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

const Promise = require("bluebird").Promise;

const joynr = require("joynr");
const testbase = require("test-base");
const prettyLog = testbase.logging.prettyLog;

const IltUtil = require("./IltUtil.js");
const ExtendedEnumerationWithPartlyDefinedValues = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedEnumerationWithPartlyDefinedValues.js");
const ExtendedTypeCollectionEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection.js");
const Enumeration = require("../generated-javascript/joynr/interlanguagetest/Enumeration.js");

const MethodWithAnonymousErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithAnonymousErrorEnumErrorEnum.js");
const ExtendedErrorEnumTc = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedErrorEnumTc.js");
const MethodWithExtendedErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithExtendedErrorEnumErrorEnum.js");
const MapStringString = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/MapStringString.js");

// Attributes
let attributeUInt8 = 0;
let attributeDouble = 0.0;
let attributeBooleanReadonly = false;
let attributeStringNoSubscriptions = "";
let attributeInt8readonlyNoSubscriptions = 0;
let attributeArrayOfStringImplicit = [""];
let attributeByteBuffer;
let attributeEnumeration;
let attributeExtendedEnumerationReadonly;
let attributeBaseStruct;
let attributeExtendedExtendedBaseStruct;
let attributeMapStringString;
let attributeFireAndForget = 0;
const typeDefValues = {
    attributeInt64: 1,
    attributeString: "TypeDefString",
    attributeStruct: IltUtil.createBaseStruct(),
    attributeMap: new MapStringString(),
    attributeEnum: Enumeration.ENUM_0_VALUE_1,
    attributeByteBufferTypeDef: IltUtil.createByteArray(),
    attributeArrayTypeDef: IltUtil.createStringArray()
};

function genericSetterGetter(attributeName) {
    return {
        set: async value => {
            prettyLog(`IltProvider.set called for attribute ${attributeName}`);
            typeDefValues[attributeName] = value;
        },
        get: async () => {
            prettyLog(`IltProvider.get called for attribute ${attributeName}`);
            return typeDefValues[attributeName];
        }
    };
}

const IltProvider = {
    // attribute getter and setter
    attributeUInt8: {
        get() {
            prettyLog("IltProvider.attributeUInt8.get() called");
            return Promise.resolve(attributeUInt8);
        },
        set(value) {
            prettyLog(`IltProvider.attributeUInt8.set(${value}) called`);
            attributeUInt8 = value;
            IltProvider.attributeUInt8.valueChanged(attributeUInt8);
            return Promise.resolve();
        }
    },

    attributeDouble: {
        get() {
            prettyLog("IltProvider.attributeDouble.get() called");
            return Promise.resolve(attributeDouble);
        },
        set(value) {
            prettyLog(`IltProvider.attributeDouble.set(${value}) called`);
            attributeDouble = value;
            IltProvider.attributeDouble.valueChanged(attributeDouble);
            return Promise.resolve();
        }
    },

    attributeBooleanReadonly: {
        get() {
            prettyLog("IltProvider.attributeBooleanReadonly.get() called");
            attributeBooleanReadonly = true;
            return Promise.resolve(attributeBooleanReadonly);
        }
    },

    attributeStringNoSubscriptions: {
        get() {
            prettyLog("IltProvider.attributeStringNoSubscriptions.get() called");
            return Promise.resolve(attributeStringNoSubscriptions);
        },
        set(value) {
            prettyLog(`IltProvider.attributeStringNoSubscriptions.set(${value}) called`);
            attributeStringNoSubscriptions = value;
            return Promise.resolve();
        }
    },

    attributeInt8readonlyNoSubscriptions: {
        get() {
            prettyLog("IltProvider.attributeInt8readonlyNoSubscriptions.get() called");
            attributeInt8readonlyNoSubscriptions = -128;
            return Promise.resolve(attributeInt8readonlyNoSubscriptions);
        }
    },

    attributeArrayOfStringImplicit: {
        get() {
            prettyLog("IltProvider.attributeArrayOfStringImplicit.get() called");
            return Promise.resolve(attributeArrayOfStringImplicit);
        },
        set(value) {
            prettyLog(`IltProvider.attributeArrayOfStringImplicit.set(${value}) called`);
            attributeArrayOfStringImplicit = value;
            IltProvider.attributeArrayOfStringImplicit.valueChanged(attributeArrayOfStringImplicit);
            return Promise.resolve();
        }
    },

    attributeByteBuffer: {
        get() {
            prettyLog("IltProvider.attributeByteBuffer.get() called");
            return Promise.resolve(attributeByteBuffer);
        },
        set(value) {
            prettyLog(`IltProvider.attributeByteBuffer.set(${value}) called`);
            attributeByteBuffer = value;
            IltProvider.attributeByteBuffer.valueChanged(attributeByteBuffer);
            return Promise.resolve();
        }
    },

    attributeEnumeration: {
        get() {
            prettyLog("IltProvider.attributeEnumeration.get() called");
            return Promise.resolve(attributeEnumeration);
        },
        set(value) {
            prettyLog(`IltProvider.attributeEnumeration.set(${value}) called`);
            attributeEnumeration = value;
            IltProvider.attributeEnumeration.valueChanged(attributeEnumeration);
            return Promise.resolve();
        }
    },

    attributeExtendedEnumerationReadonly: {
        get() {
            prettyLog("IltProvider.attributeExtendedEnumerationReadonly.get() called");
            attributeExtendedEnumerationReadonly =
                ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
            return Promise.resolve(attributeExtendedEnumerationReadonly);
        }
    },

    attributeBaseStruct: {
        get() {
            prettyLog("IltProvider.attributeBaseStruct.get() called");
            return Promise.resolve(attributeBaseStruct);
        },
        set(value) {
            prettyLog(`IltProvider.attributeBaseStruct.set(${value}) called`);
            attributeBaseStruct = value;
            IltProvider.attributeBaseStruct.valueChanged(attributeBaseStruct);
            return Promise.resolve();
        }
    },

    attributeExtendedExtendedBaseStruct: {
        get() {
            prettyLog("IltProvider.attributeExtendedExtendedBaseStruct.get() called");
            return Promise.resolve(attributeExtendedExtendedBaseStruct);
        },
        set(value) {
            prettyLog(`IltProvider.attributeExtendedExtendedBaseStruct.set(${value}) called`);
            attributeExtendedExtendedBaseStruct = value;
            IltProvider.attributeExtendedExtendedBaseStruct.valueChanged(attributeExtendedExtendedBaseStruct);
            return Promise.resolve();
        }
    },

    attributeMapStringString: {
        get() {
            prettyLog("IltProvider.attributeMapStringString.get() called");
            return Promise.resolve(attributeMapStringString);
        },
        set(value) {
            prettyLog(`IltProvider.attributeMapStringString.set(${JSON.stringify(value)}) called`);
            attributeMapStringString = value;
            IltProvider.attributeMapStringString.valueChanged(attributeMapStringString);
            return Promise.resolve();
        }
    },

    attributeInt64TypeDef: genericSetterGetter("attributeInt64"),

    attributeStringTypeDef: genericSetterGetter("attributeString"),

    attributeStructTypeDef: genericSetterGetter("attributeStruct"),

    attributeMapTypeDef: genericSetterGetter("attributeMap"),

    attributeEnumTypeDef: genericSetterGetter("attributeEnum"),

    attributeByteBufferTypeDef: genericSetterGetter("attributeByteBuffer"),

    attributeArrayTypeDef: genericSetterGetter("attributeArray"),

    attributeFireAndForget: {
        get() {
            prettyLog("IltProvider.attributeFireAndForget.get() called");
            return Promise.resolve(attributeFireAndForget);
        },
        set(value) {
            prettyLog(`IltProvider.attributeFireAndForget.set(${value}) called`);

            attributeFireAndForget = value;
            IltProvider.attributeFireAndForget.valueChanged(attributeFireAndForget);
            return Promise.resolve();
        }
    },

    attributeWithExceptionFromGetter: {
        get() {
            prettyLog("IltProvider.attributeWithExceptionFromGetter.get() called");

            const settings = {};
            settings.detailMessage = "Exception from getAttributeWithExceptionFromGetter";
            return Promise.reject(new joynr.exceptions.ProviderRuntimeException(settings));
        }
    },

    attributeWithExceptionFromSetter: {
        get() {
            prettyLog("IltProvider.attributeWithExceptionFromSetter.get() called");
            return Promise.resolve(false);
        },
        set(value) {
            prettyLog(`IltProvider.attributeWithExceptionFromSetter.set(${value}) called`);

            const settings = {};
            settings.detailMessage = "Exception from setAttributeWithExceptionFromSetter";
            return Promise.reject(new joynr.exceptions.ProviderRuntimeException(settings));
        }
    },

    // methods
    methodWithoutParameters() {
        prettyLog("IltProvider.methodWithoutParameters() called");
        return Promise.resolve();
    },

    methodWithoutInputParameter() {
        prettyLog("IltProvider.methodWithoutInputParameter() called");
        return Promise.resolve({ booleanOut: true });
    },

    methodWithoutOutputParameter(opArgs) {
        prettyLog(`IltProvider.methodWithoutOutputParameter(${JSON.stringify(opArgs)}) called`);

        if (opArgs.booleanArg === undefined || opArgs.booleanArg === null || opArgs.booleanArg !== false) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithoutOutputParameter: received wrong argument"
                })
            );
        } else {
            return Promise.resolve();
        }
    },

    methodWithSinglePrimitiveParameters(opArgs) {
        prettyLog(`IltProvider.methodWithSinglePrimitiveParameters(${JSON.stringify(opArgs)}) called`);
        if (opArgs.uInt16Arg === undefined || opArgs.uInt16Arg === null || opArgs.uInt16Arg !== 32767) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithSinglePrimitiveParameters: invalid argument uInt16Arg"
                })
            );
        } else {
            opArgs.uInt16Arg = 32767;
            return Promise.resolve({ stringOut: opArgs.uInt16Arg.toString() });
        }
    },

    methodWithMultiplePrimitiveParameters(opArgs) {
        prettyLog(`IltProvider.methodWithMultiplePrimitiveParameters(${JSON.stringify(opArgs)}) called`);

        if (opArgs.int32Arg === undefined || opArgs.int32Arg === null || opArgs.int32Arg !== 2147483647) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultiplePrimitiveParameters: invalid argument int32Arg"
                })
            );
        } else if (
            opArgs.floatArg === undefined ||
            opArgs.floatArg === null ||
            !IltUtil.cmpFloat(opArgs.floatArg, 47.11)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultiplePrimitiveParameters: invalid argument floatArg"
                })
            );
        }
        if (opArgs.booleanArg === undefined || opArgs.booleanArg === null || opArgs.booleanArg !== false) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultiplePrimitiveParameters: invalid argument booleanArg"
                })
            );
        } else {
            return Promise.resolve({
                stringOut: opArgs.int32Arg.toString(),
                doubleOut: opArgs.floatArg
            });
        }
    },

    methodWithSingleArrayParameters(opArgs) {
        prettyLog(`IltProvider.methodWithSingleArrayParameters(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.doubleArrayArg === undefined ||
            opArgs.doubleArrayArg === null ||
            !IltUtil.checkDoubleArray(opArgs.doubleArrayArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithSingleArrayParameters: invalid argument doubleArrayArg"
                })
            );
        } else {
            return Promise.resolve({ stringArrayOut: IltUtil.createStringArray() });
        }
    },

    methodWithMultipleArrayParameters(opArgs) {
        prettyLog(`IltProvider.methodWithMultipleArrayParameters(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.stringArrayArg === undefined ||
            opArgs.stringArrayArg === null ||
            !IltUtil.checkStringArray(opArgs.stringArrayArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleArrayParameters: invalid argument stringArrayArg"
                })
            );
        } else if (
            opArgs.int8ArrayArg === undefined ||
            opArgs.int8ArrayArg === null ||
            !IltUtil.checkByteArray(opArgs.int8ArrayArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleArrayParameters: invalid argument int8ArrayArg"
                })
            );
        } else if (
            opArgs.enumArrayArg === undefined ||
            opArgs.enumArrayArg === null ||
            !IltUtil.checkExtendedInterfaceEnumerationInTypeCollectionArray(opArgs.enumArrayArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleArrayParameters: invalid argument enumArrayArg"
                })
            );
        } else if (
            opArgs.structWithStringArrayArrayArg === undefined ||
            opArgs.structWithStringArrayArrayArg === null ||
            !IltUtil.checkStructWithStringArrayArray(opArgs.structWithStringArrayArrayArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleArrayParameters: invalid argument structWithStringArrayArrayArg"
                })
            );
        } else {
            return Promise.resolve({
                uInt64ArrayOut: IltUtil.createUInt64Array(),
                structWithStringArrayArrayOut: [
                    IltUtil.createStructWithStringArray(),
                    IltUtil.createStructWithStringArray()
                ]
            });
        }
    },

    methodWithSingleByteBufferParameter(opArgs) {
        prettyLog(`IltProvider.methodWithSingleByteBufferParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.byteBufferIn === undefined || opArgs.byteBufferIn === null) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithSingleByteBufferParameter: received wrong argument"
                })
            );
        }
        return Promise.resolve({ byteBufferOut: opArgs.byteBufferIn });
    },

    methodWithMultipleByteBufferParameters(opArgs) {
        prettyLog(`IltProvider.methodWithMultipleByteBufferParameters(${JSON.stringify(opArgs)}) called`);
        if (opArgs.byteBufferIn1 === undefined || opArgs.byteBufferIn1 === null) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleByteBufferParameters: invalid argument byteBufferIn1"
                })
            );
        }
        if (opArgs.byteBufferIn2 === undefined || opArgs.byteBufferIn2 === null) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleByteBufferParameters: invalid argument byteBufferIn2"
                })
            );
        }
        return Promise.resolve({ byteBufferOut: opArgs.byteBufferIn1.concat(opArgs.byteBufferIn2) });
    },

    async methodWithInt64TypeDefParameter(opArgs) {
        prettyLog(`IltProvider.methodWithInt64TypeDefParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.int64TypeDefIn === undefined || opArgs.int64TypeDefIn === null) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "methodWithInt64TypeDefParameter: received wrong argument"
            });
        }
        return { int64TypeDefOut: opArgs.int64TypeDefIn };
    },

    async methodWithStringTypeDefParameter(opArgs) {
        prettyLog(`IltProvider.methodWithStringTypeDefParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.stringTypeDefIn === undefined || opArgs.stringTypeDefIn === null) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "methodWithStringTypeDefParameter: received wrong argument"
            });
        }
        return { stringTypeDefOut: opArgs.stringTypeDefIn };
    },

    async methodWithStructTypeDefParameter(opArgs) {
        prettyLog(`IltProvider.methodWithStructTypeDefParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.structTypeDefIn === undefined || opArgs.structTypeDefIn === null) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "methodWithStructTypeDefParameter: received wrong argument"
            });
        }
        return { structTypeDefOut: opArgs.structTypeDefIn };
    },

    async methodWithMapTypeDefParameter(opArgs) {
        prettyLog(`IltProvider.methodWithMapTypeDefParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.mapTypeDefIn === undefined || opArgs.mapTypeDefIn === null) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "methodWithMapTypeDefParameter: received wrong argument"
            });
        }
        return { mapTypeDefOut: opArgs.mapTypeDefIn };
    },

    async methodWithEnumTypeDefParameter(opArgs) {
        prettyLog(`IltProvider.methodWithEnumTypeDefParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.enumTypeDefIn === undefined || opArgs.enumTypeDefIn === null) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "methodWithEnumTypeDefParameter: received wrong argument"
            });
        }
        return { enumTypeDefOut: opArgs.enumTypeDefIn };
    },

    async methodWithByteBufferTypeDefParameter(opArgs) {
        prettyLog(`IltProvider.methodWithByteBufferTypeDefParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.byteBufferTypeDefIn === undefined || opArgs.byteBufferTypeDefIn === null) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "methodWithByteBufferTypeDefParameter: received wrong argument"
            });
        }
        return { byteBufferTypeDefOut: opArgs.byteBufferTypeDefIn };
    },

    async methodWithArrayTypeDefParameter(opArgs) {
        prettyLog(`IltProvider.methodWithArrayTypeDefParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.arrayTypeDefIn === undefined || opArgs.arrayTypeDefIn === null) {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "methodWithArrayTypeDefParameter: received wrong argument"
            });
        }
        return { arrayTypeDefOut: opArgs.arrayTypeDefIn };
    },

    methodWithSingleEnumParameters(opArgs) {
        prettyLog(`IltProvider.methodWithSingleEnumParameters(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.enumerationArg === undefined ||
            opArgs.enumerationArg === null ||
            opArgs.enumerationArg !==
                ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithSingleEnumParameters: invalid argument enumerationArg"
                })
            );
        } else {
            return Promise.resolve({
                enumerationOut:
                    ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
            });
        }
    },

    methodWithMultipleEnumParameters(opArgs) {
        prettyLog(`IltProvider.methodWithMultipleEnumParameters(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.enumerationArg === undefined ||
            opArgs.enumerationArg === null ||
            opArgs.enumerationArg !== Enumeration.ENUM_0_VALUE_3
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleEnumParameters: invalid argument enumerationArg"
                })
            );
        } else if (
            opArgs.extendedEnumerationArg === undefined ||
            opArgs.extendedEnumerationArg === null ||
            opArgs.extendedEnumerationArg !==
                ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleEnumParameters: invalid argument extendedEnumerationArg"
                })
            );
        } else {
            return Promise.resolve({
                extendedEnumerationOut:
                    ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES,
                enumerationOut: Enumeration.ENUM_0_VALUE_1
            });
        }
    },

    methodWithSingleMapParameters(opArgs) {
        prettyLog(`IltProvider.methodWithSingleMapParameters(${JSON.stringify(opArgs)}) called`);

        if (opArgs.mapArg === undefined || opArgs.mapArg === null) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithSingleMapParameters: invalid argument mapArg"
                })
            );
        } else {
            const mapOut = new MapStringString();
            for (let i = 1; i <= 3; i++) {
                mapOut.put(opArgs.mapArg.get(`keyString${i}`), `keyString${i}`);
            }
            return Promise.resolve({ mapOut });
        }
    },

    methodWithSingleStructParameters(opArgs) {
        prettyLog(`IltProvider.methodWithSingleStructParameters(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.extendedBaseStructArg === undefined ||
            opArgs.extendedBaseStructArg === null ||
            !IltUtil.checkExtendedBaseStruct(opArgs.extendedBaseStructArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithSingleStructParameters: invalid argument extendedBaseStructArg"
                })
            );
        } else {
            return Promise.resolve({ extendedStructOfPrimitivesOut: IltUtil.createExtendedStructOfPrimitives() });
        }
    },

    methodWithMultipleStructParameters(opArgs) {
        prettyLog(`IltProvider.methodWithMultipleStructParameters(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.extendedStructOfPrimitivesArg === undefined ||
            opArgs.extendedStructOfPrimitivesArg === null ||
            !IltUtil.checkExtendedStructOfPrimitives(opArgs.extendedStructOfPrimitivesArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleStructParameters: invalid argument extendedStructOfPrimitivesArg"
                })
            );
        } else if (
            opArgs.baseStructArg === undefined ||
            opArgs.baseStructArg === null ||
            !IltUtil.checkBaseStruct(opArgs.baseStructArg)
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithMultipleStructParameters: invalid argument baseStructArg"
                })
            );
        } else {
            return Promise.resolve({
                baseStructWithoutElementsOut: IltUtil.createBaseStructWithoutElements(),
                extendedExtendedBaseStructOut: IltUtil.createExtendedExtendedBaseStruct()
            });
        }
    },

    methodWithStringsAndSpecifiedStringOutLength(opArgs) {
        prettyLog(`IltProvider.methodWithStringsAndSpecifiedStringOutLength(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.int32StringLengthArg === undefined ||
            opArgs.int32StringLengthArg === null ||
            opArgs.int32StringLengthArg > 1024 * 1024
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodWithStringsAndSpecifiedStringOutLength: Maximum length exceeded"
                })
            );
        } else {
            let stringOutValue = "";
            for (let i = 0; i < opArgs.int32StringLengthArg; i++) {
                stringOutValue += "A";
            }
            return Promise.resolve({ stringOut: stringOutValue });
        }
    },

    // FIRE-AND-FORGET METHODS
    methodFireAndForgetWithoutParameter(opArgs) {
        prettyLog(`IltProvider.methodFireAndForgetWithoutParameter(${JSON.stringify(opArgs)}) called`);
        IltProvider.attributeFireAndForget.set(attributeFireAndForget + 1);
    },

    methodFireAndForgetWithInputParameter(opArgs) {
        prettyLog(`IltProvider.methodFireAndForgetWithInputParameter(${JSON.stringify(opArgs)}) called`);
        if (opArgs.int32Arg === undefined || opArgs.int32Arg === null || typeof opArgs.int32Arg !== "number") {
            prettyLog("methodFireAndForgetWithInputParameter: invalid argument int32Arg");
            IltProvider.attributeFireAndForget.set(-1);
        } else {
            IltProvider.attributeFireAndForget.set(opArgs.int32Arg);
        }
    },

    // OVERLOADED METHODS
    overloadedMethod(opArgs) {
        prettyLog(`IltProvider.overloadedMethod(${JSON.stringify(opArgs)}) called`);
        if (
            opArgs.int64Arg !== undefined &&
            opArgs.int64Arg !== null &&
            opArgs.booleanArg !== undefined &&
            opArgs.booleanArg !== null &&
            opArgs.enumArrayArg !== undefined &&
            opArgs.enumArrayArg !== null &&
            opArgs.baseStructArg !== undefined &&
            opArgs.baseStructArg !== null
        ) {
            prettyLog("IltProvider.overloadedMethod (3)");

            if (opArgs.int64Arg !== 1) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethod_3: invalid argument int64Arg"
                    })
                );
            } else if (opArgs.booleanArg !== false) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethod_3: invalid argument booleanArg"
                    })
                );
            } else if (!IltUtil.checkExtendedExtendedEnumerationArray(opArgs.enumArrayArg)) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethod_3: invalid argument enumArrayArg"
                    })
                );
            } else if (!IltUtil.checkBaseStruct(opArgs.baseStructArg)) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethod_3: invalid argument baseStructArg"
                    })
                );
            } else {
                return Promise.resolve({
                    doubleOut: 0,
                    stringArrayOut: IltUtil.createStringArray(),
                    extendedBaseStructOut: IltUtil.createExtendedBaseStruct()
                });
            }
        } else if (opArgs.booleanArg !== undefined && opArgs.booleanArg !== null) {
            prettyLog("IltProvider.overloadedMethod (2)");

            if (opArgs.booleanArg !== false) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethod_2: invalid argument booleanArg"
                    })
                );
            } else {
                return Promise.resolve({ stringOut: "TestString 2" });
            }
        } else if (opArgs !== undefined && opArgs !== null) {
            prettyLog("IltProvider.overloadedMethod (1)");

            return Promise.resolve({ stringOut: "TestString 1" });
        } else {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "overloadedMethod: invalid arguments"
                })
            );
        }
    },

    overloadedMethodWithSelector(opArgs) {
        prettyLog(`IltProvider.overloadedMethodWithSelector(${JSON.stringify(opArgs)}) called`);
        if (
            opArgs.int64Arg !== undefined &&
            opArgs.int64Arg !== null &&
            opArgs.booleanArg !== undefined &&
            opArgs.booleanArg !== null &&
            opArgs.enumArrayArg !== undefined &&
            opArgs.enumArrayArg !== null &&
            opArgs.baseStructArg !== undefined &&
            opArgs.baseStructArg !== null
        ) {
            prettyLog("IltProvider.overloadedMethodWithSelector (3)");

            if (opArgs.int64Arg !== 1) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethodWithSelector_3: invalid argument int64Arg"
                    })
                );
            } else if (opArgs.booleanArg !== false) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethodWithSelector_3: invalid argument booleanArg"
                    })
                );
            } else if (!IltUtil.checkExtendedExtendedEnumerationArray(opArgs.enumArrayArg)) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethodWithSelector_3: invalid argument enumArrayArg"
                    })
                );
            } else if (!IltUtil.checkBaseStruct(opArgs.baseStructArg)) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethodWithSelector_3: invalid argument baseStructArg"
                    })
                );
            } else {
                return Promise.resolve({
                    doubleOut: 1.1,
                    stringArrayOut: IltUtil.createStringArray(),
                    extendedBaseStructOut: IltUtil.createExtendedBaseStruct()
                });
            }
        } else if (opArgs.booleanArg !== undefined && opArgs.booleanArg !== null) {
            prettyLog("IltProvider.overloadedMethodWithSelector (2)");

            if (opArgs.booleanArg !== false) {
                return Promise.reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "overloadedMethodWithSelector_2: invalid argument booleanArg"
                    })
                );
            } else {
                return Promise.resolve({ stringOut: "Return value from overloadedMethodWithSelector 2" });
            }
        } else if (opArgs !== undefined && opArgs !== null) {
            prettyLog("IltProvider.overloadedMethodWithSelector (1)");

            return Promise.resolve({ stringOut: "Return value from overloadedMethodWithSelector 1" });
        } else {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "overloadedMethodWithSelector: invalid arguments"
                })
            );
        }
    },

    // METHODS WITH ERROR DEFINITIONS

    methodWithoutErrorEnum(opArgs) {
        prettyLog(`IltProvider.methodWithoutErrorEnum(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ProviderRuntimeException"
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "Exception from methodWithoutErrorEnum"
                })
            );
        } else {
            return Promise.resolve();
        }
    },

    methodWithAnonymousErrorEnum(opArgs) {
        prettyLog(`IltProvider.methodWithAnonymousErrorEnum(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ProviderRuntimeException"
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "Exception from methodWithAnonymousErrorEnum"
                })
            );
        } else if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ApplicationException"
        ) {
            return Promise.reject(MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC);
        } else {
            return Promise.resolve();
        }
    },

    methodWithExistingErrorEnum(opArgs) {
        prettyLog(`IltProvider.methodWithExistingErrorEnum(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ProviderRuntimeException"
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "Exception from methodWithExistingErrorEnum"
                })
            );
        } else if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ApplicationException_1"
        ) {
            return Promise.reject(ExtendedErrorEnumTc.ERROR_2_3_TC2);
        } else if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ApplicationException_2"
        ) {
            return Promise.reject(ExtendedErrorEnumTc.ERROR_1_2_TC_2);
        } else {
            return Promise.resolve();
        }
    },

    methodWithExtendedErrorEnum(opArgs) {
        prettyLog(`IltProvider.methodWithExtendedErrorEnum(${JSON.stringify(opArgs)}) called`);

        if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ProviderRuntimeException"
        ) {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "Exception from methodWithExtendedErrorEnum"
                })
            );
        } else if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ApplicationException_1"
        ) {
            return Promise.reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC);
        } else if (
            opArgs.wantedExceptionArg !== undefined &&
            opArgs.wantedExceptionArg !== null &&
            opArgs.wantedExceptionArg === "ApplicationException_2"
        ) {
            return Promise.reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_2_1_TC2);
        } else {
            return Promise.resolve();
        }
    },

    // BROADCASTS aka events

    broadcastWithSinglePrimitiveParameter: {},
    broadcastWithMultiplePrimitiveParameters: {},
    broadcastWithSingleArrayParameter: {},
    broadcastWithMultipleArrayParameters: {},
    broadcastWithSingleByteBufferParameter: {},
    broadcastWithMultipleByteBufferParameters: {},
    broadcastWithSingleEnumerationParameter: {},
    broadcastWithMultipleEnumerationParameters: {},
    broadcastWithSingleStructParameter: {},
    broadcastWithMultipleStructParameters: {},
    broadcastWithFiltering: {},

    methodToFireBroadcastWithSinglePrimitiveParameter(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithSinglePrimitiveParameter(${JSON.stringify(opArgs)}) called`);

        const stringOut = "boom";
        const outputParameters = IltProvider.broadcastWithSinglePrimitiveParameter.createBroadcastOutputParameters();
        outputParameters.setStringOut(stringOut);
        IltProvider.broadcastWithSinglePrimitiveParameter.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithMultiplePrimitiveParameters(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithMultiplePrimitiveParameters(${JSON.stringify(opArgs)}) called`);

        const stringOut = "boom";
        const doubleOut = 1.1;
        const outputParameters = IltProvider.broadcastWithMultiplePrimitiveParameters.createBroadcastOutputParameters();
        outputParameters.setStringOut(stringOut);
        outputParameters.setDoubleOut(doubleOut);
        IltProvider.broadcastWithMultiplePrimitiveParameters.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithSingleArrayParameter(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithSingleArrayParameter(${JSON.stringify(opArgs)}) called`);

        const stringArrayOut = IltUtil.createStringArray();
        const outputParameters = IltProvider.broadcastWithSingleArrayParameter.createBroadcastOutputParameters();
        outputParameters.setStringArrayOut(stringArrayOut);
        IltProvider.broadcastWithSingleArrayParameter.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithMultipleArrayParameters(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithMultipleArrayParameters(${JSON.stringify(opArgs)}) called`);

        const uInt64ArrayOut = IltUtil.createUInt64Array();
        const structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();
        const outputParameters = IltProvider.broadcastWithMultipleArrayParameters.createBroadcastOutputParameters();
        outputParameters.setUInt64ArrayOut(uInt64ArrayOut);
        outputParameters.setStructWithStringArrayArrayOut(structWithStringArrayArrayOut);
        IltProvider.broadcastWithMultipleArrayParameters.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithSingleByteBufferParameter(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithSingleByteBufferParameter(${JSON.stringify(opArgs)}) called`);
        const outputParameters = IltProvider.broadcastWithSingleByteBufferParameter.createBroadcastOutputParameters();
        outputParameters.setByteBufferOut(opArgs.byteBufferIn);
        IltProvider.broadcastWithSingleByteBufferParameter.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithMultipleByteBufferParameters(opArgs) {
        prettyLog(
            `IltProvider.methodToFireBroadcastWithMultipleByteBufferParameters(${JSON.stringify(opArgs)}) called`
        );
        const outputParameters = IltProvider.broadcastWithMultipleByteBufferParameters.createBroadcastOutputParameters();
        outputParameters.setByteBufferOut1(opArgs.byteBufferIn1);
        outputParameters.setByteBufferOut2(opArgs.byteBufferIn2);
        IltProvider.broadcastWithMultipleByteBufferParameters.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithSingleEnumerationParameter(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithSingleEnumerationParameter(${JSON.stringify(opArgs)}) called`);

        const enumerationOut =
            ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        const outputParameters = IltProvider.broadcastWithSingleEnumerationParameter.createBroadcastOutputParameters();
        outputParameters.setEnumerationOut(enumerationOut);
        IltProvider.broadcastWithSingleEnumerationParameter.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithMultipleEnumerationParameters(opArgs) {
        prettyLog(
            `IltProvider.methodToFireBroadcastWithMultipleEnumerationParameters(${JSON.stringify(opArgs)}) called`
        );

        const extendedEnumerationOut =
            ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        const enumerationOut = Enumeration.ENUM_0_VALUE_1;
        const outputParameters = IltProvider.broadcastWithMultipleEnumerationParameters.createBroadcastOutputParameters();
        outputParameters.setExtendedEnumerationOut(extendedEnumerationOut);
        outputParameters.setEnumerationOut(enumerationOut);
        IltProvider.broadcastWithMultipleEnumerationParameters.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithSingleStructParameter(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithSingleStructParameter(${JSON.stringify(opArgs)}) called`);

        const extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
        const outputParameters = IltProvider.broadcastWithSingleStructParameter.createBroadcastOutputParameters();
        outputParameters.setExtendedStructOfPrimitivesOut(extendedStructOfPrimitivesOut);
        IltProvider.broadcastWithSingleStructParameter.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithMultipleStructParameters(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithMultipleStructParameters(${JSON.stringify(opArgs)}) called`);

        const baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
        const extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();
        const outputParameters = IltProvider.broadcastWithMultipleStructParameters.createBroadcastOutputParameters();
        outputParameters.setBaseStructWithoutElementsOut(baseStructWithoutElementsOut);
        outputParameters.setExtendedExtendedBaseStructOut(extendedExtendedBaseStructOut);
        IltProvider.broadcastWithMultipleStructParameters.fire(outputParameters, opArgs.partitions);
        return Promise.resolve();
    },

    methodToFireBroadcastWithFiltering(opArgs) {
        prettyLog(`IltProvider.methodToFireBroadcastWithFiltering(${JSON.stringify(opArgs)}) called`);

        const stringOut = opArgs.stringArg;
        const stringArrayOut = IltUtil.createStringArray();
        const enumerationOut =
            ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        const structWithStringArrayOut = IltUtil.createStructWithStringArray();
        const structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();
        const outputParameters = IltProvider.broadcastWithFiltering.createBroadcastOutputParameters();
        if (stringOut === undefined || stringOut === null || typeof stringOut !== "string") {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "methodToFireBroadcastWithFiltering: received wrong argument"
                })
            );
        } else {
            outputParameters.setStringOut(stringOut);
            outputParameters.setStringArrayOut(stringArrayOut);
            outputParameters.setEnumerationOut(enumerationOut);
            outputParameters.setStructWithStringArrayOut(structWithStringArrayOut);
            outputParameters.setStructWithStringArrayArrayOut(structWithStringArrayArrayOut);
            IltProvider.broadcastWithFiltering.fire(outputParameters);
            return Promise.resolve();
        }
    }
};

exports.implementation = IltProvider;
