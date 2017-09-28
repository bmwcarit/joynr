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
var TypeRegistrySingleton = require('../../joynr/types/TypeRegistrySingleton');
var Typing = require('../util/Typing');
var Util = require('../util/UtilInternal');
var JoynrRuntimeException = require('./JoynrRuntimeException');
var LoggerFactory = require('../system/LoggerFactory');
    var defaultSettings;

    /**
     * @classdesc
     *
     * @summary
     * Constructor of SubscriptionException object used for reporting
     * error conditions when creating a subscription (e.g. the
     * provided subscription parameters are not correct etc.) that should
     * be transmitted back to consumer side.
     *
     * @constructor
     * @name SubscriptionException
     *
     * @param {Object}
     *            settings - the settings object for the constructor call
     * @param {String}
     *            [settings.detailMessage] message containing details
     *            about the error
     * @param {String}
     *            settings.subscriptionId - Id of the subscription
     * @returns {SubscriptionException}
     *            The newly created SubscriptionException object
     */
    function SubscriptionException(settings) {
        if (!(this instanceof SubscriptionException)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new SubscriptionException(settings);
        }

        var log = LoggerFactory.getLogger("joynr.exceptions.SubscriptionException");
        var runtimeException = new JoynrRuntimeException(settings);

        /**
         * Used for serialization.
         * @name SubscriptionException#_typeName
         * @type String
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.exceptions.SubscriptionException");

        if (settings) {
            Typing.checkPropertyIfDefined(
                    settings.subscriptionId,
                    "String",
                    "settings.subscriptionId");
        }

        Util.extend(this, defaultSettings, settings, runtimeException);
    }

    defaultSettings = {};

    TypeRegistrySingleton.getInstance().addType(
            "joynr.exceptions.SubscriptionException",
            SubscriptionException);

    SubscriptionException.prototype = new Error();
    SubscriptionException.prototype.constructor = SubscriptionException;
    SubscriptionException.prototype.name = "SubscriptionException";

    module.exports = SubscriptionException;
