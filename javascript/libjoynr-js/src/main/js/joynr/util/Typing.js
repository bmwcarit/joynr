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

define("joynr/util/Typing", [
    "joynr",
    "joynr/TypesEnum",
    "joynr/types/TypeRegistrySingleton"
], function(joynr, TypesEnum, TypeRegistrySingleton) {

    var translateJoynrTypeToJavascriptTypeTable = {};
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.BOOL] = "Boolean";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.BYTE] = "Number";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.SHORT] = "Number";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.INT] = "Number";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.LONG] = "Number";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.FLOAT] = "Number";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.DOUBLE] = "Number";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.STRING] = "String";
    translateJoynrTypeToJavascriptTypeTable[TypesEnum.LIST] = "Array";

    /**
     * @name Typing
     * @class
     */
    var Typing = {};

    /**
     * @function Typing#getObjectType
     * @param {Boolean|Number|String|Object}
     *            obj the object to determine the type of
     * @returns {String} the object type
     */
    Typing.getObjectType = function(obj) {
        if (obj === null || obj === undefined) {
            throw new Error("cannot determine the type of an undefined object");
        }
        var funcNameRegex = /function ([$\w]+)\(/;
        var results = funcNameRegex.exec(obj.constructor.toString());
        return (results && results.length > 1) ? results[1] : "";
    };

    /**
     * Translates the Joynr Types to Javascript Types
     *
     * @function Typing#translateJoynrTypeToJavascriptType
     * @param {String} joynrType the joynr type string
     * @returns {String} the javascript type or the joynrType if not found
     * @throws {Error} an error when input parameters are nullable (undefined or null)
     */
    Typing.translateJoynrTypeToJavascriptType = function(joynrType) {
        if (joynrType === undefined || joynrType === null) {
            throw new Error("cannot determine javascript type of \"" + joynrType + "\"");
        }

        return translateJoynrTypeToJavascriptTypeTable[joynrType] || joynrType;
    };

    /**
     * Recursively deep copies a given object and augments type information on the way if a
     * constructor is registered in the typeRegistry for the value of the object's
     * member "_typeName"
     *
     * @function Typing#augmentTypes
     * @param {Boolean|Number|String|Array|Object}
     *            untyped the untyped object
     * @param {TypeRegistry}
     *            typeRegistry the typeRegistry to retrieve type information from
     * @returns a deep copy of the untyped object with the types being augmented
     * @throws {Error}
     *             if in any of the objects contains a member of type "Function" or the type of the
     *             untyped object is not (Boolean|Number|String|Array|Object)
     */
    Typing.augmentTypes =
            function(untyped, typeRegistry) {
                var i, typedObj;

                // return nullable values immediately
                if (untyped === null || untyped === undefined) {
                    return untyped;
                }

                // retrieve the javascript runtime type info
                var type = Typing.getObjectType(untyped);

                // what should we do with a function?
                if (type === "Function") {
                    // functions should not at all appear here!!!
                    throw new Error("cannot augment object type \"" + type + "\"");
                }

                // leave integral data types untyped
                if (type === "Boolean" || type === "Number" || type === "String") {
                    typedObj = untyped;
                }
                // try to type each single element of an array
                else if (type === "Array") {
                    typedObj = [];
                    for (i = 0; i < untyped.length; ++i) {
                        typedObj.push(Typing.augmentTypes(untyped[i], typeRegistry));
                    }
                }
                // try to type each single member of an object, and use registered constructor if
                // available
                else if (type === "Object") {
                    /*jslint nomen: true */
                    var Constructor = typeRegistry.getConstructor(untyped._typeName);
                    /*jslint nomen: false */

                    // if object has joynr type name and corresponding constructor is found in the
                    // type registry, construct it, otherwise throw an error
                    if (Constructor) {
                        typedObj = new Constructor();
                    } else {
                        throw new Error(
                                "type must contain a _typeName that is registered in the type registry: "
                                    + JSON.stringify(untyped));
                    }

                    // copy over and type each single member
                    for (i in untyped) {
                        if (untyped.hasOwnProperty(i)) {
                            typedObj[i] = Typing.augmentTypes(untyped[i], typeRegistry);
                        }
                    }
                } else {
                    // encountered an unknown object type, that should not appear here
                    throw new Error("encountered unknown object \""
                        + JSON.stringify(untyped)
                        + "\" of type \""
                        + type
                        + "\" while augmenting types");
                }

                return typedObj;
            };

    /**
     * Augments the javascript runtime type into the member "_typeName"
     *
     * @function Typing#augmentTypeName
     * @param {?} obj the object to augment the typeName for
     * @param {String} [packageName] an optional packageName that is used as starting string for
     *             _typeName
     * @param {String} [memberName] an optional member name that is used instead of _typeName
     *
     * @returns {?} the same object with the typeName set
     */
    Typing.augmentTypeName = function(obj, packageName, memberName) {
        packageName = packageName === undefined ? "" : packageName + ".";
        obj[memberName || "_typeName"] = packageName + Typing.getObjectType(obj);
        return obj;
    };

    /**
     * Returns true if the object is a joynr complex type modelled in Franca
     * @function Typing#isComplexJoynrType
     */
    Typing.isComplexJoynrType = function isComplexJoynrType(value) {
        var valuePrototype = Object.getPrototypeOf(value);
        return (valuePrototype && valuePrototype instanceof joynr.JoynrObject);
    };

    /**
     * Returns true if the object is a joynr enum type modelled in Franca
     * @function Typing#isEnumType
     */
    Typing.isEnumType =
            function isEnumType(value) {
                /*jslint nomen: true */
                var result =
                        typeof value === "object"
                            && Typing.isComplexJoynrType(value)
                            && TypeRegistrySingleton.getInstance().isEnumType(value._typeName);
                /*jslint nomen: false */
                return result;
            };

    return Typing;

});