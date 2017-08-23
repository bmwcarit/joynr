/*jslint es5: true, nomen: true */

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

define("joynr/util/InProcessSkeleton", [], function() {

    /**
     * Note: This skeleton is merely the holder of the proxy object and does not get informed about
     * calls from the stub to the proxy object (stub calls methods directly on proxy object).
     *
     * @name InProcessSkeleton
     * @constructor
     *
     * @param {Object} proxyObject the proxy object that can be accessed through the Stub
     */
    function InProcessSkeleton(proxyObject) {
        if (!(this instanceof InProcessSkeleton)) {
            // in case someone calls constructor without new keyword
            // (e.g. var c = Constructor({..}))
            return new InProcessSkeleton(proxyObject);
        }
        this._proxyObject = proxyObject;

    }

    /**
     * Getter for the proxy object
     *
     * @name InProcessSkeleton#getProxyObject
     * @function
     *
     * @returns the proxy object this is the skeleton for
     */
    InProcessSkeleton.prototype.getProxyObject = function() {
        return this._proxyObject;
    };

    return InProcessSkeleton;
});