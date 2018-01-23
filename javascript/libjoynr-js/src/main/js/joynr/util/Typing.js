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
var joynr = require("joynr");
var TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");

/**
 * @name Typing
 * @class
 */
var Typing = {};

Typing.checkProperty = function(obj, type, description) {
    if (obj === undefined) {
        throw new Error(description + " is undefined");
    }
    if (typeof type === "string" && Typing.getObjectType(obj) !== type) {
        throw new Error(description + " is not of type " + type + ". Actual type is " + Typing.getObjectType(obj));
    }
    if (Array.isArray(type) && !Typing.getObjectType(obj).match("^" + type.join("$|^") + "$")) {
        throw new Error(
            description + " is not of a type from " + type + ". Actual type is " + Typing.getObjectType(obj)
        );
    }
    if (typeof type === "function" && !(obj instanceof type)) {
        throw new Error(
            description +
                " is not of type " +
                Typing.getObjectType(type) +
                ". Actual type is " +
                Typing.getObjectType(obj)
        );
    }
};

/**
 * Checks if the variable is of specified type
 * @function Typing#checkPropertyIfDefined
 *
 * @param {?}
 *            obj the object
 * @param {String|Function}
 *            type a string representation of the the data type (e.g. "Boolean", "Number",
 *             "String", "Array", "Object", "Function"
 *            "MyCustomType", "Object|MyCustomType") or a constructor function to check against
 *             using instanceof (e.g. obj
 *            instanceof type)
 * @param {String}
 *            description a description for the thrown error
 * @throws an
 *             error if the object not of the given type
 */
Typing.checkPropertyIfDefined = function(obj, type, description) {
    if (obj !== undefined && obj !== null) {
        Typing.checkProperty(obj, type, description);
    }
};

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
    if (Array.isArray(obj)) {
        return "Array";
    }
    return obj.constructor.name || "";
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
 * @param {String}
 *            typeHint optional parameter which provides the type informed of the untyped object.
 *            This is used i.e. for enums, where the type information is not included in the untyped object itself
 * @returns a deep copy of the untyped object with the types being augmented
 * @throws {Error}
 *             if in any of the objects contains a member of type "Function" or the type of the
 *             untyped object is not (Boolean|Number|String|Array|Object)
 */
Typing.augmentTypes = function(untyped, typeRegistry, typeHint) {
    var i, typedObj, typeName;

    // return nullable values immediately
    if (untyped === null || untyped === undefined) {
        return untyped;
    }

    // return already typed objects immediately
    if (Typing.isComplexJoynrObject(untyped)) {
        return untyped;
    }

    // retrieve the javascript runtime type info
    var type = Typing.getObjectType(untyped);

    // what should we do with a function?
    if (type === "Function") {
        // functions should not at all appear here!!!
        throw new Error('cannot augment object type "' + type + '"');
    }

    // try to type each single element of an array
    if (type === "Array") {
        typedObj = [];
        for (i = 0; i < untyped.length; ++i) {
            var filteredTypeHint =
                typeHint !== undefined && typeHint.length > 2 && typeHint.substr(typeHint.length - 2, 2) === "[]"
                    ? typeHint.substring(0, typeHint.length - 2)
                    : typeHint;
            typedObj.push(Typing.augmentTypes(untyped[i], typeRegistry, filteredTypeHint));
        }
    } else if (typeHint !== undefined && typeRegistry.isEnumType(typeHint)) {
        //check if provisioned type name is given. In this case, check for special considerations
        typedObj = typeRegistry.getConstructor(typeHint)[untyped];
    } else if (type === "Boolean" || type === "Number" || type === "String") {
        // leave integral data types untyped
        typedObj = untyped;
    } else if (type === "Object") {
        // try to type each single member of an object, and use registered constructor if available
        /*jslint nomen: true */
        var Constructor = typeRegistry.getConstructor(untyped._typeName);
        var isEnumType = typeRegistry.isEnumType(untyped._typeName);
        /*jslint nomen: false */

        // if object has joynr type name and corresponding constructor is found in the
        // type registry, construct it, otherwise throw an error
        if (Constructor) {
            if (isEnumType && untyped.name !== undefined) {
                typedObj = Constructor[untyped.name];
            } else {
                typedObj = new Constructor();
                // copy over and type each single member
                for (i in untyped) {
                    if (untyped.hasOwnProperty(i)) {
                        if (Constructor.getMemberType !== undefined) {
                            typedObj[i] = Typing.augmentTypes(untyped[i], typeRegistry, Constructor.getMemberType(i));
                        } else {
                            typedObj[i] = Typing.augmentTypes(untyped[i], typeRegistry);
                        }
                    }
                }
            }
        } else {
            throw new Error(
                "type must contain a _typeName that is registered in the type registry: " + JSON.stringify(untyped)
            );
        }
    } else {
        // encountered an unknown object type, that should not appear here
        throw new Error(
            'encountered unknown object "' + JSON.stringify(untyped) + '" of type "' + type + '" while augmenting types'
        );
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
 * @function Typing#isComplexJoynrObject
 */
Typing.isComplexJoynrObject = function isComplexJoynrObject(value) {
    try {
        var valuePrototype = Object.getPrototypeOf(value);
        return valuePrototype && valuePrototype instanceof joynr.JoynrObject;
    } catch (error) {
        // This can be the case when the value is a primitive type
    }
    return false;
};

/**
 * Returns true if the object is a joynr enum type modelled in Franca
 * @function Typing#isEnumType
 * @param {Object} value the object to be check for typing
 * @param {Boolean} checkForJoynrObject an optional member. If set to true,
 *                  the parameter value is forced to be an instance of the root
 *                  joynr object type
 * @returns {Boolean} true if the provided value is an enum type
 */
Typing.isEnumType = function isEnumType(value, checkForJoynrObject) {
    var isJoynrObject =
        checkForJoynrObject === undefined || (!checkForJoynrObject || Typing.isComplexJoynrObject(value));
    /*jslint nomen: true */
    var result =
        value !== undefined &&
        value !== null &&
        typeof value === "object" &&
        isJoynrObject &&
        TypeRegistrySingleton.getInstance().isEnumType(value._typeName);
    /*jslint nomen: false */
    return result;
};

module.exports = Typing;
