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

define("joynr/proxy/PeriodicSubscriptionQos", [
    "joynr/util/UtilInternal",
    "joynr/proxy/SubscriptionQos"
], function(Util, SubscriptionQos) {

    var defaultSettings;

    /**
     * @classdesc
     * Class representing the quality of service settings for subscriptions based
     * on time periods.<br/>
     * This class stores quality of service settings used for subscriptions to
     * <b>attributes</b> in generated proxy objects. Notifications will only be
     * sent if the period has expired. The subscription will automatically expire
     * after the expiry date is reached. If no publications were received for
     * alertAfter interval, publicationMissed will be called.
     *
     * @summary
     * Constructor of PeriodicSubscriptionQos object used for subscriptions
     * to <b>attributes</b> in generated proxy objects.
     *
     * @constructor
     * @name PeriodicSubscriptionQos
     *
     * @param {Object}
     *            [settings] the settings object for the constructor call
     * @param {Number}
     *            [settings.period=50] defines how often an update may be sent
     *            even if the value did not change (independently from value
     *            changes).<br/>
     *            <br/>
     *            <b>Minimum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link PeriodicSubscriptionQos.MIN_PERIOD}</li>
     *              <li>default value: {@link PeriodicSubscriptionQos.MIN_PERIOD}</li>
     *            </ul>
     * @param {Number}
     *            [settings.expiryDate] how long is the subscription valid
     * @param {Number}
     *            [settings.alertAfterInterval=0] defines how long to wait for an
     *            update before publicationMissed is called.<br/>
     *            <br/>
     *            <b>Minimum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link PeriodicSubscriptionQos#period}</li>
     *              <li>default value: {@link PeriodicSubscriptionQos.NEVER_ALERT}</li>
     *            </ul>
     * @param {Number}
     *            [settings.publicationTtl] Time to live for publication messages
     *
     * @returns {PeriodicSubscriptionQos} a subscription Qos Object for subscriptions
     *            on <b>attributes</b>
     *
     * @see {@link SubscriptionQos} for more information on <b>expiryDate</b>
     * and <b>publicationTtl</b>
     */
    function PeriodicSubscriptionQos(settings) {
        if (!(this instanceof PeriodicSubscriptionQos)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new PeriodicSubscriptionQos(settings);
        }

        var subscriptionQos = new SubscriptionQos(settings);

        /**
         * Used for serialization.
         * @name PeriodicSubscriptionQos#_typeName
         * @type String
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.PeriodicSubscriptionQos");
        Util.checkPropertyIfDefined(settings, "Object", "settings");
        if (settings) {
            Util.checkPropertyIfDefined(settings.period, "Number", "settings.period");
            Util.checkPropertyIfDefined(
                    settings.alertAfterInterval,
                    "Number",
                    "settings.alertAfterInterval");
        }

        /**
         * See [constructor description]{@link PeriodicSubscriptionQos}.
         * @name PeriodicSubscriptionQos#period
         * @type Number
         */
        /**
         * See [constructor description]{@link PeriodicSubscriptionQos}.
         * @name PeriodicSubscriptionQos#expiryDate
         * @type Number
         */
        /**
         * See [constructor description]{@link PeriodicSubscriptionQos}.
         * @name PeriodicSubscriptionQos#alertAfterInterval
         * @type Number
         */
        /**
         * See [constructor description]{@link PeriodicSubscriptionQos}.
         * @name PeriodicSubscriptionQos#publicationTtl
         * @type Number
         */
        Util.extend(this, defaultSettings, settings, subscriptionQos);

        if (this.period < PeriodicSubscriptionQos.MIN_PERIOD) {
            throw new Error("Wrong period with value "
                + this.period
                + ": it shall be higher than "
                + PeriodicSubscriptionQos.MIN_PERIOD);
        }

        if (this.alertAfterInterval !== PeriodicSubscriptionQos.NEVER_ALERT
            && this.alertAfterInterval < this.period) {
            throw new Error("Wrong alertAfterInterval with value "
                + this.alertAfterInterval
                + ": it shall be higher than the specified period of "
                + this.period);
        }

    }

    /**
     * Minimal and default value for [period]{@link PeriodicSubscriptionQos#period}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     *
     * @name PeriodicSubscriptionQos.MIN_PERIOD
     * @type Number
     * @default 50
     * @static
     * @readonly
     */
    PeriodicSubscriptionQos.MIN_PERIOD = 50;
    /**
     * Default value for [alertAfterInterval]{@link PeriodicSubscriptionQos#alertAfterInterval}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     *
     * @name PeriodicSubscriptionQos.NEVER_ALERT
     * @type Number
     * @default 0
     * @static
     * @readonly
     */
    PeriodicSubscriptionQos.NEVER_ALERT = 0;

    defaultSettings = {
        period : PeriodicSubscriptionQos.MIN_PERIOD,
        alertAfterInterval : PeriodicSubscriptionQos.NEVER_ALERT
    };

    return PeriodicSubscriptionQos;

});
