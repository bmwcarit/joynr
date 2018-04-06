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
var InProcessSkeleton = require("./InProcessSkeleton");

/**
 * Creates a proxy function that calls the proxyObjectFunction with the original arguments
 * in a this-context of proxyObject
 *
 * @param {Object}
 *            proxyObject
 * @param {Function}
 *            proxyObjectFunction
 * @returns {Function} the proxy function
 */
function createProxyFunction(proxyObject, proxyObjectFunction) {
    return function() {
        // and call corresponding proxy object function in context of the proxyObject with the
        // arguments of this function call
        return proxyObjectFunction.apply(proxyObject, arguments);
    };
}

/**
 * @name InProcessStub
 * @constructor
 *
 * @param {InProcessSkeleton}
 *            [inProcessSkeleton] connects the inProcessStub to its skeleton
 */
function InProcessStub(inProcessSkeleton) {
    if (!(this instanceof InProcessStub)) {
        // in case someone calls constructor without new keyword
        // (e.g. var c = Constructor({..}))
        return new InProcessStub(inProcessSkeleton);
    }

    if (inProcessSkeleton !== undefined) {
        this.setSkeleton(inProcessSkeleton);
    }
}

/**
 * Can set (new) inProcess Skeleton, overwrites members
 *
 * @name InProcessStub#setSkeleton
 * @function
 *
 * @param {InProcessSkeleton} inProcessSkeleton
 * @throws {Error} if type of inProcessSkeleton is wrong
 */
InProcessStub.prototype.setSkeleton = function(inProcessSkeleton) {
    if (inProcessSkeleton === undefined || inProcessSkeleton === null) {
        throw new Error("InProcessStub is undefined or null");
    }

    if (!(inProcessSkeleton instanceof InProcessSkeleton)) {
        throw new Error("InProcessStub should be of type InProcessSkeleton");
    }

    // get proxy object from skeleton
    var key,
        proxyObject = inProcessSkeleton.getProxyObject();

    // cycle over all members in the proxy object
    for (key in proxyObject) {
        if (proxyObject.hasOwnProperty(key)) {
            // get the member of the proxy object
            var proxyObjectMember = proxyObject[key];
            // if it is a function
            if (typeof proxyObjectMember === "function") {
                // attach a function to this object
                this[key] = createProxyFunction(proxyObject, proxyObjectMember);
            }
            // else: not a function, do not proxy member, maybe implement this here using
            // getter/setter with Object.defineProperty not required until now
        }
    }
};

module.exports = InProcessStub;
