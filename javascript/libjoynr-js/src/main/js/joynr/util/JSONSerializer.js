/*global unescape: true, Blob: true, Array:true */
/*jslint es5: true */

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

define("joynr/util/JSONSerializer", [ "joynr/util/Typing"
], function(Typing) {

    /**
     * @name JSONSerializer
     * @class
     * @classdesc Provides functions for the serialization from arbitrary objects
     *            to Strings in JSON notation
     */
    var JSONSerializer = {};

    /**
     * This function wraps the JSON.stringify call, by altering the stringification process
     * for joynr types
     * @function JSONSerializer#stringify
     * @param {Object}
     *          value to be stringified
     * @param {Boolean} omitJoynrStringReplacement an optional member. If set to false,
     *                  special string replacement for joynr objects is omitted
     * @returns {String}
     *          the value in JSON notation
     */
    JSONSerializer.stringify = function stringify(value, omitJoynrStringReplacement) {
        var replacerFunction; /* undefined by default */
        if (omitJoynrStringReplacement === undefined || !omitJoynrStringReplacement) {
            replacerFunction = function replacerFunction(key, src) {
                if (Typing.isEnumType(src)) {
                    return src.name;
                }
                return src;
            };
        }
        return JSON.stringify(value, replacerFunction);
    };

    return JSONSerializer;

});
