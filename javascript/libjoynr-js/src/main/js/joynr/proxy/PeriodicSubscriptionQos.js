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
        "joynr/proxy/PeriodicSubscriptionQos",
        [
            "joynr/util/UtilInternal",
            "joynr/proxy/SubscriptionQos",
            "joynr/system/LoggerFactory"
        ],
        function(Util, SubscriptionQos, LoggerFactory) {

            var defaultSettings;

            /**
             * @classdesc
             * Class representing the quality of service settings for subscriptions based
             * on time periods.<br/>
             * This class stores quality of service settings used for subscriptions to
             * <b>attributes</b> in generated proxy objects. Notifications will only be
             * sent if the period has expired. The subscription will automatically expire
             * after the expiry date is reached. If no publications were received for
             * alertAfterIntervalMs, publicationMissed will be called.
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
             *            [settings.period] Deprecated parameter. Use settings.periodMs instead
             * @param {Number}
             *            [settings.periodMs=PeriodicSubscriptionQos.MIN_PERIOD_MS] defines 
             *            how often an update may be sent even if the value did not change
             *            (independently from value changes).<br/>
             *            <br/>
             *            <b>Minimum and Default Values:</b>
             *            <ul>
             *              <li>minimum value: {@link PeriodicSubscriptionQos.MIN_PERIOD_MS}</li>
             *              <li>default value: {@link PeriodicSubscriptionQos.MIN_PERIOD_MS}</li>
             *            </ul>
             * @param {Number}
             *            [settings.expiryDate] Deprecated parameter. Use settings.expiryDateMs instead
             * @param {Number}
             *            [settings.expiryDateMs] how long is the subscription valid
             * @param {Number}
             *            [settings.alertAfterInterval] Deprecated parameter. Use settings.alertAfterIntervalMs instead
             * @param {Number}
             *            [settings.alertAfterIntervalMs=PeriodicSubscriptionQos.NEVER_ALERT] defines how long to wait for an
             *            update before publicationMissed is called.<br/>
             *            <br/>
             *            <b>Minimum and Default Values:</b>
             *            <ul>
             *              <li>minimum value: {@link PeriodicSubscriptionQos#period}</li>
             *              <li>default value: {@link PeriodicSubscriptionQos.NEVER_ALERT}</li>
             *            </ul>
             * @param {Number}
             *            [settings.publicationTtl] Deprecated parameter. Use settings.publicationTtlMs instead
             * @param {Number}
             *            [settings.publicationTtlMs] Time to live for publication messages
             *
             * @returns {PeriodicSubscriptionQos} a subscription Qos Object for subscriptions
             *            on <b>attributes</b>
             *
             * @see {@link SubscriptionQos} for more information on <b>expiryDateMs</b>
             * and <b>publicationTtlMs</b>
             */
            function PeriodicSubscriptionQos(settings) {
                if (!(this instanceof PeriodicSubscriptionQos)) {
                    // in case someone calls constructor without new keyword (e.g. var c
                    // = Constructor({..}))
                    return new PeriodicSubscriptionQos(settings);
                }

                var subscriptionQos = new SubscriptionQos(settings);
                var log = LoggerFactory.getLogger("joynr.proxy.PeriodicSubscriptionQos");

                /**
                 * Used for serialization.
                 * @name PeriodicSubscriptionQos#_typeName
                 * @type String
                 */
                Util.objectDefineProperty(this, "_typeName", "joynr.PeriodicSubscriptionQos");
                Util.checkPropertyIfDefined(settings, "Object", "settings");
                if (settings) {
                    if (settings.period !== undefined) {
                        log
                                .warn("PeriodicSubscriptionQos has been invoked with deprecated settings member \"period\". "
                                    + "By 2017-01-01, the min interval can only be specified with member \"periodMs\".");
                        settings.periodMs = settings.period;
                        settings.period = undefined;
                    }
                    Util.checkPropertyIfDefined(settings.periodMs, "Number", "settings.periodMs");
                    if (settings.alertAfterInterval !== undefined) {
                        log
                                .warn("PeriodicSubscriptionQos has been invoked with deprecated settings member \"alertAfterInterval\". "
                                    + "By 2017-01-01, the min interval can only be specified with member \"alertAfterIntervalMs\".");
                        settings.alertAfterIntervalMs = settings.alertAfterInterval;
                        settings.alertAfterInterval = undefined;
                    }
                    Util.checkPropertyIfDefined(
                            settings.alertAfterIntervalMs,
                            "Number",
                            "settings.alertAfterIntervalMs");
                }

                /**
                 * See [constructor description]{@link PeriodicSubscriptionQos}.
                 * @name PeriodicSubscriptionQos#periodMs
                 * @type Number
                 */
                /**
                 * See [constructor description]{@link PeriodicSubscriptionQos}.
                 * @name PeriodicSubscriptionQos#expiryDateMs
                 * @type Number
                 */
                /**
                 * See [constructor description]{@link PeriodicSubscriptionQos}.
                 * @name PeriodicSubscriptionQos#alertAfterIntervalMs
                 * @type Number
                 */
                /**
                 * See [constructor description]{@link PeriodicSubscriptionQos}.
                 * @name PeriodicSubscriptionQos#publicationTtlMs
                 * @type Number
                 */
                Util.extend(this, defaultSettings, settings, subscriptionQos);

                if (this.periodMs < PeriodicSubscriptionQos.MIN_PERIOD_MS) {
                    throw new Error("Wrong periodMs with value "
                        + this.periodMs
                        + ": it shall be higher than "
                        + PeriodicSubscriptionQos.MIN_PERIOD_MS);
                }

                if (this.alertAfterIntervalMs !== PeriodicSubscriptionQos.NEVER_ALERT
                    && this.alertAfterIntervalMs < this.periodMs) {
                    throw new Error("Wrong alertAfterIntervalMs with value "
                        + this.alertAfterIntervalMs
                        + ": it shall be higher than the specified periodMs of "
                        + this.periodMs);
                }

            }

            /**
             * Minimal and default value for [periodMs]{@link PeriodicSubscriptionQos#periodMs}.
             * See [constructor description]{@link PeriodicSubscriptionQos}.
             *
             * @name PeriodicSubscriptionQos.MIN_PERIOD_MS
             * @type Number
             * @default 50
             * @static
             * @readonly
             */
            PeriodicSubscriptionQos.MIN_PERIOD_MS = 50;
            PeriodicSubscriptionQos.MIN_PERIOD = PeriodicSubscriptionQos.MIN_PERIOD_MS;
            /**
             * Default value for [alertAfterIntervalMs]{@link PeriodicSubscriptionQos#alertAfterIntervalMs}.
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
                periodMs : PeriodicSubscriptionQos.MIN_PERIOD_MS,
                alertAfterIntervalMs : PeriodicSubscriptionQos.NEVER_ALERT
            };

            return PeriodicSubscriptionQos;

        });
