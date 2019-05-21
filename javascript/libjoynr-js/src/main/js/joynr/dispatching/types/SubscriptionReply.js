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
const UtilInternal = require("../../util/UtilInternal");
const Typing = require("../../util/Typing");

class SubscriptionReply {
    /**
     * @name SubscriptionReply
     * @constructor
     *
     * @param {Object} settings
     * @param {String} settings.subscriptionId
     * @param {Object} [settings.error] The exception object in case of subscription request failure
     */
    constructor(settings) {
        Typing.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");

        Typing.checkPropertyIfDefined(settings.error, ["Object", "SubscriptionException"], "settings.error");

        /**
         * @name SubscriptionReply#subscriptionId
         * @type String
         */
        UtilInternal.extend(this, settings);

        /**
         * The joynr type name
         *
         * @name SubscriptionReply#_typeName
         * @type String
         */
        Typing.augmentTypeName(this, "joynr");
    }
}

module.exports = SubscriptionReply;
