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

define("joynr/proxy/OnChangeWithKeepAliveSubscriptionQos", [
    "joynr/util/UtilInternal",
    "joynr/proxy/OnChangeSubscriptionQos"
], function(Util, OnChangeSubscriptionQos) {

    var defaultSettings;

    /**
     * @classdesc
     * Class representing the quality of service settings for subscriptions
     * based on changes and time periods
     * <br/>
     * This class stores quality of service settings used for subscriptions to
     * <b>attributes</b> in generated proxy objects. Using it for subscriptions
     * to events is theoretically possible because of inheritance but makes
     * no sense (in this case the additional members will be ignored).
     * <br/>
     * Notifications will be sent if the subscribed value has changed or a time
     * interval without notifications has expired. The subscription will
     * automatically expire after the expiry date is reached. If no publications
     * were received for alertAfter Interval, publicationMissed will be called.
     * <br/>
     * minIntervalMs can be used to prevent too many messages being sent.
     *
     * @summary
     * Constructor of OnChangeWithKeepAliveSubscriptionQos object used
     * for subscriptions to <b>attributes</b> in generated proxy objects
     *
     * @constructor
     * @name OnChangeWithKeepAliveSubscriptionQos
     *
     * @param {Object}
     *            [settings] the settings object for the constructor call
     * @param {Number}
     *            [settings.minInterval] Deprecated parameter. Use settings.minIntervalMs instead
     * @param {Number}
     *            [settings.minIntervalMs] defines how often an update may
     *            be sent
     * @param {Number}
     *            [settings.maxInterval] defines how long to wait before
     *            sending an update even if the value did not change<br/>
     *            The provider will send publications every maximum interval in
     *            milliseconds, even if the value didn't change. It will send
     *            notifications more often if on-change notifications are enabled,
     *            the value changes more often, and the minimum interval QoS does
     *            not prevent it. The maximum interval can thus be seen as a sort
     *            of heart beat or keep alive interval, if no other publication
     *            has been sent within that time.<br/>
     *            <br/>
     *            <b>Minimum and Maximum Values</b>
     *            <ul>
     *              <li>minimum value: {@link OnChangeWithKeepAliveSubscriptionQos#minIntervalMs}</li>
     *              <li>maximum value: unlimited</li>
     *            </ul>
     * @param {Number}
     *            [settings.expiryDate] Deprecated parameter. Use settings.expiryDateMs instead
     * @param {Number}
     *            [settings.expiryDateMs] how long is the subscription
     *            valid
     * @param {Number}
     *            [settings.alertAfterInterval] defines how long to wait
     *            for an update before publicationMissed is called<br/>
     *            <br/>
     *            <b>Minimum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link OnChangeWithKeepAliveSubscriptionQos#maxInterval}</li>
     *              <li>default value: {@link OnChangeWithKeepAliveSubscriptionQos.NEVER_ALERT}</li>
     *            </ul>
     * @param {Number}
     *            [settings.publicationTtl] Deprecated parameter. Use settings.publicationTtlMs instead
     * @param {Number}
     *            [settings.publicationTtlMs] time to live for publication
     *            messages
     *
     * @returns {OnChangeWithKeepAliveSubscriptionQos} a subscription
     *          Qos Object for subscriptions on <b>attributes</b>
     *
     * @see {@link OnChangeSubscriptionQos} for more information on <b>minIntervalMs</b>
     * @see {@link SubscriptionQos} for more information on <b>expiryDateMs</b>
     * and <b>publicationTtlMs</b>
     */
    function OnChangeWithKeepAliveSubscriptionQos(settings) {
        if (!(this instanceof OnChangeWithKeepAliveSubscriptionQos)) {
            // in case someone calls constructor without new keyword
            // (e.g. var c
            // = Constructor({..}))
            return new OnChangeWithKeepAliveSubscriptionQos(settings);
        }

        var onChangeSubscriptionQos = new OnChangeSubscriptionQos(settings);

        /**
         * Used for serialization.
         * @name OnChangeWithKeepAliveSubscriptionQos#_typeName
         * @type String
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.OnChangeWithKeepAliveSubscriptionQos");
        Util.checkPropertyIfDefined(settings, "Object", "settings");
        if (settings) {
            Util.checkPropertyIfDefined(settings.maxInterval, "Number", "settings.maxInterval");
            Util.checkPropertyIfDefined(
                    settings.alertAfterInterval,
                    "Number",
                    "settings.alertAfterInterval");
        }

        /**
         * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
         * @name OnChangeWithKeepAliveSubscriptionQos#minIntervalMs
         * @type Number
         */
        /**
         * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
         * @name OnChangeWithKeepAliveSubscriptionQos#maxInterval
         * @type Number
         */
        /**
         * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
         * @name OnChangeWithKeepAliveSubscriptionQos#expiryDateMs
         * @type Number
         */
        /**
         * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
         * @name OnChangeWithKeepAliveSubscriptionQos#alertAfterInterval
         * @type Number
         */
        /**
         * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
         * @name OnChangeWithKeepAliveSubscriptionQos#publicationTtlMs
         * @type Number
         */
        Util.extend(this, defaultSettings, settings, onChangeSubscriptionQos);

        if (this.maxInterval < this.minIntervalMs) {
            throw new Error("Wrong maxInterval with value "
                + this.maxInterval
                + ": it shall be higher than the specified minIntervalMs of "
                + this.minIntervalMs);
        }

        if (this.alertAfterInterval !== OnChangeWithKeepAliveSubscriptionQos.NEVER_ALERT
            && this.alertAfterInterval < this.maxInterval) {
            throw new Error("Wrong alertAfterInterval with value "
                + this.alertAfterInterval
                + ": it shall be higher than the specified maxInterval of "
                + this.maxInterval);
        }
    }

    /**
     * Default value for [alertAfterInterval]{@link OnChangeWithKeepAliveSubscriptionQos#alertAfterInterval}.
     * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
     *
     * @name OnChangeWithKeepAliveSubscriptionQos.NEVER_ALERT
     * @type Number
     * @default 0
     * @static
     * @readonly
     */
    OnChangeWithKeepAliveSubscriptionQos.NEVER_ALERT = 0;

    defaultSettings = {
        alertAfterInterval : OnChangeWithKeepAliveSubscriptionQos.NEVER_ALERT
    };

    return OnChangeWithKeepAliveSubscriptionQos;

});
