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

define("joynr/util/Util", [
    "joynr/util/Typing",
    "joynr/types/TypeRegistrySingleton"
], function(Typing, TypeRegistrySingleton) {

    /**
     * @name Util
     * @class
     */
    var Util = {};

    Util.checkProperty =
            function(obj, type, description) {
                if (obj === undefined) {
                    throw new Error(description + " is undefined");
                }
                if (typeof type === "string" && Typing.getObjectType(obj) !== type) {
                    throw new Error(description
                        + " is not of type "
                        + type
                        + ". Actual type is "
                        + Typing.getObjectType(obj));
                }
                if (Object.prototype.toString.call(type) === "[object Array]"
                    && !Typing.getObjectType(obj).match("^" + type.join("$|^") + "$")) {
                    throw new Error(description
                        + " is not of a type from "
                        + type
                        + ". Actual type is "
                        + Typing.getObjectType(obj));
                }
                if (typeof type === "function" && !(obj instanceof type)) {
                    throw new Error(description
                        + " is not of type "
                        + Typing.getObjectType(type)
                        + ". Actual type is "
                        + Typing.getObjectType(obj));
                }
            };

    /**
     * Checks if the variable is of specified type
     * @function Util#checkPropertyIfDefined
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
    Util.checkPropertyIfDefined = function(obj, type, description) {
        if (obj !== undefined && obj !== null) {
            Util.checkProperty(obj, type, description);
        }
    };

    /**
     * @function Util#ensureTypedValues
     * @param {Object} value
     * @param {Object} typeRegistry
     */
    Util.ensureTypedValues = function(value, typeRegistry) {
        var i;

        typeRegistry = typeRegistry || TypeRegistrySingleton.getInstance();

        if (value !== undefined && value !== null) {
            if (value.constructor.name === "Array") {
                for (i = 0; i < value.length; i++) {
                    value[i] = Util.ensureTypedValues(value[i]);
                }
            } else if (typeof value === "object" && !Typing.isComplexJoynrObject(value)) {
                value = Typing.augmentTypes(value, typeRegistry);
                value.checkMembers(Util.checkPropertyIfDefined);
            }
        }

        return value;
    };

    return Util;

});
