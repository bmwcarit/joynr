/*jslint node: true */

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

var Promise = require("bluebird").Promise;

var joynr = require("joynr");
var testbase = require("test-base");
var log = testbase.logging.log;
var prettyLog = testbase.logging.prettyLog;
var error = testbase.logging.error;

var IltUtil = require("./IltUtil.js");
var ExtendedEnumerationWithPartlyDefinedValues = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedEnumerationWithPartlyDefinedValues.js");
var ExtendedTypeCollectionEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection.js");
var Enumeration = require("../generated-javascript/joynr/interlanguagetest/Enumeration.js");

var MethodWithAnonymousErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithAnonymousErrorEnumErrorEnum.js");
var ExtendedErrorEnumTc = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedErrorEnumTc.js")
var MethodWithExtendedErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithExtendedErrorEnumErrorEnum.js");
var MapStringString = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/MapStringString.js");

var iltProvider;

// Attributes
var attributeUInt8 = 0;
var attributeDouble = 0.0;
var attributeBooleanReadonly = false;
var attributeStringNoSubscriptions = "";
var attributeInt8readonlyNoSubscriptions = 0;
var attributeArrayOfStringImplicit = [ "" ];
var attributeEnumeration;
var attributeExtendedEnumerationReadonly;
var attributeBaseStruct;
var attributeExtendedExtendedBaseStruct;
var attributeMapStringString;
var attributeFireAndForget = 0;

exports.setProvider = function(provider) {
    iltProvider = provider;
};

