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
const Typing = require("../util/Typing");
const UtilInternal = require("../util/UtilInternal");
const SubscriptionQos = require("./SubscriptionQos");
const LoggingManager = require("../system/LoggingManager");

/*eslint-disable prefer-const*/
let defaultSettings;
/*eslint-enable prefer-const*/

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
 *            [settings.periodMs=PeriodicSubscriptionQos.DEFAULT_PERIOD_MS] defines
 *            how often an update may be sent even if the value did not change
 *            (independently from value changes).<br/>
 *            <br/>
 *            <b>Minimum, Maximum and Default Values:</b>
 *            <ul>
 *              <li>minimum value: {@link PeriodicSubscriptionQos.MIN_PERIOD_MS}</li>
 *              <li>maximum value: {@link PeriodicSubscriptionQos.MAX_PERIOD_MS}</li>
 *              <li>default value: {@link PeriodicSubscriptionQos.DEFAULT_PERIOD_MS}</li>
 *            </ul>
 * @param {Number}
 *            [settings.expiryDateMs] how long is the subscription valid
 * @param {Number}
 *            [settings.validityMs] The validity of the subscription relative to the current time.
 * @param {Number}
 *            [settings.alertAfterIntervalMs=PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS] defines how long to wait for an
 *            update before publicationMissed is called.<br/>
 *            <br/>
 *            <b>Minimum, Maximum and Default Values:</b>
 *            <ul>
 *              <li>minimum value: {@link PeriodicSubscriptionQos#period}</li>
 *              <li>maximum value: {@link PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS}</li>
 *              <li>default value: {@link PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS}</li>
 *            </ul>
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

    const subscriptionQos = new SubscriptionQos(settings);
    const log = LoggingManager.getLogger("joynr.proxy.PeriodicSubscriptionQos");

    /**
     * Used for serialization.
     * @name PeriodicSubscriptionQos#_typeName
     * @type String
     */
    UtilInternal.objectDefineProperty(this, "_typeName", "joynr.PeriodicSubscriptionQos");
    Typing.checkPropertyIfDefined(settings, "Object", "settings");
    if (settings && !(settings instanceof PeriodicSubscriptionQos)) {
        Typing.checkPropertyIfDefined(settings.periodMs, "Number", "settings.periodMs");
        Typing.checkPropertyIfDefined(settings.alertAfterIntervalMs, "Number", "settings.alertAfterIntervalMs");
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
    UtilInternal.extend(this, defaultSettings, settings, subscriptionQos);

    if (this.periodMs < PeriodicSubscriptionQos.MIN_PERIOD_MS) {
        throw new Error(
            `Wrong periodMs with value ${this.periodMs}: it shall be higher than ${
                PeriodicSubscriptionQos.MIN_PERIOD_MS
            }`
        );
    }

    if (this.periodMs > PeriodicSubscriptionQos.MAX_PERIOD_MS) {
        throw new Error(
            `Wrong periodMs with value ${this.periodMs}: it shall be lower than ${
                PeriodicSubscriptionQos.MAX_PERIOD_MS
            }`
        );
    }

    if (
        this.alertAfterIntervalMs !== PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL &&
        this.alertAfterIntervalMs < this.periodMs
    ) {
        log.warn(`alertAfterIntervalMs < periodMs. Using periodMs: ${this.periodMs}`);
        this.alertAfterIntervalMs = this.periodMs;
    }

    if (this.alertAfterIntervalMs > PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS) {
        log.warn(
            `alertAfterIntervalMs > MAX_ALERT_AFTER_INTERVAL_MS. Using MAX_ALERT_AFTER_INTERVAL_MS: ${
                PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
            }`
        );
        this.alertAfterIntervalMs = PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS;
    }

    /**
     * The function clearAlertAfterInterval resets the alter after interval to
     * the value PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL
     *
     *
     * @name PeriodicSubscriptionQos#clearAlertAfterInterval
     * @function
     */
    this.clearAlertAfterInterval = function clearAlertAfterInterval() {
        this.alertAfterIntervalMs = PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL;
    };
}

/**
 * Minimal value for [periodMs]{@link PeriodicSubscriptionQos#periodMs}.
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
 * Maximum value for [periodMs]{@link PeriodicSubscriptionQos#periodMs}.
 * See [constructor description]{@link PeriodicSubscriptionQos}.
 *
 * @name PeriodicSubscriptionQos.MAX_PERIOD_MS
 * @type Number
 * @default 2 592 000 000 (30 days)
 * @static
 * @readonly
 */
PeriodicSubscriptionQos.MAX_PERIOD_MS = 2592000000;

/**
 * Default value for [periodMs]{@link PeriodicSubscriptionQos#periodMs}.
 * See [constructor description]{@link PeriodicSubscriptionQos}.
 *
 * @name PeriodicSubscriptionQos.DEFAULT_PERIOD_MS
 * @type Number
 * @default 60000
 * @static
 * @readonly
 */
PeriodicSubscriptionQos.DEFAULT_PERIOD_MS = 60000;

/**
 * Default value for [alertAfterIntervalMs]{@link PeriodicSubscriptionQos#alertAfterIntervalMs}.
 * See [constructor description]{@link PeriodicSubscriptionQos}.
 *
 * @name PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL
 * @type Number
 * @default 0
 * @static
 * @readonly
 */
PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL = 0;

/**
 * Maximum value for [alertAfterIntervalMs]{@link PeriodicSubscriptionQos#alertAfterIntervalMs}.
 * See [constructor description]{@link PeriodicSubscriptionQos}.
 *
 * @name PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
 * @type Number
 * @default 2 592 000 000 (30 days)
 * @static
 * @readonly
 */
PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS = 2592000000;

/**
 * Default value for [alertAfterIntervalMs]{@link PeriodicSubscriptionQos#alertAfterIntervalMs}.
 * See [constructor description]{@link PeriodicSubscriptionQos}.
 *
 * @name PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS
 * @type Number
 * @default PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL
 * @static
 * @readonly
 */
PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS = PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL;

defaultSettings = {
    periodMs: PeriodicSubscriptionQos.DEFAULT_PERIOD_MS,
    alertAfterIntervalMs: PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS
};

module.exports = PeriodicSubscriptionQos;
