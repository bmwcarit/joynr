/*global unescape: true, Blob: true, Array:true */
/*jslint es5: true, node: true, node: true */
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
var Promise = require("../../global/Promise");
var UtilExternal = require("./Util");

/**
 * @name UtilInternal
 * @class
 * @classdesc extends the Util class by additional methods
 */
var UtilInternal = {};

var isArray = function isArray(object) {
    return Object.prototype.toString.call(object) === "[object Array]";
};

function extend(to, from, deep) {
    var i, key, args;

    if (from) {
        for (key in from) {
            if (from.hasOwnProperty(key)) {
                if (deep && typeof from[key] === "object") {
                    if (isArray(from[key]) && !isArray(to[key])) {
                        to[key] = [];
                    } else if (typeof to[key] !== "object") {
                        to[key] = {};
                    }
                    extend(to[key], from[key], deep);
                } else if (from[key] !== undefined) {
                    to[key] = from[key];
                }
            }
        }
    }
    return to;
}

/**
 * Copies all attributes to a given out parameter from optional in parameters
 * @function UtilInternal#extend
 */
UtilInternal.extend = function(out) {
    var i, key, args;
    // calling using prototype because slice is not available on
    // special arguments array
    args = Array.prototype.slice.call(arguments, 1);
    for (i = 0; i < args.length; i++) {
        extend(out, args[i], false);
    }
    return out;
};

/**
 * Forwards all methods from provider to receiver and thus creating privacy
 * @param receiver
 * @param provider
 * @returns {*}
 */
UtilInternal.forward = function forward(receiver, provider) {
    var methodName;
    for (methodName in provider) {
        if (provider.hasOwnProperty(methodName) && typeof provider[methodName] === "function") {
            receiver[methodName] = provider[methodName].bind(provider);
        }
    }

    return receiver;
};

/**
 * @function UtilInternal#isArray
 * @param {?} object
 */
UtilInternal.isArray = isArray;

/**
 * Deeply copies all attributes to a given out parameter from optional in parameters
 * @function UtilInternal#extendDeep
 */
UtilInternal.extendDeep = function(out) {
    var i, key, args;
    // calling using prototype because slice is not available on
    // special arguments array
    args = Array.prototype.slice.call(arguments, 1);
    for (i = 0; i < args.length; i++) {
        extend(out, args[i], true);
    }
    return out;
};

/**
 * Transforms the incoming array "from" according to the transformFunction
 * @function UtilInternal#transform
 */
UtilInternal.transform = function transform(from, transformFunction) {
    var i,
        value,
        transformedArray = [];
    for (i = 0; i < from.length; i++) {
        value = from[i];
        transformedArray.push(transformFunction(value, i));
    }
    return transformedArray;
};

/**
 * Checks explicitly if value is null or undefined, use if you don't want !!"" to become false,
 * but !Util.checkNullUndefined("") to be true
 * @function UtilInternal#checkNullUndefined
 *
 * @param {?}
 *            value the value to be checked for null or undefined
 * @returns {Boolean} if the value is null or undefined
 */
UtilInternal.checkNullUndefined = function checkNullUndefined(value) {
    return value === null || value === undefined;
};

/**
 * Converts the first character of the string to lower case
 * @function UtilInternal#firstLower
 *
 * @param {String}
 *            str the string to be converted
 * @returns {String} the converted string
 * @throws {TypeError}
 *             on nullable input (null, undefined)
 */
UtilInternal.firstLower = function firstLower(str) {
    return str.substr(0, 1).toLowerCase() + str.substr(1);
};

/**
 * Capitalizes the first letter
 * @function UtilInternal#firstUpper
 *
 * @param {String}
 *            str the string to be converted
 * @returns {String} the converted string
 * @throws {TypeError}
 *             on nullable input (null, undefined)
 */
UtilInternal.firstUpper = function firstUpper(str) {
    return str.substr(0, 1).toUpperCase() + str.substr(1);
};

/**
 * Checks if the provided argument is an A+ promise object
 * @function UtilInternal#isPromise
 *
 * @param {object} arg
 *            the argument to be evaluated
 * @returns true if provided argument is a promise, otherwise falses
 */
