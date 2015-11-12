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

define("joynr/proxy/OnChangeSubscriptionQos", [
    "joynr/util/UtilInternal",
    "joynr/proxy/SubscriptionQos"
], function(Util, SubscriptionQos) {

    var defaultSettings;

    /**
     * @classdesc
     * Class representing the quality of service settings for subscriptions
     * based on changes.<br/>
     * This class stores quality of service settings used for subscriptions to
     * <b>broadcasts and attributes</b> in generated proxy objects. Notifications
     * will only be sent if the subscribed value has changed. The subscription
     * will automatically expire after the expiry date is reached. If no
     * publications were received for alertAfterInterval, publicationMissed
     * will be called.<br/>
     * minInterval can be used to prevent too many messages being sent.
     *
     * @summary
     * Constructor of OnChangeSubscriptionQos object used for subscriptions
     * to <b>events and attributes</b> in generated proxy objects.
     *
     * @constructor
     * @name OnChangeSubscriptionQos
     *
     * @param {Object}
     *            [settings] the settings object for the constructor call
     * @param {Number}
     *            [settings.minInterval=0] defines how often an update may be
     *            sent<br/>
     *            It is used to prevent flooding. Publications will be sent
     *            maintaining this minimum interval provided, even if the value
     *            changes more often. This prevents the consumer from being
     *            flooded by updated values. The filtering happens on the
     *            provider's side, thus also preventing excessive network
     *            traffic.<br/>
     *            <br/>
     *            <b>Minimum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link OnChangeSubscriptionQos.MIN_INTERVAL}</li>
     *              <li>default value: {@link OnChangeSubscriptionQos.MIN_INTERVAL}</li>
     *            </ul>
     * @param {Number}
     *            [settings.expiryDate] how long is the subscription valid
     * @param {Number}
     *            [settings.publicationTtl] time to live for publication messages
     *
     * @returns {OnChangeSubscriptionQos} a subscription Qos Object for subscriptions
     *            on <b>attributes and events</b>
     *
     * @see {@link SubscriptionQos} for more information on <b>expiryDate</b>
     * and <b>publicationTtl</b>
     */
    function OnChangeSubscriptionQos(settings) {
        if (!(this instanceof OnChangeSubscriptionQos)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new OnChangeSubscriptionQos(settings);
        }

        var subscriptionQos = new SubscriptionQos(settings);

        /**
         * Used for serialization.
         * @name OnChangeSubscriptionQos#_typeName
         * @type String
         * @field
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.OnChangeSubscriptionQos");
        Util.checkPropertyIfDefined(settings, "Object", "settings");
        if (settings) {
            Util.checkPropertyIfDefined(settings.minInterval, "Number", "settings.minInterval");
        }

        /**
         * See [constructor description]{@link OnChangeSubscriptionQos}.
         * @name OnChangeSubscriptionQos#minInterval
         * @type Number
         * @field
         */
        /**
         * See [constructor description]{@link OnChangeSubscriptionQos}.
         * @name OnChangeSubscriptionQos#expiryDate
         * @type Number
         * @field
         */
        /**
         * See [constructor description]{@link OnChangeSubscriptionQos}.
         * @name OnChangeSubscriptionQos#publicationTtl
         * @type Number
         * @field
         */
        Util.extend(this, defaultSettings, settings, subscriptionQos);
        if (this.minInterval < OnChangeSubscriptionQos.MIN_INTERVAL) {
            throw new Error("Wrong minInterval with value "
                + this.minInterval
                + ": it shall be higher than "
                + OnChangeSubscriptionQos.MIN_INTERVAL);
        }

    }

    /**
     * Minimum and default value for [minInterval]{@link OnChangeSubscriptionQos#minInterval}.
     * See [constructor description]{@link OnChangeSubscriptionQos}.
     *
     * @name OnChangeSubscriptionQos.MIN_INTERVAL
     * @type Number
     * @default 0
     * @static
     * @readonly
     */
    OnChangeSubscriptionQos.MIN_INTERVAL = 0;

    defaultSettings = {
        minInterval : OnChangeSubscriptionQos.MIN_INTERVAL
    };

    return OnChangeSubscriptionQos;

});