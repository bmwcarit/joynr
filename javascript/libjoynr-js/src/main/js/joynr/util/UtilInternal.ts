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
import LongTimer from "./LongTimer";
import util from "util";

/* istanbul ignore file */

function extendInternal<T, U>(to: any, from: U, deep: boolean): T & U {
    if (from) {
        for (const key in from) {
            if (Object.prototype.hasOwnProperty.call(from, key)) {
                if (deep && typeof from[key] === "object") {
                    if (Array.isArray(from[key]) && !Array.isArray((to as any)[key])) {
                        (to as any)[key] = [];
                    } else if (typeof (to as any)[key] !== "object") {
                        (to as any)[key] = {};
                    }
                    extendInternal((to as any)[key], from[key], deep);
                } else if (from[key] !== undefined) {
                    (to as any)[key] = from[key];
                }
            }
        }
    }
    return to as any;
}

/**
 * Copies all attributes to a given out parameter from optional in parameters
 * @function UtilInternal#extend
 */
export function extend(out: any, ...args: any[]): any {
    for (let i = 0; i < args.length; i++) {
        extendInternal(out, args[i], false);
    }
    return out;
}

/**
 * Forwards all methods from provider to receiver and thus creating privacy
 * @param receiver
 * @param provider
 * @returns
 */
export function forward(receiver: any, provider: any): any {
    let methodName;
    for (methodName in provider) {
        if (Object.prototype.hasOwnProperty.call(provider, methodName) && typeof provider[methodName] === "function") {
            receiver[methodName] = provider[methodName].bind(provider);
        }
    }
    return receiver;
}

/**
 * Deeply copies all attributes to a given out parameter from optional in parameters
 * @function UtilInternal#extendDeep
 */
export function extendDeep<T>(out: T, ...args: any[]): T {
    for (let i = 0; i < args.length; i++) {
        extendInternal(out, args[i], true);
    }
    return out;
}

/**
 * Transforms the incoming array "from" according to the transformFunction
 * @function UtilInternal#transform
 */
export function transform(from: any[], transformFunction: Function): any[] {
    const transformedArray = [];
    for (let i = 0; i < from.length; i++) {
        const value = from[i];
        transformedArray.push(transformFunction(value, i));
    }
    return transformedArray;
}

/**
 * Checks explicitly if value is null or undefined, use if you don't want !!"" to become false,
 * but !UtilInternal.checkNullUndefined("") to be true
 *
 * @param value the value to be checked for null or undefined
 * @returns if the value is null or undefined
 */
export function checkNullUndefined(value: any): boolean {
    return value === null || value === undefined;
}

/**
 * Converts the first character of the string to lower case
 * @function UtilInternal#firstLower
 *
 * @param str the string to be converted
 * @returns the converted string
 * @throws {TypeError} on nullable input (null, undefined)
 */
export function firstLower(str: string): string {
    return str.substr(0, 1).toLowerCase() + str.substr(1);
}

/**
 * Capitalizes the first letter
 * @function UtilInternal#firstUpper
 *
 * @param str the string to be converted
 * @returns the converted string
 * @throws {TypeError} on nullable input (null, undefined)
 */
export function firstUpper(str: string): string {
    return str.substr(0, 1).toUpperCase() + str.substr(1);
}

/**
 * Checks if the provided argument is an A+ promise object
 *
 * @param arg the argument to be evaluated
 * @returns true if provided argument is a promise, otherwise false
 */
export function isPromise(arg: any): boolean {
    return arg !== undefined && typeof arg.then === "function" && typeof arg.catch === "function";
}

/**
 * Shorthand function for Object.defineProperty
 *
 * @param object
 * @param memberName
 * @param value default value is undefined
 * @param writable default value is false
 * @param enumerable default value is true
 * @param configurable default value is false
 */
export function objectDefineProperty(
    object: object,
    memberName: string,
    value: any,
    writable: boolean,
    enumerable: boolean,
    configurable: boolean
): void {
    Object.defineProperty(object, memberName, {
        value,
        writable: writable === undefined ? false : writable,
        enumerable: enumerable === undefined ? true : enumerable,
        configurable: configurable === undefined ? false : configurable
    });
}

/**
 * Returns the byte length of the provided string encoded in UTF-8
 *
 * @param text as UTF-8 encoded string
 * @returns the length in bytes
 */
export function getLengthInBytes(text: string): number {
    // blob is twice as fast, and unescape is actually deprecated
    //return new Blob([text]).size;
    return unescape(encodeURIComponent(text)).length;
}

/**
 * Returns the max supported long value. According to the ECMA spec this is 2^53 -1 See
 * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number/MAX_SAFE_INTEGER
 *
 * Actually this should be Math.pow(2, 63) - 1, but this evaluates to 9223372036854776000
 * instead of 9223372036854775808, which is larger than Long.MAX_VALUE of 2^63-1 and wraps
 * around to negative values in Java and will cause problems in C++ too
 *
 */