UtilInternal.isPromise = function isPromise(arg) {
    return arg !== undefined && typeof arg.then === "function" && typeof arg.catch === "function";
};

/**
 * Shorthand function for Object.defineProperty
 * @function UtilInternal#objectDefineProperty
 *
 * @param {Object}
 *            object
 * @param {String}
 *            memberName
 * @param {?}
 *            [value] default value is undefined
 * @param {Boolean}
 *            [readable] default value is true
 * @param {Boolean}
 *            [writable] default value is false
 * @param {Boolean}
 *            [enumerable] default value is true
 * @param {Boolean}
 *            [configurable] default value is false
 */
UtilInternal.objectDefineProperty = function objectDefineProperty(
    object,
    memberName,
    value,
    readable,
    writable,
    enumerable,
    configurable
) {
    Object.defineProperty(object, memberName, {
        value: value,
        readable: readable === undefined ? true : readable,
        writable: writable === undefined ? false : writable,
        enumerable: enumerable === undefined ? true : enumerable,
        configurable: configurable === undefined ? false : configurable
    });
};

/**
 * Returns the byte length of the provided string encoded in UTF-8
 * @function UtilInternal#getLengthInBytes
 *
 * @param {String}
 *            text as UTF-8 encoded string
 * @returns {Number} the length in bytes
 */
UtilInternal.getLengthInBytes = function getLengthInBytes(text) {
    // blob is twice as fast, and unescape is actually deprecated
    //return new Blob([text]).size;
    return unescape(encodeURIComponent(text)).length;
};

/**
 * Returns the max supported long value. According to the ECMA spec this is 2^53 -1 See
 * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number/MAX_SAFE_INTEGER
 *
 * Actually this should be Math.pow(2, 63) - 1, but this evaluates to 9223372036854776000
 * instead of 9223372036854775808, which is larger than Long.MAX_VALUE of 2^63-1 and wraps
 * around to negative values in Java and will cause problems in C++ too
 *
 * @function UtilInternal#getMaxLongValue
 */
UtilInternal.getMaxLongValue = function getMaxLongValue() {
    return Math.pow(2, 53) - 1;
};

/**
 * Removes item from array
 * @function UtilInternal#removeElementFromArray
 * @param {Array}
 *          array to be edited
 * @param {Object}
 *          item to be removed
 */
UtilInternal.removeElementFromArray = function removeElementFromArray(array, item) {
    var i = array.indexOf(item);
    if (i > -1) {
        array.splice(i, 1);
    }
};

/**
 * Invokes all callbacks with the provided data
 * @function UtilInternal#fire
 * @param {Array}
 *          callbacks to be invoked
 * @param {Object}
 *          data provided as callback argument
 * @param {Object}
 *          data.broadcastOutputParameters broadcasts output params
 * @param {Object}
 *          data.filters filter array provided as callback argument
 */
UtilInternal.fire = function fire(callbacks, data) {
    var callbackFct;
    for (callbackFct in callbacks) {
        if (callbacks.hasOwnProperty(callbackFct)) {
            callbacks[callbackFct](data);
        }
    }
};

UtilInternal.enrichObjectWithSetPrototypeOf = function() {
    //if Object.setPrototypeOf already exists? -> do nothing;
    Object.setPrototypeOf =
        Object.setPrototypeOf ||
        function(object, prototype) {
            /*jslint sub: true*/
            object["__proto__"] = prototype;
            /*jslint sub: false*/
        };
};

function timeoutToPromise(time) {
    return new Promise(function(resolve, reject) {
        setTimeout(resolve, time);
    });
}

UtilInternal.timeoutPromise = function(promise, timeoutMs) {
    return new Promise(function(resolve, reject) {
        promise.then(resolve).catch(reject);
        timeoutToPromise(timeoutMs).then(reject);
    });
};

UtilInternal.createDeferred = function() {
    var deferred = {};
    deferred.promise = new Promise(function(resolve, reject) {
        deferred.resolve = resolve;
        deferred.reject = reject;
    });
    return deferred;
};

UtilInternal.extend(UtilInternal, UtilExternal);
module.exports = UtilInternal;
