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
const Util = require("../util/UtilInternal");
const OnChangeSubscriptionQos = require("./OnChangeSubscriptionQos");
const LoggingManager = require("../system/LoggingManager");

let defaultSettings;

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
 * were received for alertAfterIntervalMs, publicationMissed will be called.
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
 *            [settings.minIntervalMs] defines how often an update may
 *            be sent
 * @param {Number}
 *            [settings.maxIntervalMs=OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS]
 *            defines how long to wait before sending an update even if the value did not change<br/>
 *            The provider will send publications every maximum interval in
 *            milliseconds, even if the value didn't change. It will send
 *            notifications more often if on-change notifications are enabled,
 *            the value changes more often, and the minimum interval QoS does
 *            not prevent it. The maximum interval can thus be seen as a sort
 *            of heart beat or keep alive interval, if no other publication
 *            has been sent within that time.<br/>
 *            <br/>
 *            <b>Minimum, Maximum and Default Values</b>
 *            <ul>
 *              <li>minimum value: {@link OnChangeWithKeepAliveSubscriptionQos#minIntervalMs} or
 *                                 {@link OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS}</li>
 *              <li>maximum value: {@link OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS}</li>
 *              <li>default value: {@link OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS}</li>
 *            </ul>
 * @param {Number}
 *            [settings.expiryDateMs] how long is the subscription valid
 * @param {Number}
 *            [settings.validityMs] The validity of the subscription relative to the current time.
 * @param {Number}
 *            [settings.alertAfterIntervalMs=OnChangeWithKeepAliveSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS]
 *            defines how long to wait for an update before publicationMissed is called<br/>
 *            <br/>
 *            <b>Minimum, Maximum and Default Values:</b>
 *            <ul>
 *              <li>minimum value: {@link OnChangeWithKeepAliveSubscriptionQos#maxIntervalMs}</li>
 *              <li>maximum value: {@link OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS}</li>
 *              <li>default value: {@link OnChangeWithKeepAliveSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS}</li>
 *            </ul>
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

    const onChangeSubscriptionQos = new OnChangeSubscriptionQos(settings);
    const log = LoggingManager.getLogger("joynr.proxy.OnChangeWithKeepAliveSubscriptionQos");

    /**
     * Used for serialization.
     * @name OnChangeWithKeepAliveSubscriptionQos#_typeName
     * @type String
     */
    Util.objectDefineProperty(this, "_typeName", "joynr.OnChangeWithKeepAliveSubscriptionQos");
    Typing.checkPropertyIfDefined(settings, "Object", "settings");
    if (settings && !(settings instanceof OnChangeWithKeepAliveSubscriptionQos)) {
        Typing.checkPropertyIfDefined(settings.maxIntervalMs, "Number", "settings.maxIntervalMs");
        Typing.checkPropertyIfDefined(settings.alertAfterIntervalMs, "Number", "settings.alertAfterIntervalMs");
    }

    /**
     * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
     * @name OnChangeWithKeepAliveSubscriptionQos#minIntervalMs
     * @type Number
     */
    /**
     * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
     * @name OnChangeWithKeepAliveSubscriptionQos#maxIntervalMs
     * @type Number
     */
    /**
     * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
     * @name OnChangeWithKeepAliveSubscriptionQos#expiryDateMs
     * @type Number
     */
    /**
     * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
     * @name OnChangeWithKeepAliveSubscriptionQos#alertAfterIntervalMs
     * @type Number
     */
    /**
     * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
     * @name OnChangeWithKeepAliveSubscriptionQos#publicationTtlMs
     * @type Number
     */
    Util.extend(this, defaultSettings, settings, onChangeSubscriptionQos);

    if (this.maxIntervalMs < OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS) {
        log.warn(
            "maxIntervalMs < MIN_MAX_INTERVAL_MS. Using MIN_MAX_INTERVAL_MS: " +
                OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS
        );
        this.maxIntervalMs = OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS;
    }

    if (this.maxIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS) {
        log.warn(
            "maxIntervalMs > MAX_MAX_INTERVAL_MS. Using MAX_MAX_INTERVAL_MS: " +
                OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS
        );
        this.maxIntervalMs = OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS;
    }

    if (this.maxIntervalMs < this.minIntervalMs) {
        log.warn("maxIntervalMs < minIntervalMs. Using minIntervalMs: " + this.minIntervalMs);
        this.maxIntervalMs = this.minIntervalMs;
    }

    if (
        this.alertAfterIntervalMs !== OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL &&
        this.alertAfterIntervalMs < this.maxIntervalMs
    ) {
        log.warn("alertAfterIntervalMs < maxIntervalMs. Using maxIntervalMs: " + this.maxIntervalMs);
        this.alertAfterIntervalMs = this.maxIntervalMs;
    }

    if (this.alertAfterIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS) {
        log.warn(
            "alertAfterIntervalMs > MAX_ALERT_AFTER_INTERVAL_MS. Using MAX_ALERT_AFTER_INTERVAL_MS: " +
                OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
        );
        this.alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS;
    }

    /**
     * The function clearAlertAfterInterval resets the alter after interval to
     * the value OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL
     *
     *
     * @name OnChangeWithKeepAliveSubscriptionQos#clearAlertAfterInterval
     * @function
     */
    this.clearAlertAfterInterval = function clearAlertAfterInterval() {
        this.alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL;
    };
}

/**
 * Default value for [alertAfterIntervalMs]{@link OnChangeWithKeepAliveSubscriptionQos#alertAfterIntervalMs}.
 * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
 *
 * @name OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL
 * @type Number
 * @default 0
 * @static
 * @readonly
 */
OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL = 0;

/**
 * Maximum value for [alertAfterIntervalMs]{@link OnChangeWithKeepAliveSubscriptionQos#alertAfterIntervalMs}.
 * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
 *
 * @name OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
 * @type Number
 * @default 2 592 000 000 (30 days)
 * @static
 * @readonly
 */
OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS = 2592000000;

/**
 * Default value for [alertAfterIntervalMs]{@link OnChangeWithKeepAliveSubscriptionQos#alertAfterIntervalMs}.
 * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
 *
 * @name OnChangeWithKeepAliveSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS
 * @type Number
 * @default OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL
 * @static
 * @readonly
 */
OnChangeWithKeepAliveSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS =
    OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL;

/**
 * Minimal value for [maxIntervalMs]{@link OnChangeWithKeepAliveSubscriptionQos#maxIntervalMs}.
 * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
 *
 * @name OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS
 * @type Number
 * @default 50
 * @static
 * @readonly
 */
OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS = 50;

/**
 * Maximum value for [maxIntervalMs]{@link OnChangeWithKeepAliveSubscriptionQos#maxIntervalMs}.
 * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
 *
 * @name OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS
 * @type Number
 * @default 2 592 000 000 (30 days)
 * @static
 * @readonly
 */
OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS = 2592000000;

/**
 * Default value for [maxIntervalMs]{@link OnChangeWithKeepAliveSubscriptionQos#maxIntervalMs}.
 * See [constructor description]{@link OnChangeWithKeepAliveSubscriptionQos}.
 *
 * @name OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS
 * @type Number
 * @default 60000
 * @static
 * @readonly
 */
OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS = 60000;

defaultSettings = {
    alertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS,
    maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS
};

module.exports = OnChangeWithKeepAliveSubscriptionQos;