exports.implementation = {
    // attribute getter and setter
    attributeUInt8 : {
        get : function() {
            prettyLog("IltProvider.attributeUInt8.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeUInt8);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeUInt8.set(" + value + ") called");
            attributeUInt8 = value;
            self.attributeUInt8.valueChanged(attributeUInt8);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeDouble : {
        get : function() {
            prettyLog("IltProvider.attributeDouble.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeDouble);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeDouble.set(" + value + ") called");
            attributeDouble = value;
            self.attributeDouble.valueChanged(attributeDouble);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeBooleanReadonly : {
        get : function() {
            prettyLog("IltProvider.attributeBooleanReadonly.get() called");
            return new Promise(function(resolve, reject) {
                attributeBooleanReadonly = true;
                resolve(attributeBooleanReadonly);
            });
        }
    },


    attributeStringNoSubscriptions : {
        get : function() {
            prettyLog("IltProvider.attributeStringNoSubscriptions.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeStringNoSubscriptions);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeStringNoSubscriptions.set(" + value + ") called");
            attributeStringNoSubscriptions = value;
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeInt8readonlyNoSubscriptions : {
        get : function() {
            prettyLog("IltProvider.attributeInt8readonlyNoSubscriptions.get() called");
            return new Promise(function(resolve, reject) {
                attributeInt8readonlyNoSubscriptions = -128;
                resolve(attributeInt8readonlyNoSubscriptions);
            });
        }
    },

    attributeArrayOfStringImplicit : {
        get : function() {
            prettyLog("IltProvider.attributeArrayOfStringImplicit.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeArrayOfStringImplicit);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeArrayOfStringImplicit.set(" + value + ") called");
            attributeArrayOfStringImplicit = value;
            self.attributeArrayOfStringImplicit.valueChanged(attributeArrayOfStringImplicit);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeEnumeration : {
        get : function() {
            prettyLog("IltProvider.attributeEnumeration.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeEnumeration);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeEnumeration.set(" + value + ") called");
            attributeEnumeration = value;
            self.attributeEnumeration.valueChanged(attributeEnumeration);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeExtendedEnumerationReadonly : {
        get : function() {
            prettyLog("IltProvider.attributeExtendedEnumerationReadonly.get() called");
            return new Promise(function(resolve, reject) {
                attributeExtendedEnumerationReadonly = ExtendedEnumerationWithPartlyDefinedValues
                    .ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
                resolve(attributeExtendedEnumerationReadonly);
            });
        }
    },

    attributeBaseStruct : {
        get : function() {
            prettyLog("IltProvider.attributeBaseStruct.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeBaseStruct);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeBaseStruct.set(" + value + ") called");
            attributeBaseStruct = value;
            self.attributeBaseStruct.valueChanged(attributeBaseStruct);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeExtendedExtendedBaseStruct : {
        get : function() {
            prettyLog("IltProvider.attributeExtendedExtendedBaseStruct.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeExtendedExtendedBaseStruct);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeExtendedExtendedBaseStruct.set(" + value + ") called");
            attributeExtendedExtendedBaseStruct = value;
            self.attributeExtendedExtendedBaseStruct.valueChanged(attributeExtendedExtendedBaseStruct);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeMapStringString : {
        get : function() {
            prettyLog("IltProvider.attributeMapStringString.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeMapStringString);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeMapStringString.set(" + JSON.stringify(value) + ") called");
            attributeMapStringString = value;
            self.attributeMapStringString.valueChanged(attributeMapStringString);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    attributeFireAndForget : {
        get : function() {
            prettyLog("IltProvider.attributeFireAndForget.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeFireAndForget);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeFireAndForget.set(" + value + ") called");
            return new Promise(function(resolve, reject) {
                attributeFireAndForget = value;
                self.attributeFireAndForget.valueChanged(attributeFireAndForget);
                resolve();
            });
        }
    },

    attributeWithExceptionFromGetter : {
        get : function() {
            prettyLog("IltProvider.attributeWithExceptionFromGetter.get() called");
            return new Promise(function(resolve, reject) {
                settings = {};
                settings.detailMessage = "Exception from getAttributeWithExceptionFromGetter";
                reject(new joynr.exceptions.ProviderRuntimeException(settings));
            });
        }
    },

    attributeWithExceptionFromSetter : {
        get : function() {
            prettyLog("IltProvider.attributeWithExceptionFromSetter.get() called");
            return new Promise(function(resolve, reject) {
                resolve(false);
            });
        },
        set : function(value) {
            prettyLog("IltProvider.attributeWithExceptionFromSetter.set(" + value + ") called");
            return new Promise(function(resolve, reject) {
                settings = {};
                settings.detailMessage = "Exception from setAttributeWithExceptionFromSetter";
                reject(new joynr.exceptions.ProviderRuntimeException(settings));
            });
        }
    },

    // methods
    methodWithoutParameters : function() {
        prettyLog("IltProvider.methodWithoutParameters() called");
        return new Promise(function(resolve, reject) {
            resolve();
        });
    },

    methodWithoutInputParameter : function() {
        prettyLog("IltProvider.methodWithoutInputParameter() called");
        return new Promise(function(resolve, reject) {
            resolve({booleanOut: true});
        });
    },

    methodWithoutOutputParameter : function(opArgs) {
        prettyLog("IltProvider.methodWithoutOutputParameter(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.booleanArg === undefined || opArgs.booleanArg === null || opArgs.booleanArg !== false) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithoutOutputParameter: received wrong argument"}));
            } else {
                resolve();
            }
        });
    },

    methodWithSinglePrimitiveParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithSinglePrimitiveParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.uInt16Arg === undefined || opArgs.uInt16Arg === null
                    || (opArgs.uInt16Arg !== 32767)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithSinglePrimitiveParameters: invalid argument uInt16Arg"}));
            } else {
                opArgs.uInt16Arg = 32767;
                resolve({stringOut: opArgs.uInt16Arg.toString()});
            }
        });
    },

    methodWithMultiplePrimitiveParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithMultiplePrimitiveParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.int32Arg === undefined || opArgs.int32Arg === null
                    || opArgs.int32Arg !== 2147483647) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultiplePrimitiveParameters: invalid argument int32Arg"}));
            } else if (opArgs.floatArg === undefined || opArgs.floatArg === null
                    || !IltUtil.cmpFloat(opArgs.floatArg, 47.11)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultiplePrimitiveParameters: invalid argument floatArg"}));
            } if (opArgs.booleanArg === undefined || opArgs.booleanArg === null
                    || opArgs.booleanArg !== false) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultiplePrimitiveParameters: invalid argument booleanArg"}));
            } else {
                resolve({
                    stringOut: opArgs.int32Arg.toString(),
                    doubleOut: opArgs.floatArg
                    });
            }
        });
    },

    methodWithSingleArrayParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithSingleArrayParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.doubleArrayArg === undefined || opArgs.doubleArrayArg === null
                    || !IltUtil.checkDoubleArray(opArgs.doubleArrayArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithSingleArrayParameters: invalid argument doubleArrayArg"}));
            } else {
                resolve({stringArrayOut: IltUtil.createStringArray()});
            }
        });
    },

    methodWithMultipleArrayParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithMultipleArrayParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.stringArrayArg === undefined || opArgs.stringArrayArg === null
                    || !IltUtil.checkStringArray(opArgs.stringArrayArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleArrayParameters: invalid argument stringArrayArg"}));
            } else if (opArgs.int8ArrayArg === undefined || opArgs.int8ArrayArg === null
                    || !IltUtil.checkByteArray(opArgs.int8ArrayArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleArrayParameters: invalid argument int8ArrayArg"}));
            } else if (opArgs.enumArrayArg === undefined || opArgs.enumArrayArg === null
                    || !IltUtil.checkExtendedInterfaceEnumerationInTypeCollectionArray(opArgs.enumArrayArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleArrayParameters: invalid argument enumArrayArg"}));
            } else if (opArgs.structWithStringArrayArrayArg === undefined || opArgs.structWithStringArrayArrayArg === null
                    || !IltUtil.checkStructWithStringArrayArray(opArgs.structWithStringArrayArrayArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleArrayParameters: invalid argument structWithStringArrayArrayArg"}));
            } else {
                resolve({
                    uInt64ArrayOut: IltUtil.createUInt64Array(),
                    structWithStringArrayArrayOut: [
                                                    IltUtil.createStructWithStringArray(),
                                                    IltUtil.createStructWithStringArray()
                                                    ]
                    });
            }
        });
    },

    methodWithSingleEnumParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithSingleEnumParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.enumerationArg === undefined || opArgs.enumerationArg === null
                    || opArgs.enumerationArg !== ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithSingleEnumParameters: invalid argument enumerationArg"}));
            } else {
                resolve({enumerationOut: ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION});
            }
        });
    },

    methodWithMultipleEnumParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithMultipleEnumParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.enumerationArg === undefined || opArgs.enumerationArg === null
                    || opArgs.enumerationArg !== Enumeration.ENUM_0_VALUE_3) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleEnumParameters: invalid argument enumerationArg"}));
            } else if (opArgs.extendedEnumerationArg === undefined || opArgs.extendedEnumerationArg === null
                    || opArgs.extendedEnumerationArg !== ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleEnumParameters: invalid argument extendedEnumerationArg"}));
            } else {
                resolve({
                    extendedEnumerationOut: ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES,
                    enumerationOut: Enumeration.ENUM_0_VALUE_1
                    });
            }
        });
    },

    methodWithSingleMapParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithSingleMapParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.mapArg === undefined || opArgs.mapArg === null) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithSingleMapParameters: invalid argument mapArg"}));
            } else {
                var mapOut = new MapStringString();
                for (var i = 1; i <= 3; i++) {
                    mapOut.put(opArgs.mapArg.get("keyString" + i), "keyString" + i)
                }
                resolve({mapOut: mapOut});
            }
        });
    },

    methodWithSingleStructParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithSingleStructParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.extendedBaseStructArg === undefined || opArgs.extendedBaseStructArg === null
                    || !IltUtil.checkExtendedBaseStruct(opArgs.extendedBaseStructArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithSingleStructParameters: invalid argument extendedBaseStructArg"}));
            } else {
                resolve({extendedStructOfPrimitivesOut: IltUtil.createExtendedStructOfPrimitives()});
            }
        });
    },

    methodWithMultipleStructParameters : function(opArgs) {
        prettyLog("IltProvider.methodWithMultipleStructParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.extendedStructOfPrimitivesArg === undefined || opArgs.extendedStructOfPrimitivesArg === null
                    || !IltUtil.checkExtendedStructOfPrimitives(opArgs.extendedStructOfPrimitivesArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleStructParameters: invalid argument extendedStructOfPrimitivesArg"}));
            } else if (opArgs.baseStructArg === undefined || opArgs.baseStructArg === null
                    || !IltUtil.checkBaseStruct(opArgs.baseStructArg)) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithMultipleStructParameters: invalid argument baseStructArg"}));
            } else {
                resolve({
                    baseStructWithoutElementsOut: IltUtil.createBaseStructWithoutElements(),
                    extendedExtendedBaseStructOut: IltUtil.createExtendedExtendedBaseStruct()
                    });
            }
        });
    },

    methodWithStringsAndSpecifiedStringOutLength : function(opArgs) {
        prettyLog("IltProvider.methodWithStringsAndSpecifiedStringOutLength(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.int32StringLengthArg === undefined || opArgs.int32StringLengthArg === null
                    || opArgs.int32StringLengthArg > 1024*1024) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithStringsAndSpecifiedStringOutLength: Maximum length exceeded"}));
            } else {
                var stringOutValue = "";
                for (i = 0; i < opArgs.int32StringLengthArg; i++) {
                    stringOutValue += "A";
                }
                resolve({stringOut: stringOutValue});
            }
        });
    },

    // FIRE-AND-FORGET METHODS
    methodFireAndForgetWithoutParameter : function(opArgs) {
        prettyLog("IltProvider.methodFireAndForgetWithoutParameter(" + JSON.stringify(opArgs) + ") called");
        self.attributeFireAndForget.set(attributeFireAndForget + 1);
    },

    methodFireAndForgetWithInputParameter : function(opArgs) {
        prettyLog("IltProvider.methodFireAndForgetWithInputParameter(" + JSON.stringify(opArgs) + ") called");
        if (opArgs.int32Arg === undefined || opArgs.int32Arg === null || typeof opArgs.int32Arg !== "number") {
            prettyLog("methodFireAndForgetWithInputParameter: invalid argument int32Arg")
            self.attributeFireAndForget.set(-1);
        } else {
            self.attributeFireAndForget.set(opArgs.int32Arg);
        }
    },

    // OVERLOADED METHODS
    overloadedMethod : function(opArgs) {
        prettyLog("IltProvider.overloadedMethod(" + JSON.stringify(opArgs) + ") called");
        if (opArgs.int64Arg !== undefined && opArgs.int64Arg !== null
                && opArgs.booleanArg !== undefined && opArgs.booleanArg !== null
                && opArgs.enumArrayArg !== undefined && opArgs.enumArrayArg !== null
                && opArgs.baseStructArg !== undefined && opArgs.baseStructArg !== null) {
            prettyLog("IltProvider.overloadedMethod (3)");
            return new Promise(function(resolve, reject) {
                if (opArgs.int64Arg !== 1) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethod_3: invalid argument int64Arg"}));
                } else if (opArgs.booleanArg !== false) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethod_3: invalid argument booleanArg"}));
                } else if (!IltUtil.checkExtendedExtendedEnumerationArray(opArgs.enumArrayArg)) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethod_3: invalid argument enumArrayArg"}));
                } else if (!IltUtil.checkBaseStruct(opArgs.baseStructArg)) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethod_3: invalid argument baseStructArg"}));
                } else {
                    resolve({
                        doubleOut: 0,
                        stringArrayOut: IltUtil.createStringArray(),
                        extendedBaseStructOut: IltUtil.createExtendedBaseStruct()});
                }
            });
        } else if (opArgs.booleanArg !== undefined && opArgs.booleanArg !== null) {
            prettyLog("IltProvider.overloadedMethod (2)");
            return new Promise(function(resolve, reject) {
                if (opArgs.booleanArg !== false) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethod_2: invalid argument booleanArg"}));
                } else {
                    resolve({stringOut: "TestString 2"});
                }
            });
        } else if (opArgs !== undefined && opArgs !== null) {
            prettyLog("IltProvider.overloadedMethod (1)");
            return new Promise(function(resolve, reject) {
                resolve({stringOut: "TestString 1"})
            });
        } else {
            return new Promise(function(resolve, reject) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "overloadedMethod: invalid arguments"}));
            });
        }
    },

    overloadedMethodWithSelector : function(opArgs) {
        prettyLog("IltProvider.overloadedMethodWithSelector(" + JSON.stringify(opArgs) + ") called");
        if (opArgs.int64Arg !== undefined && opArgs.int64Arg !== null
                && opArgs.booleanArg !== undefined && opArgs.booleanArg !== null
                && opArgs.enumArrayArg !== undefined && opArgs.enumArrayArg !== null
                && opArgs.baseStructArg !== undefined && opArgs.baseStructArg !== null) {
            prettyLog("IltProvider.overloadedMethodWithSelector (3)");
            return new Promise(function(resolve, reject) {
                if (opArgs.int64Arg !== 1) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethodWithSelector_3: invalid argument int64Arg"}));
                } else if (opArgs.booleanArg !== false) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethodWithSelector_3: invalid argument booleanArg"}));
                } else if (!IltUtil.checkExtendedExtendedEnumerationArray(opArgs.enumArrayArg)) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethodWithSelector_3: invalid argument enumArrayArg"}));
                } else if (!IltUtil.checkBaseStruct(opArgs.baseStructArg)) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethodWithSelector_3: invalid argument baseStructArg"}));
                } else {
                    resolve({
                        doubleOut: 1.1,
                        stringArrayOut: IltUtil.createStringArray(),
                        extendedBaseStructOut: IltUtil.createExtendedBaseStruct()});
                }
            });
        } else if (opArgs.booleanArg !== undefined && opArgs.booleanArg !== null) {
            prettyLog("IltProvider.overloadedMethodWithSelector (2)");
            return new Promise(function(resolve, reject) {
                if (opArgs.booleanArg !== false) {
                    reject(new joynr.exceptions.ProviderRuntimeException(
                            {detailMessage: "overloadedMethodWithSelector_2: invalid argument booleanArg"}));
                } else {
                    resolve({stringOut: "Return value from overloadedMethodWithSelector 2"});
                }
            });
        } else if (opArgs !== undefined && opArgs !== null) {
            prettyLog("IltProvider.overloadedMethodWithSelector (1)");
            return new Promise(function(resolve, reject) {
                resolve({stringOut: "Return value from overloadedMethodWithSelector 1"})
            });
        } else {
            return new Promise(function(resolve, reject) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "overloadedMethodWithSelector: invalid arguments"}));
            });
        }
    },

	// METHODS WITH ERROR DEFINITIONS

    methodWithoutErrorEnum : function(opArgs) {
        prettyLog("IltProvider.methodWithoutErrorEnum(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ProviderRuntimeException") {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "Exception from methodWithoutErrorEnum"}));
            } else {
                resolve();
            }
        });
    },

    methodWithAnonymousErrorEnum : function(opArgs) {
        prettyLog("IltProvider.methodWithAnonymousErrorEnum(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ProviderRuntimeException") {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "Exception from methodWithAnonymousErrorEnum"}));
            } else if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ApplicationException") {
                reject(MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC);
            } else {
                resolve();
            }
        });
    },

    methodWithExistingErrorEnum : function(opArgs) {
        prettyLog("IltProvider.methodWithExistingErrorEnum(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ProviderRuntimeException") {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "Exception from methodWithExistingErrorEnum"}));
            } else if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ApplicationException_1") {
                reject(ExtendedErrorEnumTc.ERROR_2_3_TC2);
            } else if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ApplicationException_2") {
                reject(ExtendedErrorEnumTc.ERROR_1_2_TC_2);
            } else {
                resolve();
            }
        });
    },

    methodWithExtendedErrorEnum : function(opArgs) {
        prettyLog("IltProvider.methodWithExtendedErrorEnum(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ProviderRuntimeException") {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "Exception from methodWithExtendedErrorEnum"}));
            } else if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ApplicationException_1") {
                reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC);
            } else if (opArgs.wantedExceptionArg !== undefined && opArgs.wantedExceptionArg !== null
                    && opArgs.wantedExceptionArg === "ApplicationException_2") {
                reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_2_1_TC2);
            } else {
                resolve();
            }
        });
    },

	// BROADCASTS aka events

    broadcastWithSinglePrimitiveParameter : {},
    broadcastWithMultiplePrimitiveParameters : {},
    broadcastWithSingleArrayParameter : {},
    broadcastWithMultipleArrayParameters : {},
    broadcastWithSingleEnumerationParameter : {},
    broadcastWithMultipleEnumerationParameters : {},
    broadcastWithSingleStructParameter : {},
    broadcastWithMultipleStructParameters : {},
    broadcastWithFiltering : {},

    methodToFireBroadcastWithSinglePrimitiveParameter : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithSinglePrimitiveParameter(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var stringOut = "boom";
            var outputParameters = self.broadcastWithSinglePrimitiveParameter.createBroadcastOutputParameters();
            outputParameters.setStringOut(stringOut);
            self.broadcastWithSinglePrimitiveParameter.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithMultiplePrimitiveParameters : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithMultiplePrimitiveParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var stringOut = "boom";
            var doubleOut = 1.1;
            var outputParameters = self.broadcastWithMultiplePrimitiveParameters.createBroadcastOutputParameters();
            outputParameters.setStringOut(stringOut);
            outputParameters.setDoubleOut(doubleOut);
            self.broadcastWithMultiplePrimitiveParameters.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithSingleArrayParameter : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithSingleArrayParameter(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var stringArrayOut = IltUtil.createStringArray();
            var outputParameters = self.broadcastWithSingleArrayParameter.createBroadcastOutputParameters();
            outputParameters.setStringArrayOut(stringArrayOut);
            self.broadcastWithSingleArrayParameter.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithMultipleArrayParameters : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithMultipleArrayParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var uInt64ArrayOut = IltUtil.createUInt64Array();
            var structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();
            var outputParameters = self.broadcastWithMultipleArrayParameters.createBroadcastOutputParameters();
            outputParameters.setUInt64ArrayOut(uInt64ArrayOut);
            outputParameters.setStructWithStringArrayArrayOut(structWithStringArrayArrayOut);
            self.broadcastWithMultipleArrayParameters.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithSingleEnumerationParameter : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithSingleEnumerationParameter(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
            var outputParameters = self.broadcastWithSingleEnumerationParameter.createBroadcastOutputParameters();
            outputParameters.setEnumerationOut(enumerationOut);
            self.broadcastWithSingleEnumerationParameter.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithMultipleEnumerationParameters : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithMultipleEnumerationParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var extendedEnumerationOut = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
            var enumerationOut = Enumeration.ENUM_0_VALUE_1;
            var outputParameters = self.broadcastWithMultipleEnumerationParameters.createBroadcastOutputParameters();
            outputParameters.setExtendedEnumerationOut(extendedEnumerationOut);
            outputParameters.setEnumerationOut(enumerationOut);
            self.broadcastWithMultipleEnumerationParameters.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithSingleStructParameter : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithSingleStructParameter(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
            var outputParameters = self.broadcastWithSingleStructParameter.createBroadcastOutputParameters();
            outputParameters.setExtendedStructOfPrimitivesOut(extendedStructOfPrimitivesOut);
            self.broadcastWithSingleStructParameter.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithMultipleStructParameters : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithMultipleStructParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
            var extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();
            var outputParameters = self.broadcastWithMultipleStructParameters.createBroadcastOutputParameters();
            outputParameters.setBaseStructWithoutElementsOut(baseStructWithoutElementsOut);
            outputParameters.setExtendedExtendedBaseStructOut(extendedExtendedBaseStructOut);
            self.broadcastWithMultipleStructParameters.fire(outputParameters, opArgs.partitions);
            resolve();
        });
    },

    methodToFireBroadcastWithFiltering : function(opArgs) {
        prettyLog("IltProvider.methodToFireBroadcastWithFiltering(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            var stringOut = opArgs.stringArg;
            var stringArrayOut = IltUtil.createStringArray();
            var enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
            var structWithStringArrayOut = IltUtil.createStructWithStringArray();
            var structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();
            var outputParameters = self.broadcastWithFiltering.createBroadcastOutputParameters();
            if (stringOut === undefined || stringOut === null || typeof stringOut !== "string") {
                reject(new joynr.exceptions.ProviderRuntimeException({detailMessage: "methodToFireBroadcastWithFiltering: received wrong argument"}));
            } else {
                outputParameters.setStringOut(stringOut);
                outputParameters.setStringArrayOut(stringArrayOut);
                outputParameters.setEnumerationOut(enumerationOut);
                outputParameters.setStructWithStringArrayOut(structWithStringArrayOut);
                outputParameters.setStructWithStringArrayArrayOut(structWithStringArrayArrayOut);
                self.broadcastWithFiltering.fire(outputParameters);
                resolve();
            }
        });
    }
};

self = exports.implementation;
