/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define("joynr/dispatching/types/SubscriptionPublication", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing"
], function(Util, Typing) {

    /**
     * @name SubscriptionPublication
     * @constructor
     *
     * @param {String}
     *            settings.subscriptionId
     * @param {Object}
     *            settings.response
     * @param {Object}
     *            settings.error The exception object in case of publication failure
     */
    function SubscriptionPublication(settings) {
        Util.checkProperty(settings, "Object", "settings");
        Util.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");
        if (settings.response === undefined && settings.error === undefined) {
            throw new Error(
                    "SubscriptionPublication object does neither contain response nor error");
        }
        if (settings.response && settings.error) {
            throw new Error("SubscriptionPublication object contains both response and error");
        }

        if (settings.response) {
            settings.response = Util.ensureTypedValues(settings.response);
        }

        Util.checkPropertyIfDefined(settings.error, [
            "Object",
            "ApplicationException",
            "JoynrRuntimeException",
            "PublicationMissedException",
            "ProviderRuntimeException"
        ], "settings.error");

        /**
         * @name SubscriptionPublication#subscriptionId
         * @type String
         * @field
         */
        /**
         * @name SubscriptionPublication#response
         * @type Object
         * @field
         */
        Util.extend(this, settings);

        /**
         * The joynr type name
         *
         * @name SubscriptionPublication#_typeName
         * @type String
         * @field
         */
        Typing.augmentTypeName(this, "joynr");

        return Object.freeze(this);
    }

    return SubscriptionPublication;

});
