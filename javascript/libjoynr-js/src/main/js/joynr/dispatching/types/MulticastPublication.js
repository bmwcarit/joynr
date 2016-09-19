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

define("joynr/dispatching/types/MulticastPublication", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing"
], function(Util, Typing) {

    /**
     * @name MulticastPublication
     * @constructor
     *
     * @param {String}
     *            settings.multicastId
     * @param {Object}
     *            settings.response
     * @param {Object}
     *            settings.error The exception object in case of publication failure
     */
    function MulticastPublication(settings) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.multicastId, "String", "settings.multicastId");
        if (settings.response === undefined && settings.error === undefined) {
            throw new Error("MulticastPublication object does neither contain response nor error");
        }
        if (settings.error && Util.isArray(settings.response) && settings.response.length > 0) {
            throw new Error("MulticastPublication object contains both response and error");
        }
        if (settings.response) {
            settings.response = Util.ensureTypedValues(settings.response);
        }

        Typing.checkPropertyIfDefined(settings.error, [
            "Object",
            "JoynrRuntimeException",
            "PublicationMissedException",
            "ProviderRuntimeException"
        ], "settings.error");

        /**
         * @name MulticastPublication#multicastId
         * @type String
         */
        this.multicastId = settings.multicastId;
        /**
         * @name MulticastPublication#response
         * @type Object
         */
        this.response = settings.response;
        /**
         * @name MulticastPublication#error
         * @type Object
         */
        this.error = settings.error;

        /**
         * The joynr type name
         *
         * @name MulticastPublication#_typeName
         * @type String
         */
        Typing.augmentTypeName(this, "joynr");

        return Object.freeze(this);
    }

    return MulticastPublication;

});
