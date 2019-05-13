/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
const LoggingManager = require("../system/LoggingManager");

/*eslint-disable prefer-const*/
let defaultSettings;

class SubscriptionQos {
    /*eslint-enable prefer-const*/

    /**
     * @classdesc
     * Base class representing the subscription quality of service settings.
     * This class stores quality of service settings used for subscriptions to
     * <b>attributes and broadcasts</b> in generated proxy objects.<br/>
     * Subclasses are {@link PeriodicSubscriptionQos},
     * {@link OnChangeSubscriptionQos}, and
     * {@link OnChangeWithKeepAliveSubscriptionQos}.<br/>
     * The provider will send notifications until the expiry date is reached. The
     * subscription will automatically expire after the expiry date is reached.
     *
     * @summary
     * Constructor of SubscriptionQos object used for subscriptions to
     * <b>attributes and events</b> in generated proxy objects.
     *
     * @constructor
     * @name SubscriptionQos
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {Number} [settings.expiryDateMs=SubscriptionQos.NO_EXPIRY_DATE] The expiry date is the
     *            end date of the subscription. This value is provided in milliseconds
     *            (since 1970-01-01T00:00:00.000).<br/>
     *            <br/>
     *            <b>Minimum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link SubscriptionQos.MIN_EXPIRY_MS}</li>
     *              <li>default value: {@link SubscriptionQos.NO_EXPIRY_DATE}</li>
     *            </ul>
     * @param {Number} [settings.validityMs] The validity of the subscription relative to the current time.
     *            <br/>
     *            <b>Special Values:</b>
     *            <ul>
     *              <li>minimum value: 0</li>
     *            </ul>
     * @param {Number} [settings.publicationTtlMs=SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS]
     *            Time to live for publication messages.<br/>
     *            <br/>
     *            If a notification message can not be delivered within its time
     *            to live, it will be deleted from the system. This value is
     *            provided in milliseconds.<br/>
     *            <br/>
     *            <b>Minimum, Maximum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link SubscriptionQos.MIN_PUBLICATION_TTL_MS}</li>
     *              <li>maximum value: {@link SubscriptionQos.MAX_PUBLICATION_TTL_MS}</li>
     *              <li>default value: {@link SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS}</li>
     *            </ul>
     *
     * @returns {SubscriptionQos} a subscription Qos Object for subscriptions on <b>attributes
     *            and events</b>
     */
    constructor(settings) {
        const log = LoggingManager.getLogger("joynr.proxy.SubscriptionQos");

        /**
         * Used for serialization.
         * @name SubscriptionQos#_typeName
         * @type String
         */
        UtilInternal.objectDefineProperty(this, "_typeName", "joynr.SubscriptionQos");
        Typing.checkPropertyIfDefined(settings, "Object", "settings");
        if (settings && !(settings instanceof SubscriptionQos)) {
            if (settings.validityMs !== undefined) {
                if (settings.expiryDateMs !== undefined) {
                    log.warn(
                        'SubscriptionQos has been invoked with settings member "expiryDateMs" and "validityMs".' +
                            ' Please ensure that only one of these values is set. Using "validityMs"'
                    );
                }
                settings.expiryDateMs = Date.now() + settings.validityMs;
                settings.validityMs = undefined;
            }
            Typing.checkPropertyIfDefined(settings.expiryDateMs, "Number", "settings.expiryDateMs");
            Typing.checkPropertyIfDefined(settings.publicationTtlMs, "Number", "settings.publicationTtlMs");
        }

        /**
         * See [constructor description]{@link SubscriptionQos}.
         * @name SubscriptionQos#expiryDateMs
         * @type Number
         */
        /**
         * See [constructor description]{@link SubscriptionQos}.
         * @name SubscriptionQos#publicationTtlMs
         * @type Number
         */
        UtilInternal.extend(this, defaultSettings, settings);
        if (this.publicationTtlMs < SubscriptionQos.MIN_PUBLICATION_TTL_MS) {
            log.warn(
                `publicationTtlMs < MIN_PUBLICATION_TTL_MS. Using MIN_PUBLICATION_TTL_MS: ${
                    SubscriptionQos.MIN_PUBLICATION_TTL_MS
                }`
            );
            this.publicationTtlMs = SubscriptionQos.MIN_PUBLICATION_TTL_MS;
        }

        if (this.publicationTtlMs > SubscriptionQos.MAX_PUBLICATION_TTL_MS) {
            log.warn(
                `publicationTtlMs > MAX_PUBLICATION_TTL_MS. Using MAX_PUBLICATION_TTL_MS: ${
                    SubscriptionQos.MAX_PUBLICATION_TTL_MS
                }`
            );
            this.publicationTtlMs = SubscriptionQos.MAX_PUBLICATION_TTL_MS;
        }

        if (this.expiryDateMs < SubscriptionQos.MIN_EXPIRY_MS) {
            log.warn(`expiryDateMs < MIN_EXPIRY_MS. Using MIN_EXPIRY_MS: ${SubscriptionQos.MIN_EXPIRY_MS}`);
            this.expiryDateMs = SubscriptionQos.MIN_EXPIRY_MS;
        }

        /**
         * The function clearExpiryDate resets the expiry date to the default value SubscriptionQos.NO_EXPIRY_DATE
         *
         *
         * @name SubsriptionQos#clearExpiryDate
         * @function
         */
        this.clearExpiryDate = function clearExpiryDate() {
            this.expiryDateMs = SubscriptionQos.NO_EXPIRY_DATE;
        };
    }
}

