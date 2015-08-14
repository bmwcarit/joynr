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

define("joynr/messaging/MessagingQos", [ "joynr/util/UtilInternal"
], function(Util) {

    var defaultSettings = {
        ttl : 60000
    };

    /**
     * Constructor of MessagingQos object that is used in the generation of proxy objects
     * @constructor
     * @name MessagingQos
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {Number} [settings.ttl] Roundtrip timeout for rpc requests, if missing default value is 60 seconds
     *
     * @returns {MessagingQos} a messaging Qos Object
     */
    function MessagingQos(settings) {
        if (!(this instanceof MessagingQos)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new MessagingQos(settings);
        }

        settings = Util.extend({}, defaultSettings, settings);

        /**
         * The time to live for messages
         *
         * @name MessagingQos#ttl
         * @type Number
         * @field
         */
        this.ttl = settings.ttl;
    }

    /**
     * @name MessagingQos.DEFAULT_TTL
     * @type Number
     * @default 60000
     * @static
     * @readonly
     */
    MessagingQos.DEFAULT_TTL = defaultSettings.ttl;

    return MessagingQos;

});