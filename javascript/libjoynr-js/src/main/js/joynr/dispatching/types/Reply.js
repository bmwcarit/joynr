/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define("joynr/dispatching/types/Reply", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing"
], function(Util, Typing) {

    /**
     * @name Reply
     * @constructor
     *
     * @param {Object}
     *            settings
     * @param {String}
     *            settings.requestReplyId
     * @param {Array}
     *            [settings.response] the response may be undefined
     * @param {Object}
     *            [settings.error] The exception object in case of request failure
     */
    function Reply(settings) {
        var i;
        Util.checkProperty(settings, [
            "joynr.Reply",
            "Object"
        ], "settings");
        Util.checkProperty(settings.requestReplyId, "String", "settings.requestReplyId");
        // "response" is not set in case an exception is returned
        Util.checkPropertyIfDefined(settings.response, "Array", "settings.response");
        if (settings.response) {
            for (i = 0; i < settings.response.length; i++) {
                settings.response[i] = Util.ensureTypedValues(settings.response[i]);
            }
        }
        // if present "error" must be one of the exception types
        Util.checkPropertyIfDefined(settings.error, [
            "Object",
            "ApplicationException",
            "DiscoveryException",
            "IllegalAccessException",
            "JoynrException",
            "JoynrRuntimeException",
            "MethodInvocationException",
            "ProviderRuntimeException"
        ], "settings.error");

        // must contain exactly one of the two alternatives
        if (!settings.response && !settings.error) {
            throw new Error("Reply object does neither contain response nor error");
        }
        if (settings.error && Util.isArray(settings.response) && settings.response.length > 0) {
            throw new Error("Reply object contains both response and error");
        }

        /**
         * @name Reply#requestReplyId
         * @type String
         */
        /**
         * @name Reply#response
         * @type Array
         */
        Util.extend(this, settings);

        /**
         * The joynr type name
         *
         * @name Reply#_typeName
         * @type String
         */
        Typing.augmentTypeName(this, "joynr");

        return Object.freeze(this);
    }

    return Reply;

});
