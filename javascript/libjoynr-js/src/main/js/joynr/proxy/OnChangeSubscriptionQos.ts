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
const log = LoggingManager.getLogger("joynr.proxy.OnChangeSubscriptionQos");

class OnChangeSubscriptionQos extends SubscriptionQos {
    /**
     * Default value for [minIntervalMs]{@link OnChangeSubscriptionQos#minIntervalMs}.
     * See [constructor description]{@link OnChangeSubscriptionQos}.
     */
    public static readonly DEFAULT_MIN_INTERVAL_MS = 1000;

    /**
     * Maximum value for [minIntervalMs]{@link OnChangeSubscriptionQos#minIntervalMs}.
     * See [constructor description]{@link OnChangeSubscriptionQos}.
     *
     * @default 2 592 000 000 (30 days)
     */
    public static readonly MAX_MIN_INTERVAL_MS = 2592000000;

    /**
     * Minimum value for [minIntervalMs]{@link OnChangeSubscriptionQos#minIntervalMs}.
     * See [constructor description]{@link OnChangeSubscriptionQos}.
     */
    public static readonly MIN_MIN_INTERVAL_MS = 0;

    /**
     * See [constructor description]{@link OnChangeSubscriptionQos}.
     */
    public minIntervalMs = OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS;

    /**
     * @classdesc
     * Class representing the quality of service settings for subscriptions
     * based on changes, including <b>attribute subscriptions and selective broadcasts</b>
     * <br/>
     * Notifications will only be sent if the subscribed value has changed.
     * <br/>
     * Other than the fields from SubscriptionQos, OnChangeSubscriptionQos adds:
     * minIntervalMs: can be used to prevent too many messages from being sent. See the description
     * of settings.minIntervalMs in the OnChangeSubscriptionQos constructor for more information
     *
     * @summary
     * Constructor of OnChangeSubscriptionQos object used for subscriptions
     * to <b>events and attributes</b> in generated proxy objects.
     *
     * @constructor
     * @name OnChangeSubscriptionQos
     *
     * @param [settings] the settings object for the constructor call
     * @param [settings.minIntervalMs=OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS] defines how often
     *            an update may be sent<br/>
     *            It is used to prevent flooding. Publications will be sent
     *            maintaining this minimum interval provided, even if the value
     *            changes more often. This prevents the consumer from being
     *            flooded by updated values. The filtering happens on the
     *            provider's side, thus also preventing excessive network
     *            traffic.<br/>
     *            <br/>
     *            <b>Minimum, Maximum and Default Values:</b>
     *            <ul>
     *              <li>minimum value: {@link OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS}</li>
     *              <li>maximum value: {@link OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS}</li>
     *              <li>default value: {@link OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS}</li>
     *            </ul>
     * @param [settings.expiryDateMs] how long is the subscription valid
     * @param [settings.validityMs] The validity of the subscription relative to the current time.
     * @param [settings.publicationTtlMs] time to live for publication messages
     *
     * @see {@link SubscriptionQos} for more information on <b>expiryDateMs</b>
     * and <b>publicationTtlMs</b>
     */
    public constructor(settings?: {
        expiryDateMs?: number;
        validityMs?: number;
        publicationTtlMs?: number;
        minIntervalMs?: number;
    }) {
        super(settings);
        this._typeName = "joynr.OnChangeSubscriptionQos";

        if (settings && settings.minIntervalMs !== undefined) {
            this.minIntervalMs = settings.minIntervalMs;
        }

        if (this.minIntervalMs < OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS) {
            log.warn(
                `minIntervalMs < MIN_MIN_INTERVAL_MS. Using MIN_MIN_INTERVAL_MS: ${
                    OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS
                }`
            );
            this.minIntervalMs = OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS;
        }
        if (this.minIntervalMs > OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS) {
            log.warn(
                `minIntervalMs > MAX_MIN_INTERVAL_MS. Using MAX_MIN_INTERVAL_MS: ${
                    OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS
                }`
            );
            this.minIntervalMs = OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS;
        }
    }
}

export = OnChangeSubscriptionQos;
