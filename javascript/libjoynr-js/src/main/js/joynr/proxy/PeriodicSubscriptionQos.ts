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
import SubscriptionQos from "./SubscriptionQos";
import LoggingManager from "../system/LoggingManager";

const log = LoggingManager.getLogger("joynr.proxy.PeriodicSubscriptionQos");

class PeriodicSubscriptionQos extends SubscriptionQos {
    /**
     * Default value for [alertAfterIntervalMs]{@link PeriodicSubscriptionQos#alertAfterIntervalMs}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     */
    public static readonly NO_ALERT_AFTER_INTERVAL = 0;
    /**
     * Default value for [alertAfterIntervalMs]{@link PeriodicSubscriptionQos#alertAfterIntervalMs}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     */
    public static readonly DEFAULT_ALERT_AFTER_INTERVAL_MS = PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL;

    /**
     * Maximum value for [alertAfterIntervalMs]{@link PeriodicSubscriptionQos#alertAfterIntervalMs}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     */
    public static readonly MAX_ALERT_AFTER_INTERVAL_MS = 2592000000;

    /**
     * Default value for [periodMs]{@link PeriodicSubscriptionQos#periodMs}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     */
    public static readonly DEFAULT_PERIOD_MS = 60000;

    /**
     * Maximum value for [periodMs]{@link PeriodicSubscriptionQos#periodMs}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     */
    public static readonly MAX_PERIOD_MS = 2592000000;

    /**
     * Minimal value for [periodMs]{@link PeriodicSubscriptionQos#periodMs}.
     * See [constructor description]{@link PeriodicSubscriptionQos}.
     */
    public static readonly MIN_PERIOD_MS = 50;

    public static MIN_PERIOD = PeriodicSubscriptionQos.MIN_PERIOD_MS;

    public alertAfterIntervalMs = PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS;
    public periodMs = PeriodicSubscriptionQos.DEFAULT_PERIOD_MS;

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
     * @param [settings] the settings object for the constructor call
     * @param [settings.periodMs=PeriodicSubscriptionQos.DEFAULT_PERIOD_MS] defines
     *            how often an update may be sent even if the value did not change
     *            (independently from value changes).<br/>
     *            <br/>
     *            <b>Minimum, Maximum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link PeriodicSubscriptionQos.MIN_PERIOD_MS}</li>
     *              <li>maximum value: {@link PeriodicSubscriptionQos.MAX_PERIOD_MS}</li>
     *              <li>default value: {@link PeriodicSubscriptionQos.DEFAULT_PERIOD_MS}</li>
     *            </ul>
     * @param [settings.expiryDateMs] how long is the subscription valid
     * @param [settings.validityMs] The validity of the subscription relative to the current time.
     * @param [settings.alertAfterIntervalMs=PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS] defines how long to wait for an
     *            update before publicationMissed is called.<br/>
     *            <br/>
     *            <b>Minimum, Maximum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link PeriodicSubscriptionQos#period}</li>
     *              <li>maximum value: {@link PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS}</li>
     *              <li>default value: {@link PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS}</li>
     *            </ul>
     * @param [settings.publicationTtlMs] Time to live for publication messages
     *
     * @see {@link SubscriptionQos} for more information on <b>expiryDateMs</b>
     * and <b>publicationTtlMs</b>
     */
    public constructor(settings?: {
        expiryDateMs?: number;
        validityMs?: number;
        publicationTtlMs?: number;
        periodMs?: number;
        alertAfterIntervalMs?: number;
    }) {
        super(settings);
        this._typeName = "joynr.PeriodicSubscriptionQos";
        if (settings && settings.periodMs !== undefined) this.periodMs = settings.periodMs;
        if (settings && settings.alertAfterIntervalMs !== undefined)
            this.alertAfterIntervalMs = settings.alertAfterIntervalMs;

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
    }

    /**
     * The function clearAlertAfterInterval resets the alter after interval to
     * the value PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL
     */
    public clearAlertAfterInterval(): void {
        this.alertAfterIntervalMs = PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL;
    }
}

export = PeriodicSubscriptionQos;
