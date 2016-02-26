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

define(
        "joynr/proxy/OnChangeSubscriptionQos",
        [
            "joynr/util/UtilInternal",
            "joynr/proxy/SubscriptionQos",
            "joynr/system/LoggerFactory"
        ],
        function(Util, SubscriptionQos, LoggerFactory) {

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
             * minIntervalMs can be used to prevent too many messages being sent.
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
             *            [settings.minInterval] Deprecated parameter. Use settings.minIntervalMs instead
             * @param {Number}
             *            [settings.minIntervalMs=0] defines how often an update may be
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
             *              <li>minimum value: {@link OnChangeSubscriptionQos.MIN_INTERVAL_MS}</li>
             *              <li>default value: {@link OnChangeSubscriptionQos.MIN_INTERVAL_MS}</li>
             *            </ul>
             * @param {Number}
             *            [settings.expiryDate] Deprecated parameter. Use settings.expiryDateMs instead
             * @param {Number}
             *            [settings.expiryDateMs] how long is the subscription valid
             * @param {Number}
             *            [settings.publicationTtl] Deprecated parameter. Use settings.publicationTtlMs instead
             * @param {Number}
             *            [settings.publicationTtlMs] time to live for publication messages
             *
             * @returns {OnChangeSubscriptionQos} a subscription Qos Object for subscriptions
             *            on <b>attributes and events</b>
             *
             * @see {@link SubscriptionQos} for more information on <b>expiryDateMs</b>
             * and <b>publicationTtlMs</b>
             */
            function OnChangeSubscriptionQos(settings) {
                if (!(this instanceof OnChangeSubscriptionQos)) {
                    // in case someone calls constructor without new keyword (e.g. var c
                    // = Constructor({..}))
                    return new OnChangeSubscriptionQos(settings);
                }

                var subscriptionQos = new SubscriptionQos(settings);
                var log = LoggerFactory.getLogger("joynr.proxy.OnChangeSubscriptionQos");

                /**
                 * Used for serialization.
                 * @name OnChangeSubscriptionQos#_typeName
                 * @type String
                 */
                Util.objectDefineProperty(this, "_typeName", "joynr.OnChangeSubscriptionQos");
                Util.checkPropertyIfDefined(settings, "Object", "settings");
                if (settings) {
                    if (settings.minInterval !== undefined) {
                        log
                                .warn("OnChangeSubscriptionQos has been invoked with deprecated settings member \"minIntervalMs\". "
                                    + "By 2017-01-01, the min interval can only be specified with member \"minIntervalMs\".");
                        settings.minIntervalMs = settings.minInterval;
                        settings.minInterval = undefined;
                    }
                    Util.checkPropertyIfDefined(
                            settings.minIntervalMs,
                            "Number",
                            "settings.minIntervalMs");
                }

                /**
                 * See [constructor description]{@link OnChangeSubscriptionQos}.
                 * @name OnChangeSubscriptionQos#minIntervalMs
                 * @type Number
                 */
                /**
                 * See [constructor description]{@link OnChangeSubscriptionQos}.
                 * @name OnChangeSubscriptionQos#expiryDateMs
                 * @type Number
                 */
                /**
                 * See [constructor description]{@link OnChangeSubscriptionQos}.
                 * @name OnChangeSubscriptionQos#publicationTtlMs
                 * @type Number
                 */
                Util.extend(this, defaultSettings, settings, subscriptionQos);
                if (this.minIntervalMs < OnChangeSubscriptionQos.MIN_INTERVAL_MS) {
                    throw new Error("Wrong minIntervalMs with value "
                        + this.minIntervalMs
                        + ": it shall be higher than "
                        + OnChangeSubscriptionQos.MIN_INTERVAL_MS);
                }

            }

            /**
             * Minimum and default value for [minIntervalMs]{@link OnChangeSubscriptionQos#minIntervalMs}.
             * See [constructor description]{@link OnChangeSubscriptionQos}.
             *
             * @name OnChangeSubscriptionQos.MIN_INTERVAL_MS
             * @type Number
             * @default 0
             * @static
             * @readonly
             */
            OnChangeSubscriptionQos.MIN_INTERVAL_MS = 0;
            OnChangeSubscriptionQos.MIN_INTERVAL = OnChangeSubscriptionQos.MIN_INTERVAL_MS;

            defaultSettings = {
                minIntervalMs : OnChangeSubscriptionQos.MIN_INTERVAL_MS
            };

            return OnChangeSubscriptionQos;

        });
