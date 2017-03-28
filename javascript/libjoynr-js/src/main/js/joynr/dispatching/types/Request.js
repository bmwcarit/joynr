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

define("joynr/dispatching/types/Request", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing",
    "uuid"
], function(Util, Typing, uuid) {

    var defaultSettings = {
        paramDatatypes : [],
        params : []
    };

    var rrBase = uuid();
    var rrIndex = 0;

    Util.enrichObjectWithSetPrototypeOf();

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
        settings.requestReplyId = settings.requestReplyId || (rrBase + "_" + rrIndex++);

        if (settings.params) {
            for (i = 0; i < settings.params.length; i++) {
                settings.params[i] = Util.ensureTypedValues(settings.params[i]);
            }
        }
        if (!settings.paramDatatypes) {
            settings.paramDatatypes = [];
        }

        /**
         * The joynr type name
         *
         * @name Request#_typeName
         * @type String
         */
        /*jslint nomen: true*/
        settings._typeName = "joynr.Request";
        /*jslint nomen: false */
        Object.setPrototypeOf(settings, Request.prototype);

        return settings;
    }

    return Request;

});
