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

define("joynr/dispatching/types/Request", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing",
    "uuid"
], function(Util, Typing, uuid) {

    var defaultSettings = {
        paramDatatypes : [],
        params : []
    };

    /**
     * @name Request
     * @constructor
     *
     * @param {Object}
     *            settings
     * @param {String}
     *            settings.requestReplyId
     * @param {String}
     *            settings.methodName
     * @param {Array}
     *            [settings.paramDatatypes] parameter datatypes
     * @param {String}
     *            settings.paramDatatypes.array
     * @param {Array}
     *            [settings.params] parameters
     * @param {?}
     *            settings.params.array
     */
    function Request(settings) {
        var i;
        settings.requestReplyId = settings.requestReplyId || uuid();

        Util.checkProperty(settings, [
            "joynr.Request",
            "Object"
        ], "settings");
        Util.checkProperty(settings.requestReplyId, "String", "settings.requestReplyId");
        Util.checkProperty(settings.methodName, "String", "settings.methodName");
        Util.checkPropertyIfDefined(settings.paramDatatypes, "Array", "settings.paramDatatypes");
        Util.checkPropertyIfDefined(settings.params, "Array", "settings.params");

        if (settings.params) {
            for (i = 0; i < settings.params.length; i++) {
                settings.params[i] = Util.ensureTypedValues(settings.params[i]);
            }
        }

        /**
         * @name Request#requestReplyId
         * @type String
         * @field
         */
        /**
         * @name Request#methodName
         * @type String
         * @field
         */
        /**
         * @name Request#paramDatatypes
         * @type Array
         * @field
         */
        /**
         * @name Request#params
         * @type Array
         * @field
         */
        Util.extend(this, defaultSettings, settings);

        /**
         * The joynr type name
         *
         * @name Request#_typeName
         * @type String
         * @field
         */
        Typing.augmentTypeName(this, "joynr");

        return Object.freeze(this);
    }

    return Request;

});