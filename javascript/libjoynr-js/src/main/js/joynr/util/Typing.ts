/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
/* istanbul ignore file */
import JoynrObject from "../types/JoynrObject";
import TypeRegistrySingleton from "../types/TypeRegistrySingleton";
import { checkProperty, checkPropertyIfDefined, getObjectType } from "./UtilInternal";
export { checkProperty, checkPropertyIfDefined, getObjectType };

const typeRegistry = TypeRegistrySingleton.getInstance();

export function checkPropertyAllowObject(obj: any, type: string, description: string): void {
    const objectType = getObjectType(obj);
    if (objectType !== type) {
        if (obj._typeName) {
            if (typeRegistry.getConstructor(obj._typeName).name === type) {
                return;
            }
            throw new Error(`${description} is not of type ${type} or Object. Actual type is ${objectType}`);
        }
        throw new Error(`${description} is not of type ${type}. Actual type is ${objectType}`);
    }
}

/**
 * Recursively deep copies a given object and augments type information on the way if a
 * constructor is registered in the typeRegistry for the value of the object's
 * member "_typeName"
 *
 * @param untyped the untyped object
 * @param typeHint optional parameter which provides the type informed of the untyped object.
 *            This is used i.e. for enums, where the type information is not included in the untyped object itself
 * @returns a deep copy of the untyped object with the types being augmented
 * @throws {Error} if in any of the objects contains a member of type "Function" or the type of the
 *             untyped object is not (Boolean|Number|String|Array|Object)
 */
export function augmentTypes(untyped: any, typeHint?: string): any {
    let typedObj;

    // return nullable values immediately
    if (untyped === null || untyped === undefined) {
        return untyped;
    }

    // return already typed objects immediately
    if (isComplexJoynrObject(untyped)) {
        const Constructor = untyped.constructor;
        if (Constructor.checkMembers) {
            Constructor.checkMembers(untyped, checkPropertyAllowObject);
        }
        return untyped;
    }

    // retrieve the javascript runtime type info
    const type = untyped.constructor.name;

    // what should we do with a function?
    if (type === "Function") {
        // functions should not at all appear here!!!
        throw new Error(`cannot augment object type "${type}"`);
    }

    // try to type each single element of an array
    if (Array.isArray(untyped)) {
        typedObj = [];
        if (untyped.length > 0) {
            const filteredTypeHint =
                typeHint !== undefined && typeHint.length > 2 && typeHint.substr(typeHint.length - 2, 2) === "[]"
                    ? typeHint.substring(0, typeHint.length - 2)
                    : typeHint;
            for (let i = 0; i < untyped.length; ++i) {
                typedObj.push(augmentTypes(untyped[i], filteredTypeHint));
            }
        }
    } else if (typeHint !== undefined && typeRegistry.isEnumType(typeHint)) {
        //check if provisioned type name is given. In this case, check for special considerations
        const Constructor = typeRegistry.getConstructor(typeHint);
        typedObj = untyped.name ? Constructor[untyped.name] : Constructor[untyped];
    } else if (type === "Boolean" || type === "Number" || type === "String") {
        // leave integral data types untyped
        typedObj = untyped;
    } else if (type === "Object") {
        // try to type each single member of an object, and use registered constructor if available
        const Constructor = typeRegistry.getConstructor(untyped._typeName);
        const isEnumType = typeRegistry.isEnumType(untyped._typeName);

        // if object has joynr type name and corresponding constructor is found in the
        // type registry, construct it, otherwise throw an error
        if (Constructor) {
            if (isEnumType && untyped.name !== undefined) {
                typedObj = Constructor[untyped.name];
            } else {
                const settings: Record<string, any> = {};

                for (const i in untyped) {
                    if (Object.prototype.hasOwnProperty.call(untyped, i)) {
                        if (Constructor.getMemberType !== undefined) {
                            settings[i] = augmentTypes(untyped[i], Constructor.getMemberType(i));
                        } else {
                            settings[i] = augmentTypes(untyped[i]);
                        }
                    }
                }
                typedObj = new Constructor(settings);

                if (Constructor.checkMembers) {
                    Constructor.checkMembers(typedObj, checkProperty);
                }
            }
        } else {
            throw new Error(
                `type must contain a _typeName that is registered in the type registry: ${JSON.stringify(untyped)}`
            );
        }
    } else {
        // encountered an unknown object type, that should not appear here
        throw new Error(
            `encountered unknown object "${JSON.stringify(untyped)}" of type "${type}" while augmenting types`
        );
    }

    return typedObj;
}

/**
 * Augments the javascript runtime type into the member "_typeName"
 *
 * @param obj the object to augment the typeName for
 * @param [packageName] an optional packageName that is used as starting string for
 *             _typeName
 * @param [memberName] an optional member name that is used instead of _typeName
 *
 * @returns the same object with the typeName set
 */
export function augmentTypeName(obj: any, packageName?: string, memberName?: string): any {
    packageName = packageName === undefined ? "" : `${packageName}.`;
    obj[memberName || "_typeName"] = packageName + getObjectType(obj);
    return obj;
}

/**
 * Returns true if the object is a joynr complex type modelled in Franca
 */
export function isComplexJoynrObject(value: any): boolean {
    return value instanceof JoynrObject;
}

/**
 * Returns true if the object is a joynr enum type modelled in Franca
 * @param value the object to be check for typing
 * @param checkForJoynrObject an optional member. If set to true,
 *                  the parameter value is forced to be an instance of the root
 *                  joynr object type
 * @returns true if the provided value is an enum type
 */
export function isEnumType(value: any, checkForJoynrObject?: boolean): boolean {
    const isJoynrObject = checkForJoynrObject === undefined || (!checkForJoynrObject || isComplexJoynrObject(value));
    return (
        value &&
        typeof value === "object" &&
        isJoynrObject &&
        TypeRegistrySingleton.getInstance().isEnumType(value._typeName)
    );
}
