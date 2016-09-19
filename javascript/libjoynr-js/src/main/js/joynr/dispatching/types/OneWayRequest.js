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

define("joynr/dispatching/types/OneWayRequest", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing"
], function(Util, Typing) {

    var defaultSettings = {
        paramDatatypes : [],
        params : []
    };

    /**
     * @name OneWayRequest
     * @constructor
     *
     * @param {Object}
     *            settings
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
    function OneWayRequest(settings) {
        if (!(this instanceof OneWayRequest)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new OneWayRequest(settings);
        }
        var i;

        Typing.checkProperty(settings, [
            "joynr.OneWayRequest",
            "Object"
        ], "settings");
        Typing.checkProperty(settings.methodName, "String", "settings.methodName");
        Typing.checkPropertyIfDefined(settings.paramDatatypes, "Array", "settings.paramDatatypes");
        Typing.checkPropertyIfDefined(settings.params, "Array", "settings.params");

        if (settings.params) {
            for (i = 0; i < settings.params.length; i++) {
                settings.params[i] = Util.ensureTypedValues(settings.params[i]);
            }
        }

        /**
         * @name OneWayRequest#methodName
         * @type String
         */
        /**
         * @name OneWayRequest#paramDatatypes
         * @type Array
         */
        /**
         * @name OneWayRequest#params
         * @type Array
         */
        Util.extend(this, defaultSettings, settings);

        /**
         * The joynr type name
         *
         * @name OneWayRequest#_typeName
         * @type String
         */
        Object.defineProperty(this, "_typeName", {
            value : "joynr.OneWayRequest",
            readable : true,
            writable : false,
            enumerable : true,
            configurable : false
        });
        return Object.freeze(this);
    }

    return OneWayRequest;

});