/**
 * Minimal value for [publicationTtlMs]{@link SubscriptionQos#publicationTtlMs}.
 * See [constructor description]{@link SubscriptionQos}.
 *
 * @name SubscriptionQos.MIN_PUBLICATION_TTL_MS
 * @type Number
 * @default 100
 * @static
 * @readonly
 */
SubscriptionQos.MIN_PUBLICATION_TTL_MS = 100;

/**
 * Maximum value for [publicationTtlMs]{@link SubscriptionQos#publicationTtlMs}.
 * See [constructor description]{@link SubscriptionQos}.
 *
 * @name SubscriptionQos.MAX_PUBLICATION_TTL_MS
 * @type Number
 * @default 2 592 000 000 (30 days)
 * @static
 * @readonly
 */
SubscriptionQos.MAX_PUBLICATION_TTL_MS = 2592000000;

/**
 * Minimal value for [expiryDateMs]{@link SubscriptionQos#expiryDateMs} in milliseconds
 * (0 secs). See [constructor description]{@link SubscriptionQos}.
 *
 * @name SubscriptionQos.MIN_EXPIRY_MS
 * @type Number
 * @default 0
 * @static
 * @readonly
 */
SubscriptionQos.MIN_EXPIRY_MS = 0;
/**
 * Default value for [expiryDateMs]{@link SubscriptionQos#expiryDateMs} in milliseconds
 * (no expiry date). See [constructor description]{@link SubscriptionQos}.
 *
 * @name SubscriptionQos.NO_EXPIRY_DATE
 * @type Number
 * @default 0
 * @static
 * @readonly
 */
SubscriptionQos.NO_EXPIRY_DATE = 0;
/**
 * @name SubscriptionQos.NO_EXPIRY_DATE_TTL
 * @type Number
 * @default UtilInternal.getMaxLongValue()
 * @static
 * @readonly
 */
SubscriptionQos.NO_EXPIRY_DATE_TTL = UtilInternal.getMaxLongValue();
/**
 * Default value for [publicationTtlMs]{@link SubscriptionQos#publicationTtlMs} in
 * milliseconds (10 secs). See [constructor description]{@link SubscriptionQos}.
 *
 * @name SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS
 * @type Number
 * @default 10 000
 * @static
 * @readonly
 */
SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS = 10000;
SubscriptionQos.DEFAULT_PUBLICATION_TTL = SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS;

defaultSettings = {
    expiryDateMs: SubscriptionQos.NO_EXPIRY_DATE,
    publicationTtlMs: SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS
};

module.exports = SubscriptionQos;
