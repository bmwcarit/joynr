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
const JoynrRuntimeException = require("./JoynrRuntimeException");

class SubscriptionException extends JoynrRuntimeException {
    /**
     * Constructor of SubscriptionException object used for reporting
     * error conditions when creating a subscription (e.g. the
     * provided subscription parameters are not correct etc.) that should
     * be transmitted back to consumer side.
     *
     * @param {Object} settings - the settings object for the constructor call
     * @param {String} [settings.detailMessage] message containing details
     *            about the error
     * @param {String} settings.subscriptionId - Id of the subscription
     * @returns {SubscriptionException} The newly created SubscriptionException object
     */
    constructor(settings = {}) {
        super(settings);

        /**
         * Used for serialization.
         * @name SubscriptionException#_typeName
         * @type String
         */
        this._typeName = "joynr.exceptions.SubscriptionException";
        this.name = "SubscriptionException";

        this.subscriptionId = settings.subscriptionId;
        if (settings) {
            Typing.checkPropertyIfDefined(settings.subscriptionId, "String", "settings.subscriptionId");
        }
    }

    static _typeName = "joynr.exceptions.SubscriptionException";
}

module.exports = SubscriptionException;
