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
var ProviderAttributeNotifyReadWrite = require('./ProviderAttributeNotifyReadWrite');
var ProviderAttributeNotifyRead = require('./ProviderAttributeNotifyRead');
var ProviderAttributeNotifyWrite = require('./ProviderAttributeNotifyWrite');
var ProviderAttributeNotify = require('./ProviderAttributeNotify');
var ProviderAttributeReadWrite = require('./ProviderAttributeReadWrite');
var ProviderAttributeRead = require('./ProviderAttributeRead');
var ProviderAttributeWrite = require('./ProviderAttributeWrite');
var ProviderOperation = require('./ProviderOperation');
var ProviderEvent = require('./ProviderEvent');
module.exports =
        (function(
                ProviderAttributeNotifyReadWrite,
                ProviderAttributeNotifyRead,
                ProviderAttributeNotifyWrite,
                ProviderAttributeNotify,
                ProviderAttributeReadWrite,
                ProviderAttributeRead,
                ProviderAttributeWrite,
                ProviderOperation,
                ProviderEvent) {

    var dependencies = {
        ProviderAttributeNotifyReadWrite : ProviderAttributeNotifyReadWrite,
        ProviderAttributeNotifyRead : ProviderAttributeNotifyRead,
        ProviderAttributeNotifyWrite : ProviderAttributeNotifyWrite,
        ProviderAttributeNotify : ProviderAttributeNotify,
        ProviderAttributeReadWrite : ProviderAttributeReadWrite,
        ProviderAttributeRead : ProviderAttributeRead,
        ProviderAttributeWrite : ProviderAttributeWrite,
        ProviderOperation : ProviderOperation,
        ProviderEvent : ProviderEvent
    };
    /**
     * @name ProviderBuilder
     * @constructor
     */
    var ProviderBuilder = function ProviderBuilder() {
        /**
         * @name ProviderBuilder#build
         * @function
         * @param {Function}
         *            ProviderConstructor - the constructor function of the generated Provider that
         *            creates a new provider instance
         * @param {Object}
         *            implementation - an object containing the same fields and public functions as
         *            exposed int he provider that implements the actual functionaltiy of the
         *            provider
         * @returns {Object} a provider of the given type
         * @throws {Error}
         *             if correct implementation was not provided
         */
        this.build = function build(ProviderConstructor, implementation) {
            return new ProviderConstructor(implementation, dependencies);
        };
    };

    return ProviderBuilder;
        }(
                ProviderAttributeNotifyReadWrite,
                ProviderAttributeNotifyRead,
                ProviderAttributeNotifyWrite,
                ProviderAttributeNotify,
                ProviderAttributeReadWrite,
                ProviderAttributeRead,
                ProviderAttributeWrite,
                ProviderOperation,
                ProviderEvent));