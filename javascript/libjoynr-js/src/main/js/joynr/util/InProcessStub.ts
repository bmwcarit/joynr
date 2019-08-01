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
import InProcessSkeleton from "./InProcessSkeleton";

/**
 * Creates a proxy function that calls the proxyObjectFunction with the original arguments
 * in a this-context of proxyObject
 *
 * @param proxyObject
 * @param proxyObjectFunction
 * @returns the proxy function
 */
function createProxyFunction(proxyObject: Record<string, any>, proxyObjectFunction: Function): Function {
    return function() {
        // and call corresponding proxy object function in context of the proxyObject with the
        // arguments of this function call
        return proxyObjectFunction.apply(proxyObject, arguments);
    };
}

class InProcessStub {
    /**
     * @constructor
     *
     * @param [inProcessSkeleton] connects the inProcessStub to its skeleton
     */
    public constructor(inProcessSkeleton?: InProcessSkeleton) {
        if (inProcessSkeleton !== undefined) {
            this.setSkeleton(inProcessSkeleton);
        }
    }

    /**
     * Can set (new) inProcess Skeleton, overwrites members
     *
     * @param inProcessSkeleton
     * @throws {Error} if type of inProcessSkeleton is wrong
     */
    public setSkeleton(inProcessSkeleton: InProcessSkeleton): void {
        if (inProcessSkeleton === undefined || inProcessSkeleton === null) {
            throw new Error("InProcessStub is undefined or null");
        }

        // get proxy object from skeleton
        const proxyObject = inProcessSkeleton.getProxyObject();

        // cycle over all members in the proxy object
        for (const key in proxyObject) {
            if (proxyObject.hasOwnProperty(key)) {
                // get the member of the proxy object
                const proxyObjectMember = proxyObject[key];
                // if it is a function
                if (typeof proxyObjectMember === "function") {
                    // attach a function to this object
                    // @ts-ignore TODO: remove typescript incompatible pattern
                    this[key] = createProxyFunction(proxyObject, proxyObjectMember);
                }
                // else: not a function, do not proxy member, maybe implement this here using
                // getter/setter with Object.defineProperty not required until now
            }
        }
    }
}

export = InProcessStub;