export function getMaxLongValue(): number {
    return Number.MAX_SAFE_INTEGER;
}

/**
 * Removes item from array
 * @param array to be edited
 * @param item to be removed
 */
export function removeElementFromArray(array: any[], item: any): void {
    const i = array.indexOf(item);
    if (i > -1) {
        array.splice(i, 1);
    }
}

/**
 * Invokes all callbacks with the provided data
 * @param callbacks to be invoked
 * @param data provided as callback argument
 * @param data.broadcastOutputParameters broadcasts output params
 * @param data.filters filter array provided as callback argument
 */
export function fire(callbacks: Function[], data: any): void {
    let callbackFct;
    for (callbackFct in callbacks) {
        if (Object.prototype.hasOwnProperty.call(callbacks, callbackFct)) {
            callbacks[callbackFct](data);
        }
    }
}

function timeoutPromiseHelper(
    promise: Promise<any>,
    timeoutMs: number,
    callback: (err: Error, result: any) => void
): void {
    const timeout = LongTimer.setTimeout((): void => {
        callback(new Error(`Promise timeout after ${timeoutMs} ms`), undefined);
    }, timeoutMs);

    promise
        .then(
            (...args): void => {
                LongTimer.clearTimeout(timeout);
                // eslint-disable-next-line promise/no-callback-in-promise
                callback(undefined as any, ...args);
            }
        )
        // eslint-disable-next-line promise/no-callback-in-promise
        .catch(callback as any);
}

export const timeoutPromise = util.promisify(timeoutPromiseHelper);
export interface Deferred {
    resolve: (value?: any) => void;
    reject: (error?: any) => void;
    promise: Promise<any>;
}

function defer(this: Deferred, resolve: (value: any) => void, reject: (error: any) => void): void {
    this.resolve = resolve;
    this.reject = reject;
}

export function createDeferred(): Deferred {
    const deferred: Deferred = {} as any;
    /*eslint-disable promise/avoid-new */
    deferred.promise = new Promise(defer.bind(deferred));
    /*eslint-enable promise/avoid-new */
    return deferred;
}

/**
 *
 * Create an ES6 Proxy Object to facilitate access of nested properties.
 *
 * @param config
 * @returns an ES6 Proxy object
 */
export function augmentConfig(config: any): any {
    let parts: string[] = [];
    const proxy: Function = new Proxy(
        (): any => {
            let level: any = config;
            for (let i = 0; i < parts.length; i++) {
                if (level === undefined) {
                    parts = [];
                    return undefined;
                }
                level = level[parts[i]];
            }
            parts = [];
            return level;
        },
        {
            has(): true {
                return true;
            },

            get(_, prop: string): any {
                parts.push(prop);
                return proxy;
            },

            set(_, prop: string, value: any): boolean {
                let level: any = config;
                for (let i = 0; i < parts.length; i++) {
                    if (!Object.prototype.hasOwnProperty.call(level, [parts[i]])) {
                        level[parts[i]] = {};
                    }
                    level = level[parts[i]];
                }
                parts = [];
                level[prop] = value;
                return true;
            }
        }
    ) as any;
    return proxy;
}

export function emptyFunction(): void {
    // do nothing
}

export function checkProperty(obj: any, type: string | string[] | Function, description: string): void {
    if (obj === undefined) {
        throw new Error(`${description} is undefined`);
    }
    const objectType = getObjectType(obj);
    if (typeof type === "string" && objectType !== type) {
        throw new Error(`${description} is not of type ${type}. Actual type is ${objectType}`);
    }
    if (Array.isArray(type) && !objectType.match(`^${type.join("$|^")}$`)) {
        throw new Error(`${description} is not of a type from ${type}. Actual type is ${objectType}`);
    }
    if (typeof type === "function" && !(obj instanceof type)) {
        throw new Error(`${description} is not of type ${getObjectType(type)}. Actual type is ${objectType}`);
    }
}

/**
 * Checks if the variable is of specified type
 *
 * @param obj the object
 * @param type a string representation of the the data type (e.g. "Boolean", "Number",
 *             "String", "Array", "Object", "Function"
 *            "MyCustomType", "Object|MyCustomType") or a constructor function to check against
 *             using instanceof (e.g. obj
 *            instanceof type)
 * @param description a description for the thrown error
 * @throws an
 *             error if the object not of the given type
 */
export function checkPropertyIfDefined(obj: any, type: string | string[] | Function, description: string): void {
    if (obj !== undefined && obj !== null) {
        checkProperty(obj, type, description);
    }
}

/**
 * @param obj the object to determine the type of
 * @returns the object type
 */
export function getObjectType(obj: any): string {
    if (obj === null || obj === undefined) {
        throw new Error("cannot determine the type of an undefined object");
    }
    if (Array.isArray(obj)) {
        return "Array";
    }
    return obj.constructor.name || "";
}
