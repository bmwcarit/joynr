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
var Typing = require('../util/Typing');
var Util = require('../util/UtilInternal');
var SubscriptionQos = require('./SubscriptionQos');

    var defaultSettings;

    /**
     * @classdesc
     * Class representing the quality of service settings for non-selective
     * broadcasts.<br/>
     * This class stores quality of service settings used for subscriptions to
     * <b>non-selective broadcasts</b> in generated proxy objects. Notifications
     * will be sent  whenever the provider fires a broadcast. The subscription
     * will automatically expire after the expiry date is reached.<br/>
     *
     * @summary
     * Constructor of MulticastSubscriptionQos object used for subscriptions
     * to <b>non-selective broadcasts</b> in generated proxy objects.
     *
     * @constructor
     * @name MulticastSubscriptionQos
     *
     * @param {Object}
     *            [settings] the settings object for the constructor call
     * @param {Number}
     *            [settings.expiryDateMs] how long is the subscription valid
     * @param {Number}
     *            [settings.validityMs] The validity of the subscription relative to the current time.
     *
     * @returns {MulticastSubscriptionQos} a subscription Qos Object for subscriptions
     *            on <b>attributes and events</b>
     *
     * @see {@link SubscriptionQos} for more information on <b>expiryDateMs</b> and <b>validityMs</b>
     */
    function MulticastSubscriptionQos(settings) {
        if (!(this instanceof MulticastSubscriptionQos)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new MulticastSubscriptionQos(settings);
        }

        var subscriptionQos = new SubscriptionQos(settings);

        /**
         * Used for serialization.
         * @name MulticastSubscriptionQos#_typeName
         * @type String
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.MulticastSubscriptionQos");
        Typing.checkPropertyIfDefined(settings, "Object", "settings");

        /**
         * See [constructor description]{@link MulticastSubscriptionQos}.
         * @name MulticastSubscriptionQos#expiryDateMs
         * @type Number
         */
        Util.extend(this, defaultSettings, settings, subscriptionQos);
    }

    defaultSettings = {};

    module.exports = MulticastSubscriptionQos;
